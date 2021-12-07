#!/bin/bash
#compilation script for Solaris
g++ -std=c++11 -lsocket -lnsl -g -o proxy.out main.cpp Cache.cpp CacheEntity.cpp Client.cpp Logger.cpp Proxy.cpp Server.cpp http_parser.c