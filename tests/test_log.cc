#include<iostream>
// #include<thread>
#include"../sylar/log.h"
#include"../sylar/util.h"

int main(int argc, char** argv){
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);
    logger->addAppender(file_appender);

    // std::cout << __FILE__ << ", " << __LINE__ << std::endl;
    // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0, std::hash<std::thread::id>()(std::this_thread::get_id()), 2, time(0)));
    // sylar::LogEvent::ptr event(new sylar::LogEvent(logger, logger->getLevel(), __FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), time(0)));
    // // event->getSS() << "hello sylar log!";
    // logger->log(sylar::LogLevel::DEBUG, event);

    SYLAR_LOG_INFO(logger) << "test define";
    SYLAR_LOG_ERROR(logger) << "test define error";

    SYLAR_LOG_FMT_ERROR(logger, "test fmt error %s", "aa");

    auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";

    // std::cout << "Hello sylar log!" << std::endl;

    return 0;
}