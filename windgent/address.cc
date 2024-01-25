#include "./address.h"
#include "./endian.h"
#include "./log.h"

#include <sstream>
#include <netdb.h>

namespace windgent {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrLen) {
    
}

int Address::getFamily() const {
    return getAddr()->sa_family;
}

std::string Address::toString() {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const {
    socklen_t minLen = std::min(getAddrLen(), rhs.getAddrLen());
    int res = memcmp(getAddr(), rhs.getAddr(), minLen);
    if(res < 0) {
        return true;
    } else if(res > 0) {
        return false;
    }
    return minLen == getAddrLen();
}

bool Address::operator==(const Address& rhs) const {
    return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const {
    return !(*this == rhs);
}

IPAddress::ptr IPAddress::Create(const char* address, uint32_t port) {

}

//IPv4
IPv4Address::IPv4Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
}

IPv4Address::IPv4Address(uint32_t address, uint32_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

IPv4Address::IPv4Address(const sockaddr_in& addr) {
    m_addr = addr;
}

IPv4Address::ptr IPv4Address::Create(const char* address, uint32_t port) {
    IPv4Address::ptr tmp(new IPv4Address);
    tmp->m_addr.sin_port = byteswapOnLittleEndian(port);
    int ret = inet_pton(AF_INET, address, &tmp->m_addr.sin_addr.s_addr);
    if(ret <= 0) {
        LOG_INFO(g_logger) << "IPv4Address::Create(" << address << ", " << port << "), ret = " << ret
                           << ", errno = " << errno << ", errstr = " << strerror(errno); 
        return nullptr;
    }
    return tmp;
}

const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "." << (addr & 0xff);
    
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastaAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in badAddr(m_addr);
    //网络地址 = IP地址 | 子网掩码
    badAddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(badAddr));
}

IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in netAddr(m_addr);
    //网络地址 = IP地址 | 子网掩码
    netAddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(netAddr));
}

IPAddress::ptr IPv4Address::subNetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint32_t val) {
    m_addr.sin_port = byteswapOnLittleEndian(val);
}

//IPv6
IPv6Address::IPv6Address(const sockaddr_in6& addr) {
    m_addr = addr;
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint32_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

IPv6Address::ptr IPv6Address::Create(const char* address, uint32_t port) {
    IPv6Address::ptr tmp(new IPv6Address);
    tmp->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int ret = inet_pton(AF_INET6, address, &tmp->m_addr.sin6_addr.s6_addr);
    if(ret <= 0) {
        LOG_INFO(g_logger) << "IPv6Address::Create(" << address << ", " << port << "), ret = " << ret
                           << ", errno = " << errno << ", errstr = " << strerror(errno); 
        return nullptr;
    }
    return tmp;
}

const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_aeros = false;
    for(size_t i = 0;i < 8; ++i) {
        if(addr[i] == 0 && !used_aeros) {
            continue;
        }
        if(i && addr[i-1] == 0 && !used_aeros) {
            os << ":";
            used_aeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    if(!used_aeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastaAddress(uint32_t prefix_len) {
    sockaddr_in6 badAddr(m_addr);
    badAddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8; i < 16; ++i) {
        badAddr.sin6_addr.s6_addr[i] = 0xff;    //255
    }
    return IPv6Address::ptr(new IPv6Address(badAddr));
}

IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
    sockaddr_in6 netAddr(m_addr);
    netAddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8; i < 16; ++i) {
        netAddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(netAddr));
}

IPAddress::ptr IPv6Address::subNetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8; i < 16; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;      //255
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint32_t val) {
    m_addr.sin6_port = byteswapOnLittleEndian(val);
}

//Unix
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;
UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    //offsetof::获取结构体中某个成员相对于结构体起始地址的偏移量
    m_len = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}


UnixAddress::UnixAddress(const std::string& path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_len = path.size() + 1;

    if(!path.empty() && path[0] == '\0') {
        --m_len;
    }

    if(m_len > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_len);
    m_len += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
    return m_len;
}

std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_len > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1, m_len - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

//Unknown
UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}

socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os << "[UnknownAddress family = " << m_addr.sa_family << "]";
    return os;
}

}