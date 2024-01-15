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

## 协程调度模块

协程是用户态的线程，借助linux ucontext族函数来实现，详细可参考：

https://zhuanlan.zhihu.com/p/535658398?utm_id=0

https://blog.csdn.net/qq_44443986/article/details/117739157

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

## socket函数库

## Http协议开发

## 分布协议