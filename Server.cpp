#include "Server.h"

bool Server::execute(int event) {
    logger->debug(TAG, "EXECUTE");
    if (cache != nullptr) {
        if (cache->isFull() || is_first_run) {
            logger->debug(TAG, "Cache is full, subscribing client");
            cache->subscribe(client_soc);
            proxy->addCacheToClient(client_soc, cache);
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
            proxy->addCacheToClient(client_soc, cache);
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
            proxy->addCacheToClient(client_soc, cache);
            return false;
        }

    } else {
        cache = proxy->getCache()->createEntity(url);
        if (nullptr == cache) {
            logger->info(TAG, "Can't create cache entity for " + url);
            return false;
        }
        if (!is_client_subscribed) {
            cache->subscribe(client_soc);
            proxy->addCacheToClient(client_soc, cache);
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

Server::Server(int server_socket, bool is_debug, Proxy *proxy) : logger(new Logger(is_debug)), proxy(proxy) {
    this->server_socket = server_socket;
    TAG = std::string("SERVER " + std::to_string(server_socket));
    logger->debug(TAG, "created");
}

void Server::sendRequest(const char *url1, const char *headers, const char *method) {
    this->url = std::string(url1);
    auto get_this = std::string(method);
    get_this.append(" ");
    get_this.append(url);
    get_this.append(" HTTP/1.0\r\n");

    auto _headers = std::string(headers);
    get_this.append(_headers);
    send(server_socket, get_this.data(), get_this.size(), 0);
    this->cache = proxy->getCache()->getEntity(url);
}

Server::~Server() {
    delete logger;
}
