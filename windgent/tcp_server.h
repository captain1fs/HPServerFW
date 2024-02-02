#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <memory>
#include <functional>
#include <vector>
#include "./iomanager.h"
#include "./address.h"
#include "./socket.h"
#include "./noncopyable.h"

namespace windgent {

class TcpServer : public std::enable_shared_from_this<TcpServer>, NonCopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(IOManager* worker = IOManager::GetThis(), IOManager* accept_worker = IOManager::GetThis());
    virtual ~TcpServer();

    //为m_socks绑定地址
    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr> addrs, std::vector<Address::ptr> fails);
    virtual bool start();
    virtual void stop();

    bool isStop() const { return m_isStop; }
    uint64_t getReadTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }
    void setReadTimeout(uint64_t v) { m_recvTimeout = v; }
    void setName(const std::string& v) { m_name = v; }
protected:
    //处理连接成功的client上的事件
    virtual void handleClient(Socket::ptr client);
    //只用于接受监听的socket上的客户端连接
    virtual void handleAccept(Socket::ptr sock);
private:
    std::vector<Socket::ptr> m_socks;   //监听的socket组
    IOManager* m_worker;                //线程池，处理accept后的sock
    IOManager* m_acceptWorker;          //只用来accept m_socks上的连接
    uint64_t m_recvTimeout;             //接收超时时间
    std::string m_name;                 //服务器名称
    bool m_isStop;                      //停止运行标志
};

}


#endif