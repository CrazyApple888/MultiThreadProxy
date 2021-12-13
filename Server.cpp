#include "Server.h"

bool Server::execute(int event) {
    logger->debug(TAG, "EXECUTE");
    if (cache != nullptr) {
        if (cache->isFull() || is_first_run) {
            logger->debug(TAG, "Cache is full, subscribing client");
            cache->subscribe(client_soc);
            //proxy->addCacheToClient(client_soc, cache);
            return false;
        }
    }
    is_first_run = false;

//    char buffer[BUFFER_SIZE];
    auto len = recv(server_socket, buffer, BUFFER_SIZE, 0);
    if (len < 0) {
        if (cache != nullptr) {
            cache->setInvalid();
        }
        logger->debug(TAG, "LEN < 0");
        return false;
    }
    if (len == 0) {
        logger->debug(TAG, "Setting status FULL to cache for " + url);
        cache->setFull();
    }

    logger->info(TAG, "GOT ANSWER, len = " + std::to_string(len));

    //auto data = std::string(buffer, len);
    if (cache != nullptr) {

        logger->debug(TAG, "CACHE != NULLPTR");
        if (!is_client_subscribed) {
            cache->subscribe(client_soc);
            //proxy->addCacheToClient(client_soc, cache);
            is_client_subscribed = true;
        }

        if (!cache->isFull()) {
            logger->debug(TAG, "CACHE ISN'T FULL");
            if (!cache->expandData(buffer, len)) {
                logger->info(TAG, "Can't allocate memory for " + url);
                cache->setInvalid();
                return false;
            }
        } else {
            logger->debug(TAG, "CACHE IS FULL");
            cache->subscribe(client_soc);
            //proxy->addCacheToClient(client_soc, cache);
            return false;
        }

    } else {
        //cache = proxy->getCache()->createEntity(url);
        if (nullptr == cache) {
            logger->info(TAG, "Can't create cache entity for " + url);
            return false;
        }
        if (!is_client_subscribed) {
            cache->subscribe(client_soc);
            //proxy->addCacheToClient(client_soc, cache);
            is_client_subscribed = true;
        }
        if (!cache->expandData(buffer, len)) {
            logger->info(TAG, "Can't allocate memory for " + url);
            cache->setInvalid();
            return false;
        }
    }

    logger->debug(TAG, "Answer sent to client " + std::to_string(client_soc));

    return true;
}

Server::Server(const std::string &_request, const std::string &_host, CacheEntity *_cache, bool is_debug) : logger(new Logger(is_debug)) {
    this->request = _request;
    this->cache = _cache;
    this->host = _host;
    TAG = std::string("SERVER " + std::to_string(server_socket));
    logger->debug(TAG, "created");
}

void Server::sendRequest() {

    logger->info(TAG, "Host = " + host);
    struct hostent *hostinfo = gethostbyname(host.data());
    if (nullptr == hostinfo) {
        return;
    }

    if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        logger->info(TAG, "Can't create socket for host" + host);
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

    send(server_socket, request.data(), request.size(), 0);
}

Server::~Server() {
    close(server_socket);
    delete logger;
}

void Server::readFromServer() {
    while (true) {
        auto len = recv(server_socket, buffer, BUFFER_SIZE, 0);
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

        if (!cache->expandData(buffer, len)) {
            logger->info(TAG, "Can't allocate memory for " + url);
            cache->setInvalid();
            return;
        }

    }
}
