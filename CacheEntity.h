#ifndef SINGLETHREADPROXY_CACHEENTITY_H
#define SINGLETHREADPROXY_CACHEENTITY_H

#include <iostream>
#include <vector>

#include "Logger.h"
#include "AtomicInt.h"

#define BUFFER_SIZE (BUFSIZ * 5)

class CacheEntity {
private:
    std::string TAG;
    std::vector<char> data;
    bool is_full = false;
    Logger *logger;
    bool is_valid = true;
    AtomicInt *subscribers_counter;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned long countOutputSize(unsigned long pos);

public:

    bool isValid();

    void setInvalid();

    void prepareForStop();

    bool hasSubscribers();

    void remake();

    CacheEntity(const std::string &url, bool is_debug);

    ~CacheEntity();

    const char *getPart(unsigned long start, unsigned long& length, std::vector<char> &targetVector);

    size_t getRecordSize();

    bool isFull();

    bool expandData(const char *newData, size_t len);

    void subscribe();

    void setFull();

    void unsubscribe();
};


#endif //SINGLETHREADPROXY_CACHEENTITY_H
