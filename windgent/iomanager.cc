#include "./iomanager.h"
#include "./macro.h"
#include "./log.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

namespace windgent {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    :Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(500);
    ASSERT(m_epfd > 0);

    int ret = pipe(m_tickleFds);
    ASSERT(ret == 0);

    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    ret = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);   //ET模式下要设为非阻塞
    ASSERT(ret == 0);
    //将m_tickleFds[0]添加到m_epfd的内部列表，监听m_tickleFds[0]上的读事件。
    //如果m_tickleFds[0]上有读事件到来，epoll_wait就会返回
    ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    ASSERT(ret == 0);

    contextResize(32);

    start();
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0;i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);
    for(size_t i = 0;i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            ASSERT2(false, "getContext");
    }
}

void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    // std::cout << "--------- IOManager::FdContext::triggerEvent() ---------" << std::endl;
    ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    //触发事件的执行
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    //先拿到fd对应的上下文对象
    FdContext* fd_ctx = nullptr;
    RWMutexType::RdLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WrLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }
    //修改事件上下文
    FdContext::MutexType::Lock lock3(fd_ctx->mutex);
    //同⼀个fd不允许重复添加相同的事件
    if(fd_ctx->events & event) {
        LOG_ERROR(g_logger) << "addEvent assert fd= " << fd << ", event= " << event
                            << ", fd_ctx.event= " << fd_ctx->events;
        ASSERT(!(fd_ctx->events & event));
    }
    //若fd_ctx->events为0，表示添加新事件
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;
    //为fd注册新事件
    int ret = epoll_ctl(m_epfd, op, fd, &epevent);
    if(ret) {
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd 
                            << ", " << epevent.events << "): " << ret << " (" << errno << ")"
                            << strerror(errno) << ")";
        return -1;
    }

    ++m_pendingEventCount;
    //找到这个fd的event事件对应的EventContext，对其中的scheduler, cb, fiber进⾏赋值
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    // 赋值scheduler和回调函数，如果回调函数为空，则把当前协程当成回调执⾏体
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::RdLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //fd上无此事件
    if(!(fd_ctx->events & event)) {
        return false;
    }

    Event new_event = (Event)(fd_ctx->events & ~event); //从fd_ctx->events中减去event对应的事件
    // std::cout << "new_event = " << new_event << std::endl;
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;

    int ret = epoll_ctl(m_epfd, op, fd, &epevent);
    if(ret) {
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd 
                            << ", " << epevent.events << "): " << ret << " (" << errno << ")"
                            <<strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    //重置FdContext
    fd_ctx->events = new_event;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::RdLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //fd上无此事件
    if(!(fd_ctx->events & event)) {
        return false;
    }
    //从events中删除这个事件，如果还有事件剩余，需要修改
    Event new_event = (Event)(fd_ctx->events & ~event); 
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;

    int ret = epoll_ctl(m_epfd, op, fd, &epevent);
    if(ret) {
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd 
                            << ", " << epevent.events << "): " << ret << " (" << errno << ")"
                            <<strerror(errno) << ")";
        return false;
    }
    //触发执行此事件
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
     RWMutexType::RdLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //fd上无事件
    if(!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int ret = epoll_ctl(m_epfd, op, fd, &epevent);      //不再监听fd
    if(ret) {
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd 
                            << ", " << epevent.events << "): " << ret << " (" << errno << ")"
                            <<strerror(errno) << ")";
        return false;
    }
    //触发全部已注册的事件
    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    //如果没有空闲的线程，不需要发通知
    if(!hasIdleThreads()) {
        return;
    }
    // std::cout << "---------- IOManager::tickle() -----------" << std::endl;
    //如果有空闲线程，往m_tickleFds写端写入一个字符，epoll_wait就会被唤醒
    int ret = write(m_tickleFds[1], "T", 1);
    ASSERT(ret == 1);
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getTimeOfNextTimer();
    return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

//IO调度器无任务时执行idle函数，阻塞在epoll_wait上等待着IO事件到来。idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件就绪或超时
//调度器⽆调度任务时会阻塞idle协程上，对IO调度器⽽⾔，idle状态应该关注两件事，⼀是有没有新的调度任务，对应Schduler::schedule()，如果有新的调度任务，那应该⽴即退出idle状态，并执⾏对应的任务；
//⼆是关注当前注册的所有IO事件有没有触发，如果有触发，那么应该执⾏
void IOManager::idle() {
    LOG_INFO(g_logger) << "IOManager::idle()";
    const uint64_t MAX_EVENTS = 64;
    epoll_event* events = new epoll_event[MAX_EVENTS]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {
        delete[] ptr;
    });

    while(true) {
        uint64_t next_timeout = 0;
        //next_timeout每次都会重置为：到下一定时器任务执行要等待的时间
        if(stopping(next_timeout)) {
            LOG_INFO(g_logger) << "name= " << getName() << ", idle stopping exit";
            break;
        }

        int ret = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            //next_timeout是到下一定时器执行要等待的时间，因此epoll_wait等待next_timeout后会立即返回，从而跳出do...while
            //去执行超时的定时器任务，而不需要等待MAX_TIMEOUT
            //三种情况能唤醒epoll_wait：1：超时时间到了 2：socket上有数据到来  3：tickle往pipe中写入了1，通知有任务来了
            ret = epoll_wait(m_epfd, events, 64, (int)next_timeout);
            // LOG_INFO(g_logger) << "epoll_wait ret = " << ret;
            if(ret < 0 && errno == EINTR) {
                ;
            } else {
                break;
            }
        }while(true);

        //获取超时的定时器任务
        std::vector<std::function<void()> > cbs;
        listExpiredCbs(cbs);
        //如果有超时的定时器任务，全部加入任务队列中调度
        if(!cbs.empty()) {
            // std::cout << "--------- schedule(cbs.begin(), cbs.end()) ---------" << std::endl;
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }
        //遍历所有发⽣的事件，根据epoll_event的私有指针找到对应的FdContext，进⾏事件处理
        for(int i = 0;i < ret; ++i) {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                //若该fd是pipefd，将发来的1读取完
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                // std::cout << "dummy[0] = " << dummy[0] << std::endl;
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }

            int real_event = NONE;
            if(event.events & EPOLLIN) {
                real_event |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_event |= WRITE;
            }
            //fd_ctx->events是通过addEvent添加的事件，而real_event是从fd上监听到的事件
            if((fd_ctx->events & real_event) == NONE) {
                continue;
            }
            //剔除已经发⽣的事件，将剩下的事件重新加⼊epoll_wait，如果剩下的事件为0，表示这个fd已经不需要关注了，直接从epoll中删除
            int left_events = (fd_ctx->events & ~real_event);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;
            int ret2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(ret2) {
                LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd_ctx->fd 
                            << ", " << event.events << "): " << ret2 << " (" << errno << ")"
                            <<strerror(errno) << ")";
                continue;  
            }
            // 处理已经发⽣的事件，也就是让调度器调度指定的函数或协程
            if(real_event & READ) {
                // std::cout << "--------- fd_ctx->triggerEvent(READ) ---------" << std::endl;
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_event & WRITE) {
                // std::cout << "--------- fd_ctx->triggerEvent(WRITE) ---------" << std::endl;
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        //一旦处理完所有的事件，idle协程yield，这样可以让调度协程(Scheduler::run)重新检查是否有新任务要调度
        //上⾯triggerEvent实际也只是把对应的fiber重新加⼊调度，要执⾏的话还要等idle协程退出
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();   //回到调度器的主协程
    }
}

void IOManager::onTimerInsertedAtFront() {
    // std::cout << "IOManager::onTimerInsertedAtFront()" << std::endl;
    tickle();
}

}