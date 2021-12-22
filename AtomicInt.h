#ifndef MULTITHREADPROXY_ATOMICINT_H
#define MULTITHREADPROXY_ATOMICINT_H

#include <iostream>

class AtomicInt {
private:
    int value;
    pthread_mutex_t mutex;
public:
    AtomicInt(int initial_value) {
        pthread_mutex_init(&mutex, nullptr);
        this->value = initial_value;
    }

    ~AtomicInt() {
        pthread_mutex_destroy(&mutex);
    }

    void inc() {
        pthread_mutex_lock(&mutex);
        value++;
        pthread_mutex_unlock(&mutex);
    }

    void dec() {
        pthread_mutex_lock(&mutex);
        value--;
        pthread_mutex_unlock(&mutex);
    }

    int get() {
        pthread_mutex_lock(&mutex);
        auto tmp = value;
        pthread_mutex_unlock(&mutex);

        return tmp;
    }

    bool equals(int other) {
        pthread_mutex_lock(&mutex);
        auto tmp = (value == other);
        pthread_mutex_unlock(&mutex);

        return tmp;
    }
};

#endif //MULTITHREADPROXY_ATOMICINT_H
