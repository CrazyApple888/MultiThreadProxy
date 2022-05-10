#include <iostream>
#include <csignal>
#include "Proxy.h"


/// $ prog_name port is_debug
int main(int argc, char *argv[]) {
    sigset(SIGPIPE, SIG_IGN);
    bool is_debug = (argc == 3 && strcmp("-d", argv[2]) == 0);
    int port;
    try {
        port = std::stoi(argv[1]);
    } catch (std::exception &exc) {
        std::cerr << exc.what() << std::endl;
        return EXIT_FAILURE;
    }
    auto *proxy = new Proxy(is_debug);
    proxy->start(port);
    delete proxy;

    std::cout << "Proxy finished" << std::endl;
    return EXIT_SUCCESS;
}