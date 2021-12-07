#ifndef SINGLETHREADPROXY_CLIENT_H
#define SINGLETHREADPROXY_CLIENT_H


#include <string>
#include "Handler.h"
#include "Logger.h"
#include "Proxy.h"
#include "http_parser.h"
#include "Server.h"

#define BUFFER_SIZE (BUFSIZ * 5)

class Proxy;

class Server;

class Client : public Handler {
private:
    char buffer[BUFFER_SIZE];
    std::string TAG;
    Logger *logger;
    Proxy *proxy;
    http_parser_settings settings{};
    http_parser parser{};
    Server *server;

    bool readRequest();

    bool readAnswer();

    CacheEntity *cached_data = nullptr;
    size_t current_pos = 0;
public:

    int client_socket;
    std::string url;
    std::string method;

    std::string h_field = "";
    std::string headers = "";

    Client() = default;
    ~Client();

    Client(int client_socket, bool is_debug, Proxy *proxy);

    Logger *getLogger() { return logger; }

    std::string getTag() { return TAG; }

    bool execute(int event) override;

    bool createServerConnection(const std::string &host);

    void addServer(Server *ser);

    void sendServerRequest();

    void addCache(CacheEntity *cache);
};


#endif //SINGLETHREADPROXY_CLIENT_H
