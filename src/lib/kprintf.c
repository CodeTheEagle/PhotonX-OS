/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        kprintf.c
 * Module:      Kernel Standard Output Library
 * Description:
 * A lightweight, dependency-free implementation of printf() optimized for
 * embedded systems. It supports standard format specifiers.
 * ======================================================================================
 */

#include "drivers/uart_ps.h"
#include <stdarg.h> /* Compiler builtin for variable arguments */
#include <stdint.h>

/* Internal buffer for number conversion */
static char num_buffer[64];

/*
 * itoa: Integer to ASCII
 * Converts a signed integer to a string in the given base.
 */
static char* itoa(int64_t value, char* str, int base) {
    char* rc;
    char* ptr;
    char* low;
    
    // Check for supported base
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    rc = ptr = str;
    
    // Set flag for negative numbers
    int is_negative = (value < 0 && base == 10);
    
    // Work with positive values for calculation
    uint64_t abs_val = (uint64_t)(is_negative ? -value : value);
    
    // Conversion loop
    do {
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[abs_val % base];
        abs_val /= base;
    } while (abs_val);
    
    // Apply negative sign
    if (is_negative) {
        *ptr++ = '-';
    }
    
    // Terminate string
    *ptr-- = '\0';
    
    // Reverse string in place
    low = rc;
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    
    return rc;
}

/*
 * xtoa: Hex to ASCII (Unsigned)
 * Specialized for printing memory addresses.
 */
static char* xtoa(uint64_t value, char* str) {
    char* ptr = str;
    int i;
    
    for (i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (value >> i) & 0xF;
        *ptr++ = "0123456789ABCDEF"[nibble];
    }
    *ptr = '\0';
    return str;
}
/*
 * ======================================================================================
 * KERNEL PRINTF IMPLEMENTATION
 * ======================================================================================
 */

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char c;
    const char* str;
    int64_t i_val;
    uint64_t u_val;
    void* ptr_val;

    /* Loop through format string */
    while ((c = *format++) != 0) {
        
        if (c != '%') {
            uart_send_byte(c);
            continue;
        }

        /* Handle Format Specifier */
        c = *format++;
        if (c == 0) break;

        switch (c) {
            /* Character */
            case 'c':
                uart_send_byte((char)va_arg(args, int));
                break;

            /* String */
            case 's':
                str = va_arg(args, const char*);
                if (!str) str = "(null)";
                uart_send_string(str);
                break;

            /* Signed Decimal */
            case 'd':
            case 'i':
                i_val = va_arg(args, int);
                itoa(i_val, num_buffer, 10);
                uart_send_string(num_buffer);
                break;

            /* Unsigned Decimal */
            case 'u':
                u_val = va_arg(args, unsigned int);
                itoa(u_val, num_buffer, 10); // Re-use itoa for unsigned logic
                uart_send_string(num_buffer);
                break;

            /* Hexadecimal (Lower case) */
            case 'x':
                u_val = va_arg(args, unsigned int);
                itoa(u_val, num_buffer, 16);
                uart_send_string(num_buffer);
                break;

            /* Pointer / Address (64-bit Hex) */
            case 'p':
                ptr_val = va_arg(args, void*);
                uart_send_string("0x");
                xtoa((uint64_t)ptr_val, num_buffer);
                uart_send_string(num_buffer);
                break;
            
            /* Binary */
            case 'b':
                u_val = va_arg(args, unsigned int);
                itoa(u_val, num_buffer, 2);
                uart_send_string(num_buffer);
                break;

            /* Percent escape */
            case '%':
                uart_send_byte('%');
                break;

            default:
                uart_send_byte('%');
                uart_send_byte(c);
                break;
        }
    }
    
    va_end(args);
}
