#include "./hook.h"
#include "./fiber.h"
#include "./scheduler.h"
#include "./iomanager.h"
#include "./log.h"
#include "./fd_manager.h"
#include "./config.h"

#include <dlfcn.h>
#include <stdarg.h>

static windgent::Logger::ptr g_logger = LOG_NAME("system");
static windgent::ConfigVar<int>::ptr g_tcp_connect_timeout = 
        windgent::ConfigMgr::Lookup<int>("tcp.connect.timeout", 5000, "tcp connect timeout");

namespace windgent {

//线程局部变量t_hook_enable，⽤于表示当前线程是否启⽤hook，使⽤线程局部变量表示hook模块是线程粒度的，各个线程可单独启用或关闭
static thread_local bool t_hook_enable = false;

#define HOOK_FUNC(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if(is_inited){
        return;
    }
//找到name对应的系统函数，初始化给name_f
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUNC(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HOOKIniter {
    _HOOKIniter() {
        hook_init();
        //从配置文件中获取connect的超时时间，并监听器是否改变
        s_connect_timeout = g_tcp_connect_timeout->getVal();
        g_tcp_connect_timeout->addListener([](const int& old_val, const int& new_val){
            LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_val << " to " << new_val;
            s_connect_timeout = new_val;
        });
    }
};

static _HOOKIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}
void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

//用于条件定时器的条件
struct timer_cond {
    int cancelled = 0;
};

//IO相关的API不仅需要添加定时器，还需要注册事件
template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun func, const char* hook_func_name, uint32_t event,
                     int timeout_type, Args&&... args) {
    if(!windgent::t_hook_enable) {
        return func(fd, std::forward<Args>(args)...);
    }

    windgent::FdCtx::ptr ctx = windgent::FdMgr::GetInstance()->get(fd);
    //若干ctx为空，表示fd不是socket fd，则调用原系统函数
    if(!ctx) {
        return func(fd, std::forward<Args>(args)...);
    }
    //若sockfd已经close
    if(ctx->isClosed()) {
        errno = EBADF;
        return -1;
    }
    //若不是socket或者是用户设置的非阻塞
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return func(fd, std::forward<Args>(args)...);
    }

    //获取socket的超时时间
    uint64_t to = ctx->getTimeout(timeout_type);            
    std::shared_ptr<timer_cond> tcond(new timer_cond);

retry:
    //尝试执行func，若返回值 != -1，则成功直接返回n
    ssize_t n = func(fd, std::forward<Args>(args)...);
    /*
        accept、connect、recv、recvfrom、send、sendto等这些从socket读写的函数，如果socket上没有可得数据，它们会一直等待,即阻塞。
        除非socket被设为非阻塞，在这种情况下，这些api会立即返回-1且erno被设置为EAGAIN
    */

    //若出错且func过程被中断，继续尝试读/写数据
    while(n == -1 && errno == EINTR) {
        n = func(fd, std::forward<Args>(args)...);
    }
    //从上可知，当返回值为-1且errno=EAGAIN，表示socket上还没有数据到来，需要添加定时器，转化为异步行为
    if(n == -1 && errno == EAGAIN) {
        windgent::IOManager* iom = windgent::IOManager::GetThis();  //当前线程所在的IOManager
        windgent::Timer::ptr timer;
        std::weak_ptr<timer_cond> wcond(tcond);     //条件
        //有超时时间，添加条件定时器
        if(to != (uint64_t)-1) {
            timer = iom->addCondTimer(to, [wcond, iom, fd, event](){
                auto t = wcond.lock();
                //若没有条件或者条件设置为错误，直接返回
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                //取消事件，若事件存在，则触发事件执行
                iom->cancelEvent(fd, (windgent::IOManager::Event)event);
            }, wcond);
        }

        //将fd及其事件纳入监听。因为cb为空，则会以当前协程为回调参数
        int ret = iom->addEvent(fd, (windgent::IOManager::Event)event);
        //添加事件失败
        if(ret) {
            LOG_ERROR(g_logger) << hook_func_name << ", addEvent(" << fd << ", " << event << ")";
            //取消定时器
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            //添加事件成功，当前协程让出执行权。两种被唤醒情况：
            //1.如果addEvent后在超时时间内，fd上有事件到来表示socket有数据可读写。那么取消定时器，若不取消，定时器任务cancelEvent会被执行（无意义了）。然后再次尝试读写socket上的数据；
            //2.addCondTimer添加的定时器超时了，执行定时任务cancelEvent，也会唤醒YieldToHold。因为在定时器的回调函数中tcond->cancelled被设置为ETIMEDOUT，则if条件通过，返回-1表示IO操作发生超时错误。
            windgent::Fiber::YieldToHold();
            //当从addEvent唤醒后，若定时器还存在，将其取消
            if(timer) {
                timer->cancel();
            }
            //若cancelled有值，说明是通过定时器任务cancelEvent唤醒的，说明已经超时且socket上还是没有数据可以读写
            if(tcond->cancelled) {
                errno = tcond->cancelled;
                return -1;
            }
            //若tcond不成立，说明有IO事件到来，需要重新去读/写
            goto retry;
        }
    }
    return n;       
}

extern "C" {

#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUNC(XX);
#undef XX

//实现自己的sleep函数
unsigned int sleep(unsigned int seconds) {
    //若没有hook，使用系统函数
    if(!windgent::is_hook_enable()) {
        return sleep_f(seconds);
    }
    //本来sleep会使线程阻塞，但下面code将其转换成了一个定时器任务加入调度，然后协程让出了执行权，线程可以继续运行
    //当到达定时器执行时刻时，执行其回调函数。这样表现出协程sleep了，而且线程也没有一直阻塞。
    windgent::Fiber::ptr fiber = windgent::Fiber::GetThis();
    windgent::IOManager* iom = windgent::IOManager::GetThis();
    //模板方法bind的时候需要声明模板类型和方法参数
    iom->addTimer(seconds * 1000, std::bind((void(windgent::Scheduler::*)
                 (windgent::Fiber::ptr, int thread))&windgent::IOManager::schedule, iom, fiber, -1));
    windgent::Fiber::YieldToHold();
    // fiber->YieldToHold();
    // std::cout << "------- after YieldToHold() ---------" << std::endl;
    return 0;
}

int usleep(useconds_t usec) {
    //若没有hook，使用系统函数
    if(!windgent::is_hook_enable()) {
        return usleep_f(usec);
    }

    windgent::Fiber::ptr fiber = windgent::Fiber::GetThis();
    windgent::IOManager* iom = windgent::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(windgent::Scheduler::*)
                 (windgent::Fiber::ptr, int thread))&windgent::IOManager::schedule, iom, fiber, -1));
    windgent::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
      //若没有hook，使用系统函数
    if(!windgent::is_hook_enable()) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    windgent::Fiber::ptr fiber = windgent::Fiber::GetThis();
    windgent::IOManager* iom = windgent::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(windgent::Scheduler::*)
                 (windgent::Fiber::ptr, int thread))&windgent::IOManager::schedule, iom, fiber, -1));
    windgent::Fiber::YieldToHold();
    return 0;
}

//socket
int socket(int domain, int type, int protocol) {
    if(!windgent::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    //为socket fd创建上下文
    windgent::FdMgr::GetInstance()->get(fd, true);
    // std::cout << "---------- use hook socket() ----------" << std::endl;
    return fd;
}

int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timerout_ms) {
    if(!windgent::t_hook_enable) {
        return connect_f(sockfd, addr, addrlen);
    }
    windgent::FdCtx::ptr ctx = windgent::FdMgr::GetInstance()->get(sockfd);
    if(!ctx || ctx->isClosed()) {
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket()) {
        return connect_f(sockfd, addr, addrlen);
    }
    //不理会用户设置的O_NONBLOCK
    if(ctx->getUserNonblock()) {
        return connect_f(sockfd, addr, addrlen);
    }

    int n = connect_f(sockfd, addr, addrlen);
    if(0 == n) {
        return 0;   //connect成功
    } else if(n != -1 || errno != EINPROGRESS) {
        return n;
    }

    //处理connect阻塞的情况：返回值为-1且errno = EINPROGRESS (sockfd被设置为O_NONBLOCK)
    windgent::IOManager* iom = windgent::IOManager::GetThis();
    windgent::Timer::ptr timer;
    std::shared_ptr<timer_cond> tcond(new timer_cond);
    std::weak_ptr<timer_cond> wcond(tcond);

    //connect存在超时时间，则添加一个条件定时器，在定时时间到后通过t->cancelled设置超时标志并触发⼀次WRITE事件。然后添加WRITE事件并yield，等待WRITE事件触发再往下执⾏。
    if(timerout_ms != (uint64_t)-1) {
        //添加定时器，cb：超时后取消事件(如果有事件就触发)
        timer = iom->addCondTimer(timerout_ms, [wcond, iom, sockfd]() {
            auto t = wcond.lock();
            if(!t || t->cancelled) {
                return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(sockfd, windgent::IOManager::WRITE);
        }, wcond);
    }
    //监听fd上的写事件，如果在超时时间内socket上可读写，则立即从YieldToHold出唤醒；如果超时时间到仍然socket没有数据可供读写，
    //则取消事件或者触发，也会从YieldToHold出唤醒，但此时t->cancelled被设为ETIMEDOUT，在if中条件通过会直接返回-1表示错误
    int ret = iom->addEvent(sockfd, windgent::IOManager::WRITE);
    if(0 == ret) {
        //定时器超时后cancelEvent，或者connect连接成功(epoll监听到上面addEvent添加的事件)，都会将YieldToHold从此处唤醒
        windgent::Fiber::YieldToHold();
        //若唤醒后定时器还存在(fd可写)，已经没有意义，取消掉。 取消定时器会导致定时器回调被强制执⾏⼀次，但这并不会导致问题，因为只有当前协程结束后，定时器回调才会在接下来被调度，
        //由于定时器回调被执⾏时connect_with_timeout协程已经执⾏完了，所以理所当然地条件变量wcond也被释放了，所以实际上定时器回调函数什么也没做
        if(timer) {
            timer->cancel();
        }
        //若从cancelEvent唤醒后cancelled有值，说明定时器超时了，socket上还是没有数据，则返回错误
        if(tcond->cancelled) {
            errno = tcond->cancelled;
            return -1;
        }
    } else {
        LOG_ERROR(g_logger) << "connect addEvent(" << sockfd << ", WRITE) error";
        if(timer) {
            timer->cancel();
        }
    }

    //获取SO_ERROR，根据其值来判断connect是否成功(0)
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if(!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // std::cout << "---------- use hook connect() ----------" << std::endl;
    return connect_with_timeout(sockfd, addr, addrlen, windgent::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    //accept会接受sockfd上等待的一个连接请求，并返回一个新的socket fd
    int fd = do_io(sockfd, accept_f, "accept", windgent::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        windgent::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

//read
ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", windgent::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, readv_f, "readv", windgent::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    // std::cout << "---------- use hook recv() ----------" << std::endl;
    return do_io(sockfd, recv_f, "recv", windgent::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", windgent::IOManager::READ, 
                 SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", windgent::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

//write
ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", windgent::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", windgent::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    // std::cout << "---------- use hook send() ----------" << std::endl;
    return do_io(sockfd, send_f, "send", windgent::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                        const struct sockaddr *dest_addr, socklen_t addrlen) {
    return do_io(sockfd, sendto_f, "sendto", windgent::IOManager::WRITE, 
                 SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return do_io(sockfd, sendmsg_f, "sendmsg", windgent::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!windgent::t_hook_enable) {
        return close_f(fd);
    }
    windgent::FdCtx::ptr ctx = windgent::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = windgent::IOManager::GetThis();
        if(iom) {
            //取消/触发fd上的所有事件
            iom->cancelAll(fd);
        }
        windgent::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

//系统和呈现给用户的nonlock状态是不同的，fcntl函数是给用户调用的
int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);  //获取可变参数列表中第一个参数的地址，赋给va
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                windgent::FdCtx::ptr ctx = windgent::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClosed() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                //当用户调用fcntl时，判断是否设置了O_NONBLOCK
                ctx->setUserNonblock(arg & O_NONBLOCK);
                //如果已经由hook设置了O_NONBLOCK，依旧设置O_NONBLOCK，否则删除O_NONBLOCK
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                windgent::FdCtx::ptr ctx = windgent::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClosed() || !ctx->isSocket()) {
                    return arg;
                }
                //如果用户设置了O_NONBLOCK，呈现给用户的arg加上O_NONBLOCK，否则删除
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg); 
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int fd, unsigned long request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(request == FIONBIO) {
        bool user_nonblock = !!*(int*)arg;
        windgent::FdCtx::ptr ctx = windgent::FdMgr::GetInstance()->get(fd);
        if(!ctx || ctx->isClosed() || !ctx->isSocket()) {
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!windgent::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }

    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            windgent::FdCtx::ptr ctx = windgent::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* tv = (const timeval*)optval;
                ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}