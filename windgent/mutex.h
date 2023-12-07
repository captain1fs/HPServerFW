#ifndef __MUTEX_H_
#define __MUTEX_H_

#include <pthread.h>
#include <semaphore.h>

namespace windgent {

//封装POSIX信号量
class Semaphore {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();  //P操作
    void notify();  //V操作
private:
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
private:
    sem_t m_semaphore;
};

//lock_guard
template<typename T>
class ScopedLockImpl {
public:
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }
    ~ScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked = false;
};

class Mutex {
public:
    typedef windgent::ScopedLockImpl<Mutex> Lock;
    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }
    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

//Rd lock_guard
template<typename T>
class RdScopedLockImpl {
public:
    RdScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~RdScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked = false;
};
//Wr lock_guard
template<typename T>
class WrScopedLockImpl {
public:
    WrScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WrScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked = false;
};
//POSIX读写锁的封装
class RWMutex {
public:
    typedef windgent::RdScopedLockImpl<RWMutex> RdLock;
    typedef windgent::WrScopedLockImpl<RWMutex> WrLock;
    RWMutex() {
        pthread_rwlock_init(&m_rwlock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_rwlock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_rwlock);
    }
    void wrlock() {
        pthread_rwlock_wrlock(&m_rwlock);
    }
    void unlock() {
        pthread_rwlock_unlock(&m_rwlock);
    }
private:
    pthread_rwlock_t m_rwlock;
};

}

#endif