#include <unistd.h>
#include "Server.h"

Server::Server(const std::string &_request, const std::string &_host, CacheEntity *_cache, bool is_debug) : logger(
        new Logger(is_debug)) {
    this->request = _request;
    this->cache = _cache;
    this->host = _host;
    TAG = std::string("SERVER " + std::to_string(server_socket));
    logger->debug(TAG, "created");
    http_parser_init(&parser, HTTP_RESPONSE);
    http_parser_settings_init(&settings);
}

bool Server::sendRequest() {

    logger->info(TAG, "Host = " + host);
    struct hostent *hostinfo = gethostbyname(host.data());
    if (nullptr == hostinfo) {
        return false;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        logger->info(TAG, "Can't create socket for host" + host);
        return false;
    }

    struct sockaddr_in sockaddrIn{};
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(80);
    sockaddrIn.sin_addr = *((struct in_addr *) hostinfo->h_addr);

    logger->debug(TAG, "Connecting server to " + host);
    if (-1 == (connect(server_socket, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn)))) {
        logger->info(TAG, "Can't create connection to " + host);
        return false;
    }
    logger->info(TAG, "Connected server to " + host);

    ssize_t bytes_sent = 0;
    while (bytes_sent != request.size()) {
        ssize_t sent = write(server_socket, request.data(), request.size());
        if (0 > sent) {
            logger->debug(TAG, "Send returned value < 0");
            return false;
        }
        if (0 == sent) {
            return false;
        }
        bytes_sent += sent;
    }

    return true;
}

Server::~Server() {
    close(server_socket);
    delete logger;
}

bool Server::readFromServer() {
    while (true) {
        auto len = read(server_socket, buffer, BUFFER_SIZE);
        auto parsed_len = http_parser_execute(&parser, &settings, buffer, len);
        if (len < 0 || parsed_len != len) {
            if (cache != nullptr) {
                cache->setInvalid();
            }
            logger->debug(TAG, "LEN < 0");
            return false;
        }
        if (len == 0) {
            logger->debug(TAG, "Setting status FULL to cache for " + url);
            cache->setFull();
            cache->setStatus(parser.status_code);
            break;
        }

        if (!cache->hasSubscribers() || !cache->expandData(buffer, len)) {
            logger->info(TAG, "Setting status invalid for cache " + url);
            cache->setInvalid();
            return false;
        }
    }

    return true;
}