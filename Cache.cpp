#include "Cache.h"

/**
 * @return On success - Entity, otherwise - nullptr
 **/
CacheEntity *Cache::getEntity(const std::string &url) {
    if (!is_valid->equals(0)) {
        return nullptr;
    }
    if (cached_data.contains(url)) {
        auto data = cached_data.get(url);
        if (!data->isValid()) {
            return nullptr;
        }

        return data;
    } else {
        return nullptr;
    }
}

CacheEntity *Cache::createEntity(const std::string &url) {
    if (!is_valid->equals(0)) {
        return nullptr;
    }
    try {
        if (cached_data.contains(url)) {
            auto data = cached_data.get(url);
            data->remake();
            return data;
        }
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
    delete logger;
    delete is_valid;
}

void Cache::setAllInvalid() {
    is_valid->inc();
    cached_data.lock();
    auto &data = cached_data.getMap();
    for (auto &entry: data) {
        entry.second->prepareForStop();
    }
    cached_data.unlock();
}
