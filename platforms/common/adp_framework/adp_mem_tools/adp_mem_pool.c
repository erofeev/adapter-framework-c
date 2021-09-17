/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include "adp_osal.h"
#include "adp_logging.h"
#include "adp_mem_pool.h"


typedef struct {
    void*                prev;
    void*                data;
    void*                next;
} adp_mem_pool_node_t;

struct adp_mem_pool_s {
    adp_os_mutex_t       lock;
    adp_mem_pool_node_t *head;
};

adp_mem_pool_t* adp_mem_pool_create(void)
{
    adp_mem_pool_t *pool = adp_os_malloc(sizeof(adp_mem_pool_t));
    if (!pool) {
        adp_log_d("No memory left");
        return (void*)0;
    }
    pool->lock = adp_os_mutex_create();
    if (!pool->lock) {
        adp_os_free(pool);
        adp_log_d("Mutex creation failed");
        return (void*)0;
    }
    pool->head = (void*)0;
    return (void*)pool;
}

void adp_mem_pool_destroy(adp_mem_pool_t* pool)
{
    if (!pool) {
        adp_log_e("Null pool");
        return;
    }
    if (pool->lock) {
        adp_os_mutex_del(pool->lock);
    }
    adp_os_free(pool);
}

adp_result_t adp_mem_pool_push(adp_mem_pool_t* pool, void* data)
{
    if (!pool) {
        return ADP_RESULT_INVALID_PARAMETER;
    }
    adp_mem_pool_node_t *new_node = adp_os_malloc(sizeof(adp_mem_pool_node_t));
    if (!new_node) {
        return ADP_RESULT_MALLOC_FAILED;
    }
    new_node->data = data;
    new_node->next = (void*)0;

    adp_os_mutex_take(pool->lock);
    adp_mem_pool_node_t *node = pool->head;
    if (node) {
        while (node->next) {
            node = node->next;
        }
        node->next     = new_node;
        new_node->prev = node;
    } else {
        pool->head     = new_node;
        new_node->prev = (void*)0;
    }
    adp_os_mutex_give(pool->lock);
    return ADP_RESULT_SUCCESS;
}

void* adp_mem_pool_pop(adp_mem_pool_t* pool)
{
    adp_os_mutex_take(pool->lock);
    adp_mem_pool_node_t *node = pool->head;
    if (!node) {
        adp_os_mutex_give(pool->lock);
        // Empty
        return (void*)0;
    }
    adp_mem_pool_node_t *new_head = pool->head->next;
    if (new_head) {
        new_head->prev = (void*)0;
    }
    pool->head = new_head;
    void* data_ptr = node->data;
    adp_os_mutex_give(pool->lock);
    adp_os_free(node);
    return data_ptr;
}

