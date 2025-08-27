#pragma once

#include <vector>
#include <unordered_map>

namespace order_engine {
namespace network {

class Channel;
class Reactor;

/**
 * @brief 跨平台I/O多路复用器基类
 * 
 * Windows使用select或IOCP实现
 * Linux使用epoll实现
 */
class Poller {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(Reactor* reactor);
    virtual ~Poller() = default;

    // 事件轮询
    virtual void poll(int timeout_ms, ChannelList* active_channels) = 0;
    
    // Channel管理
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

protected:
    void fillActiveChannels(int num_events, ChannelList* active_channels) const;
    
    Reactor* reactor_;
    std::unordered_map<int, Channel*> channels_;
};

#ifdef _WIN32
/**
 * @brief Windows select实现
 * 
 * 简单的select实现，适用于少量连接场景
 * 后续可扩展为IOCP实现
 */
class SelectPoller : public Poller {
public:
    SelectPoller(Reactor* reactor);
    ~SelectPoller() override = default;

    void poll(int timeout_ms, ChannelList* active_channels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    std::vector<Channel*> polled_channels_;
};
#else
/**
 * @brief Linux epoll实现
 */
class EpollPoller : public Poller {
public:
    EpollPoller(Reactor* reactor);
    ~EpollPoller() override;

    void poll(int timeout_ms, ChannelList* active_channels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    void update(int operation, Channel* channel);
    
    static const int kInitEventListSize = 16;
    static const int kNew = -1;
    static const int kAdded = 1;
    static const int kDeleted = 2;
    
    int epoll_fd_;
    std::vector<struct epoll_event> events_;
};
#endif

} // namespace network
} // namespace order_engine
