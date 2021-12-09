#include "Cache.h"

/**
 * @return On success - Entity, otherwise - nullptr
 **/
CacheEntity *Cache::getEntity(const std::string &url) {
    try {
        return cached_data.get(url);
    } catch (std::out_of_range &exc) {
        return nullptr;
    }
}

CacheEntity *Cache::createEntity(const std::string &url) {
    try {
        cached_data.put(url, new CacheEntity(url, logger->isDebug()));
        return cached_data.get(url);
    } catch (std::exception &exc) {
        logger->info(TAG, "Can't create Entity for " + url);
        return nullptr;
    }
}

Cache::Cache(bool is_debug) : logger(new Logger(is_debug)) {
    this->TAG = std::string("Cache");
    logger->debug(TAG, "created");
}

Cache::~Cache() {
    cached_data.lock();
    for (auto &item: cached_data.getMap()) {
        delete item.second;
    }
    cached_data.unlock();
    cached_data.clear();
    delete logger;
}

void Cache::getUpdatedSubs(std::vector<int> &subs) {
    cached_data.lock();
    for (auto &entity : cached_data.getMap()) {
        if (entity.second->isUpdated()) {
            auto &_subs = entity.second->getSubscribers();
            subs.insert(subs.end(), _subs.begin(), _subs.end());
        }
    }
    cached_data.unlock();
}
