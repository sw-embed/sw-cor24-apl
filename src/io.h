// COR24 APL Interpreter -- I/O layer
// Wraps UART for line-oriented input with editing support.

#pragma once

#include <stdio.h>

#define IO_LINE_MAX 120

// Read a line from UART into buf (max len-1 chars + NUL).
// Handles backspace (BS=8, DEL=127): erases last char, sends BS-SPACE-BS.
// Returns number of chars read (excluding NUL), or 0 on empty line.
// Echoes characters as typed. Terminates on CR (13) or LF (10).
int io_getline(char *buf, int len) {
    int pos = 0;
    int maxpos = len - 1;

    while (1) {
        int ch = getchar();

        if (ch == 10 || ch == 13) {
            // Newline -- terminate
            putchar(10);
            buf[pos] = 0;
            return pos;
        }

        if (ch == 8 || ch == 127) {
            // Backspace or DEL -- erase last char
            if (pos > 0) {
                pos--;
                putchar(8);   // BS
                putchar(32);  // space
                putchar(8);   // BS
            }
        } else if (ch >= 32 && pos < maxpos) {
            // Printable character
            buf[pos] = ch;
            pos++;
            putchar(ch);
        }
        // Ignore other control chars and overflow
    }
}

// Print a string without trailing newline
void io_print(char *s) {
    while (*s) {
        putchar(*s);
        s = s + 1;
    }
}
