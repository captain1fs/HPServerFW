#include "mutex.h"
#include<iostream>

namespace windgent {

Cond::Cond() {
    if(pthread_mutex_init(&m_mtx, nullptr)) {
        throw std::logic_error("pthread_mutex_init error");
    }
    if(pthread_cond_init(&m_cond, nullptr)) {
        throw std::logic_error("pthread_cond_init error");
    }
}

Cond::~Cond() {
    pthread_mutex_destroy(&m_mtx);
    pthread_cond_destroy(&m_cond);
}

void Cond::wait() {
    if(pthread_cond_wait(&m_cond, &m_mtx)) {
        throw std::logic_error("pthread_cond_wait error");
    }
}
void Cond::signal() {
    if(pthread_cond_signal(&m_cond)) {
        throw std::logic_error("pthread_cond_signal error");
    }
}
void Cond::broadcast() {
    if(pthread_cond_broadcast(&m_cond)) {
        throw std::logic_error("pthread_cond_broadcast error");
    }
}

Semaphore::Semaphore(uint32_t count) {
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}


}