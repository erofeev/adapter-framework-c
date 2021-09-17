/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#ifndef ADAPTERS_ADP_MEM_TABLE_H_
#define ADAPTERS_ADP_MEM_TABLE_H_

#include <stdint.h>

#define ADP_UBYTE8                 uint8_t
#define ADP_SIGNED_INT32           int32_t
#define ADP_UNSIGNED_INT32        uint32_t
#define ADP_STRING                   char*
#define ADP_FLOAT32                  float


typedef void* adp_mem_table_row_t;
typedef struct adp_mem_table_ctx_s adp_mem_table_ctx_t;

typedef struct {
    char                      *name;
    char                    *format;
    adp_mem_table_ctx_t  *table_ctx;
} adp_mem_table_t;


adp_mem_table_t*    adp_mem_table_create(const char* name, const char *format);
void                adp_mem_table_lock(adp_mem_table_t *table);
void                adp_mem_table_unlock(adp_mem_table_t *table);
adp_mem_table_row_t adp_mem_table_row_push(adp_mem_table_t *table, ...);
adp_mem_table_row_t adp_mem_table_row_pop(adp_mem_table_t *table, ...);
void                adp_mem_table_destroy(adp_mem_table_t *table);

#endif /* ADAPTERS_ADP_MEM_TABLE_H_ */
