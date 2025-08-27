#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "../database/connection_pool.h"
#include "../cache/cache_manager.h"

namespace order_engine {
namespace services {

/**
 * @brief 库存信息结构
 */
struct InventoryInfo {
    uint64_t product_id;
    uint32_t total_stock;      // 总库存
    uint32_t available_stock;  // 可用库存
    uint32_t reserved_stock;   // 预留库存
    uint32_t sold_stock;       // 已售库存
    time_t updated_at;
    uint32_t version;          // 乐观锁版本号
};

/**
 * @brief 库存预留信息
 */
struct ReservationInfo {
    std::string reservation_id;
    uint64_t product_id;
    uint32_t quantity;
    time_t reserved_at;
    time_t expires_at;
    std::string order_id;
};

/**
 * @brief 库存服务
 * 
 * 负责商品库存管理，支持：
 * - 实时库存查询
 * - 库存扣减和释放
 * - 库存预留机制
 * - 超卖防护
 * - 分布式锁
 */
class InventoryService {
public:
    using InventoryCallback = std::function<void(bool success, const std::string& message)>;
    using QueryCallback = std::function<void(bool success, const InventoryInfo& info)>;

    InventoryService(std::shared_ptr<database::ConnectionPool> db_pool,
                     std::shared_ptr<cache::CacheManager> cache_manager);
    
    ~InventoryService();

    // 初始化和清理
    bool initialize();
    void shutdown();

    // 库存查询
    void getInventory(uint64_t product_id, const QueryCallback& callback);
    void batchGetInventory(const std::vector<uint64_t>& product_ids,
                          const std::function<void(bool, const std::vector<InventoryInfo>&)>& callback);
    
    // 库存检查
    bool checkStock(uint64_t product_id, uint32_t quantity);
    bool batchCheckStock(const std::vector<uint64_t>& product_ids, 
                        const std::vector<uint32_t>& quantities);
    
    // 库存预留
    void reserveStock(const std::vector<uint64_t>& product_ids,
                     const std::vector<uint32_t>& quantities,
                     const std::string& order_id,
                     int timeout_seconds,
                     const InventoryCallback& callback);
    
    // 库存确认扣减
    void confirmReservation(const std::string& reservation_id,
                           const InventoryCallback& callback);
    
    // 库存释放
    void releaseReservation(const std::string& reservation_id,
                           const InventoryCallback& callback);
    
    // 库存补充
    void addStock(uint64_t product_id, uint32_t quantity,
                  const InventoryCallback& callback);
    
    // 库存同步
    void syncInventoryFromDB(uint64_t product_id);
    void syncAllInventoryFromDB();

private:
    // 内部方法
    bool getInventoryFromCache(uint64_t product_id, InventoryInfo& info);
    void setInventoryToCache(const InventoryInfo& info);
    void invalidateInventoryCache(uint64_t product_id);
    
    bool getInventoryFromDB(uint64_t product_id, InventoryInfo& info);
    bool updateInventoryInDB(const InventoryInfo& info);
    
    // 分布式锁
    bool acquireDistributedLock(uint64_t product_id, int timeout_ms);
    void releaseDistributedLock(uint64_t product_id);
    
    // 预留管理
    std::string generateReservationId();
    void saveReservation(const ReservationInfo& reservation);
    bool getReservation(const std::string& reservation_id, ReservationInfo& reservation);
    void removeReservation(const std::string& reservation_id);
    void cleanupExpiredReservations();
    
    // CAS操作
    bool compareAndSwapStock(uint64_t product_id, uint32_t expected_available, 
                            uint32_t new_available, uint32_t expected_version);
    
    std::shared_ptr<database::ConnectionPool> db_pool_;
    std::shared_ptr<cache::CacheManager> cache_manager_;
    
    // 预留信息缓存
    std::unordered_map<std::string, ReservationInfo> reservations_;
    std::mutex reservations_mutex_;
    
    // 定时清理线程
    std::thread cleanup_thread_;
    std::atomic<bool> cleanup_running_;
    
    // 统计信息
    std::atomic<uint64_t> total_reservations_;
    std::atomic<uint64_t> successful_reservations_;
    std::atomic<uint64_t> failed_reservations_;
    
    static const std::string kInventoryCachePrefix;
    static const std::string kLockPrefix;
    static const int kCacheExpireTime;
    static const int kLockExpireTime;
    static const int kReservationExpireTime;
};

} // namespace services
} // namespace order_engine
