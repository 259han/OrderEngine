#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "poller.h"
#include "channel.h"

namespace order_engine {
namespace network {

/**
 * @brief Reactor事件循环
 * 
 * 基于epoll实现的高性能事件循环，支持：
 * - 事件注册和分发
 * - 定时器管理
 * - 任务队列
 * - 优雅退出
 */
class Reactor {
public:
    using Task = std::function<void()>;

    Reactor();
    ~Reactor();

    // 事件循环控制
    void loop();
    void quit();
    bool isInLoopThread() const;
    
    // Channel管理
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    
    // 任务队列
    void runInLoop(const Task& task);
    void queueInLoop(const Task& task);
    
    // 定时器
    void runAt(const Task& task, time_t when);
    void runAfter(const Task& task, double delay_seconds);
    void runEvery(const Task& task, double interval_seconds);

private:
    void wakeup();
    void handleWakeup();
    void doPendingTasks();
    int createEventfd();
    
    std::atomic<bool> quit_;
    bool calling_pending_tasks_;
    
    std::unique_ptr<Poller> poller_;
    std::vector<Channel*> active_channels_;
    
    // 任务队列
    std::vector<Task> pending_tasks_;
    std::mutex pending_tasks_mutex_;
    
    // 唤醒机制
    int wakeup_fd_;
    std::unique_ptr<Channel> wakeup_channel_;
    
    // 线程标识
    std::thread::id thread_id_;
};



} // namespace network
} // namespace order_engine
