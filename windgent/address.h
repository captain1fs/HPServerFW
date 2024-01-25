#ifndef __ADDRESS_H__
#define __ADDRESS_H__

#include <iostream>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <stdint.h>
#include <arpa/inet.h>

namespace windgent {

class Address {
public:
    typedef std::shared_ptr<Address> ptr;
    virtual ~Address() { }

    static Address::ptr Create(const sockaddr* addr, socklen_t addrLen);

    int getFamily() const;

    virtual const sockaddr* getAddr() const = 0;
    virtual socklen_t getAddrLen() const = 0;
    virtual std::ostream& insert(std::ostream& os) const = 0;
    std::string toString();

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    static IPAddress::ptr Create(const char* address, uint32_t port = 0);

    //获取具体地址的广播地址
    virtual IPAddress::ptr broadcastaAddress(uint32_t prefix_len) = 0;
    //获取具体地址的网段
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
    //获取具体地址的子网掩码
    virtual IPAddress::ptr subNetMask(uint32_t prefix_len) = 0;

    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint32_t val) = 0;
};

class IPv4Address : public IPAddress {
public: 
    typedef std::shared_ptr<IPv4Address> ptr;
    IPv4Address();
    IPv4Address(const sockaddr_in& addr);
    //通过二进制地址构造
    IPv4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
    //通过点分文本创建
    static IPv4Address::ptr Create(const char* address, uint32_t port = 0);

    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastaAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subNetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint32_t val) override;
private:
    struct sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public: 
    typedef std::shared_ptr<IPv6Address> ptr;
    IPv6Address();
    IPv6Address(const sockaddr_in6& addr);
    //通过二进制地址构造
    IPv6Address(const uint8_t address[16], uint32_t port = 0);
    //通过点分文本创建
    static IPv6Address::ptr Create(const char* address, uint32_t port = 0);

    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastaAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subNetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint32_t val) override;
private:
    struct sockaddr_in6 m_addr;
};


class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    struct sockaddr_un m_addr;
    socklen_t m_len;
};

class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);

    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    struct sockaddr m_addr;
};

}


#endif