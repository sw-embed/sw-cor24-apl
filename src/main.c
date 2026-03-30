// COR24 APL Interpreter -- main entry point
// Phase 2.1: Array data structure

#include <stdio.h>
#include "io.h"
#include "num.h"
#include "tok.h"
#include "sym.h"
#include "parse.h"
#include "eval.h"
#include "arr.h"

int main() {
    char line[IO_LINE_MAX];

    arr_reset();
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
                int root = parse(line);
                if (root < 0) {
                    io_print("  SYNTAX ERROR");
                    putchar(10);
                } else {
                    eval_err = 0;
                    int result = eval(root);
                    if (eval_err == 2) {
                        io_print("  VALUE ERROR");
                        putchar(10);
                    } else if (eval_err) {
                        io_print("  DOMAIN ERROR");
                        putchar(10);
                    } else if (node_type[root] == NODE_ASSIGN) {
                        // Assignment: no output (APL convention)
                    } else {
                        io_print("  ");
                        print_int(result);
                        putchar(10);
                    }
                }
            }
        }
    }
    return 0;
}
