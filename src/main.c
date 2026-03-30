// COR24 APL Interpreter -- main entry point
// Phase 2.2: Vector literals

#include <stdio.h>
#include "io.h"
#include "num.h"
#include "arr.h"
#include "tok.h"
#include "sym.h"
#include "parse.h"
#include "eval.h"

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
                // Save heap position for temporary reclamation
                int heap_save = heap_top;

                int root = parse(line);
                if (root < 0) {
                    heap_top = heap_save;
                    io_print("  SYNTAX ERROR");
                    putchar(10);
                } else {
                    eval_err = 0;
                    int result = eval(root);
                    if (eval_err == 2) {
                        heap_top = heap_save;
                        io_print("  VALUE ERROR");
                        putchar(10);
                    } else if (eval_err) {
                        heap_top = heap_save;
                        io_print("  DOMAIN ERROR");
                        putchar(10);
                    } else if (node_type[root] == NODE_ASSIGN) {
                        // Assignment: no output (APL convention)
                        // Don't restore heap -- variable value persists
                    } else {
                        // Print result based on rank
                        int rank = arr_rank(result);
                        if (rank == 0) {
                            io_print("  ");
                            print_int(arr_get(result, 0));
                            putchar(10);
                        } else if (rank == 1) {
                            io_print("  ");
                            int sz = arr_dim0(result);
                            int j = 0;
                            while (j < sz) {
                                if (j > 0) putchar(32);
                                print_int(arr_get(result, j));
                                j++;
                            }
                            putchar(10);
                        }
                        // Reclaim temporaries
                        heap_top = heap_save;
                    }
                }
            }
        }
    }
    return 0;
}
