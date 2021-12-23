#include "Proxy.h"

auto *routineStorage = new ConcurrentMap<pthread_t, bool>();

void *sigintListener(void *arg) {
    sigset_t sig_set;
    int ret_sig;
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGINT);

    sigwait(&sig_set, &ret_sig);
    if (ret_sig == SIGINT) {
        printf("Got SIGINT\n");
        auto *proxy = (Proxy *) arg;
        proxy->stop();
        printf("Said stop to proxy\n");
    } else {
        printf("NOT SIGINT");
        exit(EXIT_FAILURE);
    }

    return nullptr;
}

void *serverRoutine(void *arg) {
    auto *server = (Server *) arg;
    if (nullptr == server) {
        return (void *) 1;
    }

    if (server->sendRequest()) {
        server->readFromServer();
    }
    delete server;
    routineStorage->update(pthread_self(), true);

    pthread_exit(nullptr);
}

void *clientRoutine(void *arg) {
    auto *client = (Client *) arg;
    if (client == nullptr) {
        pthread_exit(nullptr);
    }

    auto *logger = client->getLogger();
    auto tag = client->getTag();
    if (!client->readRequest()) {
        pthread_testcancel();
        logger->info(tag, "Unable to read request");
        delete client;
        pthread_exit(nullptr);
    }

    logger->debug(tag, "Getting cache");

    //If cache exists, read it and die
    if (client->isCacheExist()) {
        auto cache = client->createCacheEntity();
        cache->subscribe();
        if (!client->readData()) {
            pthread_testcancel();
            logger->info(tag, "Read unsuccessful");
            cache->unsubscribe();
            delete client;
            pthread_exit(nullptr);
        }
        cache->unsubscribe();
        logger->info(tag, "All data read");
        delete client;
        pthread_exit(nullptr);
    }

    //Else
    auto *cache = client->createCacheEntity();
    if (cache == nullptr) {
        pthread_testcancel();
        logger->debug(tag, "CACHE IS NULLPTR");
        delete client;
        pthread_exit(nullptr);
    }
    cache->subscribe();
    logger->debug(tag, "Got CacheEntity, creating server routine");
    auto *server = new Server(client->getRequest(), client->host, cache, client->getLogger()->isDebug());
    pthread_t server_thread;

    if (0 != pthread_create(&server_thread, nullptr, serverRoutine, (void *) server)) {
        logger->info(tag, "Unable to start server routine");
        delete client;
        pthread_exit(nullptr);
    }
    routineStorage->put(server_thread, false);

    if (!client->readData()) {
        pthread_testcancel();
        logger->debug(tag, "Client read unsuccessful");
    }
    logger->info(tag, "Client shutting down");

    delete client;
    logger->info(tag, "Client shutting down");
    routineStorage->update(pthread_self(), true);

    return nullptr;
}

Proxy::Proxy(bool is_debug) : logger(new Logger(is_debug)) {
    this->cache = new Cache(is_debug);
    this->is_working = new AtomicInt(0);
}

int Proxy::start(int port) {

    if (0 != initSigHandler()) {
        logger->info(TAG, "Unable to start signal handler, shutting down");
        return 1;
    }

    this->proxy_port = port;
    if (1 == initProxySocket()) {
        logger->info(TAG, "Can't init socket");
        return 1;
    }
    initProxyPollFd();

    while (is_working->get() == 0) {
        if (poll(&proxy_fd, 1, -1) < 0) {
            logger->info(TAG, "Poll returned value < 0");
            break;
        }
        if (is_working->get() != 0) {
            logger->debug(TAG, "Proxy stopped");
            break;
        }
        Client *client;
        try {
            client = acceptClient();
        } catch (std::bad_alloc &exc) {
            logger->info(TAG, "Unable to create client");
            continue;
        }

        pthread_t client_thread;
        if (0 != pthread_create(&client_thread, nullptr, clientRoutine, (void *) client)) {
            logger->info(TAG, "Unable to create new routine for client");
            delete client;
            continue;
        }
        routineStorage->put(client_thread, false);
        proxy_fd.revents = 0;
        joinFinishedThreads();
    }
    cache->setAllInvalid();
    joinAll();
    routineStorage->clear();
    delete routineStorage;

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
    close(proxy_socket);
    delete cache;
    delete logger;
    delete is_working;
}

void Proxy::stop() {
    is_working->inc();
    close(proxy_socket);
}

int Proxy::initSigHandler() {
    sigset_t sig_set;
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGINT);
    if (0 != pthread_sigmask(SIG_BLOCK, &sig_set, nullptr)) {
        printf("Can't init sigmask in main thread\n");
        exit(EXIT_FAILURE);
    }

    pthread_t signalHandler;
    if (0 != pthread_create(&signalHandler, nullptr, sigintListener, this)) {
        logger->info(TAG, "Can't create sigint handler routine");
        return 1;
    }
    if (0 != pthread_detach(signalHandler)) {
        logger->info(TAG, "Unable to detach thread for sigHandler");
        if (0 != pthread_cancel(signalHandler)) {
            logger->info(TAG, "Unable to cancel thread for sigHandler");
            return 1;
        }
    }

    return 0;
}

void Proxy::joinFinishedThreads() {
    routineStorage->lock();
    auto &routines = routineStorage->getMap();
    for (auto &entry: routines) {
        if (entry.second) {
            if (0 != pthread_join(entry.first, nullptr)) {
                logger->info(TAG, "Unable to join thread");
            }
        }
    }
    auto iter = routines.begin();
    auto endIter = routines.end();
    for (; iter != endIter;) {
        if (iter->second) {
            iter = routines.erase(iter);
        } else {
            ++iter;
        }
    }
    routineStorage->unlock();
}

void Proxy::joinAll() {
    logger->debug(TAG, "Starting joining threads...");
    //routineStorage->lock();
    auto &routines = routineStorage->getMap();
    for (auto &entry: routines) {
        if (0 != pthread_join(entry.first, nullptr)) {
            logger->info(TAG, "Unable to join thread");
        }
        logger->debug(TAG, "Joined thread");
    }
    //routineStorage->unlock();
    logger->debug(TAG, "Threads joined");
}