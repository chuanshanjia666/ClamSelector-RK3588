#include "myqueue.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 初始化环形队列
 * @param queue 队列结构体指针
 * @param buffer 数据缓冲区
 * @param size 队列容量（元素个数）
 * @param element_size 单个元素大小
 * @return 操作结果
 */
QueueResult_t queue_init(CircularQueue_t *queue, uint8_t *buffer, uint16_t size, uint16_t element_size)
{
    if (queue == NULL || buffer == NULL)
    {
        return QUEUE_NULL_POINTER;
    }

    if (size == 0 || element_size == 0)
    {
        return QUEUE_INVALID_SIZE;
    }

    queue->buffer = buffer;
    queue->head = 0;
    queue->tail = 0;
    queue->size = size;
    queue->element_size = element_size;
    queue->count = 0;

    // 清空缓冲区
    memset(buffer, 0, size * element_size);

    return QUEUE_OK;
}

/**
 * @brief 入队操作
 * @param queue 队列结构体指针
 * @param data 要入队的数据
 * @return 操作结果
 */
QueueResult_t queue_enqueue(CircularQueue_t *queue, const void *data)
{
    if (queue == NULL || data == NULL)
    {
        return QUEUE_NULL_POINTER;
    }

    if (queue_is_full(queue))
    {
        return QUEUE_FULL;
    }

    // 复制数据到队列
    uint8_t *dest = queue->buffer + (queue->tail * queue->element_size);
    memcpy(dest, data, queue->element_size);

    // 更新尾指针和计数
    queue->tail = (queue->tail + 1) % queue->size;
    queue->count++;

    return QUEUE_OK;
}

/**
 * @brief 出队操作
 * @param queue 队列结构体指针
 * @param data 存储出队数据的缓冲区
 * @return 操作结果
 */
QueueResult_t queue_dequeue(CircularQueue_t *queue, void *data)
{
    if (queue == NULL || data == NULL)
    {
        return QUEUE_NULL_POINTER;
    }

    if (queue_is_empty(queue))
    {
        return QUEUE_EMPTY;
    }

    // 从队列复制数据
    uint8_t *src = queue->buffer + (queue->head * queue->element_size);
    memcpy(data, src, queue->element_size);

    // 更新头指针和计数
    queue->head = (queue->head + 1) % queue->size;
    queue->count--;

    return QUEUE_OK;
}

/**
 * @brief 查看队首元素（不移除）
 * @param queue 队列结构体指针
 * @param data 存储数据的缓冲区
 * @return 操作结果
 */
QueueResult_t queue_peek(const CircularQueue_t *queue, void *data)
{
    if (queue == NULL || data == NULL)
    {
        return QUEUE_NULL_POINTER;
    }

    if (queue_is_empty(queue))
    {
        return QUEUE_EMPTY;
    }

    // 从队列复制数据但不移除
    uint8_t *src = queue->buffer + (queue->head * queue->element_size);
    memcpy(data, src, queue->element_size);

    return QUEUE_OK;
}

/**
 * @brief 清空队列
 * @param queue 队列结构体指针
 * @return 操作结果
 */
QueueResult_t queue_clear(CircularQueue_t *queue)
{
    if (queue == NULL)
    {
        return QUEUE_NULL_POINTER;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    return QUEUE_OK;
}

/**
 * @brief 检查队列是否为空
 * @param queue 队列结构体指针
 * @return true表示空，false表示非空
 */
bool queue_is_empty(const CircularQueue_t *queue)
{
    if (queue == NULL)
    {
        return true;
    }
    return (queue->count == 0);
}

/**
 * @brief 检查队列是否已满
 * @param queue 队列结构体指针
 * @return true表示满，false表示未满
 */
bool queue_is_full(const CircularQueue_t *queue)
{
    if (queue == NULL)
    {
        return true;
    }
    return (queue->count >= queue->size);
}

/**
 * @brief 获取队列中元素数量
 * @param queue 队列结构体指针
 * @return 元素数量
 */
uint16_t queue_get_count(const CircularQueue_t *queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    return queue->count;
}

/**
 * @brief 获取队列剩余空间
 * @param queue 队列结构体指针
 * @return 剩余空间大小
 */
uint16_t queue_get_free_space(const CircularQueue_t *queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    return (queue->size - queue->count);
}

/**
 * @brief 获取队列使用率百分比
 * @param queue 队列结构体指针
 * @return 使用率百分比 (0.0 - 100.0)
 */
float queue_get_usage_percent(const CircularQueue_t *queue)
{
    if (queue == NULL || queue->size == 0)
    {
        return 0.0f;
    }
    return ((float)queue->count / queue->size) * 100.0f;
}

/**
 * @brief 批量入队
 * @param queue 队列结构体指针
 * @param data 数据数组
 * @param count 要入队的元素数量
 * @return 操作结果
 */
QueueResult_t queue_enqueue_multiple(CircularQueue_t *queue, const void *data, uint16_t count)
{
    if (queue == NULL || data == NULL || count == 0)
    {
        return QUEUE_NULL_POINTER;
    }

    if (queue_get_free_space(queue) < count)
    {
        return QUEUE_FULL;
    }

    const uint8_t *src = (const uint8_t *)data;
    for (uint16_t i = 0; i < count; i++)
    {
        QueueResult_t result = queue_enqueue(queue, src + (i * queue->element_size));
        if (result != QUEUE_OK)
        {
            return result;
        }
    }

    return QUEUE_OK;
}

/**
 * @brief 批量出队
 * @param queue 队列结构体指针
 * @param data 存储数据的缓冲区
 * @param count 要出队的元素数量
 * @param actual_count 实际出队的元素数量
 * @return 操作结果
 */
QueueResult_t queue_dequeue_multiple(CircularQueue_t *queue, void *data, uint16_t count, uint16_t *actual_count)
{
    if (queue == NULL || data == NULL)
    {
        return QUEUE_NULL_POINTER;
    }

    uint16_t available = queue_get_count(queue);
    uint16_t to_dequeue = (count < available) ? count : available;

    if (actual_count != NULL)
    {
        *actual_count = to_dequeue;
    }

    if (to_dequeue == 0)
    {
        return QUEUE_EMPTY;
    }

    uint8_t *dest = (uint8_t *)data;
    for (uint16_t i = 0; i < to_dequeue; i++)
    {
        QueueResult_t result = queue_dequeue(queue, dest + (i * queue->element_size));
        if (result != QUEUE_OK)
        {
            return result;
        }
    }

    return QUEUE_OK;
}

/**
 * @brief 查看指定位置的元素
 * @param queue 队列结构体指针
 * @param data 存储数据的缓冲区
 * @param index 相对于队首的索引
 * @return 操作结果
 */
QueueResult_t queue_peek_at(const CircularQueue_t *queue, void *data, uint16_t index)
{
    if (queue == NULL || data == NULL)
    {
        return QUEUE_NULL_POINTER;
    }

    if (index >= queue->count)
    {
        return QUEUE_EMPTY;
    }

    uint16_t actual_index = (queue->head + index) % queue->size;
    uint8_t *src = queue->buffer + (actual_index * queue->element_size);
    memcpy(data, src, queue->element_size);

    return QUEUE_OK;
}

/**
 * @brief 打印队列状态信息
 * @param queue 队列结构体指针
 * @param name 队列名称
 */
void queue_print_status(const CircularQueue_t *queue, const char *name)
{
    if (queue == NULL)
    {
        printf("[%s] Queue is NULL\r\n", name ? name : "Unknown");
        return;
    }

    printf("[%s] Status:\r\n", name ? name : "Queue");
    printf("  Size: %d, Count: %d, Free: %d\r\n",
           queue->size, queue->count, queue_get_free_space(queue));
    printf("  Head: %d, Tail: %d, Usage: %.1f%%\r\n",
           queue->head, queue->tail, queue_get_usage_percent(queue));
    printf("  Empty: %s, Full: %s\r\n",
           queue_is_empty(queue) ? "Yes" : "No",
           queue_is_full(queue) ? "Yes" : "No");
}

/**
 * @brief 转储队列数据（调试用）
 * @param queue 队列结构体指针
 * @param name 队列名称
 */
void queue_dump_data(const CircularQueue_t *queue, const char *name)
{
    if (queue == NULL)
    {
        printf("[%s] Queue is NULL\r\n", name ? name : "Unknown");
        return;
    }

    printf("[%s] Data dump (%d elements):\r\n", name ? name : "Queue", queue->count);

    for (uint16_t i = 0; i < queue->count; i++)
    {
        uint16_t index = (queue->head + i) % queue->size;
        uint8_t *element = queue->buffer + (index * queue->element_size);

        printf("  [%d]: ", i);
        for (uint16_t j = 0; j < queue->element_size; j++)
        {
            printf("%02X ", element[j]);
        }
        printf("\r\n");
    }
}
