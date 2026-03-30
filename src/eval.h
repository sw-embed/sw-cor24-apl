// COR24 APL Interpreter -- Evaluator
// Tree-walking evaluator for scalar expressions.
// COR24 has no hardware divide; tc24r emits software div/mod.

#pragma once

int eval_err;

// Evaluate AST node, return scalar result.
int eval(int n) {
    if (eval_err) return 0;
    if (n < 0 || n >= node_count) {
        eval_err = 1;
        return 0;
    }

    int ty = node_type[n];

    if (ty == NODE_NUM) {
        return node_val[n];
    }

    if (ty == NODE_IDENT) {
        int idx = node_val[n];
        if (!sym_set_flag[idx]) {
            eval_err = 2;  // VALUE ERROR
            return 0;
        }
        return sym_val[idx];
    }

    if (ty == NODE_ASSIGN) {
        int idx = node_val[n];
        int v = eval(node_right[n]);
        if (eval_err) return 0;
        sym_val[idx] = v;
        sym_set_flag[idx] = 1;
        return v;
    }

    if (ty == NODE_NEG) {
        int v = eval(node_right[n]);
        return 0 - v;
    }

    if (ty == NODE_BINOP) {
        int lv = eval(node_left[n]);
        int rv = eval(node_right[n]);
        if (eval_err) return 0;

        int op = node_val[n];
        if (op == TOK_PLUS)  return lv + rv;
        if (op == TOK_MINUS) return lv - rv;
        if (op == TOK_STAR)  return lv * rv;
        if (op == TOK_SLASH) {
            if (rv == 0) {
                eval_err = 1;
                return 0;
            }
            return lv / rv;
        }
    }

    eval_err = 1;
    return 0;
}
