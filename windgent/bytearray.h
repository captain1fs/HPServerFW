#ifndef __BYTEARRAY_H__
#define __BYTEARRAY_H__

#include <memory>
#include <string>
#include <stdint.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>

namespace windgent {

class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    ByteArray(size_t block_size = 4096);
    ~ByteArray();

    struct Node {
        Node();
        Node(size_t s);
        ~Node();

        char* ptr;
        size_t size;
        Node* next;
    };
    //write，按照data类型长度来写，不压缩
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    //write, 压缩写
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);
    //写入string类型的数据，并写入其长度
    void writeStringF16(const std::string& str);
    void writeStringF32(const std::string& str);
    void writeStringF64(const std::string& str);
    void writeStringVint(const std::string& str);
    //不写入长度
    void writeStringWithoutLen(const std::string& str);

    //read，按照固定的data类型长度来读,string类型数据按照其写入的长度来读
    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();
    float readFloat();
    double readDouble();
    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();
    //read, 读压缩
    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    //往内存写入buf中size长度的数据
    void write(const void* buf, size_t size);
    //从内存中读取size长度的数据到buf
    void read(void* buf, size_t size);
    //从pos位置开始读size长度的数据
    void read(void* buf, size_t size, size_t pos) const;
    //向文件读取/写入内存块中的数据
    bool writeToFile(const std::string& filename) const;
    bool readFromFile(const std::string& filename);

    //other
    void clear();                                                   //释放头结点之外的所有结点
    void addCapacity(size_t size);                                  //扩容
    size_t getPosition() const { return m_position; }
    void setPosition(size_t val);                                   //设置m_position和m_cur
    size_t getBlockSize() const { return m_blocksize; }
    size_t getReadSize() const { return m_size - m_position; }      //获取可读数据大小
    size_t getSize() const { return m_size; }
    size_t getCapacity() const { return m_capacity - m_position; }  //获取当前可写入的容量
    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);
    std::string toString() const;
    std::string toHexString() const;
    //获取可供读写的缓存(Node)封装成iovec数组
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t pos) const;
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
private:
    size_t m_blocksize;     //内存块大小
    size_t m_position;      //当前操作位置
    size_t m_capacity;      //当前的总容量
    size_t m_size;          //当前数据大小
    int8_t m_endian;        //字节序，默认大端

    Node* m_root;           //第一个内存块地址
    Node* m_cur;            //当前读写的内存块地址
};

}

#endif