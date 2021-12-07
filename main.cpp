#include <iostream>
#include <csignal>
#include "Proxy.h"

Proxy *proxy = nullptr;

void handleSigint(int sig) {
    if (SIGINT == sig && nullptr != proxy) {
        proxy->stop();
    } else {
        exit(EXIT_SUCCESS);
    }
}

/// $ prog_name port is_debug
int main(int argc, char *argv[]) {
    sigset(SIGPIPE, SIG_IGN);
    sigset(SIGINT, handleSigint);
    bool is_debug = (argc == 3 && strcmp("-d", argv[2]) == 0);
    int port;
    try {
        port = std::stoi(argv[1]);
    } catch (std::exception &exc) {
        std::cerr << exc.what() << std::endl;
        return EXIT_FAILURE;
    }
    proxy = new Proxy(is_debug);
    proxy->start(port);
    delete proxy;

    std::cout << "Work finished" << std::endl;
    return EXIT_SUCCESS;
}