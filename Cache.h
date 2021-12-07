#ifndef SINGLETHREADPROXY_CACHE_H
#define SINGLETHREADPROXY_CACHE_H

#include <iostream>
#include <map>
#include "CacheEntity.h"

class Cache {
private:
    std::map<std::string, CacheEntity*> cached_data;
    Logger *logger;
    std::string TAG;
public:

    Cache(bool is_debug);

    ~Cache();

    CacheEntity *getEntity(const std::string& url);

    CacheEntity *createEntity(const std::string &url);

    void getUpdatedSubs(std::vector<int> &subs);

};


#endif //SINGLETHREADPROXY_CACHE_H
