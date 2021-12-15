#ifndef SINGLETHREADPROXY_CACHE_H
#define SINGLETHREADPROXY_CACHE_H

#include <iostream>
#include <map>
#include "CacheEntity.h"
#include "ConcurrentMap.h"

class Cache {
private:
    ConcurrentMap<std::string, CacheEntity*> cached_data;
    Logger *logger;
    std::string TAG;
public:

    Cache(bool is_debug);

    ~Cache();

    CacheEntity *getEntity(const std::string& url);

    CacheEntity *createEntity(const std::string &url);

};


#endif //SINGLETHREADPROXY_CACHE_H
