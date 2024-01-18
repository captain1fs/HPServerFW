#include "./hook.h"
#include "./fiber.h"
#include "./scheduler.h"
#include "./iomanager.h"
#include "./log.h"
#include "./fd_manager.h"

#include <dlfcn.h>

static windgent::Logger::ptr g_logger = LOG_NAME("system");

namespace windgent {

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

struct _HOOKIniter {
    _HOOKIniter() {
        hook_init();
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
    //sockfd是否已经close
    if(ctx->isClose()) {
        errno = EBADF;
        return -1;
    }
    //若不是socket或者是用户设置的非阻塞
    if(!ctx->m_isSocket || ctx->getUserNonblock()) {
        return func(fd, std::forward<Args>(args)...);
    }

    //获取socket的超时时间
    uint64_t to = ctx->getTimeout(timeout_type);            
    std::shared_ptr<timer_cond> tcond(new timer_cond);

retry:
    //尝试执行func，若返回值 != -1，则成功直接返回n
    ssize_t n = func(fd, std::forward<Args>(args)...);
    //若出错且因为中断，继续尝试
    while(n == -1 && errno == EINTR) {
        n = func(fd, std::forward<Args>(args)...);
    }
    //阻塞了，需要添加定时器，转化为异步行为
    if(n == -1 && errno == EAGAIN) {
        windgent::IOManager* iom = windgent::IOManager::GetThis();
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
                //取消事件，若事件存在，则触发事件执行，也就是去做IO操作
                iom->cancelEvent(fd, (windgent::IOManager::Event)event);
            }, wcond);
        }
        //没有超时事件，添加一个IO事件。因为cb为空，则会以当前协程为唤醒对象
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
            //添加定时器成功，将当前协程切换至后台，设为HOLD状态。当有数据到来时，此处可以唤醒上面cancelEvent和addEvent
            windgent::Fiber::YieldToHold();
            //当协程返回后，若定时器还存在，将其取消
            if(timer) {
                timer->cancel();
            }
            //若条件已经被cancel，则设置下错误码并返回-1
            if(tcond->cancelled) {
                errno = tcond->cancelled;
                return -1;
            }
            //上面的事件只是通知有数据到来，因此需要继续尝试去读取，该过程又是上面的一个循环
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
    iom->addTimer(seconds * 1000, [fiber, iom]() {
        iom->schedule(fiber);
    });
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
    iom->addTimer(usec / 1000, [fiber, iom]() {
        iom->schedule(fiber);
    });
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
    iom->addTimer(timeout_ms, [fiber, iom]() {
        iom->schedule(fiber);
    });
    windgent::Fiber::YieldToHold();
    return 0;
}

//socket
int socket(int domain, int type, int protocol);

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

//read
ssize_t read(int fd, void *buf, size_t count);

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t recv(int sockfd, void *buf, size_t len, int flags);

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);

//write
ssize_t write(int fd, const void *buf, size_t count);

ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

ssize_t send(int sockfd, const void *buf, size_t len, int flags);

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                        const struct sockaddr *dest_addr, socklen_t addrlen);

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);

int close(int fd);

int fcntl(int fd, int cmd, ... /* arg */ );

int ioctl(int fd, unsigned long request, ...);

int getsockopt(int sockfd, int level, int optname,
                      void *optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen);

}