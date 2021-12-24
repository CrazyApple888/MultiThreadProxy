#ifndef MULTITHREADPROXY_PROXY_H
#define MULTITHREADPROXY_PROXY_H


#include <cstdlib>
#include <poll.h>
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <sys/un.h>
#include <pthread.h>
#include <csignal>

#include "Logger.h"
#include "Client.h"
#include "Server.h"
#include "Cache.h"
#include "AtomicInt.h"

class Proxy {
private:
    Logger *logger;
    const std::string TAG = "PROXY";
    const int backlog = 20;
    int proxy_port;
    int proxy_socket;
    struct pollfd proxy_fd;
    Cache *cache;

    AtomicInt *is_working;

    int initProxySocket();

    void initProxyPollFd();

    int initSigHandler();

    Client *acceptClient();

    void joinFinishedThreads();

    void joinAll();

    void stopAll();

public:

    void stop();

    explicit Proxy(bool is_debug);

    ~Proxy();

    int start(int port);

};

#endif //MULTITHREADPROXY_PROXY_H
