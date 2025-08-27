#include "network/channel.h"
#include "network/reactor.h"
#include "common/logger.h"
#include <cassert>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/epoll.h>
#endif

namespace order_engine {
namespace network {

// Channel类的静态成员定义
const int Channel::kNoneEvent = 0;
#ifdef _WIN32
const int Channel::kReadEvent = 1;
const int Channel::kWriteEvent = 2; 
const int Channel::kErrorEvent = 4;
#else
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;
const int Channel::kErrorEvent = EPOLLERR;
#endif

Channel::Channel(Reactor* reactor, int fd)
    : reactor_(reactor)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1) {
    LOG_TRACE("Channel created");
}

Channel::~Channel() {
    assert(!isReading() && !isWriting());
    LOG_TRACE("Channel destroyed");
}

void Channel::handleEvent() {
    LOG_TRACE("Channel::handleEvent() called");
    
#ifdef _WIN32
    // Windows select 实现
    if (revents_ & kErrorEvent) {
        LOG_ERROR("Channel::handleEvent() ERROR");
        if (error_callback_) {
            error_callback_();
        }
    }
    
    if (revents_ & kReadEvent) {
        if (read_callback_) {
            read_callback_();
        }
    }
    
    if (revents_ & kWriteEvent) {
        if (write_callback_) {
            write_callback_();
        }
    }
#else
    // Linux epoll 实现  
    // 处理挂起事件
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        LOG_WARN("Channel::handleEvent() EPOLLHUP");
        if (close_callback_) {
            close_callback_();
        }
    }
    
    // 处理错误事件
    if (revents_ & (EPOLLERR | EPOLLNVAL)) {
        LOG_ERROR("Channel::handleEvent() EPOLLERR");
        if (error_callback_) {
            error_callback_();
        }
    }
    
    // 处理读事件
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (read_callback_) {
            read_callback_();
        }
    }
    
    // 处理写事件
    if (revents_ & EPOLLOUT) {
        if (write_callback_) {
            write_callback_();
        }
    }
#endif
}

void Channel::update() {
    reactor_->updateChannel(this);
}

void Channel::remove() {
    assert(isNoneEvent());
    reactor_->removeChannel(this);
}

} // namespace network
} // namespace order_engine
