-- OrderEngine 数据库初始化脚本
-- 创建高性能电商订单系统的核心表结构

-- 创建数据库
CREATE DATABASE IF NOT EXISTS order_engine 
DEFAULT CHARACTER SET utf8mb4 
DEFAULT COLLATE utf8mb4_unicode_ci;

USE order_engine;

-- 用户表
CREATE TABLE IF NOT EXISTS users (
    user_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '用户ID',
    username VARCHAR(64) NOT NULL COMMENT '用户名',
    email VARCHAR(128) NOT NULL COMMENT '邮箱',
    phone VARCHAR(20) DEFAULT NULL COMMENT '手机号',
    password_hash VARCHAR(128) NOT NULL COMMENT '密码哈希',
    salt VARCHAR(32) NOT NULL COMMENT '盐值',
    status TINYINT NOT NULL DEFAULT 1 COMMENT '状态：0-禁用，1-正常',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (user_id),
    UNIQUE KEY uk_username (username),
    UNIQUE KEY uk_email (email),
    KEY idx_phone (phone),
    KEY idx_status_created (status, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户表';

-- 商品表
CREATE TABLE IF NOT EXISTS products (
    product_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '商品ID',
    product_name VARCHAR(256) NOT NULL COMMENT '商品名称',
    product_code VARCHAR(64) NOT NULL COMMENT '商品编码',
    category_id INT UNSIGNED NOT NULL COMMENT '分类ID',
    price DECIMAL(10,2) NOT NULL COMMENT '价格',
    cost_price DECIMAL(10,2) DEFAULT NULL COMMENT '成本价',
    description TEXT COMMENT '商品描述',
    images JSON COMMENT '商品图片',
    attributes JSON COMMENT '商品属性',
    status TINYINT NOT NULL DEFAULT 1 COMMENT '状态：0-下架，1-上架',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (product_id),
    UNIQUE KEY uk_product_code (product_code),
    KEY idx_category_status (category_id, status),
    KEY idx_price (price),
    KEY idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='商品表';

-- 库存表 (支持分库分表)
CREATE TABLE IF NOT EXISTS inventory (
    inventory_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '库存ID',
    product_id BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    total_stock INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '总库存',
    available_stock INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '可用库存',
    reserved_stock INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '预留库存',
    sold_stock INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '已售库存',
    version INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '版本号（乐观锁）',
    warehouse_id INT UNSIGNED DEFAULT NULL COMMENT '仓库ID',
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (inventory_id),
    UNIQUE KEY uk_product_warehouse (product_id, warehouse_id),
    KEY idx_available_stock (available_stock),
    KEY idx_updated_at (updated_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='库存表';

-- 订单表 (按月分表)
CREATE TABLE IF NOT EXISTS orders_202504 (
    order_id BIGINT UNSIGNED NOT NULL COMMENT '订单ID',
    order_number VARCHAR(32) NOT NULL COMMENT '订单号',
    user_id BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    total_amount DECIMAL(10,2) NOT NULL COMMENT '订单总金额',
    discount_amount DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT '优惠金额',
    actual_amount DECIMAL(10,2) NOT NULL COMMENT '实付金额',
    shipping_fee DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT '运费',
    status TINYINT NOT NULL DEFAULT 0 COMMENT '订单状态：0-待支付，1-已支付，2-已发货，3-已送达，4-已取消，5-已退款',
    payment_method VARCHAR(32) DEFAULT NULL COMMENT '支付方式',
    payment_time TIMESTAMP NULL DEFAULT NULL COMMENT '支付时间',
    shipping_address JSON COMMENT '收货地址',
    remarks VARCHAR(512) DEFAULT NULL COMMENT '订单备注',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (order_id),
    UNIQUE KEY uk_order_number (order_number),
    KEY idx_user_id_status (user_id, status),
    KEY idx_status_created (status, created_at),
    KEY idx_created_at (created_at),
    KEY idx_payment_time (payment_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci 
PARTITION BY RANGE(TO_DAYS(created_at)) (
    PARTITION p202504 VALUES LESS THAN (TO_DAYS('2025-05-01')),
    PARTITION p202505 VALUES LESS THAN (TO_DAYS('2025-06-01')),
    PARTITION p202506 VALUES LESS THAN (TO_DAYS('2025-07-01')),
    PARTITION p202507 VALUES LESS THAN (TO_DAYS('2025-08-01'))
) COMMENT='订单表';

-- 订单明细表
CREATE TABLE IF NOT EXISTS order_items (
    item_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '明细ID',
    order_id BIGINT UNSIGNED NOT NULL COMMENT '订单ID',
    product_id BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    product_name VARCHAR(256) NOT NULL COMMENT '商品名称',
    product_code VARCHAR(64) NOT NULL COMMENT '商品编码',
    unit_price DECIMAL(10,2) NOT NULL COMMENT '单价',
    quantity INT UNSIGNED NOT NULL COMMENT '数量',
    total_price DECIMAL(10,2) NOT NULL COMMENT '小计',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (item_id),
    KEY idx_order_id (order_id),
    KEY idx_product_id (product_id),
    KEY idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='订单明细表';

-- 支付表
CREATE TABLE IF NOT EXISTS payments (
    payment_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '支付ID',
    order_id BIGINT UNSIGNED NOT NULL COMMENT '订单ID',
    payment_number VARCHAR(32) NOT NULL COMMENT '支付流水号',
    payment_method VARCHAR(32) NOT NULL COMMENT '支付方式',
    payment_channel VARCHAR(32) NOT NULL COMMENT '支付渠道',
    amount DECIMAL(10,2) NOT NULL COMMENT '支付金额',
    status TINYINT NOT NULL DEFAULT 0 COMMENT '支付状态：0-待支付，1-支付中，2-支付成功，3-支付失败，4-已退款',
    third_party_transaction_id VARCHAR(64) DEFAULT NULL COMMENT '第三方交易ID',
    callback_time TIMESTAMP NULL DEFAULT NULL COMMENT '回调时间',
    remarks VARCHAR(512) DEFAULT NULL COMMENT '备注',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (payment_id),
    UNIQUE KEY uk_payment_number (payment_number),
    KEY idx_order_id (order_id),
    KEY idx_status_created (status, created_at),
    KEY idx_third_party_id (third_party_transaction_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='支付表';

-- 库存变更日志表
CREATE TABLE IF NOT EXISTS inventory_logs (
    log_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志ID',
    product_id BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    operation_type TINYINT NOT NULL COMMENT '操作类型：1-入库，2-出库，3-预留，4-释放',
    quantity_change INT NOT NULL COMMENT '数量变化',
    before_stock INT UNSIGNED NOT NULL COMMENT '变更前库存',
    after_stock INT UNSIGNED NOT NULL COMMENT '变更后库存',
    related_order_id BIGINT UNSIGNED DEFAULT NULL COMMENT '关联订单ID',
    operator_id BIGINT UNSIGNED DEFAULT NULL COMMENT '操作员ID',
    remarks VARCHAR(256) DEFAULT NULL COMMENT '备注',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (log_id),
    KEY idx_product_id_created (product_id, created_at),
    KEY idx_operation_type (operation_type),
    KEY idx_order_id (related_order_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='库存变更日志表';

-- 分布式锁表
CREATE TABLE IF NOT EXISTS distributed_locks (
    lock_key VARCHAR(128) NOT NULL COMMENT '锁键',
    lock_value VARCHAR(64) NOT NULL COMMENT '锁值',
    expire_time TIMESTAMP NOT NULL COMMENT '过期时间',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (lock_key),
    KEY idx_expire_time (expire_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='分布式锁表';

-- 库存预留表
CREATE TABLE IF NOT EXISTS inventory_reservations (
    reservation_id VARCHAR(64) NOT NULL COMMENT '预留ID',
    product_id BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    quantity INT UNSIGNED NOT NULL COMMENT '预留数量',
    order_id BIGINT UNSIGNED NOT NULL COMMENT '订单ID',
    status TINYINT NOT NULL DEFAULT 0 COMMENT '状态：0-预留中，1-已确认，2-已释放，3-已过期',
    expire_time TIMESTAMP NOT NULL COMMENT '过期时间',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (reservation_id),
    KEY idx_product_id (product_id),
    KEY idx_order_id (order_id),
    KEY idx_expire_time (expire_time),
    KEY idx_status_created (status, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='库存预留表';

-- 插入测试数据

-- 插入测试用户
INSERT INTO users (username, email, phone, password_hash, salt) VALUES 
('testuser1', 'test1@example.com', '13800001111', 'hash1', 'salt1'),
('testuser2', 'test2@example.com', '13800002222', 'hash2', 'salt2'),
('testuser3', 'test3@example.com', '13800003333', 'hash3', 'salt3');

-- 插入测试商品
INSERT INTO products (product_name, product_code, category_id, price, cost_price, description) VALUES 
('iPhone 15 Pro', 'IPHONE15PRO', 1, 7999.00, 6000.00, '苹果iPhone 15 Pro 128GB'),
('MacBook Pro', 'MACBOOKPRO', 2, 12999.00, 10000.00, '苹果MacBook Pro 14英寸'),
('AirPods Pro', 'AIRPODSPRO', 3, 1799.00, 1200.00, '苹果AirPods Pro 2代');

-- 插入测试库存
INSERT INTO inventory (product_id, total_stock, available_stock, reserved_stock, sold_stock) VALUES 
(1, 1000, 900, 50, 50),
(2, 500, 450, 25, 25),
(3, 2000, 1800, 100, 100);

-- 创建索引优化查询性能
-- 复合索引优化订单查询
ALTER TABLE orders_202504 ADD INDEX idx_user_status_created (user_id, status, created_at);
ALTER TABLE orders_202504 ADD INDEX idx_status_payment_time (status, payment_time);

-- 创建视图简化查询
CREATE VIEW order_summary AS
SELECT 
    o.order_id,
    o.order_number,
    o.user_id,
    u.username,
    o.total_amount,
    o.actual_amount,
    o.status,
    o.created_at,
    COUNT(oi.item_id) as item_count
FROM orders_202504 o
LEFT JOIN users u ON o.user_id = u.user_id
LEFT JOIN order_items oi ON o.order_id = oi.order_id
GROUP BY o.order_id;

-- 创建存储过程用于高频操作
DELIMITER $$

-- 库存扣减存储过程（带乐观锁）
CREATE PROCEDURE DecrementInventory(
    IN p_product_id BIGINT UNSIGNED,
    IN p_quantity INT UNSIGNED,
    IN p_expected_version INT UNSIGNED,
    OUT p_result INT,
    OUT p_message VARCHAR(255)
)
BEGIN
    DECLARE current_available INT UNSIGNED DEFAULT 0;
    DECLARE current_version INT UNSIGNED DEFAULT 0;
    DECLARE affected_rows INT DEFAULT 0;
    
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        SET p_result = -1;
        SET p_message = 'Database error occurred';
    END;
    
    START TRANSACTION;
    
    -- 获取当前库存和版本号
    SELECT available_stock, version INTO current_available, current_version
    FROM inventory 
    WHERE product_id = p_product_id;
    
    -- 检查版本号
    IF current_version != p_expected_version THEN
        SET p_result = 1;
        SET p_message = 'Version conflict, please retry';
        ROLLBACK;
        LEAVE root;
    END IF;
    
    -- 检查库存是否充足
    IF current_available < p_quantity THEN
        SET p_result = 2;
        SET p_message = 'Insufficient inventory';
        ROLLBACK;
        LEAVE root;
    END IF;
    
    -- 扣减库存
    UPDATE inventory 
    SET available_stock = available_stock - p_quantity,
        reserved_stock = reserved_stock + p_quantity,
        version = version + 1,
        updated_at = CURRENT_TIMESTAMP
    WHERE product_id = p_product_id AND version = p_expected_version;
    
    SET affected_rows = ROW_COUNT();
    
    IF affected_rows = 1 THEN
        SET p_result = 0;
        SET p_message = 'Success';
        COMMIT;
    ELSE
        SET p_result = 3;
        SET p_message = 'Update failed';
        ROLLBACK;
    END IF;
    
END$$

DELIMITER ;

-- 创建触发器记录库存变更
DELIMITER $$

CREATE TRIGGER tr_inventory_update_log
AFTER UPDATE ON inventory
FOR EACH ROW
BEGIN
    INSERT INTO inventory_logs (
        product_id,
        operation_type,
        quantity_change,
        before_stock,
        after_stock,
        created_at
    ) VALUES (
        NEW.product_id,
        CASE 
            WHEN NEW.available_stock < OLD.available_stock THEN 2  -- 出库
            WHEN NEW.available_stock > OLD.available_stock THEN 1  -- 入库
            ELSE 0  -- 其他
        END,
        NEW.available_stock - OLD.available_stock,
        OLD.available_stock,
        NEW.available_stock,
        CURRENT_TIMESTAMP
    );
END$$

DELIMITER ;

-- 创建定时清理过期预留的事件
SET GLOBAL event_scheduler = ON;

CREATE EVENT IF NOT EXISTS cleanup_expired_reservations
ON SCHEDULE EVERY 1 MINUTE
DO
BEGIN
    -- 释放过期的预留
    UPDATE inventory_reservations 
    SET status = 3, updated_at = CURRENT_TIMESTAMP
    WHERE status = 0 AND expire_time < CURRENT_TIMESTAMP;
    
    -- 恢复库存
    UPDATE inventory i
    INNER JOIN inventory_reservations r ON i.product_id = r.product_id
    SET i.available_stock = i.available_stock + r.quantity,
        i.reserved_stock = i.reserved_stock - r.quantity,
        i.updated_at = CURRENT_TIMESTAMP
    WHERE r.status = 3 AND r.updated_at >= DATE_SUB(CURRENT_TIMESTAMP, INTERVAL 1 MINUTE);
END;
