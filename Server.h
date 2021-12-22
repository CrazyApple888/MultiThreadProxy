#ifndef SINGLETHREADPROXY_SERVER_H
#define SINGLETHREADPROXY_SERVER_H


#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <iostream>

#include "CacheEntity.h"
#include "AtomicInt.h"

#define BUFFER_SIZE (BUFSIZ * 5)

class Server {
private:
    char buffer[BUFFER_SIZE];
    int server_socket;
    std::string TAG;
    Logger *logger;
    std::string url;
    CacheEntity *cache = nullptr;
    std::string request;
    std::string host;
    AtomicInt *is_finished;
public:
    int client_soc;

    Server(const std::string &_request, const std::string &_host, CacheEntity *_cache, bool is_debug);

    ~Server();

    void sendRequest();

    void readFromServer();
};


#endif //SINGLETHREADPROXY_SERVER_H
