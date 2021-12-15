cmake_minimum_required(VERSION 3.21)
project(MultiThreadProxy)

set(CMAKE_CXX_STANDARD 11)

add_executable(MultiThreadProxy main.cpp Logger.cpp Proxy.cpp Cache.cpp http_parser.c CacheEntity.cpp Server.cpp Client.cpp ConcurrentMap.h AtomicInt.h)
