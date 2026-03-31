// COR24 APL Interpreter -- main entry point
// Phase 6: shared variable I/O (qled, qsw, qsvo)

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
        } else if (line[0] == 41) {
            // System command: starts with ')'
            if (str_match(line, 1, "CLEAR") == 5 && (line[6] == 0 || line[6] == 32)) {
                arr_reset();
                sym_reset();
                io_print("  CLEAR WS");
                putchar(10);
            } else if (str_match(line, 1, "VARS") == 4 && (line[5] == 0 || line[5] == 32)) {
                int i = 0;
                int any = 0;
                while (i < sym_count) {
                    if (sym_set_flag[i]) {
                        if (any) io_print("  ");
                        int off = sym_name_off[i];
                        while (sym_name_buf[off]) {
                            putchar(sym_name_buf[off]);
                            off++;
                        }
                        any = 1;
                    }
                    i++;
                }
                if (any) putchar(10);
            } else if (str_match(line, 1, "OFF") == 3 && (line[4] == 0 || line[4] == 32)) {
                return 0;
            } else {
                io_print("  SYNTAX ERROR");
                putchar(10);
            }
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
                    } else if (eval_err == 3) {
                        heap_top = heap_save;
                        io_print("  LENGTH ERROR");
                        putchar(10);
                    } else if (eval_err == 4) {
                        heap_top = heap_save;
                        io_print("  RANK ERROR");
                        putchar(10);
                    } else if (eval_err == 5) {
                        heap_top = heap_save;
                        io_print("  WS FULL");
                        putchar(10);
                    } else if (eval_err) {
                        heap_top = heap_save;
                        io_print("  DOMAIN ERROR");
                        putchar(10);
                    } else if (node_type[root] == NODE_ASSIGN) {
                        // Assignment: no output (APL convention)
                        // Don't restore heap -- variable value persists
                    } else if (node_type[root] == NODE_QLED_ASSIGN) {
                        // Quad LED assignment: no output
                        heap_top = heap_save;
                    } else if (node_type[root] == NODE_SVO_WRITE) {
                        // Shared variable indexed write: no output
                        heap_top = heap_save;
                    } else {
                        // Print result based on rank
                        int rank = arr_rank(result);
                        if (rank == 0) {
                            io_print("  ");
                            print_int(arr_get(result, 0));
                            putchar(10);
                        } else if (rank == 1) {
                            int sz = arr_dim0(result);
                            // Find max width across all elements
                            int maxw = 1;
                            int j = 0;
                            while (j < sz) {
                                int w = num_width(arr_get(result, j));
                                if (w > maxw) maxw = w;
                                j++;
                            }
                            // Print each element right-justified
                            j = 0;
                            while (j < sz) {
                                if (j == 0) putchar(32);
                                print_int_rj(arr_get(result, j), maxw);
                                if (j + 1 < sz) putchar(32);
                                j++;
                            }
                            putchar(10);
                        } else if (rank == 2) {
                            int rows = arr_dim0(result);
                            int cols = arr_dim1(result);
                            int sz = rows * cols;
                            // Find max width across all elements
                            int maxw = 1;
                            int j = 0;
                            while (j < sz) {
                                int w = num_width(arr_get(result, j));
                                if (w > maxw) maxw = w;
                                j++;
                            }
                            // Print each row on its own line
                            int r = 0;
                            while (r < rows) {
                                int c = 0;
                                while (c < cols) {
                                    if (c == 0) putchar(32);
                                    print_int_rj(arr_get(result, r * cols + c), maxw);
                                    if (c + 1 < cols) putchar(32);
                                    c++;
                                }
                                putchar(10);
                                r++;
                            }
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
