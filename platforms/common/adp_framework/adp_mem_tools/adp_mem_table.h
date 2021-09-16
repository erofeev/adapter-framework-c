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

typedef struct {
    const char                *name;
    const char              *format;
    uint32_t               row_size;
    uint32_t               rows_cnt;
} adp_mem_table_t;


adp_mem_table_row_t adp_mem_table_row_add(adp_mem_table_t *table, ...);
void                adp_mem_table_row_get(adp_mem_table_t *table, adp_mem_table_row_t row, ...);


#endif /* ADAPTERS_ADP_MEM_TABLE_H_ */
