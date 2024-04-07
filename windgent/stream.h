#ifndef __STREAM_H__
#define __STREAM_H__

#include <memory>
#include "./bytearray.h"

namespace windgent {

//Stream类规定了一个流必须具备read/write接口，继承自Stream的类必须实现这些接口
class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;
    virtual ~Stream() { }
    //从流中读数据
    virtual int read(void* buffer, size_t length) = 0;
    virtual int read(ByteArray::ptr ba, size_t length) = 0;
    virtual int readFixedSize(void* buffer, size_t length);
    virtual int readFixedSize(ByteArray::ptr ba, size_t length);
    //往流中写数据
    virtual int write(const void* buffer, size_t length) = 0;
    virtual int write(ByteArray::ptr ba, size_t length) = 0;
    virtual int writeFixedSize(const void* buffer, size_t length);
    virtual int writeFixedSize(ByteArray::ptr ba, size_t length);

    virtual void close() = 0;
};

}

#endif