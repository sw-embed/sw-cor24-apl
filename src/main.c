// COR24 APL Interpreter -- main entry point
// Phase 0: Number I/O with parse and print

#include <stdio.h>
#include "io.h"
#include "num.h"

int main() {
    char line[IO_LINE_MAX];

    puts("COR24 APL v0.1");

    while (1) {
        io_print("      ");
        int n = io_getline(line, IO_LINE_MAX);
        if (n == 0) {
            // Empty line -- just re-prompt
        } else {
            int end;
            int val = parse_int(line, 0, &end);
            if (end > 0 && line[end] == 0) {
                // Valid number -- print it back
                io_print("  ");
                print_int(val);
                putchar(10);
            } else {
                // Not a number -- echo as text
                io_print("  ");
                puts(line);
            }
        }
    }
    return 0;
}
