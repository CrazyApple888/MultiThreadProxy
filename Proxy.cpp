#include "Proxy.h"

Proxy::Proxy(bool is_debug) : logger(new Logger(is_debug)) {
    this->cache = new Cache(is_debug);
}

int Proxy::start(int port) {

    std::vector<int> subs;

    this->proxy_port = port;
    if (1 == initProxySocket()) {
        logger->info(TAG, "Can't init socket");
        return 1;
    }
    initProxyPollFd();

    int events_activated;
    while (!is_stopped) {
        events_activated = poll(clientsPollFd.data(), clientsPollFd.size(), -1);
        if (events_activated <= 0) {
            logger->info(TAG, "EVENTS = " + std::to_string(events_activated));
            break;
        }
        logger->debug(TAG, "Events activated = " + std::to_string(events_activated));
        ///New client
        if (POLLIN == clientsPollFd[0].revents) {
            logger->debug(TAG, "Proxy POLLIN " + std::to_string(clientsPollFd[0].revents));
            clientsPollFd[0].revents = 0;
            acceptClient();
        }

        ///Checking for new messages from clients
        for (auto i = 1; i < clientsPollFd.size(); i++) {
            logger->debug(TAG, "KAVO");
            //todo maybe ==, maybe without POLLOUT
            if ((POLLIN | POLLOUT) & clientsPollFd[i].revents) {
                bool is_success = false;
                try {
                    is_success = handlers.at(clientsPollFd[i].fd)->execute(clientsPollFd[i].revents);
                } catch (std::out_of_range &exc) {
                    logger->info(TAG, R"(GOT FATAL EXCEPTION \/\/\/\/\/\/, SHUTTING DOWN...)");
                    logger->info(TAG, exc.what());
                    return 1;
                }
                if (!is_success) {
                    logger->debug(TAG, "Execute isn't successful for" + std::to_string(clientsPollFd[i].fd));
                    disconnectClient(clientsPollFd[i], i);
                    continue;
                }

                if ((POLLHUP | POLLIN) == clientsPollFd[i].revents) {
                    logger->info(TAG, "POLLHUP | POLLIN " + std::to_string(clientsPollFd[i].revents));
                    disconnectClient(clientsPollFd[i], i);
                }
            }
            if ((POLLERR | POLLIN) == clientsPollFd[i].revents) {
                disconnectClient(clientsPollFd[i], i);
            }

            clientsPollFd[i].revents = 0;
        }
        logger->debug(TAG, "Poll iteration completed");

        cache->getUpdatedSubs(subs);
        for (auto sub : subs) {
            notify(sub);
        }
        subs.clear();

    }

    logger->info(TAG, "proxy finished");
    return 0;
}

void Proxy::testRead(int fd) {
    char buffer[BUFSIZ];
    read(fd, buffer, BUFSIZ);
    logger->info(TAG, buffer);
}

void Proxy::disconnectClient(struct pollfd client, size_t index) {
    //auto _client = handlers.at(client.fd);
    handlers.erase(client.fd);
    auto iter = clientsPollFd.begin() + index;
    logger->debug(TAG, "Deleting pollfd with soc = " + std::to_string(iter->fd));
    clientsPollFd.erase(iter);
    close(client.fd);
    //delete _client;
    logger->info(TAG, "Disconnected client with descriptor " + std::to_string(client.fd));
}

void Proxy::acceptClient() {
    int client_socket;
    //todo may be made it nonblock
    if ((client_socket = accept(proxy_socket, nullptr, nullptr)) < 0) {
        logger->info(TAG, "Can't accept client");
        return;
    }
    auto client = new Client(client_socket, logger->isDebug(), this);
    handlers.insert(std::make_pair(client_socket, client));
    initClientPollFd(client_socket);
    logger->info(TAG, "Accepted new client with descriptor " + std::to_string(client_socket));
}

void Proxy::initClientPollFd(int socket) {
    struct pollfd client{
        .fd = socket,
        .events = POLLIN | POLLHUP,
        .revents = 0
    };

    clientsPollFd.push_back(client);
}

void Proxy::initProxyPollFd() {
    struct pollfd serverFd{};
    serverFd.fd = proxy_socket;
    serverFd.events = POLLIN | POLLHUP;
    serverFd.revents = 0;

    clientsPollFd.push_back(serverFd);
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

//    if (-1 == fcntl(proxy_socket, F_SETFL, O_NONBLOCK)) {
//        close(proxy_socket);
//        return EXIT_FAILURE;
//    }

    if (-1 == listen(proxy_socket, backlog)) {
        close(proxy_socket);
        logger->info(TAG, "Can't listen socket");
        return 1;
    }

    return 0;
}

Proxy::~Proxy() {
    for (auto &item : clientsPollFd) {
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

bool Proxy::createServerConnection(const std::string &host, Client *client) {
    logger->info(TAG, "Host = " + host);
    struct hostent *hostinfo = gethostbyname(host.data());
    if (nullptr == hostinfo) {
        return false;
    }

    int soc;
    if ((soc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        logger->info(TAG, "Can't create socket for host" + host);
        return false;
    }

    struct sockaddr_in sockaddrIn{};
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(80);
    sockaddrIn.sin_addr = *((struct in_addr *) hostinfo->h_addr);

    logger->debug(TAG, "Connecting server to " + host);
    if (-1 == (connect(soc, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn)))) {
        logger->info(TAG, "Can't create connection to " + host);
        //--
        free(hostinfo);
        return false;
    }
    logger->info(TAG, "Connected server to " + host);

    initClientPollFd(soc);
    auto server = new Server(soc, logger->isDebug(), this);
    handlers.insert(std::make_pair(soc, server));

    client->addServer(server);
    server->client_soc = client->client_socket;

    logger->info(TAG, "Added server with descriptor " + std::to_string(soc));

    //
    free(hostinfo);
    return true;
}

void Proxy::disconnectSocket(int soc) {
    struct pollfd client{};
    int index = -1;
    for (int i = 0; i < clientsPollFd.size(); ++i) {
        if (clientsPollFd[i].fd == soc) {
            client = clientsPollFd[i];
            index = i;
            break;
        }
    }
    if (index != -1) {
        disconnectClient(client, index);
    }
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
