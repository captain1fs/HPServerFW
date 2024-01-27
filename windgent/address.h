#ifndef __ADDRESS_H__
#define __ADDRESS_H__

#include <iostream>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <stdint.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

namespace windgent {

class IPAddress;

class Address {
public:
    typedef std::shared_ptr<Address> ptr;
    virtual ~Address() { }

    static Address::ptr Create(const sockaddr* addr, socklen_t addrLen);
    //通过域名返回所有对应的Address，比如www.baidu.com
    static bool getAllAddrFromHost(std::vector<Address::ptr>& res, const std::string& host,
                                   int family = AF_INET, int type = 0, int protocol = 0);
    //通过域名返回任意对应的Address
    static Address::ptr getAnyAddrFromHost(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);
    //通过域名获取任意对应的的IPAddress
    static std::shared_ptr<IPAddress> getAnyIPAddrFromHost(const std::string& host, int family = AF_INET,
                                                           int type = 0, int protocol = 0);
    //获取本机网卡的所有<网卡名，地址，子网掩码位数>
    static bool getInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result, int family = AF_INET);
    //获取指定网卡的<地址，子网掩码位数>
    static bool getInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t> >& result,
                                    const std::string& iface, int family = AF_INET);

    int getFamily() const;
    std::string toString();

    virtual const sockaddr* getAddr() const = 0;
    virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrLen() const = 0;
    virtual std::ostream& insert(std::ostream& os) const = 0;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    //获取具体地址的广播地址
    virtual IPAddress::ptr broadcastaAddress(uint32_t prefix_len) = 0;
    //获取具体地址的网段
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
    //获取具体地址的子网掩码
    virtual IPAddress::ptr subNetMask(uint32_t prefix_len) = 0;

    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t val) = 0;         //uint32_t时报错，无法connect
};

class IPv4Address : public IPAddress {
public: 
    typedef std::shared_ptr<IPv4Address> ptr;
    IPv4Address(const sockaddr_in& addr);
    //通过二进制地址构造
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
    //通过点分文本创建
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastaAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subNetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t val) override;
private:
    struct sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public: 
    typedef std::shared_ptr<IPv6Address> ptr;
    IPv6Address();
    IPv6Address(const sockaddr_in6& addr);
    //通过二进制地址构造
    IPv6Address(const uint8_t address[16], uint16_t port = 0);
    //通过点分文本创建
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastaAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subNetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t val) override;
private:
    struct sockaddr_in6 m_addr;
};


class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t val);
    std::ostream& insert(std::ostream& os) const override;
private:
    struct sockaddr_un m_addr;
    socklen_t m_len;
};

class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    struct sockaddr m_addr;
};

}


#endif