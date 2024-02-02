#include "./stream.h"

namespace windgent {

int Stream::readFixedSize(void* buffer, size_t length) {
    uint64_t left = length;
    size_t offset = 0;
    while(left > 0) {
        uint64_t len = read((char*)buffer + offset, left);
        if(len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int Stream::readFixedSize(ByteArray::ptr ba, size_t length) {
    uint64_t left = length;
    while(left > 0) {
        uint64_t len = read(ba, left);
        if(len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}

int Stream::writeFixedSize(const void* buffer, size_t length) {
    uint64_t left = length;
    size_t offset = 0;
    while(left > 0) {
        uint64_t len = write((const char*)buffer + offset, left);
        if(len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int Stream::writeFixedSize(ByteArray::ptr ba, size_t length) {
    uint64_t left = length;
    while(left > 0) {
        uint64_t len = write(ba, left);
        if(len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}

}