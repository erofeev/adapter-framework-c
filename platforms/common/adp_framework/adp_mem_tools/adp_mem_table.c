/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdarg.h>
#include <string.h>

#include "adp_osal.h"
#include "adp_logging.h"
#include "adp_mem_table.h"

#ifdef ADP_MEM_TABLE_MODULE_NO_DEBUG
    #undef  adp_log_d
    #undef  adp_log_dd
    #define adp_log_d(...)
    #define adp_log_dd(...)
#endif


typedef struct {
    char                   type;
    uint8_t              length;
} adp_mem_table_cell_descr_t;


static
adp_mem_table_cell_descr_t cell[] = {
        /* 1 unsigned char                */       { 'b' , sizeof(ADP_UBYTE8)         },
        /* 4 bytes signed int             */       { 'd' , sizeof(ADP_SIGNED_INT32)   },
        /* 4 bytes unsigned int           */       { 'u' , sizeof(ADP_UNSIGNED_INT32) },
        /* Pointer to string              */       { 's' , sizeof(ADP_STRING)         },
        /* Float (typically IEEE 4 bytes) */       { 'f' , sizeof(ADP_FLOAT32)        },
};


adp_mem_table_row_t adp_mem_table_row_add(adp_mem_table_t *table, ...)
{
    if (!table) {
        adp_log_e("Table is NULL");
        return (void*)0;
    }
    if (!table->format) {
        adp_log_e("Table format is NULL");
        return (adp_mem_table_row_t)0;
    }

    char *ch = (char*)&table->format[0];
    if (!table->row_size) {
        while (*ch) {
            if (*ch++ != '%') {
                continue;
            }
            for (uint32_t i = 0; i < sizeof(cell)/sizeof(adp_mem_table_cell_descr_t); i++) {
                if (cell[i].type == *ch) {
                    adp_log_dd("Size of %c = %d bytes", *ch, cell[i].length);
                    table->row_size = table->row_size + cell[i].length;
                }
            }
        }
    }
    ch = (char*)&table->format[0];
    uint8_t *row = adp_os_malloc(table->row_size);
    if (!row) {
        return (adp_mem_table_row_t)0;
    }

    uint8_t *row_ptr = row;

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
                    char * str = adp_os_malloc(strlen(t) + 1);
                    if (!str) {
                        adp_log_e("Mem alloc error, drop string & set NULL");
                        memset(row_ptr, 0x00, sizeof(ADP_STRING));
                    } else {
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

    return row;
}


void adp_mem_table_row_get(adp_mem_table_t *table, adp_mem_table_row_t row, ...)
{
    char* ch = (char*)&table->format[0];
    char *raw_pos = (char*)row;
    va_list args;
    va_start(args, row);
    while (*ch) {
        if (*ch++ != '%') {
            continue;
        }
        for (uint32_t i = 0; i < sizeof(cell)/sizeof(adp_mem_table_cell_descr_t); i++) {
            if (cell[i].type == *ch) {
                char* ref = va_arg(args, void*);
                memcpy(ref, raw_pos, cell[i].length);
                raw_pos += cell[i].length;
                break;
            }
        }
    }
    va_end(args);
}

