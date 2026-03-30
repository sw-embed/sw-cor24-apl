// COR24 APL Interpreter -- main entry point
// Phase 1.3: Scalar evaluator

#include <stdio.h>
#include "io.h"
#include "num.h"
#include "tok.h"
#include "parse.h"
#include "eval.h"

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
                int root = parse();
                if (root < 0) {
                    io_print("  SYNTAX ERROR");
                    putchar(10);
                } else {
                    eval_err = 0;
                    int result = eval(root);
                    if (eval_err) {
                        io_print("  DOMAIN ERROR");
                        putchar(10);
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
