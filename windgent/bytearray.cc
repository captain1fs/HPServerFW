#include "./bytearray.h"
#include "./endian.h"
#include "./log.h"

#include <math.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>

namespace windgent {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

ByteArray::Node::Node():ptr(nullptr), size(0), next(nullptr) {
}

ByteArray::Node::Node(size_t s):ptr(new char[s]), size(s), next(nullptr) {
}

ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t block_size)
    :m_blocksize(block_size), m_position(0), m_capacity(block_size), m_size(0)
    ,m_endian(WINDGENT_BIG_ENDIAN), m_root(new Node(block_size)), m_cur(m_root) {

}

ByteArray::~ByteArray() {
    Node* tmp = m_root;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

void ByteArray::writeFint8(int8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8(uint8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFint16(int16_t value) {
    if(m_endian != WINDGENT_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value) {
    if(m_endian != WINDGENT_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value) {
    if(m_endian != WINDGENT_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value) {
    if(m_endian != WINDGENT_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value) {
    if(m_endian != WINDGENT_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value) {
    if(m_endian != WINDGENT_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFloat(float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

void ByteArray::writeDouble(double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

//将有符号数据转为无符号，节省存储字节
static uint32_t EncodeZigzag32(int32_t val) {
    if(val < 0) {
        return ((uint32_t)(-val)) * 2 - 1;
    } else {
        return val * 2;
    }
}

static uint64_t EncodeZigzag64(int64_t val) {
    if(val < 0) {
        return ((uint64_t)(-val)) * 2 - 1;
    } else {
        return val * 2;
    }
}

static int32_t DecodeZigzag32(uint32_t val){
    return (val >> 1) ^ -(val & 1);
}

static int64_t DecodeZigzag64(uint64_t val){
    return (val >> 1) ^ -(val & 1);
}

void ByteArray::writeInt32(int32_t value) {
    writeUint32(EncodeZigzag32(value));
}

//压缩算法：每7位保存一位，前面1位为符号位（非有符号的符号位，而是如果为0表示后面没有数据，1表示有数据
void ByteArray::writeUint32(uint32_t value) {
    //最多压缩到5个字节
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7; 
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeInt64(int64_t value) {
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64(uint64_t value) {
    //最多压缩到10个字节
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeStringF16(const std::string& str) {
    //先写入长度，再写入数据
    writeFuint16(str.size());     //string的长度最多由2B表示，即最大2^16 = 64KB
    write(str.c_str(), str.size());
}

void ByteArray::writeStringF32(const std::string& str) {
    writeFuint32(str.size());
    write(str.c_str(), str.size());
}

void ByteArray::writeStringF64(const std::string& str) {
    writeFuint64(str.size());
    write(str.c_str(), str.size());
}

void ByteArray::writeStringVint(const std::string& str) {
    writeUint64(str.size());
    write(str.c_str(), str.size());
}

void ByteArray::writeStringWithoutLen(const std::string& str) {
    write(str.c_str(), str.size());
}

int8_t ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t ByteArray::readFuint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == WINDGENT_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }

int16_t ByteArray::readFint16() {
    XX(int16_t);
}

uint16_t ByteArray::readFuint16() {
    XX(uint16_t);
}

int32_t ByteArray::readFint32() {
    XX(int32_t);
}

uint32_t ByteArray::readFuint32() {
    XX(uint32_t);
}

int64_t ByteArray::readFint64() {
    XX(int64_t);
}

uint64_t ByteArray::readFuint64() {
    XX(uint64_t);
}

float ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float val;
    memcpy(&val, &v, sizeof(v));
    return val;
}

double ByteArray::readDouble() {
    uint64_t v = readFuint64();
    double val;
    memcpy(&val, &v, sizeof(v));
    return val;
}

std::string ByteArray::readStringF16() {
    uint16_t size = readFuint16();
    std::string str;
    str.resize(size);
    read(&str[0], size);
    return str;
}

std::string ByteArray::readStringF32() {
    uint32_t size = readFuint32();
    std::string str;
    str.resize(size);
    read(&str[0], size);
    return str;
}

std::string ByteArray::readStringF64() {
    uint64_t size = readFuint64();
    std::string str;
    str.resize(size);
    read(&str[0], size);
    return str;
}

std::string ByteArray::readStringVint() {
    uint64_t size = readUint64();
    std::string str;
    str.resize(size);
    read(&str[0], size);
    return str;
}

int32_t ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for(int i = 0;i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= (((uint32_t)b) << i);
            break;
        } else {
            result |= (((uint32_t)(b & 0x7F)) << i);
        }
    }
    return result;
}

int64_t ByteArray::readInt64() {
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for(int i = 0;i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= (((uint64_t)b) << i);
            break;
        } else {
            result |= (((uint64_t)(b & 0x7F)) << i);
        }
    }
    return result;
}

//最基本的写操作函数
void ByteArray::write(const void* buf, size_t size) {
    if(0 == size) {
        return;
    }
    addCapacity(size);  //扩容

    size_t npos = m_position % m_blocksize;     //获取要写入的内存块的写入位置
    size_t ncap = m_cur->size - npos;           //该内存块还有多少空间可写
    size_t bpos = 0;                            //buf中数据的起始读取位置

    //如果当前结点剩余空间足够写完则写入所有，否则将当前结点空间写满再去写入下一个结点
    while(size > 0) {
        if(ncap >= size) {
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
    if(m_position > m_size) {
        m_size = m_position;
    }
}

void ByteArray::read(void* buf, size_t size) {
    if(size > getReadSize()) {
        throw std::out_of_range("have no enough data");
    }

    size_t npos = m_position % m_blocksize;     //要读取内存块的起始读取位置
    size_t ncap = m_cur->size - npos;           //当前内存块还有多少数据可读
    size_t bpos = 0;                            //buf的起始写入位置

    //如果当前内存块内数据足够，则读取size长度的数据，否则把当前内存块的剩余数据读完，再去读下一个内存块
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            size -= ncap;
            bpos += ncap;
            npos = 0;
            m_cur = m_cur->next;
            ncap = m_cur->size;
        }
    }
}

void ByteArray::read(void* buf, size_t size, size_t pos) const {
    if(size > getReadSize()) {
        throw std::out_of_range("have no enough data");
    }

    size_t npos = pos % m_blocksize;     //要读取内存块的起始读取位置
    size_t ncap = m_cur->size - npos;           //当前内存块还有多少数据可读
    size_t bpos = 0;                            //buf的起始写入位置

    Node* cur = m_cur;
    //如果当前内存块内数据足够，则读取size长度的数据，否则把当前内存块的剩余数据读完，再去读下一个内存块
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            pos += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            pos += ncap;
            size -= ncap;
            bpos += ncap;
            npos = 0;
            cur = cur->next;
            ncap = cur->size;
        }
    }
}

bool ByteArray::writeToFile(const std::string& filename) const {
    std::ofstream ofs;
    ofs.open(filename, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        LOG_ERROR(g_logger) << "writeToFile " << filename << " error, errno = " 
                            << errno << ", errstr = " << strerror(errno);
        return false;
    }
    
    uint64_t read_size = getReadSize();
    uint64_t pos = m_position;
    Node* cur = m_cur;

    while(read_size > 0) {
        int diff = pos % m_blocksize;       //当前内存块的偏移量
        // int64_t len = (read_size > (uint64_t)m_blocksize ? m_blocksize : read_size) - diff;
        int64_t len = read_size > (uint64_t)m_blocksize ? m_blocksize - diff : read_size;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }
    return true;
}

bool ByteArray::readFromFile(const std::string& filename) {
    std::ifstream ifs;
    ifs.open(filename, std::ios::binary);
    if(!ifs) {
        LOG_ERROR(g_logger) << "readFromFile " << filename << " error, errno = " 
                            << errno << ", errstr = " << strerror(errno);
        return false;
    }
    std::shared_ptr<char> buff(new char[m_blocksize], [](char* ptr) {delete[] ptr;});
    while(!ifs.eof()) {
        ifs.read(buff.get(), m_blocksize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

void ByteArray::addCapacity(size_t size) {
    //如果size=0或者当前剩余容量足够，不用扩容
    if(0 == size) {
        return;
    }
    size_t last_cap = getCapacity();
    if(last_cap >= size) {
        return;
    }
    //否则在最后一个结点之后再添加内存块
    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }
    size -= last_cap;
    size_t block_cnt = ceil(1.0 * size / m_blocksize);
    Node* fst = nullptr;
    for(size_t i = 0;i < block_cnt; ++i) {
        tmp->next = new Node(m_blocksize);
        if(!fst) {
            fst = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_blocksize;
    }
    //如果当前容量正好剩余0，m_cur应指向最新添加的第一个内存块
    if(0 == last_cap) {
        m_cur = fst;
    }
}

void ByteArray::clear() {
    m_position = m_size = 0;
    m_capacity = m_blocksize;
    Node* tmp = m_root->next;
    while(tmp){
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

void ByteArray::setPosition(size_t val) {
    if(val > m_capacity) {
        throw std::out_of_range("setPosition out of range");
    }
    m_position = val;
    if(m_position > m_size) {
        m_size = m_position;
    }
    m_cur = m_root;
    while(val > m_cur->size) {
        val -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(val == m_cur->size) {
        m_cur = m_cur->next;
    }
}

std::string ByteArray::toString() const {
    std::string str;
    str.resize(getReadSize());
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}

std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;

    for(size_t i = 0;i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
    }
    return ss.str();
}

bool ByteArray::isLittleEndian() const {
    return m_endian == WINDGENT_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = WINDGENT_LITTLE_ENDIAN;
    } else {
        m_endian = WINDGENT_BIG_ENDIAN;
    }
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
    if(0 == len) {
        return 0;
    }
    len = len > getReadSize() ? getReadSize() : len;
    uint64_t size = len;

    size_t npos = m_position % m_blocksize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + ncap;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            npos = 0;
            ncap = cur->size;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t pos) const {
    if(0 == len) {
        return 0;
    }
    len = len > getReadSize() ? getReadSize() : len;
    uint64_t size = len;

    size_t npos = pos % m_blocksize;
    size_t block_cnt = pos / m_blocksize;
    Node* cur = m_root;
    while(block_cnt > 0) {
        --block_cnt;
        cur = cur->next;
    }

    size_t ncap = cur->size - npos;
    struct iovec iov;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + ncap;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            npos = 0;
            ncap = cur->size;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    if(0 == len) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_blocksize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + ncap;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            npos = 0;
            ncap = cur->size;
        }
        buffers.push_back(iov);
    }
    return size;
}

}