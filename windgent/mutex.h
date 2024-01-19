#ifndef __MUTEX_H_
#define __MUTEX_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>

#include "./noncopyable.h"

namespace windgent {

//封装POSIX条件变量
class Cond : NonCopyable {
public:
    Cond();
    ~Cond();

    void wait();
    void signal();
    void broadcast();
private:
    pthread_cond_t m_cond;
    pthread_mutex_t m_mtx;
};

//封装POSIX信号量
class Semaphore : NonCopyable {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();  //P操作
    void notify();  //V操作
private:
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

class Mutex : NonCopyable {
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

class NullMutex : NonCopyable {
public:
    typedef windgent::ScopedLockImpl<NullMutex> Lock;
    NullMutex() { }
    ~NullMutex() { }

    void lock() { }
    void unlock() { }
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
class RWMutex : NonCopyable {
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

class NullRWMutex : NonCopyable {
public:
    typedef windgent::RdScopedLockImpl<NullRWMutex> RdLock;
    typedef windgent::WrScopedLockImpl<NullRWMutex> WrLock;
    NullRWMutex() { }

    ~NullRWMutex() { }

    void rdlock() { }
    void wrlock() { }
    void unlock() { }
};

//自旋锁：与互斥锁有点类似，只是自旋锁不会引起调用者睡眠，如果自旋锁已经被别的执行单元保持，调用者就一直循环在那里看是否该自旋锁的保持者已经释放了锁，“自旋锁”的作用是为了解决某项资源的互斥使用。
//因为自旋锁不会引起调用者睡眠，所以自旋锁的效率远高于互斥锁。
//自旋锁的不足之处：（1）自旋锁一直占用着CPU，他在未获得锁的情况下，一直运行（自旋），所以占用着CPU，如果不能在很短的时间内获得锁，这无疑会使CPU效率降低。
//（2）在用自旋锁时有可能造成死锁，当递归调用时有可能造成死锁，调用有些其他函数也可能造成死锁，如 copy_to_user()、copy_from_user()、kmalloc()等。
//自旋锁适用于锁使用者保持锁时间比较短的情况下。
class SpinLock : NonCopyable {
public:
    typedef ScopedLockImpl<SpinLock> Lock;
    SpinLock() {
        pthread_spin_init(&m_spinlock, 0);
    }

    ~SpinLock() {
        pthread_spin_destroy(&m_spinlock);
    }

    void lock() {
        pthread_spin_lock(&m_spinlock);
    }
    void unlock() {
        pthread_spin_unlock(&m_spinlock);
    }
private:
    pthread_spinlock_t m_spinlock;
};

//CAS锁：Compare And Swap的缩写，也就是比较和替换，这也正是它的核心。CAS机制中用到了三个基本操作数，内存地址V，旧预期值A，新预期值B。
//当我们需要对一个变量进行修改时，会对内存地址V和旧预期值进行比较，如果两者相同，则将旧预期值A替换成新预期值B。而如果不同，则将V中的值作为旧预期值，继续重复以上操作，即自旋。
class CASLock : NonCopyable {
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() {
        m_mutex.clear();
    }
    ~CASLock() { }

    void lock() {
        while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));  //在获取到锁之前自旋
    }
    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;
};

}

#endif