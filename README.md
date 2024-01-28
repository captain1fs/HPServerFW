# High Performance Server FrameWork
高性能服务器框架

## 日志系统

日志系统的执行流程：   

Logger::log() --> LogAppender::log() --> LogFormatter::format() --> FormatItem::format()  

当使用日志系统时，会先实例化出一个Logger对象，该对象针对不同的级别调用不同的处理函数，log函数会遍历所有日志输出器对象，调用它们的log函数。该log函数会通过日志格式器对象m_formatter来调用格式化输出函数format，而m_formatter对象在构造过程中已经将用户定义的输出格式解析为一个个对应的item对象，每个item都重写了format函数以定义如何输出每种日志信息，其中通过LogEvent对象获取到所有的日志信息。

```cpp
//日志事件器：描述了所有的日志信息，比如消息体、时间、线程id、协程id、文件名、行号
class LogEvent;

//日志级别
class LogLevel;

//日志格式
class LogFormatter {
public:
    //日志格式的解析
    void init();
    //遍历所有item，调用各自的format函数输出对应信息
    void format();
    //每种日志信息条目都有专属的类，都继承自FormatItem,各自实现format来输出对应信息，比如MessageFormatItem、LevelFormatItem、ThreadIdFormatem ...
    class FormatItem {
        virtual void format() = 0;
    }
private:
    std::string m_pattern;  //用户定义的日志格式：%p%m%n...
    std::vector<FormatItem> m_items;
};

//日志输出器：定义日志输出方式，即输出到控制台还是文件，分别有StdoutFileAppender和FileAppender继承此类
class LogAppender {
public:
    //调用m_formatter对象的format函数格式化输出日志信息
    virtual void log() = 0;
private:
    LogFormatter m_formatter;
};

//日志器
class Logger {
public:
    //遍历Appender列表，调用每个appender的log函数
    void log();

    //都调用log函数
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
private:
    std::list<LogAppender::ptr> m_appenders;    //Appender列表
};

//管理日志器
class LoggerMgr {
public:
    Logger::ptr getLogger(const std::string& name);
    std::string toYamlString();
private:
    Logger::ptr m_root;         //初始logger
    std::map<std::string, Logger::ptr> m_loggers;    //后续添加/更改/删除的logger
}
```

## 配置系统

主要用于读取yaml配置文件，获取配置信息

```cpp
//接口类
class ConfigVarBase {
public:
    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
};

template<class T>
class ConfigVar {
public:
    std::string toString();          //T --> string
    bool fromString(const std::string& val);     //string --> T
private:
    T m_val;
}
//配置管理类
class ConfigMgr {
public:
    ConfigVar<T>::ptr Lookup();
    //从yaml文件中加载所有配置内容，其中会调用ConfigVar::fromString() --> LexicalCast<string, T>()转换为自定义配置参数，比如LogDefine/LogAppenderDefine
    static void LoadFromYaml(const YAML::Node& root);
private:
    //s_datas在被调用前就已经通过局部静态变量方式完成初始化,管理着所有指向具体某种配置参数的ConfigVar类
    static std::map<std::string, ConfigVarBase::ptr> s_datas;
}
```

```cpp
//定义struct LogDefine和LogAppenderDefine，并偏特化LogDefine,实现日志配置解析 \\

```
//配置模块与日志模块怎样交互？ LogDefine和LogAppenderDefine与yaml文件中log的配置项关联   

执行流程是：  

1. 日志模块初始化时，在ConfigMgr::Lookup函数中会先调用ConfigVar<set\<LogDefine\>>("logs", set\<LogDefine\>(), "logs config")生成一个指向ConfigVar对象的指针g_log_defines，然后为它注册了一个回调函数（当yaml配置文件中的内容改变时，根据新的配置项更改写日志的logger）。   

2. 当通过windgent::ConfigMgr::LoadFromYaml(root)加载yaml配置内容时，调用ConfigVar::fromString(str) --> setVal(val)改变了成员m_val的值，此时会执行已经注册好的回调函数：根据新的配置内容更改logger属性，比如变更日志格式、输出文件等。


## 线程模块

首先，封装了POSIX线程Thread、信号量Semaphore、条件变量Cond、互斥锁Mutex、读写锁RWMutex、自旋锁SoinLock、CAS锁CASLock。

## 协程模块

协程是用户态的线程，借助linux ucontext族函数来实现，详细可参考：

https://zhuanlan.zhihu.com/p/535658398?utm_id=0

https://blog.csdn.net/qq_44443986/article/details/117739157


```cpp
//协程的封装
class Fiber : public std::enable_shared_from_this<Fiber> {
    friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };
public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    //重置协程函数和状态
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换当前协程到后台执行
    void swapOut();

    //主协程执行此函数，切换到当前协程执行
    void call();
    //当前协程执行此函数，切换到主协程执行
    void back();

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }

    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static Fiber::ptr GetThis();
    //切换协程到后台，并设为READY状态
    static void YieldToReady();
    //切换协程到后台，并设为HOLD状态
    static void YieldToHold();
    //总协程数量
    static uint64_t TotalFibers();
    static uint64_t GetFiberId();

    //上下文入口函数，在每个协程的独立栈空间上执行
    static void MainFunc();
    static void CallerMainFunc();
private:
    //只用来设置初始协程，即当前线程状态
    Fiber();
private:
    uint64_t m_id = 0;              //协程id
    uint32_t m_stacksize = 0;       //所用栈大小
    State m_state = INIT;           //协程状态

    ucontext_t m_ctx;               //上下文信息
    void* m_stack = nullptr;        //栈空间
    std::function<void()> m_cb;     //协程执行的函数
};

```

每个线程有一个主协程，主协程可切换为子协程执行，执行结束后会自动切换为主协程。

协程调度器如何工作?协程调度器内有一个线程池,这一组工作线程都运行run函数,不断地检查任务队列(协程)中是否有协程存在.
如果有,则拿出这个协程来执行,否则这些线程都在执行空闲协程.

线程数：协程数 = N : M

协程调度模块下面有N个线程，用于执行M个协程，这M个协程会在N个线程之间切换。协程被看作是一个个任务，线程可以执行这些任务。

```cpp
//协程调度器,内有一个线程池
class Scheduler {
public:
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    //启动协程调度器,创建一组线程,每个线程都执行run函数
    void start();
    void stop();

    //协程调度:可以指定协程在某个线程中执行
    template<class FiberOrcb>
    void schedule(FiberOrcb fc, int thd = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mtx);
            need_tickle = scheduleNoLock(fc, thd);
        }
        if(need_tickle) {
            tickle();   //通知线程有协程到来
        }
    }
    //协程调度函数,不断地从任务队列中拿一个协程(函数)去执行:swapIn
    void run();
    virtual void idle();    //协程无任务可调度时执行idle协程
private:
    //工作线程要执行的协程/函数,看作是任务队列中的一个任务
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int threadId;
    };
private:
    MutexType m_mtx;
    std::vector<Thread::ptr> m_threads;     //工作线程
    std::list<FiberAndThread> m_fibers;     //待执行的协程（任务）队列
    Fiber::ptr m_rootFiber;                 //use_caller为true时有效, 调度协程
    int m_rootThread = 0;       //主线程id（user_caller）
};
```

### IO协程调度模块

```cpp
class IOManager : public Scheduler {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    //事件类型
    enum Event {
        NONE = 0x0;
        READ = 0x1;
        WRITE = 0x2;
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

        EventContext read;          //读事件
        EventContext write;         //写事件
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
    //没有协程任务时，协程调度器会执行idle函数，其中通过epoll_wait监听是否有IO事件到来。如果有IO事件到来，根据事件类型触发事件的执行。
    //当通过addEvent将fd及其事件纳入监听后，如果有就绪事件到来，epoll_wait会立即返回，根据事件的类型触发执行：获取事件上下文，生成一个IO协程供调度器来调度。
    //执行流程：triggerEvent() --->Scheduler::schedule() ---> IOManager::tickle()
    void idle() override;
    //主动调用tickle函数来唤醒epoll_wait，从而执行到时的定时器任务
    void onTimerInsertedAtFront() override;
    //初始化事件列表
    void contextResize();
private:
    int m_epfd = 0;        //epoll fd
    int m_tickleFds[2];   //管道通信

    std::atomic<size_t> m_pendingEventCount = {0};  //等待的事件数
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;           //事件列表
};
```

### 定时器的封装
```cpp
//定时器类
class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    //取消定时器
    bool cancel();
    //刷新定时器的执行时刻
    bool refresh();
    //重置定时器
    bool reset(uint64_t ms, bool from_now);
private:
    Timer(uint64_t ms, std::function<void()> cb, bool isRecur, TimerManager* manager);
    Timer(uint64_t next);   //为了便于根据m_next查找
private:
    bool m_isRecur = false;             //是否是循环定时器
    uint64_t m_ms = 0;                  //执行周期
    uint64_t m_next = 0;                //精确的执行时间，用于什么时候超时
    std::function<void()> m_cb;         //回调函数
    TimerManager* m_manager = nullptr;  //定时器所属的TimeManager
private:
    struct Comparator {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs);
    };
};

//定时器管理类，使用set来管理
class TimerManager {
    friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool isRecur = false);
    Timer::ptr addCondTimer(uint64_t ms, std::function<void()> cb
                            , std::weak_ptr<void> weak_cond, bool isRecur = false);
    //获取下一定时器执行要等待的时间
    uint64_t getTimeOfNextTimer();
    //获取所有超时的定时器的回调函数，从而创建出协程来schedule
    void listExpiredCbs(std::vector<std::function<void()> >& cbs);
    bool hasTimer();
protected:
    //如果有新的定时器插入到首部时，表示这个定时器任务很快就会执行，此时应主动将IOManager从epoll_wait中唤醒来执行此任务
    //因为epoll_wait等待的时间TIMEOUT可能太长，大于定时器的执行时刻
    virtual void onTimerInsertedAtFront() = 0;
    void addTimer(Timer::ptr timer, RWMutexType::WrLock& lock);
private:
    //检测服务器时间是否延后
    bool detectClockRollover(uint64_t now_ms);
private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    //是否触发onTimerInsertedAtFront,因为可能发生频繁修改
    bool m_tickled = false;
    uint64_t m_previouseTime = 0;
};
```
### 模块关系

调度器直接管理线程，线程负责执行协程。IOManager负责IO事件和定时器任务。
```cpp
         [Fiber]              [Timer]
            ^ M                  ^
            |                    |
            |                    |
            | 1                  |
         [Thread]          [TimerManager]
            ^ N                  ^
            |                    |
            |                    |
            | 1                  |
        [Scheduler] <----- [IOManager(epoll)]
```

## HOOK
hook系统底层和socket相关的API，socket io相关的API，以及sleep系列的API。hook的开启控制是线程粒度的。可以自由选择。通过hook模块，可以使一些不具异步功能的API，展现出异步的性能。

原理：首先，像connect、accept、recv、send等读写socket的函数，如果socket上没有数据到来，则线程会阻塞在此等待。如果将socket设置为非阻塞O_NONBLOCK，这些函数会立即返回-1且errno被设为EINPROGRESS/EAGAIN。

HOOK模块通过hook自定义的同名API到系统API，改变了这些API的原有行为，使原来的同步操作变成了异步操作，从而充分利用协程阻塞的时间去执行其它任务。
具体来说，如果有阻塞行为，比如recv原本要等待一段时间接受数据，但hook会添加一个定时器(监听socket上是否有读事件到来)并开始监听fd上的读事件，然后当前协程让出执行权。 当socket上有读事件到来时，唤醒当前协程，去读取socket上的数据。
因此，添加定时器并注册事件--->协程让出执行权--->事件到来，定时器超时触发--->唤醒协程执行任务，同步操作就转换成了异步操作。

## socket函数库

封装IPv4、IPv4、Unix地址，以及socket相关的API

## ByteArray序列化模块

提供对二进制数据的序列化操作，支持多种数据类型int8_t、uint8_t、int16_t、uint16_t,...。内部维护了一个链表用于管理内存块作为缓存，用于缓存socket上的读写数据。

## Http协议开发

## 分布协议