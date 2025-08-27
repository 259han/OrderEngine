#pragma once

#include <memory>
#include <string>

// 前向声明避免包含冲突
namespace spdlog {
    class logger;
}

namespace order_engine {
namespace common {

/**
 * @brief 高性能日志管理器
 * 
 * 支持异步日志写入，结构化日志格式，多级别日志管理
 * 为高并发场景优化，减少日志写入对业务性能的影响
 */
class Logger {
public:
    enum class Level {
        TRACE = 0,
        DEBUG_LEVEL = 1,
        INFO = 2,
        WARN = 3,
        ERROR_LEVEL = 4,
        CRITICAL = 5
    };

    static Logger& getInstance();
    
    bool initialize(const std::string& config_file);
    void shutdown();

    // 日志记录接口
    void trace(const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);

    // 性能日志记录
    void logPerformance(const std::string& operation, double duration_ms);
    
    // 业务日志记录
    void logBusiness(const std::string& event, const std::string& data);

    // 设置日志级别
    void setLevel(Level level);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::logger> perf_logger_;
    std::shared_ptr<spdlog::logger> business_logger_;
};

// 便捷宏定义
#define LOG_TRACE(...) order_engine::common::Logger::getInstance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) order_engine::common::Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) order_engine::common::Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...) order_engine::common::Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) order_engine::common::Logger::getInstance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) order_engine::common::Logger::getInstance().critical(__VA_ARGS__)

#define LOG_PERF(op, duration) order_engine::common::Logger::getInstance().logPerformance(op, duration)
#define LOG_BUSINESS(event, data) order_engine::common::Logger::getInstance().logBusiness(event, data)

// 全局便捷函数
template<typename... Args>
void trace(const std::string& format, Args&&... args) {
    Logger::getInstance().trace(format, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(const std::string& format, Args&&... args) {
    Logger::getInstance().debug(format, std::forward<Args>(args)...);
}

template<typename... Args>
void info(const std::string& format, Args&&... args) {
    Logger::getInstance().info(format, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(const std::string& format, Args&&... args) {
    Logger::getInstance().warn(format, std::forward<Args>(args)...);
}

template<typename... Args>
void error(const std::string& format, Args&&... args) {
    Logger::getInstance().error(format, std::forward<Args>(args)...);
}

template<typename... Args>
void critical(const std::string& format, Args&&... args) {
    Logger::getInstance().critical(format, std::forward<Args>(args)...);
}

} // namespace common
} // namespace order_engine
