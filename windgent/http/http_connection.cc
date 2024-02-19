#include "./http_connection.h"
#include "./http_parser.h"
#include "../log.h"
#include "../address.h"
#include "../socket.h"
#include "../util.h"

#include <sstream>

namespace windgent {
namespace http {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

HttpResult::HttpResult(int _result, HttpResponse::ptr _response, const std::string& _error)
    :result(_result), response(_response), error(_error) {
}

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result= " << result << ", error= " << error << ", response= " 
        << (response ? response->toString() : "nullptr") << "]";
    return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr socket, bool owner)
    :SocketStream(socket, owner) {
}

HttpConnection::~HttpConnection() {
    LOG_DEBUG(g_logger) << "delete HttpConnection";
}

HttpResult::ptr HttpConnection::doGet(const std::string& url, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }
    return doGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::doGet(Uri::ptr uri, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::doPost(const std::string& url, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }
    return doPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::doPost(Uri::ptr uri, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(HttpMethod::POST, uri, timeout_ms, headers, body);                            
}

HttpResult::ptr HttpConnection::doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
    }
    return doRequest(method, uri, timeout_ms, headers, body);                                  
}

HttpResult::ptr HttpConnection::doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setMethod(method);
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setBody(body);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        req->setHeader("Host", uri->getHost());
    }
    return doRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::doRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms) {
    //首先根据地址创建Socket
    Address::ptr addr = uri->getAddrFromUri();
    // LOG_INFO(g_logger) << addr->toString();
    if(!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr
                                           ,"Invalid host: " + uri->getHost());
    }
    Socket::ptr sock = Socket::createTCP(addr);
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR, nullptr
                                           ,"Create socket failed: " + addr->toString() 
                                           + ", errno= " + std::to_string(errno)
                                           + ", errstr= " + std::string(strerror(errno)));
    }
    if(!sock->connect(addr)) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAILED, nullptr
                                           ,"Connect failed: " + addr->toString());
    }
    sock->setRecvTimeout(timeout_ms);
    //然后创建HttpConnection
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    //发送请求
    int ret = conn->sendRequest(req);
    if(ret == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CLOSED_BY_PEER, nullptr
                                           ,"Request closed by peer: " + addr->toString());
    }
    if(ret < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr
                                           ,"Send socket error, errno= " + std::to_string(errno)
                                           + ", errstr= " + std::string(strerror(errno)));
    }
    //接收响应
    auto rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "Recv response timeout: " 
                            + sock->getRemoteAddress()->toString() + ", timeout_ms= " + std::to_string(timeout_ms));
    }
    // LOG_INFO(g_logger) << *rsp;
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}

HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buffer_size = HttpResponseParser::getHttpResponseBufferSize();
    std::shared_ptr<char> buffer(new char[buffer_size], [](char* ptr){ delete[] ptr; });
    char* data = buffer.get();
    uint64_t last_size = 0;      //data中剩余的未解析完的数据大小

    do {
        int len = read(data + last_size, buffer_size - last_size);      //SockStream::read
        if(len <= 0) {              //error
            close();
            return nullptr;
        }
        len += last_size;
        size_t nparse = parser->execute(data, len, false);     //解析数据并从data中移除
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        last_size = len - nparse;
        if(last_size == buffer_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    //已经填充完状态行和首部，开始填充消息体
    auto& client_parser = parser->getParser();
    std::string body;
    if(client_parser.chunked) {
       int len = last_size;
        do {
            bool begin = true;
            do {
                if(!begin || len == 0) {
                    int rt = read(data + len, buffer_size - len);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    len += rt;
                }
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true);
                if(parser->hasError()) {
                    close();
                    return nullptr;
                }
                len -= nparse;
                if(len == (int)buffer_size) {
                    close();
                    return nullptr;
                }
                begin = false;
            } while(!parser->isFinished());
            //len -= 2;
            
            LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;
            if(client_parser.content_len + 2 <= len) {
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len + 2
                        , len - client_parser.content_len - 2);
                len -= client_parser.content_len + 2;
            } else {
                body.append(data, len);
                int left = client_parser.content_len - len + 2;
                while(left > 0) {
                    int rt = read(data, left > (int)buffer_size ? (int)buffer_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, rt);
                    left -= rt;
                }
                body.resize(body.size() - 2);
                len = 0;
            }
        } while(!client_parser.chunks_done);
        parser->getData()->setBody(body);
    } else {
        uint64_t length = parser->getContentLength();
        if(length > 0) {
            body.resize(length);
            int len = 0;
            if(length >= last_size) {
                memcpy(&body[0], data, last_size);
                len = last_size;
            } else {
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= last_size;
            if(length > 0) {
                if(readFixedSize(&body[len], length) <= 0) {
                    close();
                    return nullptr;
                }
            }
            parser->getData()->setBody(body);
        }
    }

    return parser->getData();
}

int HttpConnection::sendRequest (HttpRequest::ptr req) {
    std::stringstream ss;
    ss << *req;
    std::string data = ss.str();
    return writeFixedSize(data.c_str(), data.size());
}

HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port
                      , uint32_t maxSize, uint32_t maxAliveTime, uint32_t maxRequest)
    :m_host(host), m_vhost(vhost), m_port(port), m_maxSize(maxSize)
    ,m_maxAliveTime(maxAliveTime), m_maxRequest(maxRequest) {
}

HttpConnection::ptr HttpConnectionPool::getConnection() {
    uint64_t now_ms = windgent::GetCurrentMS();
    HttpConnection* ptr = nullptr;
    MutexType::Lock lock(m_mutex);
    std::vector<HttpConnection*> invalid_conns;
    while(!m_conns.empty()) {
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        if(!conn->isConnected()) {
            invalid_conns.push_back(conn);
            continue;
        }
        // LOG_DEBUG(g_logger) << "create time = " << conn->m_createTime;
        if((conn->m_createTime + m_maxAliveTime) < now_ms) {
            invalid_conns.push_back(conn);
            continue;
        }
        ptr = conn;
        // LOG_DEBUG(g_logger) << "get conn from pool";
        break;
    }
    lock.unlock();
    for(auto& i : invalid_conns) {
        delete i;
    }
    m_total -= invalid_conns.size();
    //连接池中没有可用的连接，单独创建
    if(!ptr) {
        IPAddress::ptr addr = Address::getAnyIPAddrFromHost(m_host);
        if(!addr) {
            LOG_ERROR(g_logger) << "Get address failed: " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);

        Socket::ptr sock = Socket::createTCP(addr);
        if(!sock) {
            LOG_ERROR(g_logger) << "Create socket failed: " << *addr;
            return nullptr;
        }
        if(!sock->connect(addr)) {
            LOG_ERROR(g_logger) << "Connect failed: " << *addr;
            return nullptr;
        }
        ptr = new HttpConnection(sock);
        ptr->setCreateTime(windgent::GetCurrentMS());
        ++m_total;
        // LOG_DEBUG(g_logger) << "create a new conn";
    }
    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::releasePtr, std::placeholders::_1, this));
}

HttpResult::ptr HttpConnectionPool::doGet(const std::string& url, uint64_t timeout_ms
                        ,const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri, uint64_t timeout_ms
                        ,const std::map<std::string, std::string>& headers, const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string& url, uint64_t timeout_ms
                        ,const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri, uint64_t timeout_ms
                        ,const std::map<std::string, std::string>& headers , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers, const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setMethod(method);
    req->setPath(url);
    req->setClose(false);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }

    if(!has_host) {
        if(m_vhost.empty()) {
            req->setHeader("Host", m_host);
        } else {
            req->setHeader("Host", m_vhost);
        }
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers, const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req, uint64_t timeout_ms) {
    HttpConnection::ptr conn = getConnection();
    if(!conn) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONN_FAILED, nullptr
                                           ,"Pool host: " + m_host + " port: " + std::to_string(m_port));
    }
    Socket::ptr sock = conn->getSocket();
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONN, nullptr
                                           ,"Pool host: " + m_host + " port: " + std::to_string(m_port));
    }
    sock->setRecvTimeout(timeout_ms);
    //发送请求
    int ret = conn->sendRequest(req);
    if(ret == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CLOSED_BY_PEER, nullptr
                                           ,"Request closed by peer: " + sock->getRemoteAddress()->toString());
    }
    if(ret < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr
                                           ,"Send socket error, errno= " + std::to_string(errno)
                                           + ", errstr= " + std::string(strerror(errno)));
    }
    //接收响应
    auto rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "Recv response timeout: " 
                            + sock->getRemoteAddress()->toString() + ", timeout_ms= " + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}

void HttpConnectionPool::releasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ++ptr->m_request;
    if(!ptr->isConnected() || (ptr->m_createTime + pool->m_maxAliveTime) < windgent::GetCurrentMS()
        || ptr->m_request > pool->m_maxRequest) {
        delete ptr;
        --pool->m_total;
        return;
    }
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}

}
}