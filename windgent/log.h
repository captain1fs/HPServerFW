#ifndef __LOG_H__
#define __LOG_H__

#include<iostream>
#include<string>
#include<list>
#include<vector>
#include<map>
#include<set>
#include<stdint.h>
#include<sstream>
#include<fstream>
#include<memory>
#include<ctime>
#include<cstring>
#include<stdarg.h>
#include<yaml-cpp/yaml.h>

#include "singleton.h"
#include "util.h"

#define LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        windgent::LogEventWarp(windgent::LogEvent::ptr(new windgent::LogEvent(logger, level, __FILE__, __LINE__, 0, windgent::GetThreadId(), windgent::GetFiberId(), time(0)))).getSS()

#define LOG_DEBUG(logger) LOG_LEVEL(logger, windgent::LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, windgent::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, windgent::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, windgent::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, windgent::LogLevel::FATAL)

#define LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        windgent::LogEventWarp(windgent::LogEvent::ptr(new windgent::LogEvent(logger, level, __FILE__, __LINE__, 0, \
            windgent::GetThreadId(), windgent::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, windgent::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, windgent::LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, windgent::LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, windgent::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, windgent::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_ROOT() windgent::LoggerMgr::GetInstance()->getRoot()
#define LOG_NAME(name) windgent::LoggerMgr::GetInstance()->getLogger(name)

namespace windgent {

class Logger;
class LoggerManager;

//日志级别
class LogLevel{
public:
    enum Level {
        UNKNOWN = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };

    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};

//日志事件：包含每条日志的所有信息
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse, uint32_t threadId, uint32_t fiberID, uint64_t time);

    const char* getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    std::string getContent() const { return m_ss.str(); }

    std::stringstream& getSS() { return m_ss; }
    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);
private:
    const char* m_file = nullptr;    //日志文件名
    int32_t m_line = 0;              //行号
    uint32_t m_elapse = 0;           //程序启动到现在的毫秒数
    uint32_t m_threadId = 0;         //线程id
    uint32_t m_fiberId = 0;          //协程id
    uint64_t m_time;                 //时间戳
    std::stringstream m_ss;           //日志信息

    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

class LogEventWarp {
public:
    LogEventWarp(LogEvent::ptr e);
    ~LogEventWarp();
    std::stringstream& getSS();

    LogEvent::ptr getEvent() const { return m_event; }
private:
    LogEvent::ptr m_event;
};

//日志格式器
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);

    //格式化输出日志
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    bool isError() const { return m_error; }
public:
    //日志信息条目
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() { }
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };
    //日志格式的解析
    void init();
private:
    std::string m_pattern;      //日志格式
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};

//日志输出地
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() { }

    //写日志
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    LogFormatter::ptr getFormatter() const { return m_formatter; }
    void setFormatter(LogFormatter::ptr formatter) { m_formatter = formatter; }

    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level; }
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
};

//日志器：当使用日志系统时，会先实例化出一个Logger对象，该对象针对不同的级别调用不同的处理函数，log函数会遍历所有日志输出器对象，调用它们的log函数
//该log函数会通过日志格式器对象m_formatter来调用格式化输出函数format，而m_formatter对象在构造过程中已经将用户定义的输出格式解析为一个个对应的item对象，
//每个item都重写了format函数以定义如何输出每种日志信息，其中通过LogEvent对象获取到所有的日志信息
class Logger : public std::enable_shared_from_this<Logger> {
    friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name = "root");

    //写日志
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppenders(LogAppender::ptr appender);
    void delAppenders(LogAppender::ptr appender);
    void clearAppenders();
    LogLevel::Level getLevel() { return m_level; };
    void setLevel(LogLevel::Level level) { m_level = level; }

    const std::string getName() const { return m_name; }
    void setFormatter(LogFormatter::ptr val);
    void setFormatter(std::string val);
    LogFormatter::ptr getFormatter();
private:
    std::string m_name;             //日志名称
    LogLevel::Level m_level;        //日志级别
    std::list<LogAppender::ptr> m_appenders;    //Appender列表
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

//输出到控制台的LogAppender
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
};

//输出到文件的LogAppender
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    ~FileLogAppender();
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;
};

class LoggerManager {
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot() const { return m_root; }
private:
    Logger::ptr m_root;
    std::map<std::string, Logger::ptr> m_loggers;
};

typedef windgent::Singleton<LoggerManager> LoggerMgr;

}

#endif