// COR24 APL Interpreter -- main entry point
// Phase 1: Tokenizer test mode

#include <stdio.h>
#include "io.h"
#include "num.h"
#include "tok.h"

int main() {
    char line[IO_LINE_MAX];

    puts("COR24 APL v0.1");

    while (1) {
        io_print("      ");
        int n = io_getline(line, IO_LINE_MAX);
        if (n == 0) {
            // Empty line -- just re-prompt
        } else {
            int ntok = tokenize(line);
            if (ntok < 0) {
                io_print("  SYNTAX ERROR");
                putchar(10);
            } else {
                io_print("  ");
                tok_dump(line);
            }
        }
    }
    return 0;
}
