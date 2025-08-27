#include "network/poller.h"
#include "network/channel.h"
#include "network/reactor.h"
#include "common/logger.h"
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/epoll.h>
#include <unistd.h>
#endif

namespace order_engine {
namespace network {

Poller::Poller(Reactor* reactor) : reactor_(reactor) {}

void Poller::fillActiveChannels(int num_events, ChannelList* active_channels) const {
    // 具体实现由子类提供
}

#ifdef _WIN32

SelectPoller::SelectPoller(Reactor* reactor) : Poller(reactor) {
    LOG_INFO("Using SelectPoller for Windows");
}

void SelectPoller::poll(int timeout_ms, ChannelList* active_channels) {
    // Windows select实现
    if (polled_channels_.empty()) {
        // 没有要监听的channel，直接返回
        return;
    }
    
    fd_set read_fds, write_fds, except_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    
    int max_fd = 0;
    for (auto* channel : polled_channels_) {
        int fd = channel->fd();
        if (channel->isReading()) {
            FD_SET(fd, &read_fds);
        }
        if (channel->isWriting()) {
            FD_SET(fd, &write_fds);
        }
        FD_SET(fd, &except_fds);
        max_fd = std::max(max_fd, fd);
    }
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(max_fd + 1, &read_fds, &write_fds, &except_fds, 
                       timeout_ms < 0 ? nullptr : &tv);
    
    if (result > 0) {
        for (auto* channel : polled_channels_) {
            int fd = channel->fd();
            int revents = 0;
            
            if (FD_ISSET(fd, &read_fds)) {
                revents |= Channel::kReadEvent;
            }
            if (FD_ISSET(fd, &write_fds)) {
                revents |= Channel::kWriteEvent;
            }
            if (FD_ISSET(fd, &except_fds)) {
                revents |= Channel::kErrorEvent;
            }
            
            if (revents != 0) {
                channel->setRevents(revents);
                active_channels->push_back(channel);
            }
        }
    } else if (result < 0) {
        LOG_ERROR("select() failed");
    }
}

void SelectPoller::updateChannel(Channel* channel) {
    int fd = channel->fd();
    auto it = channels_.find(fd);
    
    if (it == channels_.end()) {
        // 新的channel
        channels_[fd] = channel;
        polled_channels_.push_back(channel);
        LOG_TRACE("Added channel to poller");
    } else {
        // 已存在的channel，已经在polled_channels_中
        LOG_TRACE("Updated channel in poller");
    }
}

void SelectPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    auto it = channels_.find(fd);
    
    if (it != channels_.end()) {
        channels_.erase(it);
        
        // 从polled_channels_中移除
        auto vec_it = std::find(polled_channels_.begin(), 
                               polled_channels_.end(), channel);
        if (vec_it != polled_channels_.end()) {
            polled_channels_.erase(vec_it);
        }
        
        LOG_TRACE("Removed channel from poller");
    }
}

#else

EpollPoller::EpollPoller(Reactor* reactor) 
    : Poller(reactor), 
      epoll_fd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    if (epoll_fd_ < 0) {
        LOG_CRITICAL("epoll_create1() failed");
        abort();
    }
    LOG_INFO("Using EpollPoller for Linux");
}

EpollPoller::~EpollPoller() {
    close(epoll_fd_);
}

void EpollPoller::poll(int timeout_ms, ChannelList* active_channels) {
    int num_events = epoll_wait(epoll_fd_, &*events_.begin(), 
                               static_cast<int>(events_.size()), timeout_ms);
    
    if (num_events > 0) {
        LOG_TRACE("epoll_wait returned events");
        fillActiveChannels(num_events, active_channels);
        
        if (static_cast<size_t>(num_events) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (num_events == 0) {
        LOG_TRACE("epoll_wait timeout");
    } else {
        LOG_ERROR("epoll_wait() failed");
    }
}

void EpollPoller::fillActiveChannels(int num_events, ChannelList* active_channels) const {
    for (int i = 0; i < num_events; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        active_channels->push_back(channel);
    }
}

void EpollPoller::updateChannel(Channel* channel) {
    const int index = channel->index();
    int fd = channel->fd();
    
    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            channels_[fd] = channel;
        }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);
    
    int index = channel->index();
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}

void EpollPoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    
    int fd = channel->fd();
    if (epoll_ctl(epoll_fd_, operation, fd, &event) < 0) {
        LOG_ERROR("epoll_ctl failed");
    }
}

#endif

} // namespace network
} // namespace order_engine
