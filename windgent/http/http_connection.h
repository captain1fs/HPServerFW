#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include "../socket_stream.h"
#include "../uri.h"
#include "../mutex.h"
#include "http.h"
#include <map>
#include <list>

namespace windgent {
namespace http {

struct HttpResult {
    typedef std::shared_ptr<HttpResult> ptr;

    enum class Error {
        OK = 0,
        INVALID_URL = 1,
        INVALID_HOST = 2,
        CONNECT_FAILED = 3,
        CLOSED_BY_PEER = 4,
        SEND_SOCKET_ERROR = 5,
        TIMEOUT = 6,
        CREATE_SOCKET_ERROR = 7,
        POOL_GET_CONN_FAILED = 8,
        POOL_INVALID_CONN = 9,
    };

    HttpResult(int _result, HttpResponse::ptr _response, const std::string& _error);
    std::string toString() const;
    int result;                     //错误码
    HttpResponse::ptr response;
    std::string error;              //错误描述
};

class HttpConnectionPool;
//客户端使用
class HttpConnection : public SocketStream {
    friend class HttpConnectionPool;
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr socket, bool owner = true);
    ~HttpConnection();
    //更方便的使用方法，无需创建socket等等，直接通过uri与服务器通信
    static HttpResult::ptr doGet(const std::string& url, uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr doGet(Uri::ptr uri, uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr doPost(const std::string& url, uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr doPost(Uri::ptr uri, uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms
                                    ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms
                                    ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr doRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms);

    //流程：接收m_socket上发来的响应报文 --> 通过HttpResponseParser::execute执行解析响应报文的所有回调函数(on_response_http_field...)来填充m_data的所有内容(状态行、首部、消息体...)
    HttpResponse::ptr recvResponse();
    int sendRequest (HttpRequest::ptr req);

    void setCreateTime(uint64_t v) { m_createTime = v; }
private:
    uint64_t m_createTime = 0;
    uint64_t m_request = 0;
};

class HttpConnectionPool {
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;
    typedef Mutex MutexType;

    HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port
                      , uint32_t maxSize, uint32_t maxAliveTime, uint32_t maxRequest);
    HttpConnection::ptr getConnection();

    HttpResult::ptr doGet(const std::string& url, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    HttpResult::ptr doGet(Uri::ptr uri, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    HttpResult::ptr doPost(const std::string& url, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    HttpResult::ptr doPost(Uri::ptr uri, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms
                              ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms
                             ,const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);
private:
    //当连接释放时调用此函数，决定是直接释放此链接，还是加入连接池重用
    static void releasePtr(HttpConnection* ptr, HttpConnectionPool* pool);
private:
    std::string m_host;
    std::string m_vhost;
    uint32_t m_port;
    uint32_t m_maxSize;         //连接池中的最大连接数量
    uint32_t m_maxAliveTime;    //连接存活的最长时间
    uint32_t m_maxRequest;      //每条连接处理的最大请求数

    MutexType m_mutex;
    std::list<HttpConnection*> m_conns;
    std::atomic<int32_t> m_total = {0};
};

}
}

#endif