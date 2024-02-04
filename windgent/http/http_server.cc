#include "./http_server.h"
#include "../log.h"

namespace windgent {
namespace http {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

HttpServer::HttpServer(bool keep_alive, IOManager* worker, IOManager* accept_worker)
    :TcpServer(worker, accept_worker), m_isKeepAlive(keep_alive), m_dispatcher(new ServletDispatcher) {
}

void HttpServer::handleClient(Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));
    do {
        HttpRequest::ptr req = session->recvRequest();
        if(!req) {
            LOG_INFO(g_logger) << "session recv http request error, errno = " << errno << ", errstr = " 
                << strerror(errno) << ", client: " << *client << ", keep_alive = " << m_isKeepAlive;
        }
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepAlive));
        rsp->setHeader("Server", getName());
        m_dispatcher->handle(req, rsp, session);
        session->sendResponse(rsp);

        if(!m_isKeepAlive || req->isClose()) {
            break;
        }
    }while(m_isKeepAlive);
    session->close();
}

}
}