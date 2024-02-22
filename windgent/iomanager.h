#ifndef __IOMANAGER_H__
#define __IOMANAGER_H__

#include "./scheduler.h"
#include "./timer.h"

namespace windgent {

//基于epoll的IO协程调度器
class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    //事件类型
    enum Event {
        NONE = 0x0,
        READ = 0x1,     //EPOLLIN
        WRITE = 0x4,    //EPOLLOUT
    };
private:
    //每个socket fd都对应⼀个FdContext，包括fd的值，fd上的事件，以及fd的读写事件上下⽂
    struct FdContext {
        typedef Mutex MutexType;
        //事件的上下文
        struct EventContext {
            Scheduler* scheduler = nullptr;     //事件执行的scheduler
            Fiber::ptr fiber;                   //事件回调协程
            std::function<void()> cb;           //事件回调函数
        };
        //获取事件上下文
        EventContext& getContext(Event event);
        //重置事件上下文
        void resetContext(EventContext& ctx);
        //触发事件的执行
        void triggerEvent(Event event);

        EventContext read;          //读事件上下文
        EventContext write;         //写事件上下文
        Event events = NONE;        //该fd添加了哪些事件的回调函数，或者说该fd关⼼哪些事件
        int fd = 0;                 //事件关联的句柄
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    //添加事件
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    //删除事件
    bool delEvent(int fd, Event event);
    //取消事件：强制触发执行该事件
    bool cancelEvent(int fd, Event event);
    //取消fd上的所有事件
    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    //往m_tickleFds写端写，用于主动通知线程有协程任务到来，此时epoll_wait会立即返回
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;

    //初始化事件列表
    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);
private:
    int m_epfd = 0;        //epoll fd
    int m_tickleFds[2];    //管道通信

    std::atomic<size_t> m_pendingEventCount = {0};  //等待执行的事件数
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;           //socket事件上下⽂的容器
};

}

#endif