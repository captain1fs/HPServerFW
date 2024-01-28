#include "../windgent/bytearray.h"
#include "../windgent/log.h"
#include "../windgent/macro.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test_bytearray() {
#define XX(type, len, write_fun, read_fun, block_size) { \
    std::vector<type> vec; \
    for(int i = 0;i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    windgent::ByteArray::ptr ba(new windgent::ByteArray(block_size)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        ASSERT(v == vec[i]); \
    } \
    ASSERT(ba->getReadSize() == 0); \
    LOG_INFO(g_logger) << #write_fun "/" #read_fun "( #type ) len = " << len << ", block_size = " \
                       << block_size << ", data size = " << ba->getSize(); \
}

    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t, 100, writeFint32, readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 100, writeInt32, readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX

#define XX(type, len, write_fun, read_fun, block_size) { \
    std::vector<type> vec; \
    for(int i = 0;i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    windgent::ByteArray::ptr ba(new windgent::ByteArray(block_size)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        ASSERT(v == vec[i]); \
    } \
    ASSERT(ba->getReadSize() == 0); \
    LOG_INFO(g_logger) << #write_fun "/" #read_fun "( #type ) len = " << len << ", block_size = " \
                       << block_size << ", data size = " << ba->getSize(); \
    ba->setPosition(0); \
    ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    windgent::ByteArray::ptr ba2(new windgent::ByteArray(block_size * 2)); \
    ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    ba2->setPosition(0); \
    ASSERT(ba->toString() == ba2->toString()); \
    ASSERT(ba->getPosition() == 0); \
    ASSERT(ba2->getPosition() == 0); \
}

    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t, 100, writeFint32, readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 100, writeInt32, readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX

}

int main() {
    test_bytearray();
    // test();

    return 0;
}