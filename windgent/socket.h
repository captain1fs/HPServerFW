#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "./address.h"
#include "./noncopyable.h"

#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

namespace windgent {

class Socket : public std::enable_shared_from_this<Socket>, NonCopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> w_ptr;

    enum Type {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM,
    };
    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX,
    };

    Socket(int family, int type, int protocol);
    ~Socket();

    static Socket::ptr createTCP(Address::ptr addr);
    static Socket::ptr createUDP(Address::ptr addr);
    static Socket::ptr createTCPSocket();
    static Socket::ptr createUDPSocket();
    static Socket::ptr createTCPSocket6();
    static Socket::ptr createUDPSocket6();
    static Socket::ptr createUnixTCPSocket();
    static Socket::ptr createUnixUDPSocket();

    //timeout
    int64_t getSendTimeout();
    void setSendTimeout(int64_t val);
    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t val);

    //getsockopt & setsockopt
    bool getSockOpt(int level, int optname, void* optval, socklen_t* optlen);
    template<class T>
    bool getSockOpt(int level, int optname, T& optval) {
        size_t length = sizeof(T);
        return getSockOpt(level, optname, &optval, &length);
    }

    bool setSockOpt(int level, int optname, const void* optval, socklen_t optlen);
    template<class T>
    bool setSockOpt(int level, int optname, const T& optval) {
        return setSockOpt(level, optname, &optval, sizeof(T));
    }

    //socket
    bool bind(const Address::ptr addr);
    bool listen(int backlog = SOMAXCONN);
    bool connect(const Address::ptr addr, uint64_t timeout = -1);
    Socket::ptr accept();
    bool close();

    //IO
    int send(const void* buffer, size_t length, int flags = 0);
    int send(const struct iovec* buffers, size_t length, int flags = 0);
    int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    int sendTo(const struct iovec* buffers, size_t length, const Address::ptr to, int flags = 0);
    int recv(void* buffer, size_t length, int flags = 0);
    int recv(struct iovec* buffers, size_t length, int flags = 0);
    int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
    int recvFrom(struct iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    //other
    //获取/初始化本地、远端地址，只有客户端一方才会使用到getRemoteAddress()，因为服务端的socket可以与多个客户端连接
    Address::ptr getLocalAddress();
    Address::ptr getRemoteAddress();
    int getSocket() const { return m_sockfd; }
    int getFamily() const { return m_family; }
    int getType() const { return m_type; }
    int getProtocol() const { return m_protocol; }
    bool isConnected() const { return m_isConnected; }

    bool isValid() const;
    int getError();
    std::ostream& dump(std::ostream& os) const;
    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();
private:
    //为参数sockfd创建一个Socket对象并初始化
    bool init(int sockfd);
    //调用setSockOpt设置socket的一些状态
    void initSocket();
    //创建socket
    void newSocket();
private:
    int m_sockfd;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;
    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};

//流式输出socket
std::ostream& operator<<(std::ostream& os, const Socket& sock);

}

#endif