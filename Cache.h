#ifndef MULTITHREADPROXY_CACHE_H
#define MULTITHREADPROXY_CACHE_H

#include <iostream>
#include <map>
#include "CacheEntity.h"
#include "ConcurrentMap.h"

class Cache {
private:
    ConcurrentMap<std::string, CacheEntity*> cached_data;
    Logger *logger;
    std::string TAG;
    AtomicInt *is_valid = new AtomicInt(0);
public:

    Cache(bool is_debug);

    ~Cache();

    CacheEntity *getEntity(const std::string& url);

    CacheEntity *createEntity(const std::string &url);

    void setAllInvalid();

};


#endif //MULTITHREADPROXY_CACHE_H
