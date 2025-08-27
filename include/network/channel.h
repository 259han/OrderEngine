#pragma once

#include <functional>

namespace order_engine {
namespace network {

class Reactor;

/**
 * @brief 事件通道
 * 
 * 封装文件描述符和其感兴趣的事件
 */
class Channel {
public:
    using EventCallback = std::function<void()>;

    Channel(Reactor* reactor, int fd);
    ~Channel();

    // 事件处理
    void handleEvent();
    
    // 事件设置
    void setReadCallback(const EventCallback& cb) { read_callback_ = cb; }
    void setWriteCallback(const EventCallback& cb) { write_callback_ = cb; }
    void setCloseCallback(const EventCallback& cb) { close_callback_ = cb; }
    void setErrorCallback(const EventCallback& cb) { error_callback_ = cb; }
    
    // 事件启用/禁用
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    
    // 状态查询
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    
    int fd() const { return fd_; }
    int events() const { return events_; }
    void setRevents(int revents) { revents_ = revents; }
    
    // Poller相关
    int index() const { return index_; }
    void setIndex(int index) { index_ = index; }
    
    // 获取所属Reactor
    Reactor* ownerReactor() const { return reactor_; }
    
    // 从Reactor中移除
    void remove();

    // 事件常量
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    static const int kErrorEvent;

private:
    void update();
    
    Reactor* reactor_;
    int fd_;
    int events_;
    int revents_;
    int index_; // used by Poller
    
    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

} // namespace network
} // namespace order_engine
