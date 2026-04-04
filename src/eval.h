// COR24 APL Interpreter -- Evaluator
// Tree-walking evaluator returning array heap indices.
// Every result is an array (scalars wrapped via arr_scalar).
// Supports element-wise ops on vectors with scalar extension.

#pragma once

// Error codes: 0=none, 1=DOMAIN, 2=VALUE, 3=LENGTH, 4=RANK, 5=WS FULL
int eval_err;

// Branch target: -1=no branch, 0=exit, >0=line number (1-based)
int branch_target;

// Shadow register for LED D2 (0xFF0000 is write-only for LEDs)
// Stores the user-visible value (1=on, 0=off), not the raw active-low bit

// Index origin (□IO): 0 or 1, affects iota generation
// Default 1 = standard APL (iota N → 1 2 ... N)
int io_origin = 1;

// PRNG state (LCG: Linear Congruential Generator)
// Constants for 24-bit modulus (overflow handles mod 2^24):
//   a = 1664525 (0x19660D), c = 12345 (both fit in 24 bits)
//   a mod 4 = 1, c is odd: guarantees full period
// Auto-seeded from UART keystroke counter (io_key_count)
int lcg_seed;

int lcg_next() {
    // Auto-seed from UART keystroke counter if seed is still 0
    if (lcg_seed == 0) {
        lcg_seed = io_key_count * 1664525 + 12345;
    }
    lcg_seed = lcg_seed * 1664525 + 12345;
    return lcg_seed;
}

// Return random integer in range 1..n (APL convention: 1-origin roll)
// Avoids modulo on large values (COR24 software divide is O(quotient)).
// Uses rejection sampling on a small extracted range for uniform distribution.
int lcg_roll(int n) {
    if (n <= 0) return 0;
    int r = lcg_next();
    // Extract middle bits for better distribution, make positive
    r = (r >> 8) & 4095;  // 0..4095 (shift right 8, mask 12 bits)
    // For small n (typical: dice 1-6), modulo on small dividend is fast
    return (r % n) + 1;
}

// ---- Call stack for user function recursion ----

// Save AST state per call frame (CALL_MAX * AST_MAX = 8 * 64 = 512)
int save_ntype[512];
int save_nval[512];
int save_nleft[512];
int save_nright[512];
int save_ncount[CALL_MAX];

// Save token state per call frame (CALL_MAX * TOK_MAX = 512)
int save_ttype[512];
int save_tval[512];
int save_tpos[512];
int save_tcount[CALL_MAX];

// Save branch target per call frame
int save_btarget[CALL_MAX];

// Save local variables per call frame (result, right, left = 3 per frame)
int save_lval[24];   // CALL_MAX * 3
int save_lset[24];   // CALL_MAX * 3

int call_depth;

// ---- Print array result (used for quad output in function bodies) ----

void print_array(int result) {
    int rank = arr_rank(result);

    // Boxed arrays: print each element on its own line
    if (arr_type(result) == ARR_BOXED) {
        int sz = arr_size(result);
        int j = 0;
        while (j < sz) {
            int elem = arr_get(result, j);
            if (arr_type(elem) == ARR_CHAR) {
                // String element: print with leading space
                io_print(" ");
                int esz = arr_size(elem);
                int k = 0;
                while (k < esz) {
                    putchar(arr_get(elem, k));
                    k++;
                }
                putchar(10);
            } else {
                // Numeric or other element: delegate
                print_array(elem);
            }
            j++;
        }
        return;
    }

    // Character arrays: print as string
    if (arr_type(result) == ARR_CHAR) {
        int sz = arr_size(result);
        int j = 0;
        while (j < sz) {
            putchar(arr_get(result, j));
            j++;
        }
        putchar(10);
        return;
    }

    if (rank == 0) {
        io_print("  ");
        print_int(arr_get(result, 0));
        putchar(10);
    } else if (rank == 1) {
        int sz = arr_dim0(result);
        int maxw = 1;
        int j = 0;
        while (j < sz) {
            int w = num_width(arr_get(result, j));
            if (w > maxw) maxw = w;
            j++;
        }
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
        int maxw = 1;
        int j = 0;
        while (j < sz) {
            int w = num_width(arr_get(result, j));
            if (w > maxw) maxw = w;
            j++;
        }
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
}

// ---- Forward declarations for mutual recursion ----
int eval(int n);
int eval_fncall(int n);

// ---- User function call execution ----
// Separated from eval() to reduce stack frame size for recursion.

int eval_fncall(int n) {
    int fi = node_val[n];

    // Evaluate right argument (monadic/dyadic only, not niladic)
    int ra = -1;
    if (node_right[n] >= 0) {
        ra = eval(node_right[n]);
        if (eval_err) return -1;
    }

    // Evaluate left argument (dyadic only)
    int la = -1;
    if (node_left[n] >= 0) {
        la = eval(node_left[n]);
        if (eval_err) return -1;
    }

    // Check call depth
    if (call_depth >= CALL_MAX) { eval_err = 5; return -1; }
    int d = call_depth;
    call_depth = d + 1;

    // Save AST + token state
    int base = d * 64;
    int si = 0;
    while (si < node_count) {
        save_ntype[base + si] = node_type[si];
        save_nval[base + si] = node_val[si];
        save_nleft[base + si] = node_left[si];
        save_nright[base + si] = node_right[si];
        si++;
    }
    save_ncount[d] = node_count;
    si = 0;
    while (si < tok_count) {
        save_ttype[base + si] = tok_type[si];
        save_tval[base + si] = tok_val[si];
        save_tpos[base + si] = tok_pos[si];
        si++;
    }
    save_tcount[d] = tok_count;
    save_btarget[d] = branch_target;

    // Save local variables (result, right, left)
    int lbase = d * 3;
    int rs = fn_result_sym[fi];
    int xs = fn_right_sym[fi];
    save_lval[lbase] = sym_val[rs];
    save_lset[lbase] = sym_set_flag[rs];
    if (xs >= 0) {
        save_lval[lbase + 1] = sym_val[xs];
        save_lset[lbase + 1] = sym_set_flag[xs];
    }
    if (fn_left_sym[fi] >= 0) {
        int ys = fn_left_sym[fi];
        save_lval[lbase + 2] = sym_val[ys];
        save_lset[lbase + 2] = sym_set_flag[ys];
    }

    // Set arguments (skip for niladic)
    if (xs >= 0) {
        sym_val[xs] = ra;
        sym_set_flag[xs] = 1;
    }
    if (fn_left_sym[fi] >= 0 && la >= 0) {
        sym_val[fn_left_sym[fi]] = la;
        sym_set_flag[fn_left_sym[fi]] = 1;
    }
    sym_set_flag[rs] = 0;  // result initially undefined

    // Pre-scan labels in function body
    fn_scan_labels(fi);

    // Execute function body lines
    int fpc = 0;
    int fdone = 0;
    int fheap_save = 0;
    branch_target = -1;

    while (fpc < fn_lines[fi] && !fdone) {
        char *fline = fn_get_line(fi, fpc);
        if (fline[0] == 0) { fpc++; continue; }

        // Handle label prefix
        char *fexec = fline;
        if (is_upper(fline[0])) {
            int fj = 1;
            while (is_alnum(fline[fj])) fj++;
            if (fline[fj] == 58) {  // ':'
                fj++;
                while (fline[fj] == 32) fj++;
                fexec = fline + fj;
                if (fexec[0] == 0) { fpc++; continue; }
            }
        }

        int ntok = tokenize(fexec);
        if (ntok < 0) { eval_err = 1; fdone = 1; continue; }

        // Save heap for temporary reclamation
        fheap_save = heap_top;

        int froot = parse(fexec);
        if (froot < 0) { heap_top = fheap_save; eval_err = 1; fdone = 1; continue; }

        eval_err = 0;
        branch_target = -1;
        int fres = eval(froot);

        if (eval_err) { heap_top = fheap_save; fdone = 1; continue; }

        // Print quad output inside functions, then reclaim
        if (node_type[froot] == NODE_QOUT) {
            print_array(fres);
            heap_top = fheap_save;
        } else if (node_type[froot] != NODE_ASSIGN) {
            // Non-assignment, non-output: reclaim temporaries
            heap_top = fheap_save;
        }

        // Branch handling
        if (branch_target == 0) {
            fdone = 1;
        } else if (branch_target > 0) {
            fpc = branch_target - 1;
            branch_target = -1;
        } else {
            fpc++;
        }
    }

    // Get result from result variable
    int result = -1;
    if (!eval_err) {
        if (sym_set_flag[rs]) {
            result = sym_val[rs];
        } else {
            eval_err = 2;  // VALUE ERROR: result not set
        }
    }

    // Restore local variables
    sym_val[rs] = save_lval[lbase];
    sym_set_flag[rs] = save_lset[lbase];
    if (xs >= 0) {
        sym_val[xs] = save_lval[lbase + 1];
        sym_set_flag[xs] = save_lset[lbase + 1];
    }
    if (fn_left_sym[fi] >= 0) {
        int ys = fn_left_sym[fi];
        sym_val[ys] = save_lval[lbase + 2];
        sym_set_flag[ys] = save_lset[lbase + 2];
    }

    // Restore AST + token state
    node_count = save_ncount[d];
    si = 0;
    while (si < node_count) {
        node_type[si] = save_ntype[base + si];
        node_val[si] = save_nval[base + si];
        node_left[si] = save_nleft[base + si];
        node_right[si] = save_nright[base + si];
        si++;
    }
    tok_count = save_tcount[d];
    si = 0;
    while (si < tok_count) {
        tok_type[si] = save_ttype[base + si];
        tok_val[si] = save_tval[base + si];
        tok_pos[si] = save_tpos[base + si];
        si++;
    }
    branch_target = save_btarget[d];

    call_depth = d;
    return result;
}

// Apply a binary op to two scalars
int eval_binop_scalar(int op, int a, int b) {
    if (op == TOK_PLUS)       return a + b;
    if (op == TOK_MINUS)      return a - b;
    if (op == TOK_STAR)       return a * b;
    if (op == TOK_SLASH) {
        if (b == 0) { eval_err = 1; return 0; }
        return a / b;
    }
    if (op == TOK_EQ)         return a == b;
    if (op == TOK_NE)         return a != b;
    if (op == TOK_LT)         return a < b;
    if (op == TOK_GT)         return a > b;
    if (op == TOK_LE)         return a <= b;
    if (op == TOK_GE)         return a >= b;
    if (op == TOK_CEIL)       return (a > b) ? a : b;
    if (op == TOK_FLOOR)      return (a < b) ? a : b;
    if (op == TOK_AND_OP)     return a & b;
    if (op == TOK_OR_OP)      return a | b;
    eval_err = 1;
    return 0;
}

// Evaluate AST node, return heap index of result array.
// On error, sets eval_err and returns -1.
int eval(int n) {
    if (eval_err) return -1;
    if (n < 0 || n >= node_count) {
        eval_err = 1;
        return -1;
    }

    int ty = node_type[n];

    if (ty == NODE_NUM) {
        int r = arr_scalar(node_val[n]);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_VEC) {
        return node_val[n];
    }

    if (ty == NODE_IDENT) {
        int idx = node_val[n];
        if (!sym_set_flag[idx]) {
            eval_err = 2;
            return -1;
        }
        return sym_val[idx];
    }

    if (ty == NODE_ASSIGN) {
        int idx = node_val[n];
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        // Scalar-to-scalar reassignment: reuse existing heap slot
        // to avoid heap growth in tight loops
        if (sym_set_flag[idx] && arr_rank(v) == 0 &&
            arr_rank(sym_val[idx]) == 0) {
            arr_set(sym_val[idx], 0, arr_get(v, 0));
            return sym_val[idx];
        }
        sym_val[idx] = v;
        sym_set_flag[idx] = 1;
        return v;
    }

    if (ty == NODE_FNCALL) {
        return eval_fncall(n);
    }

    if (ty == NODE_QOUT) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        return v;
    }

    if (ty == NODE_QRL) {
        // Read PRNG seed
        int r = arr_scalar(lcg_seed);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_QRL_ASSIGN) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        if (arr_rank(v) != 0) { eval_err = 4; return -1; }
        lcg_seed = arr_get(v, 0);
        return v;
    }

    if (ty == NODE_QIO) {
        // Read index origin
        int r = arr_scalar(io_origin);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_QIO_ASSIGN) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        if (arr_rank(v) != 0) { eval_err = 4; return -1; }
        int val = arr_get(v, 0);
        if (val != 0 && val != 1) { eval_err = 1; return -1; }
        io_origin = val;
        return v;
    }

    if (ty == NODE_QSVO) {
        // Shared variable offer: IDENT qsvo AP
        // Evaluate AP number, couple symbol if AP supported
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        if (arr_rank(v) != 0) { eval_err = 4; return -1; }
        int ap = arr_get(v, 0);
        int sym_idx = node_val[n];
        if (ap == 242) {
            // AP 242: MMIO byte access (FF0000+offset)
            svo_ap[sym_idx] = 242;
            int r = arr_scalar(2);
            if (r < 0) { eval_err = 5; return -1; }
            return r;
        }
        // Unknown AP: return 0 (not coupled)
        svo_ap[sym_idx] = 0;
        int r = arr_scalar(0);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_SVO_READ) {
        // Indexed read: IDENT[N]
        int sym_idx = node_val[n];
        int ap = svo_ap[sym_idx];
        if (ap != 0) {
            // Shared variable MMIO access
            int v = eval(node_right[n]);
            if (eval_err) return -1;
            if (arr_rank(v) != 0) { eval_err = 4; return -1; }
            int offset = arr_get(v, 0);
            if (ap == 242) {
                int addr = 0xFF0000 + offset;
                int val = *(char *)addr;
                val = val & 0xFF;
                int r = arr_scalar(val);
                if (r < 0) { eval_err = 5; return -1; }
                return r;
            }
            eval_err = 1;
            return -1;
        }
        // Regular vector bracket indexing: V[N]
        if (!sym_set_flag[sym_idx]) { eval_err = 2; return -1; }
        int arr = sym_val[sym_idx];
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        if (arr_rank(v) != 0) { eval_err = 4; return -1; }
        int idx = arr_get(v, 0);
        int sz = arr_size(arr);
        if (idx < 0 || idx >= sz) { eval_err = 3; return -1; }  // INDEX ERROR
        int val = arr_get(arr, idx);
        int r = arr_scalar(val);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_SVO_WRITE) {
        // Indexed write: IDENT[N] <- expr
        int sym_idx = node_val[n];
        int ap = svo_ap[sym_idx];
        if (ap != 0) {
            // Shared variable MMIO write
            int idx = eval(node_left[n]);
            if (eval_err) return -1;
            if (arr_rank(idx) != 0) { eval_err = 4; return -1; }
            int offset = arr_get(idx, 0);
            int v = eval(node_right[n]);
            if (eval_err) return -1;
            if (arr_rank(v) != 0) { eval_err = 4; return -1; }
            int val = arr_get(v, 0);
            if (ap == 242) {
                int addr = 0xFF0000 + offset;
                *(char *)addr = val & 0xFF;
                return v;
            }
            eval_err = 1;
            return -1;
        }
        // Regular vector bracket indexed write: V[N] <- expr
        if (!sym_set_flag[sym_idx]) { eval_err = 2; return -1; }
        int arr = sym_val[sym_idx];
        int idxv = eval(node_left[n]);
        if (eval_err) return -1;
        if (arr_rank(idxv) != 0) { eval_err = 4; return -1; }
        int idx = arr_get(idxv, 0);
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        if (arr_rank(v) != 0) { eval_err = 4; return -1; }
        int val = arr_get(v, 0);
        int sz = arr_size(arr);
        if (idx < 0 || idx >= sz) { eval_err = 3; return -1; }  // INDEX ERROR
        arr_set(arr, idx, val);
        return v;
    }

    if (ty == NODE_GOTO) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        if (arr_rank(v) != 0) { eval_err = 4; return -1; }
        branch_target = arr_get(v, 0);
        int r = arr_scalar(0);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_CGOTO) {
        int cv = eval(node_left[n]);
        if (eval_err) return -1;
        if (arr_rank(cv) != 0) { eval_err = 4; return -1; }
        int cond = arr_get(cv, 0);
        if (cond != 0) {
            int tv = eval(node_right[n]);
            if (eval_err) return -1;
            if (arr_rank(tv) != 0) { eval_err = 4; return -1; }
            branch_target = arr_get(tv, 0);
        }
        int r = arr_scalar(0);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_NEG) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        int rk = arr_rank(v);
        if (rk == 0) {
            int r = arr_scalar(0 - arr_get(v, 0));
            if (r < 0) { eval_err = 5; return -1; }
            return r;
        }
        // Negate each element of vector
        if (rk == 1) {
            int sz = arr_size(v);
            int r = arr_vector(sz);
            if (r < 0) { eval_err = 5; return -1; }
            int i = 0;
            while (i < sz) {
                arr_set(r, i, 0 - arr_get(v, i));
                i++;
            }
            return r;
        }
        // Negate each element of matrix
        if (rk == 2) {
            int d0 = arr_dim0(v);
            int d1 = arr_dim1(v);
            int sz = d0 * d1;
            int r = arr_new(2, d0, d1);
            if (r < 0) { eval_err = 5; return -1; }
            int i = 0;
            while (i < sz) {
                arr_set(r, i, 0 - arr_get(v, i));
                i++;
            }
            return r;
        }
        eval_err = 4;
        return -1;
    }

    if (ty == NODE_MONAD) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        int res_id = node_val[n];

        if (res_id == RES_IOTA) {
            // iota N: lazy vector io_origin .. io_origin+N-1
            // Only allocates header (4 words), elements computed on access.
            if (arr_rank(v) != 0) { eval_err = 4; return -1; }
            int count = arr_get(v, 0);
            if (count < 0) { eval_err = 1; return -1; }
            int r = arr_iota(count, io_origin);
            if (r < 0) { eval_err = 5; return -1; }
            return r;
        }

        if (res_id == RES_RHO) {
            // rho A: return shape of A
            int rk = arr_rank(v);
            if (rk == 0) {
                // Scalar: shape is empty vector
                int r = arr_vector(0);
                if (r < 0) { eval_err = 5; return -1; }
                return r;
            }
            if (rk == 1) {
                // Vector: shape is 1-element vector with length
                int r = arr_scalar(arr_dim0(v));
                if (r < 0) { eval_err = 5; return -1; }
                return r;
            }
            if (rk == 2) {
                // Matrix: shape is 2-element vector (rows cols)
                int r = arr_vector(2);
                if (r < 0) { eval_err = 5; return -1; }
                arr_set(r, 0, arr_dim0(v));
                arr_set(r, 1, arr_dim1(v));
                return r;
            }
            eval_err = 4;
            return -1;
        }

        if (res_id == RES_REV) {
            // rev A: reverse a vector
            int rk = arr_rank(v);
            if (rk == 0) {
                // Scalar: reverse is identity
                return v;
            }
            if (rk == 1) {
                int sz = arr_size(v);
                int r = arr_vector(sz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < sz) {
                    arr_set(r, i, arr_get(v, sz - 1 - i));
                    i++;
                }
                return r;
            }
            eval_err = 4;
            return -1;
        }

        if (res_id == RES_CAT) {
            // Monadic cat: ravel (flatten to 1D vector)
            int rk = arr_rank(v);
            if (rk == 0) {
                // Scalar -> 1-element vector
                int r = arr_vector(1);
                if (r < 0) { eval_err = 5; return -1; }
                arr_set(r, 0, arr_get(v, 0));
                arr_set_type(r, arr_type(v));
                return r;
            }
            if (rk == 1) {
                // Vector: already 1D, return as-is
                return v;
            }
            if (rk == 2) {
                // Matrix -> vector of all elements
                int sz = arr_size(v);
                int r = arr_vector(sz);
                if (r < 0) { eval_err = 5; return -1; }
                arr_set_type(r, arr_type(v));
                int i = 0;
                while (i < sz) {
                    arr_set(r, i, arr_get(v, i));
                    i++;
                }
                return r;
            }
            eval_err = 4;
            return -1;
        }

        if (res_id == RES_NOT) {
            // not A: bitwise NOT (complement)
            int rk = arr_rank(v);
            if (rk == 0) {
                int r = arr_scalar(~arr_get(v, 0));
                if (r < 0) { eval_err = 5; return -1; }
                return r;
            }
            if (rk == 1) {
                int sz = arr_size(v);
                int r = arr_vector(sz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < sz) {
                    arr_set(r, i, ~arr_get(v, i));
                    i++;
                }
                return r;
            }
            if (rk == 2) {
                int d0 = arr_dim0(v);
                int d1 = arr_dim1(v);
                int r = arr_new(2, d0, d1);
                if (r < 0) { eval_err = 5; return -1; }
                int sz = d0 * d1;
                int i = 0;
                while (i < sz) {
                    arr_set(r, i, ~arr_get(v, i));
                    i++;
                }
                return r;
            }
            eval_err = 4;
            return -1;
        }

        if (res_id == RES_ROLL) {
            // roll N: random integer 1..N (element-wise on vectors)
            int rk = arr_rank(v);
            if (rk == 0) {
                int n_val = arr_get(v, 0);
                if (n_val <= 0) { eval_err = 1; return -1; }
                int r = arr_scalar(lcg_roll(n_val));
                if (r < 0) { eval_err = 5; return -1; }
                return r;
            }
            if (rk == 1) {
                int sz = arr_size(v);
                int r = arr_vector(sz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < sz) {
                    int n_val = arr_get(v, i);
                    if (n_val <= 0) { eval_err = 1; return -1; }
                    arr_set(r, i, lcg_roll(n_val));
                    i++;
                }
                return r;
            }
            eval_err = 4;
            return -1;
        }

        if (res_id == RES_FMT) {
            // fmt N: convert integer to character vector
            // fmt V: convert vector to space-separated character vector
            int rk = arr_rank(v);
            if (rk == 0) {
                // Scalar: format single integer
                int val = arr_get(v, 0);
                int w = num_width(val);
                int r = arr_vector(w);
                if (r < 0) { eval_err = 5; return -1; }
                arr_set_type(r, ARR_CHAR);
                // Fill digits from right to left
                int pos = w - 1;
                int n_abs = val;
                if (val < 0) { n_abs = 0 - val; }
                if (n_abs == 0) {
                    arr_set(r, pos, 48);
                } else {
                    while (n_abs > 0) {
                        arr_set(r, pos, 48 + n_abs % 10);
                        n_abs = n_abs / 10;
                        pos--;
                    }
                }
                if (val < 0) { arr_set(r, 0, 95); } // underscore for negative
                return r;
            }
            if (rk == 1) {
                // Vector: format each element, space-separated
                int sz = arr_size(v);
                // Calculate total length
                int total = 0;
                int i = 0;
                while (i < sz) {
                    if (i > 0) total++; // space separator
                    total = total + num_width(arr_get(v, i));
                    i++;
                }
                int r = arr_vector(total);
                if (r < 0) { eval_err = 5; return -1; }
                arr_set_type(r, ARR_CHAR);
                int dp = 0; // destination position
                i = 0;
                while (i < sz) {
                    if (i > 0) { arr_set(r, dp, 32); dp++; } // space
                    int val = arr_get(v, i);
                    int w = num_width(val);
                    int n_abs = val;
                    if (val < 0) { n_abs = 0 - val; }
                    // Fill this number right-to-left within its field
                    int ep = dp + w - 1; // end position
                    if (n_abs == 0) {
                        arr_set(r, ep, 48);
                    } else {
                        int p = ep;
                        while (n_abs > 0) {
                            arr_set(r, p, 48 + n_abs % 10);
                            n_abs = n_abs / 10;
                            p--;
                        }
                    }
                    if (val < 0) { arr_set(r, dp, 95); }
                    dp = dp + w;
                    i++;
                }
                return r;
            }
            eval_err = 4;
            return -1;
        }

        if (res_id == RES_CUP) {
            // Monadic cup (unique): remove duplicates, keep first occurrence
            if (arr_rank(v) > 1) { eval_err = 4; return -1; }
            int sz = arr_size(v);
            if (sz == 0) return v;
            // First pass: count unique elements
            int uniq = 0;
            int j;
            int i = 0;
            while (i < sz) {
                int val = arr_get(v, i);
                int dup = 0;
                j = 0;
                while (j < i) {
                    if (arr_get(v, j) == val) { dup = 1; j = i; }
                    j++;
                }
                if (!dup) uniq++;
                i++;
            }
            int r = arr_vector(uniq);
            if (r < 0) { eval_err = 5; return -1; }
            if (arr_type(v) == ARR_CHAR) arr_set_type(r, ARR_CHAR);
            int ri = 0;
            i = 0;
            while (i < sz) {
                int val = arr_get(v, i);
                int dup = 0;
                j = 0;
                while (j < i) {
                    if (arr_get(v, j) == val) { dup = 1; j = i; }
                    j++;
                }
                if (!dup) { arr_set(r, ri, val); ri++; }
                i++;
            }
            return r;
        }

        // Unknown monadic function
        eval_err = 1;
        return -1;
    }

    if (ty == NODE_REDUCE) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        int op = node_val[n];
        int rk = arr_rank(v);

        // Scalar reduce: just return the scalar
        if (rk == 0) {
            return v;
        }

        // Vector reduce: right-to-left fold
        if (rk == 1) {
            int sz = arr_size(v);
            if (sz == 0) {
                // Empty vector: return identity element
                int id;
                if (op == TOK_PLUS || op == TOK_MINUS || op == TOK_OR_OP) id = 0;
                else if (op == TOK_STAR || op == TOK_SLASH || op == TOK_AND_OP) id = 1;
                else if (op == TOK_CEIL) id = -8388608;
                else if (op == TOK_FLOOR) id = 8388607;
                else { eval_err = 1; return -1; }
                int r = arr_scalar(id);
                if (r < 0) { eval_err = 5; return -1; }
                return r;
            }
            int acc = arr_get(v, sz - 1);
            int i = sz - 2;
            while (i >= 0) {
                acc = eval_binop_scalar(op, arr_get(v, i), acc);
                if (eval_err) return -1;
                i--;
            }
            int r = arr_scalar(acc);
            if (r < 0) { eval_err = 5; return -1; }
            return r;
        }

        // Unsupported rank for reduce
        eval_err = 4;
        return -1;
    }

    if (ty == NODE_DYAD) {
        int lv = eval(node_left[n]);
        if (eval_err) return -1;
        int rv = eval(node_right[n]);
        if (eval_err) return -1;
        int res_id = node_val[n];

        if (res_id == RES_RHO) {
            // S rho A: reshape A to shape S with cyclic fill
            // S is scalar -> result is vector of that length
            // S is 2-element vector -> result is matrix
            int lrk = arr_rank(lv);
            int new_rank;
            int d0;
            int d1;

            if (lrk == 0) {
                // Scalar shape -> vector
                new_rank = 1;
                d0 = arr_get(lv, 0);
                d1 = 0;
                if (d0 < 0) { eval_err = 1; return -1; }
            } else if (lrk == 1) {
                int lsz = arr_size(lv);
                if (lsz == 1) {
                    new_rank = 1;
                    d0 = arr_get(lv, 0);
                    d1 = 0;
                    if (d0 < 0) { eval_err = 1; return -1; }
                } else if (lsz == 2) {
                    new_rank = 2;
                    d0 = arr_get(lv, 0);
                    d1 = arr_get(lv, 1);
                    if (d0 < 0 || d1 < 0) { eval_err = 1; return -1; }
                } else {
                    // Only rank 1 and 2 supported
                    eval_err = 4;
                    return -1;
                }
            } else {
                eval_err = 4;
                return -1;
            }

            int r = arr_new(new_rank, d0, d1);
            if (r < 0) { eval_err = 5; return -1; }

            int total = arr_size(r);
            int src_sz = arr_size(rv);
            if (src_sz == 0) { eval_err = 3; return -1; }

            // Preserve type (char arrays stay char)
            arr_set_type(r, arr_type(rv));

            int i = 0;
            while (i < total) {
                arr_set(r, i, arr_get(rv, i % src_sz));
                i++;
            }
            return r;
        }

        if (res_id == RES_TAKE) {
            // N take A: take first N elements/rows (negative N = from end)
            if (arr_rank(lv) != 0) { eval_err = 4; return -1; }
            int count = arr_get(lv, 0);
            int rrk = arr_rank(rv);

            // Lazy iota shortcut: return new lazy iota for the slice
            if (rrk == 1 && arr_type(rv) == ARR_IOTA) {
                int sz = arr_size(rv);
                int abs_n = count;
                if (abs_n < 0) abs_n = 0 - abs_n;
                if (abs_n > sz) { eval_err = 3; return -1; }
                int origin = arr_dim1(rv);
                if (count < 0) origin = origin + sz - abs_n;
                return arr_iota(abs_n, origin);
            }

            if (rrk <= 1) {
                int sz = arr_size(rv);
                int abs_n = count;
                if (abs_n < 0) abs_n = 0 - abs_n;
                if (abs_n > sz) { eval_err = 3; return -1; }

                int r = arr_vector(abs_n);
                if (r < 0) { eval_err = 5; return -1; }
                int start = 0;
                if (count < 0) start = sz - abs_n;
                int i = 0;
                while (i < abs_n) {
                    arr_set(r, i, arr_get(rv, start + i));
                    i++;
                }
                return r;
            }

            if (rrk == 2) {
                int rows = arr_dim0(rv);
                int cols = arr_dim1(rv);
                int abs_n = count;
                if (abs_n < 0) abs_n = 0 - abs_n;
                if (abs_n > rows) { eval_err = 3; return -1; }

                int r = arr_new(2, abs_n, cols);
                if (r < 0) { eval_err = 5; return -1; }
                int start_row = 0;
                if (count < 0) start_row = rows - abs_n;
                int total = abs_n * cols;
                int i = 0;
                while (i < total) {
                    arr_set(r, i, arr_get(rv, start_row * cols + i));
                    i++;
                }
                return r;
            }

            eval_err = 4;
            return -1;
        }

        if (res_id == RES_DROP) {
            // N drop A: drop first N elements/rows (negative N = from end)
            if (arr_rank(lv) != 0) { eval_err = 4; return -1; }
            int count = arr_get(lv, 0);
            int rrk = arr_rank(rv);

            // Lazy iota shortcut: return new lazy iota for the remainder
            if (rrk == 1 && arr_type(rv) == ARR_IOTA) {
                int sz = arr_size(rv);
                int abs_n = count;
                if (abs_n < 0) abs_n = 0 - abs_n;
                if (abs_n > sz) abs_n = sz;
                int new_sz = sz - abs_n;
                int origin = arr_dim1(rv);
                if (count >= 0) origin = origin + abs_n;
                return arr_iota(new_sz, origin);
            }

            if (rrk <= 1) {
                int sz = arr_size(rv);
                int abs_n = count;
                if (abs_n < 0) abs_n = 0 - abs_n;
                if (abs_n > sz) abs_n = sz;

                int new_sz = sz - abs_n;
                int r = arr_vector(new_sz);
                if (r < 0) { eval_err = 5; return -1; }
                int start = abs_n;
                if (count < 0) start = 0;
                int i = 0;
                while (i < new_sz) {
                    arr_set(r, i, arr_get(rv, start + i));
                    i++;
                }
                return r;
            }

            if (rrk == 2) {
                int rows = arr_dim0(rv);
                int cols = arr_dim1(rv);
                int abs_n = count;
                if (abs_n < 0) abs_n = 0 - abs_n;
                if (abs_n > rows) abs_n = rows;

                int new_rows = rows - abs_n;
                int r = arr_new(2, new_rows, cols);
                if (r < 0) { eval_err = 5; return -1; }
                int start_row = abs_n;
                if (count < 0) start_row = 0;
                int total = new_rows * cols;
                int i = 0;
                while (i < total) {
                    arr_set(r, i, arr_get(rv, start_row * cols + i));
                    i++;
                }
                return r;
            }

            eval_err = 4;
            return -1;
        }

        if (res_id == RES_CAT) {
            // Dyadic cat: catenate two arrays
            int lrk = arr_rank(lv);
            int rrk = arr_rank(rv);
            int lsz = arr_size(lv);
            int rsz = arr_size(rv);
            int total = lsz + rsz;
            int r = arr_vector(total);
            if (r < 0) { eval_err = 5; return -1; }
            // Preserve char type if both operands are char
            if (arr_type(lv) == ARR_CHAR && arr_type(rv) == ARR_CHAR) {
                arr_set_type(r, ARR_CHAR);
            }
            int i = 0;
            while (i < lsz) {
                arr_set(r, i, arr_get(lv, i));
                i++;
            }
            i = 0;
            while (i < rsz) {
                arr_set(r, lsz + i, arr_get(rv, i));
                i++;
            }
            return r;
        }

        if (res_id == RES_AND || res_id == RES_OR) {
            // Bitwise AND / OR on conformable arrays
            int lrk = arr_rank(lv);
            int rrk = arr_rank(rv);

            // Scalar op scalar
            if (lrk == 0 && rrk == 0) {
                int a = arr_get(lv, 0);
                int b = arr_get(rv, 0);
                int val = (res_id == RES_AND) ? (a & b) : (a | b);
                int r = arr_scalar(val);
                if (r < 0) { eval_err = 5; return -1; }
                return r;
            }

            // Vector op vector
            if (lrk == 1 && rrk == 1) {
                int lsz = arr_size(lv);
                int rsz = arr_size(rv);
                if (lsz != rsz) { eval_err = 3; return -1; }
                int r = arr_vector(lsz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < lsz) {
                    int a = arr_get(lv, i);
                    int b = arr_get(rv, i);
                    arr_set(r, i, (res_id == RES_AND) ? (a & b) : (a | b));
                    i++;
                }
                return r;
            }

            // Scalar extension: scalar op vector
            if (lrk == 0 && rrk == 1) {
                int a = arr_get(lv, 0);
                int rsz = arr_size(rv);
                int r = arr_vector(rsz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < rsz) {
                    int b = arr_get(rv, i);
                    arr_set(r, i, (res_id == RES_AND) ? (a & b) : (a | b));
                    i++;
                }
                return r;
            }

            // Scalar extension: vector op scalar
            if (lrk == 1 && rrk == 0) {
                int b = arr_get(rv, 0);
                int lsz = arr_size(lv);
                int r = arr_vector(lsz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < lsz) {
                    int a = arr_get(lv, i);
                    arr_set(r, i, (res_id == RES_AND) ? (a & b) : (a | b));
                    i++;
                }
                return r;
            }

            eval_err = 4;
            return -1;
        }

        if (res_id == RES_CEIL || res_id == RES_FLOOR) {
            // Dyadic ceil (max) / floor (min) on conformable arrays
            int op = (res_id == RES_CEIL) ? TOK_CEIL : TOK_FLOOR;
            int lrk = arr_rank(lv);
            int rrk = arr_rank(rv);

            // Scalar op scalar
            if (lrk == 0 && rrk == 0) {
                int val = eval_binop_scalar(op, arr_get(lv, 0), arr_get(rv, 0));
                if (eval_err) return -1;
                int r = arr_scalar(val);
                if (r < 0) { eval_err = 5; return -1; }
                return r;
            }

            // Vector op vector
            if (lrk == 1 && rrk == 1) {
                int lsz = arr_size(lv);
                int rsz = arr_size(rv);
                if (lsz != rsz) { eval_err = 3; return -1; }
                int r = arr_vector(lsz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < lsz) {
                    arr_set(r, i, eval_binop_scalar(op, arr_get(lv, i), arr_get(rv, i)));
                    if (eval_err) return -1;
                    i++;
                }
                return r;
            }

            // Scalar extension: scalar op vector
            if (lrk == 0 && rrk == 1) {
                int a = arr_get(lv, 0);
                int rsz = arr_size(rv);
                int r = arr_vector(rsz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < rsz) {
                    arr_set(r, i, eval_binop_scalar(op, a, arr_get(rv, i)));
                    if (eval_err) return -1;
                    i++;
                }
                return r;
            }

            // Scalar extension: vector op scalar
            if (lrk == 1 && rrk == 0) {
                int b = arr_get(rv, 0);
                int lsz = arr_size(lv);
                int r = arr_vector(lsz);
                if (r < 0) { eval_err = 5; return -1; }
                int i = 0;
                while (i < lsz) {
                    arr_set(r, i, eval_binop_scalar(op, arr_get(lv, i), b));
                    if (eval_err) return -1;
                    i++;
                }
                return r;
            }

            eval_err = 4;
            return -1;
        }

        if (res_id == RES_COMPRESS) {
            // Boolean compress: MASK compress VECTOR
            // Selects elements of VECTOR where MASK is 1
            int lrk = arr_rank(lv);
            int rrk = arr_rank(rv);

            // Left must be a vector (or scalar treated as 1-element)
            // Right must be a vector (or scalar treated as 1-element)
            if (lrk > 1 || rrk > 1) { eval_err = 4; return -1; }

            int lsz = arr_size(lv);
            int rsz = arr_size(rv);
            if (lsz != rsz) { eval_err = 3; return -1; }

            // Count number of 1s in mask
            int count = 0;
            int i = 0;
            while (i < lsz) {
                if (arr_get(lv, i)) count++;
                i++;
            }

            int r = arr_vector(count);
            if (r < 0) { eval_err = 5; return -1; }

            int ri = 0;
            i = 0;
            while (i < lsz) {
                if (arr_get(lv, i)) {
                    arr_set(r, ri, arr_get(rv, i));
                    ri++;
                }
                i++;
            }
            return r;
        }

        if (res_id == RES_PICK) {
            // I pick V: pick element I from nested/boxed vector V
            // Returns the boxed element (string or sub-array)
            // Also works on simple vectors, returning a scalar
            int lrk = arr_rank(lv);
            int rrk = arr_rank(rv);

            // Left must be scalar (the index)
            if (lrk > 1) { eval_err = 4; return -1; }
            // Right must be vector
            if (rrk > 1) { eval_err = 4; return -1; }

            int idx = arr_get(lv, 0);
            int rsz = arr_size(rv);

            // Bounds check
            if (idx < 0 || idx >= rsz) { eval_err = 3; return -1; }

            if (arr_type(rv) == ARR_BOXED) {
                // Return the boxed element (a heap index to another array)
                return arr_get(rv, idx);
            }
            // Simple vector: return scalar
            int r = arr_scalar(arr_get(rv, idx));
            if (r < 0) { eval_err = 5; return -1; }
            arr_set_type(r, arr_type(rv));
            return r;
        }

        if (res_id == RES_CUP) {
            // Dyadic cup (union): left unchanged, plus elements of right not in left
            if (arr_rank(lv) > 1 || arr_rank(rv) > 1) { eval_err = 4; return -1; }
            int lsz = arr_size(lv);
            int rsz = arr_size(rv);
            // Count elements in right not in left
            int extra = 0;
            int i = 0;
            int j;
            while (i < rsz) {
                int val = arr_get(rv, i);
                int found = 0;
                j = 0;
                while (j < lsz) {
                    if (arr_get(lv, j) == val) { found = 1; j = lsz; }
                    j++;
                }
                if (!found) extra++;
                i++;
            }
            int r = arr_vector(lsz + extra);
            if (r < 0) { eval_err = 5; return -1; }
            if (arr_type(lv) == ARR_CHAR) arr_set_type(r, ARR_CHAR);
            i = 0;
            while (i < lsz) { arr_set(r, i, arr_get(lv, i)); i++; }
            int ri = lsz;
            i = 0;
            while (i < rsz) {
                int val = arr_get(rv, i);
                int found = 0;
                j = 0;
                while (j < lsz) {
                    if (arr_get(lv, j) == val) { found = 1; j = lsz; }
                    j++;
                }
                if (!found) { arr_set(r, ri, val); ri++; }
                i++;
            }
            return r;
        }

        if (res_id == RES_CAP) {
            // Dyadic cap (intersection): elements of left that appear in right
            if (arr_rank(lv) > 1 || arr_rank(rv) > 1) { eval_err = 4; return -1; }
            int lsz = arr_size(lv);
            int rsz = arr_size(rv);
            // Count matches
            int count = 0;
            int i = 0;
            int j;
            while (i < lsz) {
                int val = arr_get(lv, i);
                j = 0;
                while (j < rsz) {
                    if (arr_get(rv, j) == val) { count++; j = rsz; }
                    j++;
                }
                i++;
            }
            int r = arr_vector(count);
            if (r < 0) { eval_err = 5; return -1; }
            if (arr_type(lv) == ARR_CHAR) arr_set_type(r, ARR_CHAR);
            int ri = 0;
            i = 0;
            while (i < lsz) {
                int val = arr_get(lv, i);
                j = 0;
                while (j < rsz) {
                    if (arr_get(rv, j) == val) { arr_set(r, ri, val); ri++; j = rsz; }
                    j++;
                }
                i++;
            }
            return r;
        }

        // Unknown dyadic function
        eval_err = 1;
        return -1;
    }

    if (ty == NODE_BINOP) {
        int lv = eval(node_left[n]);
        if (eval_err) return -1;
        int rv = eval(node_right[n]);
        if (eval_err) return -1;

        int lrk = arr_rank(lv);
        int rrk = arr_rank(rv);
        int op = node_val[n];

        // Scalar op scalar
        if (lrk == 0 && rrk == 0) {
            int result = eval_binop_scalar(op, arr_get(lv, 0), arr_get(rv, 0));
            if (eval_err) return -1;
            int r = arr_scalar(result);
            if (r < 0) { eval_err = 5; return -1; }
            return r;
        }

        // Vector op vector: must match length
        if (lrk == 1 && rrk == 1) {
            int lsz = arr_size(lv);
            int rsz = arr_size(rv);
            if (lsz != rsz) { eval_err = 3; return -1; }
            int r = arr_vector(lsz);
            if (r < 0) { eval_err = 5; return -1; }
            int i = 0;
            while (i < lsz) {
                int val = eval_binop_scalar(op, arr_get(lv, i), arr_get(rv, i));
                if (eval_err) return -1;
                arr_set(r, i, val);
                i++;
            }
            return r;
        }

        // Scalar op vector
        if (lrk == 0 && rrk == 1) {
            int la = arr_get(lv, 0);
            int rsz = arr_size(rv);
            int r = arr_vector(rsz);
            if (r < 0) { eval_err = 5; return -1; }
            int i = 0;
            while (i < rsz) {
                int val = eval_binop_scalar(op, la, arr_get(rv, i));
                if (eval_err) return -1;
                arr_set(r, i, val);
                i++;
            }
            return r;
        }

        // Vector op scalar
        if (lrk == 1 && rrk == 0) {
            int ra = arr_get(rv, 0);
            int lsz = arr_size(lv);
            int r = arr_vector(lsz);
            if (r < 0) { eval_err = 5; return -1; }
            int i = 0;
            while (i < lsz) {
                int val = eval_binop_scalar(op, arr_get(lv, i), ra);
                if (eval_err) return -1;
                arr_set(r, i, val);
                i++;
            }
            return r;
        }

        // Matrix op matrix: must match shape
        if (lrk == 2 && rrk == 2) {
            if (arr_dim0(lv) != arr_dim0(rv) || arr_dim1(lv) != arr_dim1(rv)) {
                eval_err = 3; return -1;
            }
            int d0 = arr_dim0(lv);
            int d1 = arr_dim1(lv);
            int r = arr_new(2, d0, d1);
            if (r < 0) { eval_err = 5; return -1; }
            int sz = d0 * d1;
            int i = 0;
            while (i < sz) {
                int val = eval_binop_scalar(op, arr_get(lv, i), arr_get(rv, i));
                if (eval_err) return -1;
                arr_set(r, i, val);
                i++;
            }
            return r;
        }

        // Scalar op matrix
        if (lrk == 0 && rrk == 2) {
            int la = arr_get(lv, 0);
            int d0 = arr_dim0(rv);
            int d1 = arr_dim1(rv);
            int r = arr_new(2, d0, d1);
            if (r < 0) { eval_err = 5; return -1; }
            int sz = d0 * d1;
            int i = 0;
            while (i < sz) {
                int val = eval_binop_scalar(op, la, arr_get(rv, i));
                if (eval_err) return -1;
                arr_set(r, i, val);
                i++;
            }
            return r;
        }

        // Matrix op scalar
        if (lrk == 2 && rrk == 0) {
            int ra = arr_get(rv, 0);
            int d0 = arr_dim0(lv);
            int d1 = arr_dim1(lv);
            int r = arr_new(2, d0, d1);
            if (r < 0) { eval_err = 5; return -1; }
            int sz = d0 * d1;
            int i = 0;
            while (i < sz) {
                int val = eval_binop_scalar(op, arr_get(lv, i), ra);
                if (eval_err) return -1;
                arr_set(r, i, val);
                i++;
            }
            return r;
        }

        // Unsupported rank combination
        eval_err = 4;
        return -1;
    }

    eval_err = 1;
    return -1;
}
