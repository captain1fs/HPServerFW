#ifndef __NONCOPYABLE_H__
#define __NONCOPYABLE_H__

namespace windgent {

class NonCopyable {
public:
    NonCopyable() = default;
    ~NonCopyable() = default;
    
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

}

#endif