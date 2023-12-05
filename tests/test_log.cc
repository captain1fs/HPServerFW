#include<iostream>
// #include<thread>
#include"../windgent/log.h"
#include"../windgent/util.h"

int main(int argc, char** argv){
    windgent::Logger::ptr logger(new windgent::Logger);
    logger->addAppenders(windgent::LogAppender::ptr(new windgent::StdoutLogAppender));

    windgent::FileLogAppender::ptr file_appender(new windgent::FileLogAppender("./log.txt"));
    windgent::LogFormatter::ptr fmt(new windgent::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(windgent::LogLevel::ERROR);
    logger->addAppenders(file_appender);

    // std::cout << __FILE__ << ", " << __LINE__ << std::endl;
    // windgent::LogEvent::ptr event(new windgent::LogEvent(__FILE__, __LINE__, 0, std::hash<std::thread::id>()(std::this_thread::get_id()), 2, time(0)));
    // windgent::LogEvent::ptr event(new windgent::LogEvent(logger, logger->getLevel(), __FILE__, __LINE__, 0, windgent::GetThreadId(), windgent::GetFiberId(), time(0)));
    // // event->getSS() << "hello windgent log!";
    // logger->log(windgent::LogLevel::DEBUG, event);

    LOG_INFO(logger) << "test define";
    LOG_ERROR(logger) << "test define error";

    LOG_FMT_ERROR(logger, "test fmt error %s", "aa");

    auto l = windgent::LoggerMgr::GetInstance()->getLogger("xx");
    LOG_INFO(l) << "xxx";

    // std::cout << "Hello windgent log!" << std::endl;

    return 0;
}