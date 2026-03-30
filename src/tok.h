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

// Reserved word IDs
#define RES_RHO     0
#define RES_IOTA    1
#define RES_TAKE    2
#define RES_DROP    3
#define RES_REV     4
#define RES_CAT     5

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

        // Quad: [] (IBM 5100 ASCII convention for ⎕)
        if (line[i] == 91 && line[i + 1] == 93) {
            tok_type[t] = TOK_QUAD;
            tok_pos[t] = i;
            tok_val[t] = 0;
            t++;
            i = i + 2;
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
