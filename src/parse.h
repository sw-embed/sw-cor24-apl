// COR24 APL Interpreter -- Parser
// Right-to-left recursive descent for scalar expressions.
// APL: all operators equal precedence, right-associative.
// 2 + 3 * 4  ->  2 + (3 * 4)  ->  14

#pragma once

// AST node types
#define NODE_NUM    0   // integer literal (val = value)
#define NODE_BINOP  1   // binary op (val = operator tok type)
#define NODE_NEG    2   // monadic negate (right = operand)
#define NODE_IDENT  3   // variable reference (val = symbol index)
#define NODE_ASSIGN 4   // assignment (val = symbol index, right = expr)

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
    } else if (ty == TOK_LPAREN) {
        parse_pos++;
        left = parse_node(0);
        if (parse_err) return 0;
        if (tok_type[parse_pos] != TOK_RPAREN) {
            parse_err = 1;
            return 0;
        }
        parse_pos++;
    } else if (ty == TOK_IDENT) {
        int sym_idx = sym_lookup(parse_line, tok_val[parse_pos]);
        if (sym_idx < 0) { parse_err = 1; return 0; }
        left = ast_ident(sym_idx);
        parse_pos++;
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

    // -- If mode 0 (expr) and next is binop, parse right side --

    if (mode == 0 && is_binop(tok_type[parse_pos])) {
        int op = tok_type[parse_pos];
        parse_pos++;
        int right = parse_node(0);
        if (parse_err) return 0;
        return ast_binop(op, left, right);
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
