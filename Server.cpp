#include <unistd.h>
#include "Server.h"

Server::Server(const std::string &_request, const std::string &_host, CacheEntity *_cache, bool is_debug) : logger(
        new Logger(is_debug)) {
    this->request = _request;
    this->cache = _cache;
    this->host = _host;
    TAG = std::string("SERVER " + std::to_string(server_socket));
    logger->debug(TAG, "created");
    is_finished = new AtomicInt(0);
}

void Server::sendRequest() {

    logger->info(TAG, "Host = " + host);
    struct hostent *hostinfo = gethostbyname(host.data());
    if (nullptr == hostinfo) {
        return;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        logger->info(TAG, "Can't create socket for host" + host);
        free(hostinfo);
        return;
    }

    struct sockaddr_in sockaddrIn{};
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(80);
    sockaddrIn.sin_addr = *((struct in_addr *) hostinfo->h_addr);

    logger->debug(TAG, "Connecting server to " + host);
    if (-1 == (connect(server_socket, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn)))) {
        logger->info(TAG, "Can't create connection to " + host);
        free(hostinfo);
        return;
    }
    logger->info(TAG, "Connected server to " + host);
    free(hostinfo);

    //send(server_socket, request.data(), request.size(), 0);
    write(server_socket, request.data(), request.size());
}

Server::~Server() {
    close(server_socket);
    delete logger;
}

void Server::readFromServer() {
    while (true) {
        //auto len = recv(server_socket, buffer, BUFFER_SIZE, 0);
        auto len = read(server_socket, buffer, BUFFER_SIZE);
        if (len < 0) {
            if (cache != nullptr) {
                cache->setInvalid();
            }
            logger->debug(TAG, "LEN < 0");
            return;
        }
        if (len == 0) {
            logger->debug(TAG, "Setting status FULL to cache for " + url);
            cache->setFull();
            return;
        }

        if (!cache->hasSubscribers() || !cache->expandData(buffer, len)) {
            logger->info(TAG, "Setting status invalid for cache " + url);
            cache->setInvalid();
            return;
        }
    }
}