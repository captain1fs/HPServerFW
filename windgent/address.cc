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

template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrLen) {
    if(!addr) {
        return nullptr;
    }
    Address::ptr res;
    switch(addr->sa_family) {
        case AF_INET:
            res.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            res.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            res.reset(new UnknownAddress(*addr));
            break;
    }
    return res;
}

//通过域名返回所有对应的Address
bool Address::getAllAddrFromHost(std::vector<Address::ptr>& res, const std::string& host,
                                int family, int type, int protocol) {
    addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_flags = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char* service = NULL;

    //ipv6 address & service
    if(!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.c_str()+1, ']', host.size()-1);
        if(endipv6) {
            if(*(endipv6+1) == ':') {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if(node.empty()) {
        node = host;
    }
    int ret = getaddrinfo(node.c_str(), service, &hints, &result);
    if(ret) {
        LOG_DEBUG(g_logger) << "Address::getAllAddrFromHost(" << host << ", " << family << ", " << type << ", "
                           << protocol << ")" << ", ret = " << ret << ", errstr = " << gai_strerror(errno);
        return false;
    }

    rp = result;
    while(rp) {
        res.push_back(Create(rp->ai_addr, (socklen_t)rp->ai_addrlen));
        rp = rp->ai_next;
    }
    freeaddrinfo(result);
    return !res.empty();
}

//通过域名返回任意对应的Address
Address::ptr Address::getAnyAddrFromHost(const std::string& host, int family, int type, int protocol) {
    std::vector<Address::ptr> res;
    if(getAllAddrFromHost(res, host, family, type, protocol)) {
        return res[0];
    }
    return nullptr;
}

//通过域名获取任意对应的的IPAddress
std::shared_ptr<IPAddress> Address::getAnyIPAddrFromHost(const std::string& host, int family, int type, int protocol) {
    std::vector<Address::ptr> res;
    if(getAllAddrFromHost(res, host, family, type, protocol)) {
        for(auto& addr : res) {
            IPAddress::ptr tmp = std::dynamic_pointer_cast<IPAddress>(addr);
            if(tmp) {
                return tmp;
            }
        }
    }
    return nullptr;
}

bool Address::getInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& res, int family) {
    struct ifaddrs *next, *results;
    if(getifaddrs(&results) != 0) {
        LOG_DEBUG(g_logger) << "Address::getInterfaceAddress getifaddrs err=" << errno << ", errstr=" << strerror(errno);
        return false;
    }

    try {
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch(next->ifa_addr->sa_family) {
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }

            if(addr) {
                res.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        LOG_ERROR(g_logger) << "Address::getInterfaceAddress exception";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return !res.empty();
}

bool Address::getInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t> >& res,
                                    const std::string& iface, int family) {
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            res.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            res.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;
    if(!getInterfaceAddress(results, family)) {
        return false;
    }

    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        res.push_back(its.first->second);
    }
    return !res.empty();
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
    addrinfo hints, *res = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST;

    int ret = getaddrinfo(address, NULL, &hints, &res);
    if(ret) {
        LOG_INFO(g_logger) << "IPAddress::Create(" << address << ", " << port << "), ret = " << ret
                           << ", errno = " << errno << ", errstr = " << strerror(errno);
        return nullptr;
    }

    try {
        IPAddress::ptr ans = std::dynamic_pointer_cast<IPAddress>(Address::Create(res->ai_addr, (socklen_t)res->ai_addrlen));
        if(ans) {
            ans->setPort(port);
        }
        freeaddrinfo(res);
        return ans;
    } catch (...) {
        freeaddrinfo(res);
        return nullptr;
    }
}

//IPv4
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
        LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", " << port << "), ret = " << ret
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
        LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", " << port << "), ret = " << ret
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

UnknownAddress::UnknownAddress(const sockaddr& addr) {
    m_addr = addr;
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