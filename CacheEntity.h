#ifndef SINGLETHREADPROXY_CACHEENTITY_H
#define SINGLETHREADPROXY_CACHEENTITY_H

#include <iostream>
#include <vector>

#include "Logger.h"

class CacheEntity {
private:
    std::string TAG;
    std::vector<char> data;
    bool is_full = false;
    Logger *logger;
    std::vector<int> subscribers;
    bool is_valid = true;
    bool _isUpdated = false;

public:

    std::vector<int> &getSubscribers();

    bool isUpdated();

    bool isValid() const;

    void setInvalid();

    CacheEntity(const std::string &url, bool is_debug);

    ~CacheEntity();

    const char *getPart(unsigned long start, unsigned long length);

    size_t getRecordSize();

    bool isFull() const;

    bool expandData(const char *newData, size_t len);

    void subscribe(int soc);

    void setFull();

    void unsubscribe(int soc);
};


#endif //SINGLETHREADPROXY_CACHEENTITY_H
