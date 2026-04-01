// COR24 APL Interpreter -- main entry point
// Phase 9: batch mode support via APL image loading

#include <stdio.h>
#include "io.h"
#include "num.h"
#include "arr.h"
#include "tok.h"
#include "sym.h"
#include "fn.h"
#include "parse.h"
#include "eval.h"

// APL image format constants
// Image pointer at 0x09FF00: if non-zero, points to APL image in SRAM
// Image data loaded at 0x080000 via --load-binary
// Format: newline-separated APL lines, null-terminated
#define APL_IMAGE_PTR  0x09FF00
#define APL_IMAGE_BASE 0x080000

// Batch mode state
int batch_mode;      // 1 = reading from SRAM image, 0 = interactive UART
char *batch_ptr;     // current read position in APL image

// Read next line from APL image in SRAM.
// Copies up to len-1 chars into buf, null-terminates.
// Returns number of chars read (0 on empty line, -1 on end of image).
int batch_getline(char *buf, int len) {
    if (*batch_ptr == 0) return -1;  // end of image
    int pos = 0;
    int maxpos = len - 1;
    while (*batch_ptr && *batch_ptr != 10 && pos < maxpos) {
        buf[pos] = *batch_ptr;
        pos++;
        batch_ptr++;
    }
    buf[pos] = 0;
    if (*batch_ptr == 10) batch_ptr++;  // skip newline
    return pos;
}

// Program line buffer for multi-line programs
// 64 lines * 120 chars = 7680 bytes
#define PROG_MAX 64
#define PROG_LINE 120

char prog_buf[7680];
int prog_count;  // highest line number stored (1-based count)

void prog_store(char *src, int idx) {
    char *dst = prog_buf + idx * PROG_LINE;
    int k = 0;
    while (src[k] && k < PROG_LINE - 1) {
        dst[k] = src[k];
        k++;
    }
    dst[k] = 0;
}

// Get pointer to program line (0-based index)
char *prog_get(int idx) {
    return prog_buf + idx * PROG_LINE;
}

// Clear all program lines
void prog_clear() {
    int i = 0;
    while (i < PROG_MAX) {
        prog_buf[i * PROG_LINE] = 0;
        i++;
    }
    prog_count = 0;
}

// Pre-scan labels in stored program before execution
void prog_scan_labels() {
    int i = 0;
    while (i < prog_count) {
        char *ln = prog_get(i);
        if (is_upper(ln[0])) {
            int j = 1;
            while (is_alnum(ln[j])) j++;
            if (ln[j] == 58) {  // ':'
                int sym_idx = sym_lookup(ln, 0);
                if (sym_idx >= 0) {
                    sym_val[sym_idx] = arr_scalar(i + 1);  // 1-based line number
                    sym_set_flag[sym_idx] = 1;
                }
            }
        }
        i++;
    }
}

// Check if line starts with [N] line entry syntax
// Returns line number (1-based) if so, 0 if not
// Sets *rest to point past the "] " to the line content
int prog_parse_entry(char *line, char **rest) {
    if (line[0] != 91) return 0;  // '['
    int i = 1;
    if (!is_digit(line[i])) return 0;
    int num = 0;
    while (is_digit(line[i])) {
        num = num * 10 + (line[i] - 48);
        i++;
    }
    if (line[i] != 93) return 0;  // ']'
    i++;
    while (line[i] == 32) i++;  // skip spaces
    *rest = line + i;
    return num;
}

int main() {
    char line[IO_LINE_MAX];

    arr_reset();
    prog_clear();
    fn_reset();
    call_depth = 0;
    branch_target = -1;

    // Detect batch mode: check if image pointer is set
    // APL_IMAGE_PTR holds a 24-bit address; read as int (24-bit on COR24)
    int img_addr = *(int *)APL_IMAGE_PTR;
    if (img_addr) {
        batch_mode = 1;
        batch_ptr = (char *)img_addr;
    } else {
        batch_mode = 0;
        batch_ptr = 0;
    }

    puts("COR24 APL v0.1");

    int pc = -1;  // -1 = read from input, >=0 = replay stored line

    while (1) {
        char *cur_line;
        int cur_lineno;  // 1-based line number

        if (pc >= 0) {
            // Replaying stored line
            cur_line = prog_get(pc);
            cur_lineno = pc + 1;
        } else {
            // Read new line from input
            int n;
            if (batch_mode) {
                n = batch_getline(line, IO_LINE_MAX);
                if (n < 0) {
                    // End of image -- switch to interactive mode
                    batch_mode = 0;
                    continue;
                }
            } else {
                io_print("      ");
                n = io_getline(line, IO_LINE_MAX);
            }
            if (n == 0) continue;

            // Function definition mode: collect body lines
            if (fn_def_mode) {
                int ci = 0;
                while (line[ci] == 32) ci++;
                if (str_match(line, ci, "del") == 3 && !is_alnum(line[ci + 3])) {
                    // Closing del
                    fn_def_mode = 0;
                } else if (fn_def_line >= FN_BODY_MAX) {
                    io_print("  FN FULL");
                    putchar(10);
                    fn_def_mode = 0;
                } else {
                    fn_store_line(fn_def_idx, fn_def_line, line);
                    fn_lines[fn_def_idx] = fn_def_line + 1;
                    fn_def_line++;
                }
                continue;
            }

            // Check for [N] line entry syntax
            char *rest;
            int lnum = prog_parse_entry(line, &rest);
            if (lnum > 0 && lnum <= PROG_MAX) {
                prog_store(rest, lnum - 1);
                if (lnum > prog_count) prog_count = lnum;
                continue;
            }

            cur_line = line;
            cur_lineno = 0;  // immediate mode, no line number
        }

        // Check for label prefix: IDENT: at start of line
        char *exec_line = cur_line;
        if (is_upper(cur_line[0])) {
            int j = 1;
            while (is_alnum(cur_line[j])) j++;
            if (cur_line[j] == 58) {  // ':'
                // Record label as variable set to line number
                int sym_idx = sym_lookup(cur_line, 0);
                if (sym_idx >= 0) {
                    sym_val[sym_idx] = arr_scalar(cur_lineno);
                    sym_set_flag[sym_idx] = 1;
                }
                j++;
                while (cur_line[j] == 32) j++;
                exec_line = cur_line + j;
                if (exec_line[0] == 0) {
                    // Label-only line, no code to execute
                    if (pc >= 0) { pc++; if (pc >= prog_count) pc = -1; }
                    continue;
                }
            }
        }

        if (exec_line[0] == 0) {
            // Empty line
            if (pc >= 0) { pc++; if (pc >= prog_count) pc = -1; }
            continue;
        } else if (exec_line[0] == 41) {
            // System command: starts with ')'
            if (str_match(exec_line, 1, "CLEAR") == 5 && (exec_line[6] == 0 || exec_line[6] == 32)) {
                arr_reset();
                sym_reset();
                fn_reset();
                io_print("  CLEAR WS");
                putchar(10);
            } else if (str_match(exec_line, 1, "VARS") == 4 && (exec_line[5] == 0 || exec_line[5] == 32)) {
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
            } else if (str_match(exec_line, 1, "LIST") == 4 && (exec_line[5] == 0 || exec_line[5] == 32)) {
                int i = 0;
                while (i < prog_count) {
                    char *ln = prog_get(i);
                    if (ln[0]) {
                        putchar(91);  // '['
                        print_int(i + 1);
                        putchar(93);  // ']'
                        putchar(32);
                        io_print(ln);
                        putchar(10);
                    }
                    i++;
                }
            } else if (str_match(exec_line, 1, "RUN") == 3 && (exec_line[4] == 0 || exec_line[4] == 32)) {
                if (prog_count > 0) {
                    prog_scan_labels();
                    pc = 0;
                    branch_target = -1;
                    continue;
                }
            } else if (str_match(exec_line, 1, "ERASE") == 5 && (exec_line[6] == 0 || exec_line[6] == 32)) {
                prog_clear();
            } else if (str_match(exec_line, 1, "FNS") == 3 && (exec_line[4] == 0 || exec_line[4] == 32)) {
                int i = 0;
                int any = 0;
                while (i < fn_count) {
                    int sym = fn_name_sym[i];
                    if (any) io_print("  ");
                    int off = sym_name_off[sym];
                    while (sym_name_buf[off]) {
                        putchar(sym_name_buf[off]);
                        off++;
                    }
                    any = 1;
                    i++;
                }
                if (any) putchar(10);
            } else if (str_match(exec_line, 1, "OFF") == 3 && (exec_line[4] == 0 || exec_line[4] == 32)) {
                return 0;
            } else {
                io_print("  SYNTAX ERROR");
                putchar(10);
            }
        } else {
            // Check for "del" function definition (before tokenize, since
            // the tokenizer rejects unknown lowercase words)
            int di = 0;
            while (exec_line[di] == 32) di++;
            if (str_match(exec_line, di, "del") == 3 &&
                (exec_line[di + 3] == 0 || exec_line[di + 3] == 32)) {
                int after = di + 3;
                while (exec_line[after] == 32) after++;
                if (exec_line[after] == 0) {
                    // Bare "del" outside definition mode
                    io_print("  SYNTAX ERROR");
                    if (pc >= 0) { io_print(" ["); print_int(pc + 1); putchar(93); pc = -1; }
                    putchar(10);
                } else if (pc >= 0) {
                    // Can't define functions during program execution
                    io_print("  SYNTAX ERROR");
                    io_print(" ["); print_int(pc + 1); putchar(93); pc = -1;
                    putchar(10);
                } else {
                    // Start function definition
                    int temp = fn_count;
                    if (temp >= FN_MAX) {
                        io_print("  FN TABLE FULL");
                        putchar(10);
                    } else if (!parse_fn_header(exec_line + after, temp)) {
                        io_print("  SYNTAX ERROR");
                        putchar(10);
                    } else {
                        // Check for existing function with same name
                        int ns = fn_name_sym[temp];
                        int exi = 0;
                        int found = -1;
                        while (exi < fn_count) {
                            if (fn_name_sym[exi] == ns) found = exi;
                            exi++;
                        }
                        if (found >= 0) {
                            // Redefine existing function
                            fn_result_sym[found] = fn_result_sym[temp];
                            fn_right_sym[found] = fn_right_sym[temp];
                            fn_left_sym[found] = fn_left_sym[temp];
                            fn_lines[found] = 0;
                            fn_def_idx = found;
                        } else {
                            fn_count++;
                            fn_def_idx = temp;
                        }
                        fn_def_mode = 1;
                        fn_def_line = 0;
                    }
                }
                if (pc >= 0) { pc++; if (pc >= prog_count) pc = -1; }
                continue;
            }

            int ntok = tokenize(exec_line);
            if (ntok < 0) {
                io_print("  SYNTAX ERROR");
                if (pc >= 0) { io_print(" ["); print_int(pc + 1); putchar(93); pc = -1; }
                putchar(10);
            } else {
                // Save heap position for temporary reclamation
                int heap_save = heap_top;

                int root = parse(exec_line);
                if (root < 0) {
                    heap_top = heap_save;
                    io_print("  SYNTAX ERROR");
                    if (pc >= 0) { io_print(" ["); print_int(pc + 1); putchar(93); pc = -1; }
                    putchar(10);
                } else {
                    eval_err = 0;
                    branch_target = -1;
                    int result = eval(root);
                    if (eval_err == 2) {
                        heap_top = heap_save;
                        io_print("  VALUE ERROR");
                        if (pc >= 0) { io_print(" ["); print_int(pc + 1); putchar(93); pc = -1; }
                        putchar(10);
                    } else if (eval_err == 3) {
                        heap_top = heap_save;
                        io_print("  LENGTH ERROR");
                        if (pc >= 0) { io_print(" ["); print_int(pc + 1); putchar(93); pc = -1; }
                        putchar(10);
                    } else if (eval_err == 4) {
                        heap_top = heap_save;
                        io_print("  RANK ERROR");
                        if (pc >= 0) { io_print(" ["); print_int(pc + 1); putchar(93); pc = -1; }
                        putchar(10);
                    } else if (eval_err == 5) {
                        heap_top = heap_save;
                        io_print("  WS FULL");
                        if (pc >= 0) { io_print(" ["); print_int(pc + 1); putchar(93); pc = -1; }
                        putchar(10);
                    } else if (eval_err) {
                        heap_top = heap_save;
                        io_print("  DOMAIN ERROR");
                        if (pc >= 0) { io_print(" ["); print_int(pc + 1); putchar(93); pc = -1; }
                        putchar(10);
                    } else if (node_type[root] == NODE_ASSIGN) {
                        // Assignment: no output (APL convention)
                        // In program mode, reclaim RHS temporaries if
                        // scalar-to-scalar reuse kept the old heap slot
                        if (pc >= 0 && result == sym_val[node_val[root]] &&
                            result < heap_save) {
                            heap_top = heap_save;
                        }
                    } else if (node_type[root] == NODE_QLED_ASSIGN) {
                        // Quad LED assignment: no output
                        heap_top = heap_save;
                    } else if (node_type[root] == NODE_SVO_WRITE) {
                        // Shared variable indexed write: no output
                        heap_top = heap_save;
                    } else if (node_type[root] == NODE_GOTO || node_type[root] == NODE_CGOTO) {
                        // Branch: no output
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

        // Check for branch
        if (branch_target > 0) {
            pc = branch_target - 1;  // convert 1-based to 0-based
            branch_target = -1;
            continue;
        } else if (branch_target == 0) {
            // goto 0: stop execution
            if (pc >= 0) {
                // In program mode: return to immediate mode
                pc = -1;
                branch_target = -1;
                continue;
            }
            // In immediate mode: exit interpreter
            return 0;
        }

        // Advance PC if replaying stored lines
        if (pc >= 0) {
            pc++;
            if (pc >= prog_count) pc = -1;
        }
    }
    return 0;
}
