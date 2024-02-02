#include "./socket.h"
#include "./log.h"
#include "./fd_manager.h"
#include "./iomanager.h"
#include "./macro.h"
#include "./hook.h"

#include <time.h>

namespace windgent {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

Socket::Socket(int family, int type, int protocol)
    :m_sockfd(-1), m_family(family), m_type(type), m_protocol(protocol), m_isConnected(false) {
}

Socket::~Socket() {
    close();
}

Socket::ptr Socket::createTCP(Address::ptr addr) {
    Socket::ptr sock(new Socket(addr->getFamily(), TCP, 0));
    return sock;
}
Socket::ptr Socket::createUDP(Address::ptr addr) {
    Socket::ptr sock(new Socket(addr->getFamily(), UDP, 0));

    return sock;
}

Socket::ptr Socket::createTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::createUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));

    return sock;
}

Socket::ptr Socket::createTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

Socket::ptr Socket::createUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));

    return sock;
}

Socket::ptr Socket::createUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

Socket::ptr Socket::createUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

//timeout
int64_t Socket::getSendTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sockfd);
    if(ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

void Socket::setSendTimeout(int64_t val) {
    struct timeval tv{int(val / 1000), int(val % 1000 * 1000)};
    setSockOpt(SOL_SOCKET, SO_SNDTIMEO, tv);
}

//是否应该调用getSockOpt获得timeout？因为setRecvTimeout并未调用FdCtx::setTimeout为m_sockfd设置上下文！
int64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sockfd);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t val) {
    struct timeval tv{int(val / 1000), int(val % 1000 * 1000)};
    setSockOpt(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getSockOpt(int level, int optname, void* optval, socklen_t* optlen) {
    int ret = getsockopt(m_sockfd, level, optname, optval, optlen);
    if(ret) {
        LOG_DEBUG(g_logger) << "Socket::getSockOpt(" << m_sockfd << ", " << level << ", " << optname << "), errno = "
                            << errno << ", errstr = " << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setSockOpt(int level, int optname, const void* optval, socklen_t optlen) {
    int ret = setsockopt(m_sockfd, level, optname, optval, optlen);
    if(ret) {
        LOG_DEBUG(g_logger) << "Socket::setSockOpt sockfd = " << m_sockfd << ", level = " << level << ", optname = " 
                            << optname << ", errno = " << errno << ", errstr = " << strerror(errno);
        return false;
    }
    return true;
}

//socket
bool Socket::bind(const Address::ptr addr) {
    if(WINDGENT_UNLIKELY(!isValid())) {
        newSocket();
        if(WINDGENT_UNLIKELY(!isValid())) {
            return false;
        }
    }

    if(WINDGENT_UNLIKELY(addr->getFamily() != m_family)) {
        LOG_ERROR(g_logger) << "bind sock.family(" << m_family <<") & addr.family(" << addr->getFamily()
                            << ") not equal, addr = " << addr->toString();
        return false;
    }

    if(::bind(m_sockfd, addr->getAddr(), addr->getAddrLen())) {
        LOG_ERROR(g_logger) << "Socket::bind error, errno = " << errno << ", errstr = " << strerror(errno);
        return false;
    }
    getLocalAddress();  //初始化本地地址
    return true;
}

bool Socket::listen(int backlog) {
    if(!isValid()) {
        LOG_ERROR(g_logger) << "listen error, sockfd = -1";
        return false;
    }
    if(::listen(m_sockfd, backlog)) {
        LOG_ERROR(g_logger) << "Socket::listen error, errno = " << errno << ", errstr = " << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout) {
    if(WINDGENT_UNLIKELY(!isValid())) {
        newSocket();
        if(WINDGENT_UNLIKELY(!isValid())) {
            return false;
        }
    }
    if(WINDGENT_UNLIKELY(addr->getFamily() != m_family)) {
        LOG_ERROR(g_logger) << "connect sock.family(" << m_family <<") & addr.family(" << addr->getFamily()
                            << ") not equal, addr = " << addr->toString();
        return false;
    }

    if(timeout == (uint64_t)-1) {
        if(::connect(m_sockfd, addr->getAddr(), addr->getAddrLen())) {
            LOG_ERROR(g_logger) << "socket = " << m_sockfd << " connect(" << addr->toString() << ") timeout = "
                                << timeout << " error, errno = " << errno << ", errstr = " << strerror(errno);
            close();
            return false;
        }
    } else {
        if(::connect_with_timeout(m_sockfd, addr->getAddr(), addr->getAddrLen(), timeout)) {
            LOG_ERROR(g_logger) << "socket = " << m_sockfd << " connect_with_timeout(" << addr->toString() 
                        << ") timeout = " << timeout << " error, errno = " << errno << ", errstr = " << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getLocalAddress();
    getRemoteAddress();
    return true;
}

Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newfd = ::accept(m_sockfd, nullptr, nullptr);
    if(newfd == -1) {
        LOG_ERROR(g_logger) << "Socket::accept(" << m_sockfd << "), errno = " << errno << ", errstr = " << strerror(errno);
        return nullptr;
    }
    if(sock->init(newfd)) {
        return sock;
    }
    return nullptr;
}

bool Socket::close() {
    if(!m_isConnected && m_sockfd == -1) {
        return true;
    }
    m_isConnected = false;
    if(m_sockfd != -1) {
        ::close(m_sockfd);
        m_sockfd = -1;
    }
    return true;
}

//IO
int Socket::send(const void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::send(m_sockfd, buffer, length, flags);
    }
    return -1;
}

int Socket::send(const struct iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sockfd, &msg, flags);
    }
    return -1;
}

int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        return ::sendto(m_sockfd, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(const struct iovec* buffers, size_t length, const Address::ptr to, int flags){
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sockfd, &msg, flags);
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::recv(m_sockfd, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(struct iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sockfd, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sockfd, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

int Socket::recvFrom(struct iovec* buffers, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::sendmsg(m_sockfd, &msg, flags);
    }
    return -1;
}

//other
Address::ptr Socket::getLocalAddress() {
    if(m_localAddress) {
        return m_localAddress;
    }
    Address::ptr res;
    switch(m_family) {
        case AF_INET:
            res.reset(new IPv4Address());
            break;
        case AF_INET6:
            res.reset(new IPv6Address());
            break;
        case AF_UNIX:
            res.reset(new UnixAddress());
            break;
        default:
            res.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = res->getAddrLen();
    if(getsockname(m_sockfd, res->getAddr(), &addrlen)) {
        LOG_ERROR(g_logger) << "getsockname error, sockfd = " << m_sockfd << ", errno = " << errno
                            << ",  errstr = " << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(res);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = res;
    return m_localAddress;
}

Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }
    Address::ptr res;
    switch(m_family) {
        case AF_INET:
            res.reset(new IPv4Address());
            break;
        case AF_INET6:
            res.reset(new IPv6Address());
            break;
        case AF_UNIX:
            res.reset(new UnixAddress());
            break;
        default:
            res.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = res->getAddrLen();
    if(getpeername(m_sockfd, res->getAddr(), &addrlen)) {
        LOG_ERROR(g_logger) << "getpeername error, sockfd = " << m_sockfd << ", errno = " << errno
                            << ",  errstr = " << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(res);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = res;
    return m_remoteAddress;
}

bool Socket::isValid() const {
    return m_sockfd != -1;
}

int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if(!getSockOpt(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket socket= " << m_sockfd << ", is_connected= " << m_isConnected << ", family= " << m_family
       << ", type = " << m_type << ", protocol = " << m_protocol;
    if(m_localAddress) {
        os << ", localaddress= " << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << ", remoteaddress= " << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sockfd, windgent::IOManager::READ);
}

bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sockfd, windgent::IOManager::WRITE);
}

bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sockfd, windgent::IOManager::READ);
}

bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(m_sockfd);
}

bool Socket::init(int sockfd) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sockfd);
    if(ctx && ctx->isSocket() && !ctx->isClosed()) {
        m_sockfd = sockfd;
        m_isConnected = true;
        initSocket();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

void Socket::initSocket() {
    int val = 1;
    setSockOpt(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setSockOpt(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSocket() {
    m_sockfd = socket(m_family, m_type, m_protocol);
    if(WINDGENT_LIKELY(m_sockfd != -1)) {
        initSocket();
    } else {
        LOG_ERROR(g_logger) << "socket(" << m_family << ", " << m_type << ", " << m_protocol << "), errno = "
                            << errno << ", errstr = " << strerror(errno);
    }
}

std::ostream& operator<<(std::ostream& os, const Socket& sock) {
    return sock.dump(os);
}

}