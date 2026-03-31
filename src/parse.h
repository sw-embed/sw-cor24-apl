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
#define NODE_QLED  10   // qled read (no children)
#define NODE_QLED_ASSIGN 11  // qled <- expr (right = expr)
#define NODE_QSW   12   // qsw read (no children, read-only)
#define NODE_QSVO  13   // shared variable offer (val = sym index, right = AP expr)

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

// Check if a reserved word can be used dyadically
int is_dyadic_res(int res_id) {
    return res_id == RES_RHO || res_id == RES_TAKE || res_id == RES_DROP || res_id == RES_CAT;
}

// Temporary buffer for collecting strand elements
int strand_buf[64];

int is_binop(int ty) {
    return ty == TOK_PLUS || ty == TOK_MINUS || ty == TOK_STAR || ty == TOK_SLASH;
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
    } else if (ty == TOK_QLED) {
        int n = ast_new();
        node_type[n] = NODE_QLED;
        left = n;
        parse_pos++;
    } else if (ty == TOK_QSW) {
        int n = ast_new();
        node_type[n] = NODE_QSW;
        left = n;
        parse_pos++;
    } else if (ty == TOK_IDENT) {
        int sym_idx = sym_lookup(parse_line, tok_val[parse_pos]);
        if (sym_idx < 0) { parse_err = 1; return 0; }
        left = ast_ident(sym_idx);
        parse_pos++;
    } else if (is_binop(ty) && tok_type[parse_pos + 1] == TOK_SLASH) {
        // Reduce operator: +/ -/ */ (op followed by slash)
        int op = ty;
        parse_pos = parse_pos + 2;
        int operand = parse_node(0);
        if (parse_err) return 0;
        left = ast_reduce(op, operand);
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

    if (mode == 0 && is_binop(tok_type[parse_pos])) {
        int op = tok_type[parse_pos];
        parse_pos++;
        int right = parse_node(0);
        if (parse_err) return 0;
        return ast_binop(op, left, right);
    }

    if (mode == 0 && tok_type[parse_pos] == TOK_RES && is_dyadic_res(tok_val[parse_pos])) {
        int res_id = tok_val[parse_pos];
        parse_pos++;
        int right = parse_node(0);
        if (parse_err) return 0;
        return ast_dyad(res_id, left, right);
    }

    // Shared variable offer: IDENT qsvo EXPR
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

    return left;
}

// Parse the token stream. Returns root node index, or -1 on error.
// line is needed for identifier name resolution into the symbol table.
int parse(char *line) {
    node_count = 0;
    parse_pos = 0;
    parse_err = 0;
    parse_line = line;

    // Check for qled assignment: qled <- expr
    if (tok_type[0] == TOK_QLED && tok_type[1] == TOK_ASSIGN) {
        parse_pos = 2;
        int expr = parse_node(0);
        if (parse_err) return -1;
        if (tok_type[parse_pos] != TOK_EOL) return -1;
        int n = ast_new();
        node_type[n] = NODE_QLED_ASSIGN;
        node_right[n] = expr;
        return n;
    }

    // Check for qsw assignment: qsw is read-only, reject with SYNTAX ERROR
    if (tok_type[0] == TOK_QSW && tok_type[1] == TOK_ASSIGN) {
        return -1;
    }

    // Check for quad output: [] <- expr (IBM 5100 convention)
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
