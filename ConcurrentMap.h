#ifndef MULTITHREADPROXY_CONCURRENTMAP_H
#define MULTITHREADPROXY_CONCURRENTMAP_H

#include <iostream>
#include <map>

template<typename K, typename V>
class ConcurrentMap {
private:
    std::map<K, V> data;
    pthread_mutex_t mutex;
public:
    ConcurrentMap() {
        pthread_mutex_init(&mutex, nullptr);
    };

    ~ConcurrentMap() {
        pthread_mutex_destroy(&mutex);
        data.clear();
    };

    bool contains(K key) {
        pthread_mutex_lock(&mutex);
        auto isContains = data.find(key) != data.end();
        pthread_mutex_unlock(&mutex);

        return isContains;
    }

    V get(K key) {
        pthread_mutex_lock(&mutex);
        V value;
        try {
            value = data.at(key);
        } catch (...) {}
        pthread_mutex_unlock(&mutex);

        return value;
    };

    void update(K key, V value) {
        pthread_mutex_lock(&mutex);
        auto item = data.find(key);
        if (item != data.end()) {
            item->second = value;
        }
        pthread_mutex_unlock(&mutex);
    }

    void removeAt(K key) {
        pthread_mutex_lock(&mutex);
        data.erase(key);
        pthread_mutex_unlock(&mutex);
    };

    void put(K key, V value) {
        pthread_mutex_lock(&mutex);
        try {
            data.insert(std::make_pair(key, value));
        } catch (...) {
            std::cerr << "ALO" << std::endl;
        }
        pthread_mutex_unlock(&mutex);
    };

    void clear() {
        pthread_mutex_lock(&mutex);
        data.clear();
        pthread_mutex_unlock(&mutex);
    }

    int lock() {
        return pthread_mutex_lock(&mutex);
    }

    int unlock() {
        return pthread_mutex_unlock(&mutex);
    }

    std::map<K, V> &getMap() {
        return data;
    }

    bool isEmpty() {
        pthread_mutex_lock(&mutex);
        bool _is_empty = data.empty();
        pthread_mutex_unlock(&mutex);

        return _is_empty;
    }

};

#endif //MULTITHREADPROXY_CONCURRENTMAP_H
