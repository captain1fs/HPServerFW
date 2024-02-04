#include "./servlet.h"
#include <fnmatch.h>

namespace windgent {
namespace http {

FunctionServlet::FunctionServlet(callback cb)
    :Servlet("FunctionServlet"), m_cb(cb) {
}

int32_t FunctionServlet::handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) {
    return m_cb(request, response, session);
}

ServletDispatcher::ServletDispatcher()
    :Servlet("ServletDispatcher"), m_default(new NotFoundServlet("nothing")) {
}

int32_t ServletDispatcher::handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) {
    auto slt = getMatchedServlet(request->getPath());
    if(slt) {
        slt->handle(request, response, session);
    }
    return 0;
}

void ServletDispatcher::addServlet(const std::string& uri, Servlet::ptr slt) {
    RWMutexType::WrLock lock(m_mutex);
    m_datas[uri] = slt;
}

void ServletDispatcher::addServlet(const std::string& uri, FunctionServlet::callback cb) {
    RWMutexType::WrLock lock(m_mutex);
    m_datas[uri].reset(new FunctionServlet(cb));
}

void ServletDispatcher::addGlobServlet(const std::string& uri, Servlet::ptr slt) {
    RWMutexType::WrLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, slt));
}

void ServletDispatcher::addGlobServlet(const std::string& uri, FunctionServlet::callback cb) {
    RWMutexType::WrLock lock(m_mutex);
    addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

void ServletDispatcher::delServlet(const std::string& uri) {
    RWMutexType::WrLock lock(m_mutex);
    m_datas.erase(uri);
}

void ServletDispatcher::delGlobServlet(const std::string& uri) {
    RWMutexType::WrLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}

Servlet::ptr ServletDispatcher::getServlet(const std::string& uri) {
    RWMutexType::RdLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

Servlet::ptr ServletDispatcher::getGlobServlet(const std::string& uri) {
    RWMutexType::RdLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if(it->first == uri) {
            return it->second;
        }
    }
    return nullptr;
}

Servlet::ptr ServletDispatcher::getMatchedServlet(const std::string& uri) {
    RWMutexType::RdLock lock(m_mutex);
    auto mit = m_datas.find(uri);
    if(mit != m_datas.end()) {
        return mit->second;
    }
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it) {
        if(!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            return it->second;
        }
    }
    return m_default;
}

NotFoundServlet::NotFoundServlet(const std::string& name)
    :Servlet("NotFoundServlet"), m_name(name) {
    m_content = "<html><head><title>404 Not Found"
                "</title></head><body><center><h1>404 Not Found</h1></center>"
                "<hr><center>" + name + "</center></body></html>";
}

int32_t NotFoundServlet::handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) {
    response->setStatus(HttpStatus::NOT_FOUND);
    response->setHeader("Server", "windgent/1.0.0");
    response->setHeader("Content-Type", "text/html");
    response->setBody(m_content);
    return 0;
}

}
}