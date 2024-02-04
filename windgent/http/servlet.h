#ifndef __SERVLET_H__
#define __SERVLET_H__

#include "./http.h"
#include "./http_session.h"
#include "../mutex.h"

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace windgent {
namespace http {

class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(const std::string& name): m_name(name) { }
    virtual ~Servlet() { }

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) = 0;
    const std::string& getName() const { return m_name; }
private:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public: 
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t (HttpRequest::ptr request
            , HttpResponse::ptr response, HttpSession::ptr session)> callback;
    
    FunctionServlet(callback cb);
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;
private:
    callback m_cb;
};

class ServletDispatcher : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatcher> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatcher();
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);
    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }
    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);
    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    RWMutexType m_mutex;
    //uri到servlet的映射，精准匹配
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    //模糊匹配
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet {
public: 
    typedef std::shared_ptr<NotFoundServlet> ptr;
    
    NotFoundServlet(const std::string& name);
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;
private:
    std::string m_name;
    std::string m_content;
};

}
}

#endif