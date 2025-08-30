#include "common/logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

namespace order_engine {
namespace common {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

bool Logger::initialize(const std::string& config_file) {
    try {
        // 初始化spdlog异步日志
        spdlog::init_thread_pool(8192, 1);
        
        // 创建文件sink（按日期分割）
        auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            "logs/order_engine", 23, 59);
        daily_sink->set_level(spdlog::level::trace);
        
        // 创建控制台sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        
        // 创建主日志器
        std::vector<spdlog::sink_ptr> sinks = {daily_sink, console_sink};
        logger_ = std::make_shared<spdlog::async_logger>(
            "order_engine", sinks.begin(), sinks.end(), 
            spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        
        logger_->set_level(spdlog::level::trace);
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] %v");
        
        // 创建性能日志器
        auto perf_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            "logs/performance", 23, 59);
        perf_logger_ = std::make_shared<spdlog::async_logger>(
            "performance", perf_sink, spdlog::thread_pool());
        perf_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        
        // 创建业务日志器
        auto business_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            "logs/business", 23, 59);
        business_logger_ = std::make_shared<spdlog::async_logger>(
            "business", business_sink, spdlog::thread_pool());
        business_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        
        // 注册日志器
        spdlog::register_logger(logger_);
        spdlog::register_logger(perf_logger_);
        spdlog::register_logger(business_logger_);
        
        // 设置为默认日志器
        spdlog::set_default_logger(logger_);
        
        LOG_INFO("Logger initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Logger initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void Logger::shutdown() {
    if (logger_) {
        logger_->flush();
    }
    if (perf_logger_) {
        perf_logger_->flush();
    }
    if (business_logger_) {
        business_logger_->flush();
    }
    
    spdlog::shutdown();
}

void Logger::logPerformance(const std::string& operation, double duration_ms) {
    if (perf_logger_) {
        perf_logger_->info("PERF {} {:.3f}ms", operation, duration_ms);
    }
}

void Logger::logBusiness(const std::string& event, const std::string& data) {
    if (business_logger_) {
        business_logger_->info("BIZ {} {}", event, data);
    }
}

void Logger::trace(const std::string& message) {
    if (logger_) {
        logger_->trace(message);
    }
}

void Logger::debug(const std::string& message) {
    if (logger_) {
        logger_->debug(message);
    }
}

void Logger::info(const std::string& message) {
    if (logger_) {
        logger_->info(message);
    }
}

void Logger::warn(const std::string& message) {
    if (logger_) {
        logger_->warn(message);
    }
}

void Logger::error(const std::string& message) {
    if (logger_) {
        logger_->error(message);
    }
}

void Logger::critical(const std::string& message) {
    if (logger_) {
        logger_->critical(message);
    }
}

// 简单格式化日志接口实现
void Logger::debug_fmt(const std::string& format, const std::string& arg) {
    if (logger_) {
        logger_->debug(format, arg);
    }
}

void Logger::info_fmt(const std::string& format, const std::string& arg) {
    if (logger_) {
        logger_->info(format, arg);
    }
}

void Logger::error_fmt(const std::string& format, const std::string& arg) {
    if (logger_) {
        logger_->error(format, arg);
    }
}

void Logger::debug_fmt(const std::string& format, const std::string& arg1, const std::string& arg2) {
    if (logger_) {
        logger_->debug(format, arg1, arg2);
    }
}

void Logger::info_fmt(const std::string& format, int arg) {
    if (logger_) {
        logger_->info(format, arg);
    }
}

void Logger::setLevel(Level level) {
    if (logger_) {
        spdlog::level::level_enum spdlog_level;
        switch (level) {
            case Level::TRACE:       spdlog_level = spdlog::level::trace; break;
            case Level::DEBUG_LEVEL: spdlog_level = spdlog::level::debug; break;
            case Level::INFO:        spdlog_level = spdlog::level::info; break;
            case Level::WARN:        spdlog_level = spdlog::level::warn; break;
            case Level::ERROR_LEVEL: spdlog_level = spdlog::level::err; break;
            case Level::CRITICAL:    spdlog_level = spdlog::level::critical; break;
            default:                 spdlog_level = spdlog::level::info; break;
        }
        logger_->set_level(spdlog_level);
    }
}

} // namespace common
} // namespace order_engine
