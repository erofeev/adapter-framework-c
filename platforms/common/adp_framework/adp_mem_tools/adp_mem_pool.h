/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_MEM_POOL_H_
#define ADAPTERS_ADP_MEM_POOL_H_


typedef struct adp_mem_pool_s adp_mem_pool_t;

adp_mem_pool_t* adp_mem_pool_create(void);
adp_result_t    adp_mem_pool_push(adp_mem_pool_t *pool, void* data);
void*           adp_mem_pool_pop (adp_mem_pool_t *pool);
void            adp_mem_pool_destroy (adp_mem_pool_t *pool);

#endif /* ADAPTERS_ADP_MEM_POOL_H_ */
