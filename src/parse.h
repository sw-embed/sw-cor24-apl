// COR24 APL Interpreter -- Parser
// Right-to-left recursive descent with vector literal stranding.
// APL: all operators equal precedence, right-associative.
// 2 + 3 * 4  ->  2 + (3 * 4)  ->  14
// 1 2 3      ->  3-element vector (adjacent numbers strand)

#pragma once

// AST node types
#define NODE_NUM    0   // integer literal (val = value)
#define NODE_BINOP  1   // binary op (val = operator tok type)
#define NODE_NEG    2   // monadic negate (right = operand)
#define NODE_IDENT  3   // variable reference (val = symbol index)
#define NODE_ASSIGN 4   // assignment (val = symbol index, right = expr)
#define NODE_VEC    5   // vector literal (val = heap array index)
#define NODE_MONAD  6   // monadic primitive (val = RES_xxx, right = operand)
#define NODE_DYAD   7   // dyadic primitive (val = RES_xxx, left/right = args)
#define NODE_REDUCE 8   // reduce operator (val = op tok type, right = operand)
#define NODE_QOUT   9   // quad output (right = expr to print)
#define NODE_QSVO  13   // shared variable offer (val = sym index, right = AP expr)
#define NODE_SVO_READ  14   // shared var indexed read (val = sym index, right = index expr)
#define NODE_SVO_WRITE 15  // shared var indexed write (val = sym index, left = index, right = value)
#define NODE_GOTO  16   // unconditional branch (right = target expr)
#define NODE_CGOTO 17   // conditional branch (left = cond, right = target)
#define NODE_FNCALL 18  // user function call (val = fn index, left = left arg/-1, right = right arg)
#define NODE_QRL   19   // quad-seed read (no children — returns PRNG seed)
#define NODE_QRL_ASSIGN 20  // quad-seed assign expr (right = expr — sets PRNG seed)
#define NODE_QIO   22   // quad-origin read (no children — returns index origin)
#define NODE_QIO_ASSIGN 23  // quad-origin assign expr (right = expr — sets index origin)
#define NODE_SCAN  24   // scan operator (val = op tok type, right = operand)
#define NODE_OUTER 25   // outer product (val = op tok type, left/right = args)
#define NODE_INNER 26   // inner product (val = f_op | (g_op<<8), left/right = args)

#define AST_MAX 64

// AST storage (parallel arrays)
int node_type[AST_MAX];
int node_val[AST_MAX];
int node_left[AST_MAX];
int node_right[AST_MAX];
int node_count;

// Parser state
int parse_pos;
int parse_err;
char *parse_line;   // source line for identifier lookup

// Allocate a new AST node
int ast_new() {
    if (node_count >= AST_MAX) {
        parse_err = 1;
        return 0;
    }
    int n = node_count;
    node_count++;
    node_left[n] = -1;
    node_right[n] = -1;
    return n;
}

int ast_num(int val) {
    int n = ast_new();
    node_type[n] = NODE_NUM;
    node_val[n] = val;
    return n;
}

int ast_binop(int op, int left, int right) {
    int n = ast_new();
    node_type[n] = NODE_BINOP;
    node_val[n] = op;
    node_left[n] = left;
    node_right[n] = right;
    return n;
}

int ast_neg(int operand) {
    int n = ast_new();
    node_type[n] = NODE_NEG;
    node_right[n] = operand;
    return n;
}

int ast_ident(int sym_idx) {
    int n = ast_new();
    node_type[n] = NODE_IDENT;
    node_val[n] = sym_idx;
    return n;
}

int ast_assign(int sym_idx, int expr) {
    int n = ast_new();
    node_type[n] = NODE_ASSIGN;
    node_val[n] = sym_idx;
    node_right[n] = expr;
    return n;
}

int ast_vec(int heap_idx) {
    int n = ast_new();
    node_type[n] = NODE_VEC;
    node_val[n] = heap_idx;
    return n;
}

int ast_monad(int res_id, int operand) {
    int n = ast_new();
    node_type[n] = NODE_MONAD;
    node_val[n] = res_id;
    node_right[n] = operand;
    return n;
}

int ast_dyad(int res_id, int left, int right) {
    int n = ast_new();
    node_type[n] = NODE_DYAD;
    node_val[n] = res_id;
    node_left[n] = left;
    node_right[n] = right;
    return n;
}

int ast_reduce(int op, int operand) {
    int n = ast_new();
    node_type[n] = NODE_REDUCE;
    node_val[n] = op;
    node_right[n] = operand;
    return n;
}

int ast_scan(int op, int operand) {
    int n = ast_new();
    node_type[n] = NODE_SCAN;
    node_val[n] = op;
    node_right[n] = operand;
    return n;
}

// Check if a reserved word can be used dyadically
int is_dyadic_res(int res_id) {
    return res_id == RES_RHO || res_id == RES_TAKE || res_id == RES_DROP || res_id == RES_CAT || res_id == RES_AND || res_id == RES_OR || res_id == RES_CEIL || res_id == RES_FLOOR || res_id == RES_COMPRESS || res_id == RES_PICK || res_id == RES_CUP || res_id == RES_CAP || res_id == RES_RESIDUE || res_id == RES_BINOMIAL || res_id == RES_IOTA || res_id == RES_MEMBER || res_id == RES_WITHOUT || res_id == RES_REV || res_id == RES_ENCODE || res_id == RES_DECODE || res_id == RES_POWER;
}

// Temporary buffer for collecting strand elements
int strand_buf[64];

int is_binop(int ty) {
    return ty == TOK_PLUS || ty == TOK_MINUS || ty == TOK_STAR || ty == TOK_SLASH ||
           ty == TOK_EQ || ty == TOK_NE || ty == TOK_LT || ty == TOK_GT ||
           ty == TOK_LE || ty == TOK_GE;
}

// Check if token at pos could start an expression (for function call detection)
int can_start_expr(int pos) {
    int ty = tok_type[pos];
    if (ty == TOK_NUM || ty == TOK_LPAREN || ty == TOK_IDENT ||
        ty == TOK_MINUS || ty == TOK_RES ||
        ty == TOK_QRL || ty == TOK_QIO || ty == TOK_STRING) return 1;
    if (is_binop(ty) && tok_type[pos + 1] == TOK_SLASH) return 1;  // reduce
    if (is_binop(ty) && tok_type[pos + 1] == TOK_BSLASH) return 1;  // scan
    return 0;
}

// Combined parse function avoids mutual recursion.
// mode 0 = expression, mode 1 = value only
int parse_node(int mode);

int parse_node(int mode) {
    if (parse_err) return 0;

    int left;
    int ty = tok_type[parse_pos];

    // -- Parse a value --

    if (ty == TOK_NUM) {
        left = ast_num(tok_val[parse_pos]);
        parse_pos++;

        // Stranding: adjacent numbers form a vector (e.g. 1 2 3)
        if (tok_type[parse_pos] == TOK_NUM) {
            strand_buf[0] = node_val[left];
            int count = 1;
            while (tok_type[parse_pos] == TOK_NUM && count < 64) {
                strand_buf[count] = tok_val[parse_pos];
                parse_pos++;
                count++;
            }
            int aidx = arr_vector(count);
            if (aidx < 0) { parse_err = 1; return 0; }
            int si = 0;
            while (si < count) {
                arr_set(aidx, si, strand_buf[si]);
                si++;
            }
            left = ast_vec(aidx);
        }
    } else if (ty == TOK_LPAREN) {
        parse_pos++;
        left = parse_node(0);
        if (parse_err) return 0;
        if (tok_type[parse_pos] != TOK_RPAREN) {
            parse_err = 1;
            return 0;
        }
        parse_pos++;
    } else if (ty == TOK_QRL) {
        int n = ast_new();
        node_type[n] = NODE_QRL;
        left = n;
        parse_pos++;
    } else if (ty == TOK_QIO) {
        int n = ast_new();
        node_type[n] = NODE_QIO;
        left = n;
        parse_pos++;
    } else if (ty == TOK_STRING) {
        if (tok_type[parse_pos + 1] == TOK_QSVO) {
            // String as identifier for qsvo coupling: 'NAME' qsvo AP
            int sym_idx = sym_lookup(parse_line, tok_val[parse_pos]);
            if (sym_idx < 0) { parse_err = 1; return 0; }
            left = ast_ident(sym_idx);
            parse_pos++;
        } else {
            // Character vector literal: 'hello' -> char vector
            int start = tok_val[parse_pos];
            int end = start;
            while (parse_line[end] && parse_line[end] != 39) end++;
            int len = end - start;
            int aidx = arr_vector(len);
            if (aidx < 0) { parse_err = 1; return 0; }
            arr_set_type(aidx, ARR_CHAR);
            int ci = 0;
            while (ci < len) {
                arr_set(aidx, ci, parse_line[start + ci]);
                ci++;
            }
            parse_pos++;

            // String stranding: adjacent strings form a boxed vector
            if (tok_type[parse_pos] == TOK_STRING) {
                strand_buf[0] = aidx;
                int count = 1;
                while (tok_type[parse_pos] == TOK_STRING && count < 64) {
                    int s2 = tok_val[parse_pos];
                    int e2 = s2;
                    while (parse_line[e2] && parse_line[e2] != 39) e2++;
                    int l2 = e2 - s2;
                    int a2 = arr_vector(l2);
                    if (a2 < 0) { parse_err = 1; return 0; }
                    arr_set_type(a2, ARR_CHAR);
                    int k = 0;
                    while (k < l2) {
                        arr_set(a2, k, parse_line[s2 + k]);
                        k++;
                    }
                    strand_buf[count] = a2;
                    parse_pos++;
                    count++;
                }
                // Create boxed vector holding heap indices
                int bidx = arr_vector(count);
                if (bidx < 0) { parse_err = 1; return 0; }
                arr_set_type(bidx, ARR_BOXED);
                int bi = 0;
                while (bi < count) {
                    arr_set(bidx, bi, strand_buf[bi]);
                    bi++;
                }
                left = ast_vec(bidx);
            } else {
                left = ast_vec(aidx);
            }
        }
    } else if (ty == TOK_IDENT) {
        int sym_idx = sym_lookup(parse_line, tok_val[parse_pos]);
        if (sym_idx < 0) { parse_err = 1; return 0; }
        parse_pos++;
        // Check for function call (niladic or monadic)
        int fi = fn_lookup(sym_idx);
        if (fi >= 0 && fn_left_sym[fi] < 0 && fn_right_sym[fi] < 0) {
            // Niladic user function call: FN (no arguments)
            int nd = ast_new();
            node_type[nd] = NODE_FNCALL;
            node_val[nd] = fi;
            node_right[nd] = -1;
            node_left[nd] = -1;
            left = nd;
        } else if (fi >= 0 && fn_left_sym[fi] < 0 && can_start_expr(parse_pos)) {
            // Monadic user function call: FN expr
            int arg = parse_node(0);
            if (parse_err) return 0;
            int nd = ast_new();
            node_type[nd] = NODE_FNCALL;
            node_val[nd] = fi;
            node_right[nd] = arg;
            left = nd;
        } else if (tok_type[parse_pos] == TOK_LBRAK) {
            // IDENT[expr] -- shared variable indexed read
            parse_pos++;
            int idx_expr = parse_node(0);
            if (parse_err) return 0;
            if (tok_type[parse_pos] != TOK_RBRAK) { parse_err = 1; return 0; }
            parse_pos++;
            int nd = ast_new();
            node_type[nd] = NODE_SVO_READ;
            node_val[nd] = sym_idx;
            node_right[nd] = idx_expr;
            left = nd;
        } else {
            left = ast_ident(sym_idx);
        }
    } else if (is_binop(ty) && tok_type[parse_pos + 1] == TOK_SLASH) {
        // Reduce operator: +/ -/ */ (op followed by slash)
        int op = ty;
        parse_pos = parse_pos + 2;
        int operand = parse_node(0);
        if (parse_err) return 0;
        left = ast_reduce(op, operand);
    } else if (ty == TOK_RES && (tok_val[parse_pos] == RES_CEIL || tok_val[parse_pos] == RES_FLOOR || tok_val[parse_pos] == RES_AND || tok_val[parse_pos] == RES_OR) && tok_type[parse_pos + 1] == TOK_SLASH) {
        // Reduce with ceil/ floor/ and/ or/
        int rv = tok_val[parse_pos];
        int op = (rv == RES_CEIL) ? TOK_CEIL : (rv == RES_FLOOR) ? TOK_FLOOR : (rv == RES_AND) ? TOK_AND_OP : TOK_OR_OP;
        parse_pos = parse_pos + 2;
        int operand = parse_node(0);
        if (parse_err) return 0;
        left = ast_reduce(op, operand);
    } else if (is_binop(ty) && tok_type[parse_pos + 1] == TOK_BSLASH) {
        // Scan operator: +\ -\ *\ (op followed by backslash)
        int op = ty;
        parse_pos = parse_pos + 2;
        int operand = parse_node(0);
        if (parse_err) return 0;
        left = ast_scan(op, operand);
    } else if (ty == TOK_RES && (tok_val[parse_pos] == RES_CEIL || tok_val[parse_pos] == RES_FLOOR || tok_val[parse_pos] == RES_AND || tok_val[parse_pos] == RES_OR) && tok_type[parse_pos + 1] == TOK_BSLASH) {
        // Scan with ceil, floor, and, or (backslash-op)
        int rv = tok_val[parse_pos];
        int op = TOK_OR_OP;
        if (rv == RES_CEIL) op = TOK_CEIL;
        if (rv == RES_FLOOR) op = TOK_FLOOR;
        if (rv == RES_AND) op = TOK_AND_OP;
        parse_pos = parse_pos + 2;
        int operand = parse_node(0);
        if (parse_err) return 0;
        left = ast_scan(op, operand);
    } else if (ty == TOK_RES) {
        // Monadic primitive function (e.g. iota expr)
        int res_id = tok_val[parse_pos];
        parse_pos++;
        int operand = parse_node(0);
        if (parse_err) return 0;
        left = ast_monad(res_id, operand);
    } else if (ty == TOK_MINUS) {
        // Monadic negate: applies to entire right expression (APL rule)
        parse_pos++;
        int operand = parse_node(0);
        if (parse_err) return 0;
        left = ast_neg(operand);
    } else {
        parse_err = 1;
        return 0;
    }

    // -- If mode 0 (expr) and next is binop or dyadic reserved, parse right --

    // Inner product: left f.g right (e.g. +.*)
    // Inner product: left f.g right (e.g. +.*)
    // Also handles reserved word f-ops: or.= and.= ceil.+ etc.
    if (mode == 0 && tok_type[parse_pos + 1] == TOK_DOT) {
        int f_op = -1;
        if (is_binop(tok_type[parse_pos])) {
            f_op = tok_type[parse_pos];
        } else if (tok_type[parse_pos] == TOK_RES) {
            int rv = tok_val[parse_pos];
            if (rv == RES_CEIL) f_op = TOK_CEIL;
            if (rv == RES_FLOOR) f_op = TOK_FLOOR;
            if (rv == RES_AND) f_op = TOK_AND_OP;
            if (rv == RES_OR) f_op = TOK_OR_OP;
        }
        int g_op = -1;
        if (f_op >= 0 && is_binop(tok_type[parse_pos + 2])) {
            g_op = tok_type[parse_pos + 2];
        }
        if (f_op >= 0 && g_op >= 0) {
            parse_pos = parse_pos + 3;
            int right = parse_node(0);
            if (parse_err) return 0;
            int nd = ast_new();
            node_type[nd] = NODE_INNER;
            node_val[nd] = f_op + (g_op * 256);
            node_left[nd] = left;
            node_right[nd] = right;
            return nd;
        }
    }

    if (mode == 0 && is_binop(tok_type[parse_pos])) {
        int op = tok_type[parse_pos];
        parse_pos++;
        int right = parse_node(0);
        if (parse_err) return 0;
        return ast_binop(op, left, right);
    }

    // Outer product: left outer.OP right
    if (mode == 0 && tok_type[parse_pos] == TOK_OUTER && tok_type[parse_pos + 1] == TOK_DOT && is_binop(tok_type[parse_pos + 2])) {
        int op = tok_type[parse_pos + 2];
        parse_pos = parse_pos + 3;
        int right = parse_node(0);
        if (parse_err) return 0;
        int n = ast_new();
        node_type[n] = NODE_OUTER;
        node_val[n] = op;
        node_left[n] = left;
        node_right[n] = right;
        return n;
    }

    // Outer product with reserved word ops: outer.ceil, outer.floor, outer.and, outer.or
    if (mode == 0 && tok_type[parse_pos] == TOK_OUTER && tok_type[parse_pos + 1] == TOK_DOT && tok_type[parse_pos + 2] == TOK_RES) {
        int rv = tok_val[parse_pos + 2];
        int op = -1;
        if (rv == RES_CEIL) op = TOK_CEIL;
        if (rv == RES_FLOOR) op = TOK_FLOOR;
        if (rv == RES_AND) op = TOK_AND_OP;
        if (rv == RES_OR) op = TOK_OR_OP;
        if (op >= 0) {
            parse_pos = parse_pos + 3;
            int right = parse_node(0);
            if (parse_err) return 0;
            int n = ast_new();
            node_type[n] = NODE_OUTER;
            node_val[n] = op;
            node_left[n] = left;
            node_right[n] = right;
            return n;
        }
    }

    if (mode == 0 && tok_type[parse_pos] == TOK_RES && is_dyadic_res(tok_val[parse_pos])) {
        int res_id = tok_val[parse_pos];
        parse_pos++;
        int right = parse_node(0);
        if (parse_err) return 0;
        return ast_dyad(res_id, left, right);
    }

    // Shared variable offer: 'NAME' qsvo EXPR  (or legacy: IDENT qsvo EXPR)
    if (mode == 0 && tok_type[parse_pos] == TOK_QSVO && node_type[left] == NODE_IDENT) {
        int sym_idx = node_val[left];
        parse_pos++;
        int right = parse_node(0);
        if (parse_err) return 0;
        int n = ast_new();
        node_type[n] = NODE_QSVO;
        node_val[n] = sym_idx;
        node_right[n] = right;
        return n;
    }

    // Dyadic user function call: expr FN expr
    if (mode == 0 && tok_type[parse_pos] == TOK_IDENT) {
        int dsym = sym_lookup(parse_line, tok_val[parse_pos]);
        if (dsym >= 0) {
            int dfi = fn_lookup(dsym);
            if (dfi >= 0 && fn_left_sym[dfi] >= 0) {
                parse_pos++;
                int right = parse_node(0);
                if (parse_err) return 0;
                int nd = ast_new();
                node_type[nd] = NODE_FNCALL;
                node_val[nd] = dfi;
                node_left[nd] = left;
                node_right[nd] = right;
                return nd;
            }
        }
    }

    return left;
}

// Parse the token stream. Returns root node index, or -1 on error.
// line is needed for identifier name resolution into the symbol table.
int parse(char *line) {
    node_count = 0;
    parse_pos = 0;
    parse_err = 0;
    parse_line = line;

    // Check for quad-seed assignment: quad-seed assign expr (set PRNG seed)
    if (tok_type[0] == TOK_QRL && tok_type[1] == TOK_ASSIGN) {
        parse_pos = 2;
        int expr = parse_node(0);
        if (parse_err) return -1;
        if (tok_type[parse_pos] != TOK_EOL) return -1;
        int n = ast_new();
        node_type[n] = NODE_QRL_ASSIGN;
        node_right[n] = expr;
        return n;
    }

    // Check for quad-origin assignment: quad-origin assign expr
    if (tok_type[0] == TOK_QIO && tok_type[1] == TOK_ASSIGN) {
        parse_pos = 2;
        int expr = parse_node(0);
        if (parse_err) return -1;
        if (tok_type[parse_pos] != TOK_EOL) return -1;
        int n = ast_new();
        node_type[n] = NODE_QIO_ASSIGN;
        node_right[n] = expr;
        return n;
    }

    // Check for quad output: quad assign expr (APL ⎕←)
    if (tok_type[0] == TOK_QUAD && tok_type[1] == TOK_ASSIGN) {
        parse_pos = 2;
        int expr = parse_node(0);
        if (parse_err) return -1;
        if (tok_type[parse_pos] != TOK_EOL) return -1;
        int n = ast_new();
        node_type[n] = NODE_QOUT;
        node_right[n] = expr;
        return n;
    }

    // Check for shared variable indexed write: IDENT[expr] assign expr
    if (tok_type[0] == TOK_IDENT && tok_type[1] == TOK_LBRAK) {
        int sym_idx = sym_lookup(line, tok_val[0]);
        if (sym_idx < 0) { return -1; }
        parse_pos = 2;
        int idx_expr = parse_node(0);
        if (parse_err) return -1;
        if (tok_type[parse_pos] != TOK_RBRAK) { parse_err = 1; return -1; }
        parse_pos++;
        if (tok_type[parse_pos] == TOK_ASSIGN) {
            // IDENT[expr] <- expr (indexed write)
            parse_pos++;
            int val_expr = parse_node(0);
            if (parse_err) return -1;
            if (tok_type[parse_pos] != TOK_EOL) return -1;
            int n = ast_new();
            node_type[n] = NODE_SVO_WRITE;
            node_val[n] = sym_idx;
            node_left[n] = idx_expr;
            node_right[n] = val_expr;
            return n;
        }
        if (tok_type[parse_pos] != TOK_EOL) {
            // IDENT[expr] followed by more (could be in expression context)
            // For now, only standalone read supported at top level
            // Fall through to expression parsing below
            parse_pos = 0;
            node_count = 0;
            parse_err = 0;
        } else {
            // IDENT[expr] standalone read
            int n = ast_new();
            node_type[n] = NODE_SVO_READ;
            node_val[n] = sym_idx;
            node_right[n] = idx_expr;
            return n;
        }
    }

    // Check for goto: goto expr  or  goto (expr)/LABEL
    if (tok_type[0] == TOK_GOTO) {
        parse_pos = 1;
        if (tok_type[1] == TOK_LPAREN) {
            // Conditional: goto (expr)/TARGET
            parse_pos = 2;
            int cond = parse_node(0);
            if (parse_err) return -1;
            if (tok_type[parse_pos] != TOK_RPAREN) return -1;
            parse_pos++;
            if (tok_type[parse_pos] != TOK_SLASH) return -1;
            parse_pos++;
            int target = parse_node(0);
            if (parse_err) return -1;
            if (tok_type[parse_pos] != TOK_EOL) return -1;
            int n = ast_new();
            node_type[n] = NODE_CGOTO;
            node_left[n] = cond;
            node_right[n] = target;
            return n;
        }
        // Unconditional: goto expr
        int target = parse_node(0);
        if (parse_err) return -1;
        if (tok_type[parse_pos] != TOK_EOL) return -1;
        int n = ast_new();
        node_type[n] = NODE_GOTO;
        node_right[n] = target;
        return n;
    }

    // Check for assignment: IDENT <- expr
    if (tok_type[0] == TOK_IDENT && tok_type[1] == TOK_ASSIGN) {
        int sym_idx = sym_lookup(line, tok_val[0]);
        if (sym_idx < 0) { return -1; }
        parse_pos = 2;
        int expr = parse_node(0);
        if (parse_err) return -1;
        if (tok_type[parse_pos] != TOK_EOL) return -1;
        return ast_assign(sym_idx, expr);
    }

    int root = parse_node(0);

    if (!parse_err && tok_type[parse_pos] != TOK_EOL) {
        parse_err = 1;
    }

    if (parse_err) return -1;
    return root;
}

// Debug: dump AST with full parenthesization
void ast_dump(int n) {
    if (n < 0 || n >= node_count) return;

    int ty = node_type[n];
    if (ty == NODE_NUM) {
        print_int(node_val[n]);
    } else if (ty == NODE_BINOP) {
        putchar(40);
        ast_dump(node_left[n]);
        putchar(32);
        if (node_val[n] == TOK_PLUS)  putchar(43);
        if (node_val[n] == TOK_MINUS) putchar(45);
        if (node_val[n] == TOK_STAR)  putchar(42);
        if (node_val[n] == TOK_SLASH) putchar(47);
        putchar(32);
        ast_dump(node_right[n]);
        putchar(41);
    } else if (ty == NODE_NEG) {
        putchar(40);
        putchar(45);
        ast_dump(node_right[n]);
        putchar(41);
    }
}
