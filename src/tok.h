// COR24 APL Interpreter -- Tokenizer
// Lexes input line into token array for the parser.

#pragma once

// Token types
#define TOK_EOL     0   // end of line
#define TOK_NUM     1   // integer literal (value in tok_val)
#define TOK_IDENT   2   // user identifier (uppercase, pos in tok_val)
#define TOK_PLUS    3   // +
#define TOK_MINUS   4   // - (parser decides monadic vs dyadic)
#define TOK_STAR    5   // *
#define TOK_SLASH   6   // /
#define TOK_LPAREN  7   // (
#define TOK_RPAREN  8   // )
#define TOK_ASSIGN  9   // <-
#define TOK_RES    10   // reserved word (ID in tok_val)
#define TOK_QUAD   11   // quad ([] — bare quad for I/O)
#define TOK_QLED   12   // qled — LED D2 hardware I/O
#define TOK_QSW    13   // qsw — switch S2 hardware I/O (read-only)
#define TOK_QSVO   14   // qsvo — shared variable offer (□SVO)
#define TOK_LBRAK  15   // [ (bracket index open)
#define TOK_RBRAK  16   // ] (bracket index close)
#define TOK_GOTO   17   // goto — branch (→)
#define TOK_STRING 18   // string literal 'ABC' (pos of first char in tok_val)
#define TOK_EQ     19   // =
#define TOK_NE     20   // !=
#define TOK_LT     21   // <
#define TOK_GT     22   // >
#define TOK_LE     23   // <=
#define TOK_GE     24   // >=

// Reserved word IDs
#define RES_RHO     0
#define RES_IOTA    1
#define RES_TAKE    2
#define RES_DROP    3
#define RES_REV     4
#define RES_CAT     5
#define RES_AND     6
#define RES_OR      7
#define RES_NOT     8

#define TOK_MAX    64   // max tokens per line

// Token storage (parallel arrays)
int tok_type[TOK_MAX];  // token type
int tok_val[TOK_MAX];   // numeric value or reserved word ID
int tok_pos[TOK_MAX];   // start position in source line
int tok_count;           // number of tokens produced

// ---- String helpers ----

// Compare a substring of src starting at pos against a NUL-terminated word.
// Returns 1 if match, 0 otherwise.
int str_match(char *src, int pos, char *word) {
    int i = 0;
    while (word[i]) {
        if (src[pos + i] != word[i]) return 0;
        i++;
    }
    return i;  // return length of match (0 = no match since word is non-empty)
}

// Check if character is a lowercase letter (a-z)
int is_lower(int ch) {
    return ch >= 97 && ch <= 122;
}

// Check if character is an uppercase letter (A-Z)
int is_upper(int ch) {
    return ch >= 65 && ch <= 90;
}

// Check if character is a digit (0-9)
int is_digit(int ch) {
    return ch >= 48 && ch <= 57;
}

// Check if character is alphanumeric or underscore
int is_alnum(int ch) {
    return is_lower(ch) || is_upper(ch) || is_digit(ch);
}

// ---- Reserved word lookup ----
// Returns reserved word ID if the lowercase word starting at pos matches,
// or -1 if not a reserved word. Sets *end to position past the word.
int lookup_reserved(char *src, int pos, int *end) {
    int len;

    len = str_match(src, pos, "rho");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_RHO; }

    len = str_match(src, pos, "iota");
    if (len == 4 && !is_alnum(src[pos + 4])) { *end = pos + 4; return RES_IOTA; }

    len = str_match(src, pos, "take");
    if (len == 4 && !is_alnum(src[pos + 4])) { *end = pos + 4; return RES_TAKE; }

    len = str_match(src, pos, "drop");
    if (len == 4 && !is_alnum(src[pos + 4])) { *end = pos + 4; return RES_DROP; }

    len = str_match(src, pos, "rev");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_REV; }

    len = str_match(src, pos, "cat");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_CAT; }

    len = str_match(src, pos, "and");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_AND; }

    len = str_match(src, pos, "or");
    if (len == 2 && !is_alnum(src[pos + 2])) { *end = pos + 2; return RES_OR; }

    len = str_match(src, pos, "not");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_NOT; }

    return -1;
}

// ---- Tokenizer ----
// Tokenize input line into tok_type/tok_val/tok_pos arrays.
// Returns number of tokens (also stored in tok_count).
// On error, returns -1 (unrecognized character).
int tokenize(char *line) {
    int i = 0;
    int t = 0;

    while (line[i] && t < TOK_MAX - 1) {
        // Skip whitespace
        if (line[i] == 32) {
            i++;
            continue;
        }

        // Number: digits, or underscore followed by digit (APL negative)
        if (is_digit(line[i]) || (line[i] == 95 && is_digit(line[i + 1]))) {
            tok_type[t] = TOK_NUM;
            tok_pos[t] = i;
            int end;
            tok_val[t] = parse_int(line, i, &end);
            i = end;
            t++;
            continue;
        }

        // Lowercase letter: quad variables, reserved word, or unknown
        if (is_lower(line[i])) {
            // Check for quad variables first
            int len;
            len = str_match(line, i, "qled");
            if (len == 4 && !is_alnum(line[i + 4])) {
                tok_type[t] = TOK_QLED;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 4;
                continue;
            }

            len = str_match(line, i, "qsw");
            if (len == 3 && !is_alnum(line[i + 3])) {
                tok_type[t] = TOK_QSW;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 3;
                continue;
            }

            len = str_match(line, i, "qsvo");
            if (len == 4 && !is_alnum(line[i + 4])) {
                tok_type[t] = TOK_QSVO;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 4;
                continue;
            }

            len = str_match(line, i, "goto");
            if (len == 4 && !is_alnum(line[i + 4])) {
                tok_type[t] = TOK_GOTO;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 4;
                continue;
            }

            int end;
            int res = lookup_reserved(line, i, &end);
            if (res >= 0) {
                tok_type[t] = TOK_RES;
                tok_val[t] = res;
                tok_pos[t] = i;
                i = end;
                t++;
                continue;
            }
            // Unknown lowercase word -- error
            return -1;
        }

        // String literal: 'ABC' (single-quoted, for qsvo left argument)
        if (line[i] == 39) {
            // 39 = single quote
            i++;
            tok_type[t] = TOK_STRING;
            tok_pos[t] = i;  // position of first char
            tok_val[t] = i;  // store start of string content
            while (line[i] && line[i] != 39) i++;
            if (line[i] != 39) return -1;  // unterminated string
            i++;  // skip closing quote
            t++;
            continue;
        }

        // Uppercase letter: user identifier
        if (is_upper(line[i])) {
            tok_type[t] = TOK_IDENT;
            tok_pos[t] = i;
            tok_val[t] = i;  // store start position for name extraction
            i++;
            while (is_alnum(line[i])) i++;
            t++;
            continue;
        }

        // Operators and punctuation
        if (line[i] == 43) { tok_type[t] = TOK_PLUS;   tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 45) { tok_type[t] = TOK_MINUS;  tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 42) { tok_type[t] = TOK_STAR;   tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 47) { tok_type[t] = TOK_SLASH;  tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 40) { tok_type[t] = TOK_LPAREN; tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 41) { tok_type[t] = TOK_RPAREN; tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }

        // Quad: [] (IBM 5100 ASCII convention for ⎕) or bracket indexing [expr]
        if (line[i] == 91) {
            if (line[i + 1] == 93) {
                // [] = quad
                tok_type[t] = TOK_QUAD;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 2;
            } else {
                // [ = bracket index open
                tok_type[t] = TOK_LBRAK;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i++;
            }
            continue;
        }
        if (line[i] == 93) {
            tok_type[t] = TOK_RBRAK;
            tok_pos[t] = i;
            tok_val[t] = 0;
            t++;
            i++;
            continue;
        }

        // Assignment: <-
        if (line[i] == 60 && line[i + 1] == 45) {
            tok_type[t] = TOK_ASSIGN;
            tok_pos[t] = i;
            tok_val[t] = 0;
            t++;
            i = i + 2;
            continue;
        }

        // Comparison operators: = != >= <= > <
        // 61 '='  33 '!'  62 '>'  60 '<'
        if (line[i] == 61) { tok_type[t] = TOK_EQ; tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 33 && line[i + 1] == 61) { tok_type[t] = TOK_NE; tok_pos[t] = i; tok_val[t] = 0; t++; i = i + 2; continue; }
        if (line[i] == 62 && line[i + 1] == 61) { tok_type[t] = TOK_GE; tok_pos[t] = i; tok_val[t] = 0; t++; i = i + 2; continue; }
        if (line[i] == 60 && line[i + 1] == 61) { tok_type[t] = TOK_LE; tok_pos[t] = i; tok_val[t] = 0; t++; i = i + 2; continue; }
        if (line[i] == 62) { tok_type[t] = TOK_GT; tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 60) { tok_type[t] = TOK_LT; tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }

        // Unrecognized character
        return -1;
    }

    // Append EOL sentinel
    tok_type[t] = TOK_EOL;
    tok_pos[t] = i;
    tok_val[t] = 0;
    t++;
    tok_count = t;
    return t;
}

// ---- Debug: print token list ----
// Useful for testing the tokenizer in isolation.
void tok_dump(char *line) {
    int i = 0;
    while (i < tok_count) {
        int ty = tok_type[i];
        if (ty == TOK_EOL) {
            io_print("EOL");
        } else if (ty == TOK_NUM) {
            io_print("NUM:");
            print_int(tok_val[i]);
        } else if (ty == TOK_IDENT) {
            io_print("ID:");
            int p = tok_val[i];
            while (is_alnum(line[p])) {
                putchar(line[p]);
                p++;
            }
        } else if (ty == TOK_RES) {
            io_print("RES:");
            int r = tok_val[i];
            if (r == RES_RHO)  io_print("rho");
            if (r == RES_IOTA) io_print("iota");
            if (r == RES_TAKE) io_print("take");
            if (r == RES_DROP) io_print("drop");
            if (r == RES_REV)  io_print("rev");
            if (r == RES_CAT)  io_print("cat");
        } else if (ty == TOK_PLUS)   { putchar(43); }
          else if (ty == TOK_MINUS)  { putchar(45); }
          else if (ty == TOK_STAR)   { putchar(42); }
          else if (ty == TOK_SLASH)  { putchar(47); }
          else if (ty == TOK_LPAREN) { putchar(40); }
          else if (ty == TOK_RPAREN) { putchar(41); }
          else if (ty == TOK_ASSIGN) { io_print("<-"); }

        if (tok_type[i + 1] != TOK_EOL || ty == TOK_EOL) {
            // nothing
        }
        if (i + 1 < tok_count) putchar(32);
        i++;
    }
    putchar(10);
}
