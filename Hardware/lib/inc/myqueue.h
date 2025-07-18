#ifndef MYQUEUE_H
#define MYQUEUE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// 队列配置
#define QUEUE_MAX_SIZE 16                   // 队列最大容量（建议使用2的幂次）
#define QUEUE_ELEMENT_SIZE sizeof(uint32_t) // 队列元素大小

    // 队列错误码
    typedef enum
    {
        QUEUE_OK = 0,           // 操作成功
        QUEUE_FULL = 1,         // 队列满
        QUEUE_EMPTY = 2,        // 队列空
        QUEUE_NULL_POINTER = 3, // 空指针错误
        QUEUE_INVALID_SIZE = 4  // 无效大小
    } QueueResult_t;

    // 环形队列结构体
    typedef struct
    {
        uint8_t *buffer;       // 数据缓冲区
        uint16_t head;         // 队列头指针
        uint16_t tail;         // 队列尾指针
        uint16_t size;         // 队列最大容量
        uint16_t element_size; // 单个元素大小
        uint16_t count;        // 当前元素数量
    } CircularQueue_t;

    // 队列操作函数
    QueueResult_t queue_init(CircularQueue_t *queue, uint8_t *buffer, uint16_t size, uint16_t element_size);
    QueueResult_t queue_enqueue(CircularQueue_t *queue, const void *data);
    QueueResult_t queue_dequeue(CircularQueue_t *queue, void *data);
    QueueResult_t queue_peek(const CircularQueue_t *queue, void *data);
    QueueResult_t queue_clear(CircularQueue_t *queue);

    // 队列状态查询函数
    bool queue_is_empty(const CircularQueue_t *queue);
    bool queue_is_full(const CircularQueue_t *queue);
    uint16_t queue_get_count(const CircularQueue_t *queue);
    uint16_t queue_get_free_space(const CircularQueue_t *queue);
    float queue_get_usage_percent(const CircularQueue_t *queue);

    // 高级操作函数
    QueueResult_t queue_enqueue_multiple(CircularQueue_t *queue, const void *data, uint16_t count);
    QueueResult_t queue_dequeue_multiple(CircularQueue_t *queue, void *data, uint16_t count, uint16_t *actual_count);
    QueueResult_t queue_peek_at(const CircularQueue_t *queue, void *data, uint16_t index);

    // 调试和统计函数
    void queue_print_status(const CircularQueue_t *queue, const char *name);
    void queue_dump_data(const CircularQueue_t *queue, const char *name);

// 便利宏定义
#define DECLARE_QUEUE(name, type, size)                                         \
    static type name##_buffer[size];                                            \
    static CircularQueue_t name = {0};                                          \
    static inline QueueResult_t name##_init(void)                               \
    {                                                                           \
        return queue_init(&name, (uint8_t *)name##_buffer, size, sizeof(type)); \
    }                                                                           \
    static inline QueueResult_t name##_enqueue(const type *data)                \
    {                                                                           \
        return queue_enqueue(&name, data);                                      \
    }                                                                           \
    static inline QueueResult_t name##_dequeue(type *data)                      \
    {                                                                           \
        return queue_dequeue(&name, data);                                      \
    }                                                                           \
    static inline bool name##_is_empty(void)                                    \
    {                                                                           \
        return queue_is_empty(&name);                                           \
    }                                                                           \
    static inline bool name##_is_full(void)                                     \
    {                                                                           \
        return queue_is_full(&name);                                            \
    }                                                                           \
    static inline uint16_t name##_get_count(void)                               \
    {                                                                           \
        return queue_get_count(&name);                                          \
    }

#ifdef __cplusplus
}
#endif

#endif // MYQUEUE_H
