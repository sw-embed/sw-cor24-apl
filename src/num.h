// COR24 APL Interpreter -- Number I/O
// Integer to/from decimal string with APL underscore-negative convention.

#pragma once

#include "io.h"

// Parse a decimal integer from a string.
// APL convention: underscore prefix for negative (_42 means -42).
// Returns the parsed value. Sets *end to index past last digit consumed.
// If no digits found, *end is set to start position.
int parse_int(char *s, int start, int *end) {
    int i = start;
    int neg = 0;
    int val = 0;
    int got = 0;

    // Check for underscore-negative prefix
    if (s[i] == 95) {
        neg = 1;
        i++;
    }

    // Parse digits
    while (s[i] >= 48 && s[i] <= 57) {
        val = val * 10 + (s[i] - 48);
        i++;
        got = 1;
    }

    if (got == 0) {
        // No digits consumed
        *end = start;
        return 0;
    }

    *end = i;
    if (neg) {
        return 0 - val;
    }
    return val;
}

// Return the print width of an integer (digit count + 1 for underscore if negative).
int num_width(int n) {
    int w = 0;
    if (n < 0) { w = 1; n = 0 - n; }
    if (n == 0) return w + 1;
    while (n > 0) { w++; n = n / 10; }
    return w;
}

// Print an integer to UART using APL underscore-negative convention.
// Negative numbers are printed as _N (e.g., -42 prints as _42).
void print_int(int n) {
    if (n < 0) {
        putchar(95);  // underscore
        n = 0 - n;
    }
    if (n == 0) {
        putchar(48);
        return;
    }
    char buf[8];
    int i = 0;
    while (n > 0) {
        buf[i] = 48 + n % 10;
        n = n / 10;
        i++;
    }
    while (i > 0) {
        i--;
        putchar(buf[i]);
    }
}

// Print an integer right-justified in a field of given width.
void print_int_rj(int n, int width) {
    int w = num_width(n);
    int pad = width - w;
    while (pad > 0) { putchar(32); pad--; }
    print_int(n);
}
