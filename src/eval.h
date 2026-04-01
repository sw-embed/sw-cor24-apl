// COR24 APL Interpreter -- Evaluator
// Tree-walking evaluator returning array heap indices.
// Every result is an array (scalars wrapped via arr_scalar).
// Supports element-wise ops on vectors with scalar extension.

#pragma once

// Error codes: 0=none, 1=DOMAIN, 2=VALUE, 3=LENGTH, 4=RANK, 5=WS FULL
int eval_err;

// Shadow register for LED D2 (0xFF0000 is write-only for LEDs)
// Stores the user-visible value (1=on, 0=off), not the raw active-low bit
int qled_shadow;

// Apply a binary op to two scalars
int eval_binop_scalar(int op, int a, int b) {
    if (op == TOK_PLUS)       return a + b;
    if (op == TOK_MINUS)      return a - b;
    if (op == TOK_STAR)       return a * b;
    if (op == TOK_SLASH) {
        if (b == 0) { eval_err = 1; return 0; }
        return a / b;
    }
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
        sym_val[idx] = v;
        sym_set_flag[idx] = 1;
        return v;
    }

    if (ty == NODE_QOUT) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        return v;
    }

    if (ty == NODE_QLED) {
        // Read LED D2 from shadow register (1=on, 0=off)
        int r = arr_scalar(qled_shadow);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_QSW) {
        // Read switch S2: bit 0 of 0xFF0000, inverted (1=pressed, 0=released)
        int raw = *(char *)0xFF0000;
        int val = (raw ^ 1) & 1;
        int r = arr_scalar(val);
        if (r < 0) { eval_err = 5; return -1; }
        return r;
    }

    if (ty == NODE_QLED_ASSIGN) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        if (arr_rank(v) != 0) { eval_err = 4; return -1; }
        int val = arr_get(v, 0) & 1;
        qled_shadow = val;
        // Write LED D2: invert bit 0 (active-low: 1=on writes 0)
        *(char *)0xFF0000 = val ^ 1;
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
            // iota N: generate vector 0 1 2 ... N-1
            if (arr_rank(v) != 0) { eval_err = 4; return -1; }
            int count = arr_get(v, 0);
            if (count < 0) { eval_err = 1; return -1; }
            int r = arr_vector(count);
            if (r < 0) { eval_err = 5; return -1; }
            int i = 0;
            while (i < count) {
                arr_set(r, i, i);
                i++;
            }
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
                if (op == TOK_PLUS || op == TOK_MINUS) id = 0;
                else if (op == TOK_STAR || op == TOK_SLASH) id = 1;
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
