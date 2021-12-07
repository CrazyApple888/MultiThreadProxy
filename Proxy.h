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

#include "Logger.h"
#include "Handler.h"
#include "Client.h"
#include "Cache.h"

class Client;

class Cache;

class Proxy {
private:
    Logger *logger;
    const std::string TAG = "PROXY";
    const int backlog = 20;
    int proxy_port;
    int proxy_socket;
    std::vector<struct pollfd> clientsPollFd;
    std::map<int, Handler *> handlers;
    Cache *cache;
    bool is_stopped = false;

    int initProxySocket();

    void initProxyPollFd();

    void acceptClient();

    void initClientPollFd(int socket);

    void testRead(int fd);

    void disconnectClient(pollfd client, size_t index);

public:

    void stop();

    explicit Proxy(bool is_debug);

    ~Proxy();

    int start(int port);

    bool createServerConnection(const std::string &host, Client *client);

    void disconnectSocket(int soc);

    void notify(int soc);

    Cache *getCache();

    void addCacheToClient(int soc, CacheEntity *cache_entity);

    void disableSoc(int soc);

};


#endif //SINGLETHREADPROXY_PROXY_H
