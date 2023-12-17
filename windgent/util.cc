#include <sstream>

#include "util.h"
#include "log.h"
#include "fiber.h"

namespace windgent {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

pid_t GetThreadId(){
    return syscall(SYS_gettid);
}

uint32_t GetFiberId(){
    return windgent::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** buffer = (void**)malloc(sizeof(void*) * size);
    size_t nptr = ::backtrace(buffer, size);

    char** strings = backtrace_symbols(buffer, nptr);
    if(strings == NULL) {
        LOG_ERROR(g_logger) << "backtrace_symbols error";
        free(strings);
        free(buffer);
        return;
    }

    for(size_t i = skip; i < nptr; ++i){
        bt.push_back(strings[i]);
    }
    free(strings);
    free(buffer);
}

std::string BacktraceToString(int size, int skip, const std::string prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);

    std::stringstream ss;
    for(size_t i = 0;i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

}