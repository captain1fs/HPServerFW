#ifndef __IOMANAGER_H__
#define __IOMANAGER_H__

#include "./scheduler.h"

namespace windgent {

//基于epoll的IO协程调度器
class IOManager : public Scheduler {
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
    //句柄的上下文
    struct FdContext {
        typedef Mutex MutexType;
        //事件的上下文
        struct EventContext {
            Scheduler* scheduler = nullptr;     //事件执行的scheduler
            Fiber::ptr fiber;                   //事件协程
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
        Event events = NONE;        //已经注册的事件
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
    void tickle() override;
    bool stopping() override;
    void idle() override;

    //初始化事件列表
    void contextResize(size_t size);
private:
    int m_epfd = 0;        //epoll fd
    int m_tickleFds[2];   //管道通信

    std::atomic<size_t> m_pendingEventCount = {0};  //等待执行的事件数
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;           //句柄及其事件列表
};

}

#endif