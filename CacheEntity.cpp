#include "CacheEntity.h"

void CacheEntity::getPart(unsigned long start, unsigned long &length, std::vector<char> &target_vector) {
    pthread_mutex_lock(&mutex);
    length = countOutputSize(start);
    while (length <= 0 && !is_full && is_valid) {
        pthread_cond_wait(&cond, &mutex);
        length = countOutputSize(start);
    }
    target_vector.insert(target_vector.begin(), data.begin() + start, data.begin() + start + length);
    pthread_mutex_unlock(&mutex);
}

unsigned long CacheEntity::countOutputSize(unsigned long pos) {
    auto size = data.size() - pos;
    return size > BUFFER_SIZE ? BUFFER_SIZE : size;
}

bool CacheEntity::isFull() {
    pthread_mutex_lock(&mutex);
    auto _is_full = is_full;
    pthread_mutex_unlock(&mutex);

    return _is_full;
}

size_t CacheEntity::getRecordSize() {
    pthread_mutex_lock(&mutex);
    auto _size = data.size();
    pthread_mutex_unlock(&mutex);

    return _size;
}

/**
 * @param newData - part of data
 * @return true on success, false on bad_alloc
 */
bool CacheEntity::expandData(const char *newData, size_t len) {
    pthread_mutex_lock(&mutex);
    if (!is_valid) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    try {
        data.insert(data.end(), newData, newData + len);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
        return true;
    } catch (std::bad_alloc &exc) {
        logger->info(TAG, "bad alloc");
        is_valid = false;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
        return false;
    }
}

CacheEntity::CacheEntity(const std::string &url, bool is_debug) : logger(new Logger(is_debug)) {
    this->TAG = std::string("CacheEntity ") + url;
    subscribers_counter = new AtomicInt(0);
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
    logger->debug(TAG, "created");
}

void CacheEntity::subscribe() {
    subscribers_counter->inc();
}

void CacheEntity::setFull() {
    pthread_mutex_lock(&mutex);
    is_full = true;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

void CacheEntity::unsubscribe() {
    subscribers_counter->dec();
    pthread_mutex_lock(&mutex);
    if (subscribers_counter->equals(0) && status_code != 200u) {
        is_valid = false;
        logger->info(TAG, "Response status != 200, setting status invalid for cache");
    }
    pthread_mutex_unlock(&mutex);
}

bool CacheEntity::isValid() {
    pthread_mutex_lock(&mutex);
    auto _is_valid = is_valid;
    pthread_mutex_unlock(&mutex);

    return _is_valid;
}

void CacheEntity::setInvalid() {
    pthread_mutex_lock(&mutex);
    this->is_valid = false;
    pthread_mutex_unlock(&mutex);
}

CacheEntity::~CacheEntity() {
    data.clear();
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    delete logger;
    delete subscribers_counter;
}

bool CacheEntity::hasSubscribers() {
    return subscribers_counter->get() > 0;
}

void CacheEntity::remake() {
    pthread_mutex_lock(&mutex);
    data.clear();
    this->is_valid = true;
    pthread_mutex_unlock(&mutex);
}

void CacheEntity::prepareForStop() {
    pthread_mutex_lock(&mutex);
    is_valid = false;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

void CacheEntity::setStatus(unsigned int _status) {
    pthread_mutex_lock(&mutex);
    status_code = _status;
    pthread_mutex_unlock(&mutex);
}
