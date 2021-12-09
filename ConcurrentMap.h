#ifndef MULTITHREADPROXY_CONCURRENTMAP_H
#define MULTITHREADPROXY_CONCURRENTMAP_H

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

    V get(K key) {
        pthread_mutex_lock(&mutex);
        auto value = data.at(key);
        pthread_mutex_unlock(&mutex);

        return value;
    };

    V get(int index) {
        pthread_mutex_lock(&mutex);
        auto value = data[index];
        pthread_mutex_unlock(&mutex);

        return value;
    }

    void removeAt(K key) {
        pthread_mutex_lock(&mutex);
        data.erase(key);
        pthread_mutex_unlock(&mutex);
    };

    void put(K key, V value) {
        pthread_mutex_lock(&mutex);
        data.insert(std::make_pair(key, value));
        pthread_mutex_unlock(&mutex);
    };

    void clear() {
        pthread_mutex_lock(&mutex);
        data.clear();
        pthread_mutex_unlock(&mutex);
    }

    size_t size() {
        return data.size();
    };

    int lock() {
        return pthread_mutex_lock(&mutex);
    }

    int unlock() {
        return pthread_mutex_unlock(&mutex);
    }

    std::map<K, V> getMap() {
        return data;
    }

};

#endif //MULTITHREADPROXY_CONCURRENTMAP_H
