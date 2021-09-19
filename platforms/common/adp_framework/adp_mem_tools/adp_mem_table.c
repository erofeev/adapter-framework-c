/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdarg.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_logging.h"
#include "adp_mem_pool.h"
#include "adp_mem_table.h"

#if !ADP_MEM_TABLE_MODULE_DEBUG
    #undef  adp_log_d
    #undef  adp_log_dd
    #define adp_log_d(...)
    #define adp_log_dd(...)
#endif


typedef struct {
    char                   type;
    uint8_t              length;
} adp_mem_table_cell_descr_t;

struct adp_mem_table_ctx_s {
    uint32_t           row_size;
    uint32_t           rows_cnt;
    uint32_t         total_size;
    adp_mem_pool_t        *pool;
    adp_os_mutex_t   table_lock;
};

static
adp_mem_table_cell_descr_t cell[] = {
        /* 1 unsigned char                */       { 'b' , sizeof(ADP_UBYTE8)         },
        /* 4 bytes signed int             */       { 'd' , sizeof(ADP_SIGNED_INT32)   },
        /* 4 bytes unsigned int           */       { 'u' , sizeof(ADP_UNSIGNED_INT32) },
        /* Pointer to string              */       { 's' , sizeof(ADP_STRING)         },
        /* Float (typically IEEE 4 bytes) */       { 'f' , sizeof(ADP_FLOAT32)        },
};

void adp_mem_table_lock(adp_mem_table_t *table)
{
    adp_os_mutex_take(table->table_ctx->table_lock);
}

void adp_mem_table_unlock(adp_mem_table_t *table)
{
    adp_os_mutex_give(table->table_ctx->table_lock);
}

adp_mem_table_t* adp_mem_table_create(const char* name, const char *format)
{
    adp_mem_table_t *table;
    if ( (!name) || (!format) ) {
        adp_log_e("Invalid argument");
        return NULL;
    }

    uint32_t name_size   = strlen(name) + 1;
    uint32_t format_size = strlen(format) + 1;
    table = adp_os_malloc(sizeof(adp_mem_table_t) + sizeof(struct adp_mem_table_ctx_s) + name_size + format_size);
    if (!table) {
        adp_log_e("No memory left");
        return NULL;
    }

    table->table_ctx = (adp_mem_table_ctx_t*)((char*)(table) + sizeof(adp_mem_table_t));

    table->table_ctx->table_lock = adp_os_mutex_create();
    if (!table->table_ctx->table_lock) {
        adp_os_free(table);
        adp_log_e("No memory left");
        return NULL;
    }

    table->table_ctx->row_size   = 0;
    table->table_ctx->total_size = 0;
    table->table_ctx->rows_cnt   = 0;
    table->table_ctx->pool       = (adp_mem_pool_t*)adp_mem_pool_create();
    if (!table->table_ctx->pool) {
        adp_os_free(table);
        adp_log_e("No memory left");
        return NULL;
    }

    table->name     = (char*)(table) + sizeof(adp_mem_table_t) + sizeof(struct adp_mem_table_ctx_s);
    table->format   = (char*)(table) + sizeof(adp_mem_table_t) + sizeof(struct adp_mem_table_ctx_s) + name_size;
    memcpy(table->name  , name  , name_size);
    memcpy(table->format, format, format_size);

    const char *ch = &format[0];
    while (*ch) {
        if (*ch++ != '%') {
            continue;
        }
        for (uint32_t i = 0; i < sizeof(cell)/sizeof(adp_mem_table_cell_descr_t); i++) {
            if (cell[i].type == *ch) {
                adp_log_dd("Size of %c = %d bytes", *ch, cell[i].length);
                table->table_ctx->row_size = table->table_ctx->row_size + cell[i].length;
            }
        }
    }
    adp_log_d("Table [%s] format:'%s' row size is %d", table->name, table->format, table->table_ctx->row_size);
    return table;
}

void adp_mem_table_destroy(adp_mem_table_t *table)
{
    if (!table) {
        adp_log_e("Null table");
        return;
    }
    adp_mem_pool_destroy(table->table_ctx->pool);
    adp_os_mutex_del(table->table_ctx->table_lock);
    adp_os_free(table);
}

void adp_mem_table_row_del(adp_mem_table_t *table, adp_mem_table_row_t row)
{
    const char   *ch = table->format;
    uint8_t *row_pos = row;

    while (*ch) {
        if (*ch++ != '%') {
            continue;
        }
        for (uint32_t i = 0; i < sizeof(cell)/sizeof(adp_mem_table_cell_descr_t); i++) {
            if (cell[i].type == *ch) {
                if (*ch == 's') {
                    char* str;
                    memcpy(&str, row_pos, sizeof(char*));
                    if (row_pos != NULL) {
                        adp_os_free(str);
                    }
                }
                row_pos += cell[i].length;
                break;
            }
        }
    }
    table->table_ctx->rows_cnt--;
    adp_os_free(row);
}

adp_mem_table_row_t adp_mem_table_row_push(adp_mem_table_t *table, ...)
{
    if (!table) {
        adp_log_e("Table is NULL");
        return (adp_mem_table_row_t)0;
    }

    const char *ch = table->format;
    if (!table->table_ctx->row_size) {
        adp_log_e("Invalid row size");
        return (adp_mem_table_row_t)0;
    }
    ch = table->format;
    uint8_t *row = adp_os_malloc(table->table_ctx->row_size);
    if (!row) {
        adp_log_e("No memory left");
        return (adp_mem_table_row_t)0;
    }

    uint8_t *row_ptr    = row;
    uint32_t total_size = 0;

    va_list args;
    va_start(args, table);
    while (*ch) {
        if (*ch++ != '%') {
            continue;
        }
        switch (*ch) {
        case 'b':
            {
                ADP_UBYTE8 t = (ADP_UBYTE8)va_arg(args, int);  // int - 'case of promotion
                memcpy(row_ptr, &t, sizeof(ADP_UBYTE8));
                row_ptr += sizeof(ADP_UBYTE8);
            }
            break;
        case 'd':
            {
                ADP_SIGNED_INT32 t = (ADP_SIGNED_INT32)va_arg(args, ADP_SIGNED_INT32);
                memcpy(row_ptr, &t, sizeof(ADP_SIGNED_INT32));
                row_ptr += sizeof(ADP_SIGNED_INT32);
            }
            break;
        case 'u':
            {
                ADP_UNSIGNED_INT32 t = (ADP_UNSIGNED_INT32)va_arg(args, ADP_UNSIGNED_INT32);
                memcpy(row_ptr, &t, sizeof(ADP_UNSIGNED_INT32));
                row_ptr += sizeof(ADP_UNSIGNED_INT32);
            }
            break;
        case 's':
            {
                ADP_STRING t = (ADP_STRING)va_arg(args, ADP_STRING);
                if (t) {
                    void * str = adp_os_malloc(strlen(t) + 1);
                    if (!str) {
                        adp_log_e("Mem alloc error, drop string & set NULL");
                        memset(row_ptr, 0x00, sizeof(ADP_STRING));
                    } else {
                        total_size += strlen(t) + 1;
                        memcpy(str, t, strlen(t) + 1);
                        memcpy(row_ptr, &str, sizeof(ADP_STRING));
                    }
                } else {
                    memset(row_ptr, 0x00, sizeof(ADP_STRING));
                }
                row_ptr += sizeof(ADP_STRING);
            }
            break;
        case 'f':
            {
                ADP_FLOAT32 t = (ADP_FLOAT32)va_arg(args, double); // double - 'case of promotion
                memcpy(row_ptr, &t, sizeof(ADP_FLOAT32));
                row_ptr += sizeof(ADP_FLOAT32);
            }
            break;
        default:
            {
                adp_log_e("Unsupported format \%[%c]", *ch);
            }
            break;
        }
    }
    va_end(args);
    total_size += (char*)row_ptr - (char*)row;
    table->table_ctx->total_size += total_size;
    table->table_ctx->rows_cnt++;
    if (ADP_RESULT_SUCCESS != adp_mem_pool_push(table->table_ctx->pool, row)) {
        adp_mem_table_row_del(table, row);
        row = 0;
    }
    adp_log_dd("Table [%s]rows %d tot_size: %d bytes", table->name, table->table_ctx->rows_cnt, table->table_ctx->total_size);
    return row;
}


adp_mem_table_row_t adp_mem_table_row_pop(adp_mem_table_t *table, ...)
{
    ADP_ASSERT(table->format, "Table format is NULL");
    const char* ch = table->format;
    const adp_mem_table_row_t row = adp_mem_pool_pop(table->table_ctx->pool);
    if (!row) {
        adp_log_dd("Empty table");
        return row;
    }
    char *row_pos = (char*)row;
    va_list args;
    va_start(args, table);
    while (*ch) {
        if (*ch++ != '%') {
            continue;
        }
        for (uint32_t i = 0; i < sizeof(cell)/sizeof(adp_mem_table_cell_descr_t); i++) {
            if (cell[i].type == *ch) {
                char* ref = va_arg(args, void*);
                memcpy(ref, row_pos, cell[i].length);
                row_pos += cell[i].length;
                break;
            }
        }
    }
    va_end(args);
    adp_mem_table_row_del(table,row);
    return row;
}

