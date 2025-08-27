#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <mutex>
#include <hiredis/hiredis.h>

namespace order_engine {
namespace cache {

/**
 * @brief Redis连接配置
 */
struct RedisConfig {
    std::string host = "127.0.0.1";
    int port = 6379;
    std::string password = "";
    int database = 0;
    int connect_timeout_ms = 3000;
    int socket_timeout_ms = 3000;
    int max_retries = 3;
    bool enable_pipeline = true;
};

/**
 * @brief Redis客户端
 * 
 * 高性能Redis客户端，支持：
 * - 连接池管理
 * - 管道批量操作
 * - 自动重连
 * - 故障转移
 * - 分布式锁
 */
class RedisClient {
public:
    using RedisCallback = std::function<void(bool success, const std::string& result)>;

    explicit RedisClient(const RedisConfig& config);
    ~RedisClient();

    // 连接管理
    bool connect();
    void disconnect();
    bool isConnected() const { return connected_.load(); }
    bool ping();

    // 基础操作
    bool set(const std::string& key, const std::string& value, int expire_seconds = 0);
    bool get(const std::string& key, std::string& value);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool expire(const std::string& key, int seconds);
    bool ttl(const std::string& key, int& seconds);

    // 原子操作
    bool incr(const std::string& key, int64_t& result);
    bool decr(const std::string& key, int64_t& result);
    bool incrby(const std::string& key, int64_t increment, int64_t& result);

    // 哈希操作
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    bool hget(const std::string& key, const std::string& field, std::string& value);
    bool hdel(const std::string& key, const std::string& field);
    bool hgetall(const std::string& key, std::unordered_map<std::string, std::string>& fields);

    // 列表操作
    bool lpush(const std::string& key, const std::string& value);
    bool rpush(const std::string& key, const std::string& value);
    bool lpop(const std::string& key, std::string& value);
    bool rpop(const std::string& key, std::string& value);
    bool llen(const std::string& key, int64_t& length);

    // 集合操作
    bool sadd(const std::string& key, const std::string& member);
    bool srem(const std::string& key, const std::string& member);
    bool sismember(const std::string& key, const std::string& member);
    bool scard(const std::string& key, int64_t& count);

    // 有序集合操作
    bool zadd(const std::string& key, double score, const std::string& member);
    bool zrem(const std::string& key, const std::string& member);
    bool zscore(const std::string& key, const std::string& member, double& score);
    bool zcard(const std::string& key, int64_t& count);

    // 分布式锁
    bool acquireLock(const std::string& key, const std::string& value, int expire_seconds);
    bool releaseLock(const std::string& key, const std::string& value);
    bool extendLock(const std::string& key, const std::string& value, int expire_seconds);

    // 批量操作
    bool mset(const std::unordered_map<std::string, std::string>& key_values);
    bool mget(const std::vector<std::string>& keys, std::vector<std::string>& values);
    bool mdel(const std::vector<std::string>& keys);

    // 管道操作
    class Pipeline {
    public:
        Pipeline(RedisClient* client);
        ~Pipeline();

        void set(const std::string& key, const std::string& value);
        void get(const std::string& key);
        void del(const std::string& key);
        void incr(const std::string& key);
        
        bool execute(std::vector<std::string>& results);

    private:
        RedisClient* client_;
        std::vector<std::string> commands_;
    };

    std::unique_ptr<Pipeline> createPipeline();

    // 异步操作 (可选实现)
    void setAsync(const std::string& key, const std::string& value, 
                  int expire_seconds, const RedisCallback& callback);
    void getAsync(const std::string& key, const RedisCallback& callback);

    // 统计信息
    uint64_t getCommandCount() const { return command_count_.load(); }
    uint64_t getErrorCount() const { return error_count_.load(); }

private:
    bool reconnect();
    redisReply* executeCommand(const char* format, ...);
    bool isConnectionError(redisReply* reply);
    void freeReply(redisReply* reply);
    
    RedisConfig config_;
    redisContext* context_;
    std::atomic<bool> connected_;
    std::mutex connection_mutex_;
    
    // 统计
    std::atomic<uint64_t> command_count_;
    std::atomic<uint64_t> error_count_;
    
    // Lua脚本
    std::string acquire_lock_script_;
    std::string release_lock_script_;
    std::string extend_lock_script_;
};

} // namespace cache
} // namespace order_engine
