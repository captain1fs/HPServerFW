#ifndef __UTIL_H__
#define __UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <execinfo.h>

namespace windgent {

pid_t GetThreadId();
uint32_t GetFiberId();

//获取当前时间的毫秒数
uint64_t GetCurrentMS();
//获取当前时间的微秒数
uint64_t GetCurrentUS();


void Backtrace(std::vector<std::string>& bt, int size = 100, int skip = 1);
std::string BacktraceToString(int size = 100, int skip = 2, const std::string prefix = "    ");

}

#endif