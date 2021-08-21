/*
 ============================================================================
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>

#include "adp_uart.h"



char adp_uart_getchar(void)
{
    int c = getchar();
    return (char)c;
}


void adp_uart_putchar(char c)
{
    putchar(c);
}
