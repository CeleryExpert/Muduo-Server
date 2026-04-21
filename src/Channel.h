#pragma once
#include "noncopyable.h"
#include <functional>
#include <memory>

class EventLoop;
class Timestamp;
class Channel
{
public:
    using EventCallBack = std::function<void()>;
    using ReadEventCallBack = std::function<void(const Timestamp&)>;
    Channel(EventLoop *loop, int fd);
    ~Channel();

    void setReadCallBack(ReadEventCallBack cb)
    {
        readCallBack_ = std::move(cb);
    }
    void setWriteCallBack(EventCallBack cb)
    {
        writeCallBack_ = std::move(cb);
    }
    void setCloseCallBack(EventCallBack cb)
    {
        closeCallBack_ = std::move(cb);
    }
    void setErrorCallBack(EventCallBack cb)
    {
        errorCallBack_ = std::move(cb);
    }

    void tie(const std::shared_ptr<void> &);
    void handleEvent(const Timestamp& recievetime);

private:
    void update(); 
    void HandleEventsWithGuard(const Timestamp& recievetime);
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    int fd_;
    int event_;
    int revent_;

    ReadEventCallBack readCallBack_;
    EventCallBack writeCallBack_;
    EventCallBack closeCallBack_;
    EventCallBack errorCallBack_;

    std::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling_;
    bool addedtoLoop_;
};
