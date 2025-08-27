#include "network/reactor.h"
#include "network/channel.h"
#include "common/logger.h"
#include <cassert>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/eventfd.h>
#include <unistd.h>
#include <sys/epoll.h>
#endif

namespace order_engine {
namespace network {



// Reactor实现
Reactor::Reactor()
    : quit_(false)
    , calling_pending_tasks_(false)
#ifdef _WIN32
    , poller_(std::make_unique<SelectPoller>(this))
#else
    , poller_(std::make_unique<EpollPoller>(this))
#endif
    , wakeup_fd_(createEventfd())
    , wakeup_channel_(std::make_unique<Channel>(this, wakeup_fd_))
    , thread_id_(std::this_thread::get_id()) {
    
    LOG_DEBUG("Reactor created");
    
    wakeup_channel_->setReadCallback([this] { handleWakeup(); });
    wakeup_channel_->enableReading();
}

Reactor::~Reactor() {
    LOG_DEBUG("Reactor destroyed");
    wakeup_channel_->disableAll();
    wakeup_channel_->remove();
#ifdef _WIN32
    closesocket(wakeup_fd_);
#else
    ::close(wakeup_fd_);
#endif
}

int Reactor::createEventfd() {
#ifdef _WIN32
    // Windows简化实现：创建一个自连接的socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        LOG_CRITICAL("Failed to create wakeup socket");
        abort();
    }
    LOG_TRACE("Created wakeup socket");
    return static_cast<int>(sock);
#else
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_CRITICAL("Failed to create eventfd");
        abort();
    }
    LOG_TRACE("Created eventfd");
    return evtfd;
#endif
}

void Reactor::loop() {
    assert(!quit_);
    assert(isInLoopThread());
    
    LOG_INFO("Reactor started looping");
    
    while (!quit_) {
        active_channels_.clear();
        
        // 等待事件，超时时间1秒
        poller_->poll(1000, &active_channels_);
        
        // 处理活跃事件
        for (Channel* channel : active_channels_) {
            channel->handleEvent();
        }
        
        // 处理待执行任务
        doPendingTasks();
    }
    
    LOG_INFO("Reactor stopped looping");
}

void Reactor::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

bool Reactor::isInLoopThread() const {
    return thread_id_ == std::this_thread::get_id();
}

void Reactor::updateChannel(Channel* channel) {
    assert(channel->ownerReactor() == this);
    assert(isInLoopThread());
    poller_->updateChannel(channel);
}

void Reactor::removeChannel(Channel* channel) {
    assert(channel->ownerReactor() == this);
    assert(isInLoopThread());
    
    // 确保Channel不在活跃列表中
    if (std::find(active_channels_.begin(), active_channels_.end(), channel) 
        != active_channels_.end()) {
        LOG_WARN("Removing active channel");
    }
    
    poller_->removeChannel(channel);
}

void Reactor::runInLoop(const Task& task) {
    if (isInLoopThread()) {
        task();
    } else {
        queueInLoop(task);
    }
}

void Reactor::queueInLoop(const Task& task) {
    {
        std::lock_guard<std::mutex> lock(pending_tasks_mutex_);
        pending_tasks_.push_back(task);
    }
    
    if (!isInLoopThread() || calling_pending_tasks_) {
        wakeup();
    }
}

void Reactor::runAt(const Task& task, time_t when) {
    // 简化实现：计算延迟并使用runAfter
    time_t now = time(nullptr);
    double delay = static_cast<double>(when - now);
    if (delay <= 0) {
        runInLoop(task);
    } else {
        runAfter(task, delay);
    }
}

void Reactor::runAfter(const Task& task, double delay_seconds) {
    // 简化实现：使用线程定时器
    std::thread([this, task, delay_seconds]() {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<int>(delay_seconds * 1000)));
        runInLoop(task);
    }).detach();
}

void Reactor::runEvery(const Task& task, double interval_seconds) {
    // 简化实现：递归调用
    runAfter([this, task, interval_seconds]() {
        task();
        runEvery(task, interval_seconds);
    }, interval_seconds);
}

void Reactor::wakeup() {
#ifdef _WIN32
    // Windows简化实现：通过socket发送数据
    char one = 1;
    int n = send(wakeup_fd_, &one, sizeof(one), 0);
    if (n != sizeof(one)) {
        LOG_ERROR("Reactor::wakeup() failed");
    }
#else
    uint64_t one = 1;
    ssize_t n = ::write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("Reactor::wakeup() failed");
    }
#endif
}

void Reactor::handleWakeup() {
#ifdef _WIN32
    // Windows简化实现：从socket读取数据
    char one = 1;
    int n = recv(wakeup_fd_, &one, sizeof(one), 0);
    if (n != sizeof(one)) {
        LOG_ERROR("Reactor::handleWakeup() failed");
    }
#else
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("Reactor::handleWakeup() failed");
    }
#endif
    LOG_TRACE("Reactor woken up");
}

void Reactor::doPendingTasks() {
    std::vector<Task> tasks;
    calling_pending_tasks_ = true;
    
    {
        std::lock_guard<std::mutex> lock(pending_tasks_mutex_);
        tasks.swap(pending_tasks_);
    }
    
    for (const Task& task : tasks) {
        task();
    }
    
    calling_pending_tasks_ = false;
}

} // namespace network
} // namespace order_engine
