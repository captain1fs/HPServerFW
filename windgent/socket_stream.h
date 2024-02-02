#ifndef __SOCKET_STREAM_H__
#define __SOCKET_STREAM_H__

#include <memory>
#include "socket.h"
#include "stream.h"

namespace windgent {

class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(Socket::ptr socket, bool owner = true);
    ~SocketStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;
    virtual void close() override;

    Socket::ptr getSocket() const { return m_socket; }
    bool isConnected() const;
protected:
    Socket::ptr m_socket;
    bool m_owner;           //SocketStream是否有对m_socket的主导权
};

}

#endif