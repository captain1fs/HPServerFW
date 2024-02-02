#include "./tcp_server.h"
#include "./log.h"
#include "./config.h"

namespace windgent {

static windgent::Logger::ptr g_logger = LOG_NAME("system");
static windgent::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = windgent::ConfigMgr::Lookup<uint64_t>("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeout");

TcpServer::TcpServer(IOManager* worker, IOManager* accept_worker)
    :m_worker(worker), m_acceptWorker(accept_worker), m_recvTimeout(g_tcp_server_read_timeout->getVal())
    ,m_name("windgent/1.0.0"), m_isStop(true) {
}

TcpServer::~TcpServer() {
    for(auto& sock : m_socks) {
        sock->close();
    }
    m_socks.clear();
}

bool TcpServer::bind(Address::ptr addr) {
    std::vector<Address::ptr> addrs, fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr> addrs, std::vector<Address::ptr> fails) {
    for(auto& addr : addrs) {
        Socket::ptr sock = Socket::createTCP(addr);
        if(!sock->bind(addr)) {
            LOG_ERROR(g_logger) << "bind errno = " << errno << ", errstr = " << strerror(errno)
                                << ", addr = [" << addr->toString() << "]";
            fails.push_back(addr);
            continue; 
        }
        if(!sock->listen()) {
            LOG_ERROR(g_logger) << "listen errno = " << errno << ", errstr = " << strerror(errno)
                                << ", addr = [" << addr->toString() << "]";
            fails.push_back(addr);
            continue; 
        }
        m_socks.push_back(sock);
    }

    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }

    for(auto& sock : m_socks) {
        LOG_INFO(g_logger) << "server bind success: " << *sock;
    }
    return true;
}

bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::handleAccept, shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self](){
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client) {
    LOG_INFO(g_logger) << "handleClient: " << *client;
}

void TcpServer::handleAccept(Socket::ptr sock) {
    while(!m_isStop) {
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            m_worker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        } else {
            LOG_ERROR(g_logger) << "handleAccept errno = " << errno << ", errstr = " << strerror(errno);
        }
    }
}

}