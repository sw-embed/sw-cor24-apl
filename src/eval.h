// COR24 APL Interpreter -- Evaluator
// Tree-walking evaluator returning array heap indices.
// Every result is an array (scalars wrapped via arr_scalar).
// Supports element-wise ops on vectors with scalar extension.

#pragma once

// Error codes: 0=none, 1=DOMAIN, 2=VALUE, 3=LENGTH
int eval_err;

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
        if (r < 0) { eval_err = 1; return -1; }
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

    if (ty == NODE_NEG) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        int rk = arr_rank(v);
        if (rk == 0) {
            int r = arr_scalar(0 - arr_get(v, 0));
            if (r < 0) { eval_err = 1; return -1; }
            return r;
        }
        // Negate each element of vector
        int sz = arr_size(v);
        int r = arr_vector(sz);
        if (r < 0) { eval_err = 1; return -1; }
        int i = 0;
        while (i < sz) {
            arr_set(r, i, 0 - arr_get(v, i));
            i++;
        }
        return r;
    }

    if (ty == NODE_MONAD) {
        int v = eval(node_right[n]);
        if (eval_err) return -1;
        int res_id = node_val[n];

        if (res_id == RES_IOTA) {
            // iota N: generate vector 0 1 2 ... N-1
            if (arr_rank(v) != 0) { eval_err = 3; return -1; }
            int count = arr_get(v, 0);
            if (count < 0) { eval_err = 1; return -1; }
            int r = arr_vector(count);
            if (r < 0) { eval_err = 1; return -1; }
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
                if (r < 0) { eval_err = 1; return -1; }
                return r;
            }
            if (rk == 1) {
                // Vector: shape is 1-element vector with length
                int r = arr_scalar(arr_dim0(v));
                if (r < 0) { eval_err = 1; return -1; }
                return r;
            }
            if (rk == 2) {
                // Matrix: shape is 2-element vector (rows cols)
                int r = arr_vector(2);
                if (r < 0) { eval_err = 1; return -1; }
                arr_set(r, 0, arr_dim0(v));
                arr_set(r, 1, arr_dim1(v));
                return r;
            }
            eval_err = 1;
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
            if (sz == 0) { eval_err = 1; return -1; }
            int acc = arr_get(v, sz - 1);
            int i = sz - 2;
            while (i >= 0) {
                acc = eval_binop_scalar(op, arr_get(v, i), acc);
                if (eval_err) return -1;
                i--;
            }
            int r = arr_scalar(acc);
            if (r < 0) { eval_err = 1; return -1; }
            return r;
        }

        // Unsupported rank
        eval_err = 1;
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
                    eval_err = 1;
                    return -1;
                }
            } else {
                eval_err = 1;
                return -1;
            }

            int r = arr_new(new_rank, d0, d1);
            if (r < 0) { eval_err = 1; return -1; }

            int total = arr_size(r);
            int src_sz = arr_size(rv);
            if (src_sz == 0) { eval_err = 1; return -1; }

            int i = 0;
            while (i < total) {
                arr_set(r, i, arr_get(rv, i % src_sz));
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
            if (r < 0) { eval_err = 1; return -1; }
            return r;
        }

        // Vector op vector: must match length
        if (lrk == 1 && rrk == 1) {
            int lsz = arr_size(lv);
            int rsz = arr_size(rv);
            if (lsz != rsz) { eval_err = 3; return -1; }
            int r = arr_vector(lsz);
            if (r < 0) { eval_err = 1; return -1; }
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
            if (r < 0) { eval_err = 1; return -1; }
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
            if (r < 0) { eval_err = 1; return -1; }
            int i = 0;
            while (i < lsz) {
                int val = eval_binop_scalar(op, arr_get(lv, i), ra);
                if (eval_err) return -1;
                arr_set(r, i, val);
                i++;
            }
            return r;
        }

        // Unsupported rank combination
        eval_err = 1;
        return -1;
    }

    eval_err = 1;
    return -1;
}
