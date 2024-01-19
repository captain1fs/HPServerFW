#include "./fd_manager.h"
#include "./hook.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace windgent {

FdCtx::FdCtx(int fd):m_isInit(false), m_isSocket(false), m_sysNonblock(false), m_userNonblock(false)
                    ,m_isClosed(false), m_fd(fd), m_recvTimeout(-1), m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx() { }

bool FdCtx::init() {
    if(m_isInit) {
        return true;
    }
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    if(-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }
    if(m_isSocket) {
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        if(!(flags & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v) {
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}
uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_fds.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if(fd == -1) {
        return nullptr;
    }
    RWMutexType::RdLock lock(m_mutex);
    if((int)m_fds.size() <= fd){
        if(!auto_create) {
            return nullptr;
        }
    } else if(m_fds[fd] || !auto_create) {
        return m_fds[fd];
    }
    lock.unlock();

    RWMutexType::WrLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if(fd >= (int)m_fds.size()) {
        m_fds.resize(fd * 1.5);
    }
    m_fds[fd] = ctx;
    return ctx;
}

void FdManager::del(int fd) {
    RWMutexType::WrLock lock(m_mutex);
    if((int)m_fds.size() <= fd){
        return;
    }
    m_fds[fd].reset();
}

}