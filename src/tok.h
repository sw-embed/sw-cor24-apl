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
#define TOK_ASSIGN  9   // assign (APL ←)
#define TOK_RES    10   // reserved word (ID in tok_val)
#define TOK_QUAD   11   // quad — bare quad I/O (APL ⎕)
#define TOK_QSVO   14   // qsvo — shared variable offer (APL ⎕SVO)
#define TOK_LBRAK  15   // [ (bracket index open)
#define TOK_RBRAK  16   // ] (bracket index close)
#define TOK_GOTO   17   // goto — branch (APL →)
#define TOK_STRING 18   // string literal 'ABC' (pos of first char in tok_val)
#define TOK_EQ     19   // =
#define TOK_NE     20   // !=
#define TOK_LT     21   // <
#define TOK_GT     22   // >
#define TOK_LE     23   // <=
#define TOK_GE     24   // >=
#define TOK_CEIL   25   // ceil (max) — internal only, used in reduce nodes
#define TOK_FLOOR  26   // floor (min) — internal only, used in reduce nodes
#define TOK_QRL    27   // quad-seed — PRNG seed (APL ⎕RL)
#define TOK_AND_OP 30   // and — internal only, used in reduce nodes
#define TOK_OR_OP  31   // or — internal only, used in reduce nodes
#define TOK_QIO    32   // quad-origin — index origin (APL ⎕IO)
#define TOK_BSLASH 33   // \ (backslash — scan operator)
#define TOK_OUTER  34   // outer (outer product prefix)
#define TOK_DOT    35   // . (dot — used in outer product)

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
#define RES_CEIL    9
#define RES_FLOOR  10
#define RES_COMPRESS 11
#define RES_PICK    12
#define RES_ROLL    13
#define RES_FMT     14
#define RES_CUP     15
#define RES_CAP     16
#define RES_ABS      17
#define RES_RESIDUE  18
#define RES_SIGNUM   19
#define RES_MEMBER   22
#define RES_WITHOUT  23
#define RES_TRANSPOSE 26
#define RES_ENCODE   27
#define RES_DECODE   28
#define RES_ENCLOSE  29
#define RES_GRADEUP  24
#define RES_GRADEDN  25
#define RES_FACTORIAL 20
#define RES_BINOMIAL 21

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

// Check if character is alphanumeric or hyphen (for compound keywords like quad-origin)
int is_alnum_h(int ch) {
    return is_alnum(ch) || ch == 45;
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

    len = str_match(src, pos, "ceil");
    if (len == 4 && !is_alnum(src[pos + 4])) { *end = pos + 4; return RES_CEIL; }

    len = str_match(src, pos, "floor");
    if (len == 5 && !is_alnum(src[pos + 5])) { *end = pos + 5; return RES_FLOOR; }

    len = str_match(src, pos, "compress");
    if (len == 8 && !is_alnum(src[pos + 8])) { *end = pos + 8; return RES_COMPRESS; }

    len = str_match(src, pos, "pick");
    if (len == 4 && !is_alnum(src[pos + 4])) { *end = pos + 4; return RES_PICK; }

    len = str_match(src, pos, "roll");
    if (len == 4 && !is_alnum(src[pos + 4])) { *end = pos + 4; return RES_ROLL; }

    len = str_match(src, pos, "fmt");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_FMT; }

    len = str_match(src, pos, "cup");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_CUP; }

    len = str_match(src, pos, "cap");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_CAP; }

    len = str_match(src, pos, "abs");
    if (len == 3 && !is_alnum(src[pos + 3])) { *end = pos + 3; return RES_ABS; }

    len = str_match(src, pos, "residue");
    if (len == 7 && !is_alnum(src[pos + 7])) { *end = pos + 7; return RES_RESIDUE; }

    len = str_match(src, pos, "signum");
    if (len == 6 && !is_alnum(src[pos + 6])) { *end = pos + 6; return RES_SIGNUM; }

    len = str_match(src, pos, "factorial");
    if (len == 9 && !is_alnum(src[pos + 9])) { *end = pos + 9; return RES_FACTORIAL; }

    len = str_match(src, pos, "binomial");
    if (len == 8 && !is_alnum(src[pos + 8])) { *end = pos + 8; return RES_BINOMIAL; }

    len = str_match(src, pos, "member");
    if (len == 6 && !is_alnum(src[pos + 6])) { *end = pos + 6; return RES_MEMBER; }

    len = str_match(src, pos, "transpose");
    if (len == 9 && !is_alnum(src[pos + 9])) { *end = pos + 9; return RES_TRANSPOSE; }

    len = str_match(src, pos, "enclose");
    if (len == 7 && !is_alnum(src[pos + 7])) { *end = pos + 7; return RES_ENCLOSE; }

    len = str_match(src, pos, "encode");
    if (len == 6 && !is_alnum(src[pos + 6])) { *end = pos + 6; return RES_ENCODE; }

    len = str_match(src, pos, "decode");
    if (len == 6 && !is_alnum(src[pos + 6])) { *end = pos + 6; return RES_DECODE; }

    len = str_match(src, pos, "without");
    if (len == 7 && !is_alnum(src[pos + 7])) { *end = pos + 7; return RES_WITHOUT; }

    len = str_match(src, pos, "gradeup");
    if (len == 7 && !is_alnum(src[pos + 7])) { *end = pos + 7; return RES_GRADEUP; }

    len = str_match(src, pos, "gradedown");
    if (len == 9 && !is_alnum(src[pos + 9])) { *end = pos + 9; return RES_GRADEDN; }

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

        // Comment handled below as reserved word "comment"

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

        // Lowercase letter: keywords, quad variables, reserved words
        if (is_lower(line[i])) {
            int len;

            // Quad system variables (hyphenated compound keywords)
            len = str_match(line, i, "quad-origin");
            if (len == 11 && !is_alnum_h(line[i + 11])) {
                tok_type[t] = TOK_QIO;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 11;
                continue;
            }

            len = str_match(line, i, "quad-seed");
            if (len == 9 && !is_alnum_h(line[i + 9])) {
                tok_type[t] = TOK_QRL;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 9;
                continue;
            }

            // Bare quad I/O (APL ⎕)
            len = str_match(line, i, "quad");
            if (len == 4 && !is_alnum_h(line[i + 4])) {
                tok_type[t] = TOK_QUAD;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 4;
                continue;
            }

            // Shared variable offer (APL ⎕SVO)
            len = str_match(line, i, "qsvo");
            if (len == 4 && !is_alnum(line[i + 4])) {
                tok_type[t] = TOK_QSVO;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 4;
                continue;
            }

            // Assignment: "assign" (APL ←)
            len = str_match(line, i, "assign");
            if (len == 6 && !is_alnum(line[i + 6])) {
                tok_type[t] = TOK_ASSIGN;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 6;
                continue;
            }

            // Outer product prefix
            len = str_match(line, i, "outer");
            if (len == 5 && !is_alnum(line[i + 5])) {
                tok_type[t] = TOK_OUTER;
                tok_pos[t] = i;
                tok_val[t] = 0;
                t++;
                i = i + 5;
                continue;
            }

            // Comment: "comment" skips to end of line (APL ⍝)
            len = str_match(line, i, "comment");
            if (len == 7 && !is_alnum(line[i + 7])) {
                break;
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
        if (line[i] == 92) { tok_type[t] = TOK_BSLASH; tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 46) { tok_type[t] = TOK_DOT;    tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 40) { tok_type[t] = TOK_LPAREN; tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }
        if (line[i] == 41) { tok_type[t] = TOK_RPAREN; tok_pos[t] = i; tok_val[t] = 0; t++; i++; continue; }

        // Bracket indexing: [expr]
        if (line[i] == 91) {
            tok_type[t] = TOK_LBRAK;
            tok_pos[t] = i;
            tok_val[t] = 0;
            t++;
            i++;
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
          else if (ty == TOK_ASSIGN) { io_print("assign"); }

        if (tok_type[i + 1] != TOK_EOL || ty == TOK_EOL) {
            // nothing
        }
        if (i + 1 < tok_count) putchar(32);
        i++;
    }
    putchar(10);
}
