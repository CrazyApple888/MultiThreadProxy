#ifndef SINGLETHREADPROXY_PROXY_H
#define SINGLETHREADPROXY_PROXY_H


#include <cstdlib>
#include <poll.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include <netdb.h>
#include <sys/un.h>
#include <algorithm>
#include <bitset>
#include <pthread.h>

#include "Logger.h"
#include "Client.h"
#include "Server.h"
#include "Cache.h"

class Proxy {
private:
    Logger *logger;
    const std::string TAG = "PROXY";
    const int backlog = 20;
    int proxy_port;
    int proxy_socket;
    struct pollfd proxy_fd;
    Cache *cache;
    bool is_stopped = false;

    int initProxySocket();

    void initProxyPollFd();

    Client *acceptClient();

public:

    void stop();

    explicit Proxy(bool is_debug);

    ~Proxy();

    int start(int port);

    Cache *getCache();

};


#endif //SINGLETHREADPROXY_PROXY_H
