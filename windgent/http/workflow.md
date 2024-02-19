## HTTP模块

封装了HttpRequest、HttpResponse作为http的请求和响应报文类，提供序列化功能。
```cpp
//请求与响应报文的封装
class HttpRequest {
    //set & get function ...
    uint8_t getVersion() const;
    void setVersion(uint8_t v);
    //request data format
    HttpMethod m_method;
    uint8_t m_version;
    bool m_close;               //是否自动关闭
    //uri : https://baike.baidu.com/item/cpp/216267?fr=ge_ala
    std::string m_path;         //请求路径
    std::string m_query;        //请求参数
    std::string m_fragment;     //请求片段
    std::string m_body;         //请求消息体
    MapType m_headers;          //请求头部map
    MapType m_params;           //请求参数map
    MapType m_cookies;          //请求cookie map
};

class HttpResponse {
    //set & get function ...
    const std::string& getBody() const;
    void setBody(const std::string& v);
    //rsponse data format
    HttpStatus m_status;
    uint8_t m_version;
    bool m_close;
    std::string m_reason;   //响应原因
    std::string m_body;     //响应消息体
    MapType m_headers;      //响应首部
}

//报文解析类，借助现有的http11_parser封装了HttpRequestParser、HttpResponseParser，用于解析http请求与相应报文。
//工作原理：初始化时为m_parser中的所有函数指针指定了处理请求报文各部分的处理函数(设置m_data各成员)，通过execute调用http_parser的http_parser_execute来执行这些函数以解析报文的各个部分。
class HttpRequestParser {
public:
    HttpRequestParser();
    //解析请求报文，返回已经解析的长度，并移除已经解析的数据
    size_t execute(char* data, size_t len);
    int isFinished();
    int hasError();
private:
    http_parser m_parser;           //请求报文解析结构体
    HttpRequest::ptr m_data;
    int m_error;
};
//工作原理同上
class HttpResponseParser {
public:
    HttpResponseParser();
    size_t execute(char* data, size_t len, bool chunk);
    int isFinished();
    int hasError();
    uint64_t getContentLength();
private:
    httpclient_parser m_parser;     //响应报文解析结构体
    HttpResponse::ptr m_data;
    int m_error;
};
```

//封装Tcp_server
```cpp
//工作流程：创建socket后bind指定的IP地址并开始监听该socket，start后m_acceptWorker的所有线程只负责接受监听socket上到来的客户端连接(handleAccept)，然后将该client指定给m_worker的一个线程，该线程负责处理client上的所有活动(handleClient)
class TcpServer {
public:
    TcpServer(IOManager* worker = IOManager::GetThis(), IOManager* accept_worker = IOManager::GetThis());
    virtual ~TcpServer();
    //为m_socks绑定地址
    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr> addrs, std::vector<Address::ptr> fails);
    virtual bool start();
    virtual void stop();
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
```

HttpSession：封装Server端accept产生的Socket而成
HttpConnection: 封装Client端connect的Socket而成

```cpp
//流程：SocketStream::read接收m_socket上发来的请求报文 --> 通过HttpRequestParser::execute执行解析请求报文的所有回调函数(on_request_http_field...)来填充m_data的所有内容(请求行、首部、消息体...)
class HttpSession {
    HttpRequest::ptr recvRequest();
    //根据请求报文来填充并发送序列化后的HttpResponse
    int sendResponse (HttpResponse::ptr rsp);
};

//借助Tcp_Server和HttpSession封装了一个HttpServer
class HttpServer : public TcpServer {
public:
    HttpServer(bool keep_alive = false, IOManager* worker = IOManager::GetThis()
              ,IOManager* accept_worker = IOManager::GetThis());
protected:
    //调用HttpSession的方法来接收客户端请求并发送响应报文
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepAlive;
};
```
封装了Servlet，针对不同的uri请求提供不同的处理方法。假如要上传下载文件，只需要继承该类然后实现handle函数：拿到请求的文件内容填充响应。
整合HttpServer和Servlet，根据不同的请求使用不同的Servlet去处理。
```cpp
class Servlet {
public:
    Servlet(const std::string& name): m_name(name) { }
    virtual ~Servlet() { }
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) = 0;
private:
    std::string m_name;
};
```