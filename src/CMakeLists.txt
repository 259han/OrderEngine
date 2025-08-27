# 核心库源文件
set(CORE_SOURCES
    common/logger.cpp
    common/config.cpp
    common/thread_pool.cpp
    network/tcp_server.cpp
    network/tcp_connection.cpp
    network/reactor.cpp
    network/epoll_poller.cpp
    cache/redis_client.cpp
    cache/cache_manager.cpp
    database/mysql_connection.cpp
    database/connection_pool.cpp
    message/kafka_producer.cpp
    message/kafka_consumer.cpp
    services/user_service.cpp
    services/order_service.cpp
    services/inventory_service.cpp
    protocol/order_protocol.cpp
    utils/hash_utils.cpp
    utils/time_utils.cpp
    utils/json_utils.cpp
)

# 创建核心库
add_library(order_engine_core ${CORE_SOURCES})

target_link_libraries(order_engine_core
    ${COMMON_LIBS}
)
