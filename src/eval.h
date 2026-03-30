// COR24 APL Interpreter -- Evaluator
// Tree-walking evaluator returning array heap indices.
// Every result is an array (scalars wrapped via arr_scalar).

#pragma once

int eval_err;

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
        if (arr_rank(v) != 0) {
            eval_err = 1;
            return -1;
        }
        int r = arr_scalar(0 - arr_get(v, 0));
        if (r < 0) { eval_err = 1; return -1; }
        return r;
    }

    if (ty == NODE_BINOP) {
        int lv = eval(node_left[n]);
        if (eval_err) return -1;
        int rv = eval(node_right[n]);
        if (eval_err) return -1;

        if (arr_rank(lv) != 0 || arr_rank(rv) != 0) {
            eval_err = 1;
            return -1;
        }

        int la = arr_get(lv, 0);
        int ra = arr_get(rv, 0);
        int op = node_val[n];
        int result;
        if (op == TOK_PLUS)       result = la + ra;
        else if (op == TOK_MINUS) result = la - ra;
        else if (op == TOK_STAR)  result = la * ra;
        else if (op == TOK_SLASH) {
            if (ra == 0) {
                eval_err = 1;
                return -1;
            }
            result = la / ra;
        } else {
            eval_err = 1;
            return -1;
        }
        int r = arr_scalar(result);
        if (r < 0) { eval_err = 1; return -1; }
        return r;
    }

    eval_err = 1;
    return -1;
}
