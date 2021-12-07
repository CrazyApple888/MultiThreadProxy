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
    Proxy *proxy;
    std::string url;
    bool is_client_subscribed = false;
    CacheEntity *cache = nullptr;
    bool is_first_run = true;
public:
    int client_soc;

    Server(int server_socket, bool is_debug, Proxy *proxy);

    ~Server();

    bool execute(int event) override;

    void sendRequest(const char *url1, const char *headers, const char *method);
};


#endif //SINGLETHREADPROXY_SERVER_H
