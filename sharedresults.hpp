#ifndef SHAREDRESULTS_HPP
#define SHAREDRESULTS_HPP

#include <map>

class SharedResults {
    struct write_lock {
    public:
        explicit write_lock(pthread_rwlock_t *lock) : lock(lock) {
                pthread_rwlock_wrlock(lock);
        }

        ~write_lock() {
            pthread_rwlock_unlock(lock);
        }

    private:
        pthread_rwlock_t *lock;
    };

    struct read_lock {
    public:
        explicit read_lock(pthread_rwlock_t *lock) : lock(lock) {
            pthread_rwlock_rdlock(lock);
        }

        ~read_lock() {
            pthread_rwlock_unlock(lock);
        }

    private:
        pthread_rwlock_t *lock;
    };

public:

    bool put(InfInt n, uint64_t result) {
        write_lock wrlock(&this->lock);
        if (results.contains(n)) {
            return false;
        }
        results[n] = result;
        return true;
    }

    bool get(InfInt n, uint64_t &result) {
        read_lock rdlock(&this->lock);
        if (!results.contains(n)) {
            return false;
        }
        std::cout << "HURAAA!!" << "\n";
        result = results[n];
        return true;
    }

private:
    pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
    std::map<InfInt, uint64_t> results;
};

#endif