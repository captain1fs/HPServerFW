# sylar
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
class LoggerMgr;

```

## 配置系统

主要用于读取yaml配置文件，获取配置信息

```cpp
class ConfigVarBase {
public:
    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
};

template<class T>
class ConfigVar {
public:
    std::string toString();
    bool fromString(const std::string& val);
private:
    T m_val;
}

class ConfigMgr {
public:
    ConfigVar<T>::ptr Lookup();
    static void LoadFromYaml(const YAML::Node& root);
private:
    static std::map<std::string, ConfigVarBase::ptr> s_datas;
}

```

## 线程模块

## 协程模块

## socket函数库

## Http协议开发

## 分布协议