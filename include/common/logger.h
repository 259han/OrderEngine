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
    
    // 简单格式化日志接口（临时解决方案）
    void debug_fmt(const std::string& format, const std::string& arg);
    void info_fmt(const std::string& format, const std::string& arg);
    void error_fmt(const std::string& format, const std::string& arg);
    void debug_fmt(const std::string& format, const std::string& arg1, const std::string& arg2);
    void info_fmt(const std::string& format, int arg);

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
#define LOG_TRACE(msg) order_engine::common::Logger::getInstance().trace(msg)
#define LOG_DEBUG(msg) order_engine::common::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) order_engine::common::Logger::getInstance().info(msg)
#define LOG_WARN(msg) order_engine::common::Logger::getInstance().warn(msg)
#define LOG_ERROR(msg) order_engine::common::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) order_engine::common::Logger::getInstance().critical(msg)

// 格式化日志宏
#define LOG_DEBUG_FMT(fmt, arg) order_engine::common::Logger::getInstance().debug_fmt(fmt, arg)
#define LOG_INFO_FMT(fmt, arg) order_engine::common::Logger::getInstance().info_fmt(fmt, arg)
#define LOG_ERROR_FMT(fmt, arg) order_engine::common::Logger::getInstance().error_fmt(fmt, arg)
#define LOG_DEBUG_FMT2(fmt, arg1, arg2) order_engine::common::Logger::getInstance().debug_fmt(fmt, arg1, arg2)
#define LOG_INFO_FMT_INT(fmt, arg) order_engine::common::Logger::getInstance().info_fmt(fmt, arg)

#define LOG_PERF(op, duration) order_engine::common::Logger::getInstance().logPerformance(op, duration)
#define LOG_BUSINESS(event, data) order_engine::common::Logger::getInstance().logBusiness(event, data)

// 全局便捷函数已移至Logger类内部

} // namespace common
} // namespace order_engine
