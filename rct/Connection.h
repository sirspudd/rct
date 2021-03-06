#ifndef CONNECTION_H
#define CONNECTION_H

#include <rct/Buffer.h>
#include <rct/Message.h>
#include <rct/SocketClient.h>
#include <rct/String.h>
#include <rct/LinkedList.h>
#include <rct/Map.h>
#include <rct/ResponseMessage.h>
#include <rct/SignalSlot.h>
#include <rct/FinishMessage.h>

class ConnectionPrivate;
class SocketClient;
class Event;
class Connection : public std::enable_shared_from_this<Connection>
{
public:
    static std::shared_ptr<Connection> create(int version = 0)
    {
        return std::shared_ptr<Connection>(new Connection(version));
    }
    static std::shared_ptr<Connection> create(const std::shared_ptr<SocketClient> &client, int version = 0)
    {
        std::shared_ptr<Connection> ret(new Connection(version));
        ret->connect(client);
        return ret;
    }
    ~Connection();

    void setVersion(int version) { mVersion = version; }
    int version() const { return mVersion; }

    void setSilent(bool on) { mSilent = on; }
    bool isSilent() const { return mSilent; }

    bool connectUnix(const Path &socketFile, int timeout = 0);
    bool connectTcp(const String &host, uint16_t port, int timeout = 0);

    int pendingWrite() const;

    bool send(const Message &message);
    template <int StaticBufSize>
    bool write(const char *format, ...)
    {
        if (mSilent)
            return isConnected();
        va_list args;
        va_start(args, format);
        const String ret = String::format<StaticBufSize>(format, args);
        va_end(args);
        return send(ResponseMessage(ret));
    }
    bool write(const String &out, ResponseMessage::Type type = ResponseMessage::Stdout)
    {
        if (mSilent)
            return isConnected();
        return send(ResponseMessage(out, type));
    }

    void finish(int status = 0) { send(FinishMessage(status)); }
    template <int StaticBufSize>
    void finish(const char *format, ...)
    {
        if (!mSilent) {
            va_list args;
            va_start(args, format);
            const String ret = String::format<StaticBufSize>(format, args);
            va_end(args);
            send(ResponseMessage(ret));
        }
        send(FinishMessage(0));
    }

    void finish(const String &msg, int status = 0)
    {
        if (!mSilent)
            send(ResponseMessage(msg));
        send(FinishMessage(status));
    }

    int finishStatus() const { return mFinishStatus; }

    void close() { assert(mSocketClient); mSocketClient->close(); }

    bool isConnected() const { return mSocketClient->isConnected(); }

    Signal<std::function<void(std::shared_ptr<Connection>)> > &sendFinished() { return mSendFinished; }
    Signal<std::function<void(std::shared_ptr<Connection>)> > &connected() { return mConnected; }
    Signal<std::function<void(std::shared_ptr<Connection>)> > &disconnected() { return mDisconnected; }
    Signal<std::function<void(std::shared_ptr<Connection>)> > &error() { return mError; }
    Signal<std::function<void(std::shared_ptr<Connection>, int)> > &finished() { return mFinished; }
    Signal<std::function<void(std::shared_ptr<Connection>, const Message *)> > &aboutToSend() { return mAboutToSend; }
    Signal<std::function<void(std::shared_ptr<Message>, std::shared_ptr<Connection>)> > &newMessage() { return mNewMessage; }
    SocketClient::SharedPtr client() const { return mSocketClient; }

private:
    Connection(int version);
    void disconnect();
    void connect(const SocketClient::SharedPtr &client);
    void onClientConnected(const SocketClient::SharedPtr&) { mIsConnected = true; mConnected(shared_from_this()); }
    void onClientDisconnected(const SocketClient::SharedPtr&) { mIsConnected = false; mDisconnected(shared_from_this()); }
    void onDataAvailable(const SocketClient::SharedPtr&, Buffer&& buffer);
    void onDataWritten(const SocketClient::SharedPtr&, int);
    void onSocketError(const SocketClient::SharedPtr&, SocketClient::Error error)
    {
        ::warning() << "Socket error" << error << errno << Rct::strerror();
        mError(shared_from_this());
        mDisconnected(shared_from_this());
    }
    void checkData();

    SocketClient::SharedPtr mSocketClient;
    LinkedList<Buffer> mBuffers;
    int mPendingRead, mPendingWrite, mTimeoutTimer, mCheckTimer, mFinishStatus, mVersion;

    bool mSilent, mIsConnected, mWarned;

    Signal<std::function<void(std::shared_ptr<Message>, std::shared_ptr<Connection>)> > mNewMessage;
    Signal<std::function<void(std::shared_ptr<Connection>)> > mConnected, mDisconnected, mError, mSendFinished;
    Signal<std::function<void(std::shared_ptr<Connection>, int)> > mFinished;
    Signal<std::function<void(std::shared_ptr<Connection>, const Message *)> > mAboutToSend;

};

#endif // CONNECTION_H
