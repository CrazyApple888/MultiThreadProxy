#ifndef SINGLETHREADPROXY_SERVER_H
#define SINGLETHREADPROXY_SERVER_H


#include <sys/socket.h>
#include <string>
#include <iostream>

#include "Handler.h"
#include "CacheEntity.h"
#include "Proxy.h"

#define BUFFER_SIZE (BUFSIZ * 5)

class Proxy;

class Server : public Handler {
private:
    char buffer[BUFFER_SIZE];
    int server_socket;
    std::string TAG;
    Logger *logger;
    std::string url;
    bool is_client_subscribed = false;
    CacheEntity *cache = nullptr;
    bool is_first_run = true;
    std::string request;
    std::string host;
public:
    int client_soc;

    Server(const std::string &_request, const std::string &_host, CacheEntity *_cache, bool is_debug);

    ~Server();

    bool execute(int event) override;

    void sendRequest();

    void readFromServer();
};


#endif //SINGLETHREADPROXY_SERVER_H
