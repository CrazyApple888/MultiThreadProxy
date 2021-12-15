#include "Proxy.h"

void *serverRoutine(void *arg) {
    auto *server = (Server *) arg;
    server->sendRequest();
    server->readFromServer();

    delete server;
    return nullptr;
}

void *clientRoutine(void *arg) {
    auto *client = (Client *) arg;
    if (client == nullptr) {
        return (void *) 1;
    }
    if (!client->readRequest()) {
        client->getLogger()->info(client->getTag(), "Unable to read request");
        delete client;
        return (void *) 1;
    }

    client->getLogger()->debug(client->getTag(), "Getting cache");

    //If cache exists, read it and die
    if (client->isCacheExist()) {
        client->createCacheEntity();
        client->readData();
        delete client;
        return nullptr;
    }

    //Else
    auto *cache = client->createCacheEntity();
    if (cache == nullptr) {
        client->getLogger()->debug(client->getTag(), "CACHE IS NULLPTR");
        delete client;
        return (void *) 1;
    }
    client->getLogger()->debug(client->getTag(), "Got CacheEntity, creating server routine");
    auto *server = new Server(client->getRequest(), client->host, cache, client->getLogger()->isDebug());
    pthread_t server_thread;

    if (0 != pthread_create(&server_thread, nullptr, serverRoutine, (void *) server)) {
        client->getLogger()->info(client->getTag(), "Unable to start server routine");
        delete client;
        return (void *) 1;
    }
    if (0 != pthread_detach(server_thread)) {
        client->getLogger()->info(client->getTag(),
                                  "Unable to detach server thread for client " + std::to_string(client->client_socket));
        if (0 != pthread_cancel(server_thread)) {
            client->getLogger()->info(client->getTag(),
                                      "Unable to cancel server thread for client " +
                                      std::to_string(client->client_socket));
        }
    }

    client->readData();

    delete client;
    return nullptr;
}

Proxy::Proxy(bool is_debug) : logger(new Logger(is_debug)) {
    this->cache = new Cache(is_debug);
}

int Proxy::start(int port) {
    this->proxy_port = port;
    if (1 == initProxySocket()) {
        logger->info(TAG, "Can't init socket");
        return 1;
    }
    initProxyPollFd();

    while (!is_stopped) {
        if (poll(&proxy_fd, 1, -1) < 0) {
            logger->info(TAG, "Poll returned value < 0:");
            return 1;
        }
        auto client = acceptClient();
        if (client == nullptr) {
            logger->info(TAG, "Unable to allocate memory for new client");
            continue;
        }
        pthread_t client_thread;
        if (0 != pthread_create(&client_thread, nullptr, clientRoutine, (void *) client)) {
            logger->info(TAG, "Unable to create new routine for client");
            close(client->client_socket);
            delete client;
            continue;
        }
        if (0 != pthread_detach(client_thread)) {
            logger->info(TAG, "Unable to detach thread for client " + std::to_string(client->client_socket));
            if (0 != pthread_cancel(client_thread)) {
                logger->info(TAG, "Unable to cancel thread for client " + std::to_string(client->client_socket));
            }
        }
        proxy_fd.revents = 0;
    }

    logger->info(TAG, "proxy finished");
    return 0;
}

Client *Proxy::acceptClient() {
    int client_socket;
    if ((client_socket = accept(proxy_socket, nullptr, nullptr)) < 0) {
        logger->info(TAG, "Can't accept client");
        return nullptr;
    }
    auto client = new Client(client_socket, logger->isDebug(), cache);
    logger->info(TAG, "Accepted new client with descriptor " + std::to_string(client_socket));

    return client;
}

void Proxy::initProxyPollFd() {
    proxy_fd = {
            .fd = proxy_socket,
            .events = POLLIN | POLLHUP,
            .revents = 0
    };
}

int Proxy::initProxySocket() {
    proxy_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == proxy_socket) {
        logger->info(TAG, "Can't create socket");
        return 1;
    }

    struct sockaddr_in proxy_addr{};
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(proxy_port);
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (-1 == bind(proxy_socket, (sockaddr *) (&proxy_addr), sizeof(proxy_addr))) {
        close(proxy_socket);
        logger->info(TAG, "Can't bind socket");
        return 1;
    }

    if (-1 == listen(proxy_socket, backlog)) {
        close(proxy_socket);
        logger->info(TAG, "Can't listen socket");
        return 1;
    }

    return 0;
}

Proxy::~Proxy() {
    for (auto &item: clientsPollFd) {
        close(item.fd);
    }
    for (auto i = 0; i < handlers.size(); i++) {
        delete handlers[i];
    }
    handlers.erase(handlers.begin(), handlers.end());
    close(proxy_socket);
    clientsPollFd.clear();
    delete cache;
    delete logger;
}

void Proxy::notify(int soc) {
    for (auto &item: clientsPollFd) {
        if (soc == item.fd) {
            item.events |= POLLOUT;
            break;
        }
    }
}

void Proxy::disableSoc(int soc) {
    for (auto &item: clientsPollFd) {
        if (soc == item.fd) {
            item.events &= ~POLLOUT;
            break;
        }
    }
}

Cache *Proxy::getCache() {
    return cache;
}

void Proxy::addCacheToClient(int soc, CacheEntity *cache_entity) {
    logger->debug(TAG, "addCacheToClient " + std::to_string(soc));
    for (auto &item: clientsPollFd) {
        if (soc == item.fd) {
            logger->debug(TAG, "addCacheToClient" + std::string("SOC == FD"));
            try {
                logger->debug(TAG, "!");
                auto _client = handlers.at(soc);
                logger->debug(TAG, "!");
                dynamic_cast<Client *>(_client)->addCache(cache_entity);
                logger->debug(TAG, "!!!");
            } catch (...) {
                logger->info(TAG, "ADD CACHE TO CLIENT GOT EXCEPTION, SOCKET = " + std::to_string(soc));
                exit(EXIT_FAILURE);
            }
            break;
        }
    }
}

void Proxy::stop() {
    is_stopped = true;
}
