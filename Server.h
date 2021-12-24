#ifndef MULTITHREADPROXY_SERVER_H
#define MULTITHREADPROXY_SERVER_H


#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <iostream>

#include "CacheEntity.h"
#include "AtomicInt.h"
#include "http_parser.h"

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

    http_parser parser {};
    http_parser_settings settings{};
public:

    Server(const std::string &_request, const std::string &_host, CacheEntity *_cache, bool is_debug);

    ~Server();

    bool sendRequest();

    bool readFromServer();
};


#endif //MULTITHREADPROXY_SERVER_H
