// COR24 APL Interpreter -- main entry point
// Phase 0: UART I/O bootstrap with echo REPL

#include <stdio.h>
#include "io.h"

int main() {
    char line[IO_LINE_MAX];

    puts("COR24 APL v0.1");

    while (1) {
        io_print("      ");
        int n = io_getline(line, IO_LINE_MAX);
        if (n == 0) {
            // Empty line -- just re-prompt
        } else {
            // Echo the line back
            io_print("  ");
            puts(line);
        }
    }
    return 0;
}
