#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <memory>
#include "../tcp_server.h"
#include "./http_session.h"
#include "./servlet.h"

namespace windgent {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keep_alive = false, IOManager* worker = IOManager::GetThis()
              ,IOManager* accept_worker = IOManager::GetThis());
    
    ServletDispatcher::ptr getDispatcher() const { return m_dispatcher; }
    void setDispatcher(ServletDispatcher::ptr v) { m_dispatcher = v; }
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepAlive;
    ServletDispatcher::ptr m_dispatcher;
};

}
}

#endif