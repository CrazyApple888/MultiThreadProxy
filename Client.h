#ifndef MULTITHREADPROXY_CLIENT_H
#define MULTITHREADPROXY_CLIENT_H


#include <string>
#include "Logger.h"
#include "http_parser.h"
#include "Cache.h"
#include "AtomicInt.h"

#define BUFFER_SIZE (BUFSIZ * 5)

class Client {
private:
    char buffer[BUFFER_SIZE];
    std::string TAG;
    Logger *logger;
    http_parser_settings settings{};
    http_parser parser{};

    Cache *cache;
    CacheEntity *cached_data = nullptr;
    size_t current_pos = 0;
public:
    int client_socket;
    std::string url;
    std::string method;
    std::string host;

    std::string h_field = "";
    std::string headers = "";
    bool isAllParsed = false;

    Client() = default;

    ~Client();

    Client(int client_socket, bool is_debug, Cache *_cache);

    bool readData();

    bool isCacheExist();

    CacheEntity *createCacheEntity();

    std::string getRequest();

    bool readRequest();

    Logger *getLogger() { return logger; }

    std::string getTag() { return TAG; }
};


#endif //MULTITHREADPROXY_CLIENT_H
