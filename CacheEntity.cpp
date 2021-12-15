#include "CacheEntity.h"

const char *CacheEntity::getPart(unsigned long start, unsigned long &length) {
    pthread_mutex_lock(&mutex);
    length = countOutputSize(start);
    while (length <= 0 && !is_full && is_valid) {
        pthread_cond_wait(&cond, &mutex);
        length = countOutputSize(start);
    }
    auto _data = data.data() + start;
    pthread_mutex_unlock(&mutex);

    return _data;
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
    try {
        data.insert(data.end(), newData, newData + len);
        _isUpdated = true;
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
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
    logger->debug(TAG, "created");
}

void CacheEntity::subscribe(int soc) {
    pthread_mutex_lock(&mutex);
    subscribers.push_back(soc);
    pthread_mutex_unlock(&mutex);
    if (is_full) {
        //notifySubscribers();
    }
}

void CacheEntity::setFull() {
    pthread_mutex_lock(&mutex);
    is_full = true;
    pthread_mutex_unlock(&mutex);
}

void CacheEntity::unsubscribe(int soc) {
    pthread_mutex_lock(&mutex);
    auto iter = subscribers.begin();
    for (; iter != subscribers.end(); iter++) {
        if (soc == (*iter)) {
            subscribers.erase(iter);
            logger->debug(TAG, std::to_string(soc) + " is now unsub");
            break;
        }
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
    subscribers.clear();
    delete logger;
}

std::vector<int> &CacheEntity::getSubscribers() {
    pthread_mutex_lock(&mutex);
    _isUpdated = false;
    pthread_mutex_unlock(&mutex);
    return subscribers;
}

bool CacheEntity::isUpdated() {
    pthread_mutex_lock(&mutex);
    if (!subscribers.empty() && (is_full || _isUpdated)) {
        pthread_mutex_unlock(&mutex);
        return true;
    } else {
        pthread_mutex_unlock(&mutex);
        return false;
    }
}
