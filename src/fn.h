// COR24 APL Interpreter -- User-Defined Functions
// Function definition, storage, and call stack management.
// Supports niladic (del R assign FN), monadic (del R assign FN X),
// and dyadic (del R assign X FN Y).
// Uses del (ASCII for nabla) to open/close function definitions.

#pragma once

#define FN_MAX      8    // max user functions
#define FN_BODY_MAX 25   // max body lines per function
#define FN_BLINE    120  // max chars per body line
#define CALL_MAX    8    // max recursion depth
#define FN_LOCAL_MAX 8   // max local variables per function

// Function table
int fn_name_sym[FN_MAX];    // symbol index of function name
int fn_result_sym[FN_MAX];  // symbol index of result variable
int fn_right_sym[FN_MAX];   // symbol index of right argument
int fn_left_sym[FN_MAX];    // symbol index of left arg (-1 = monadic)
int fn_lines[FN_MAX];       // number of body lines
int fn_count;               // number of defined functions
int fn_local_count[FN_MAX]; // number of local variables
int fn_local_sym[64];       // FN_MAX * FN_LOCAL_MAX local var sym indices

char fn_body[24000];        // FN_MAX * FN_BODY_MAX * FN_BLINE

// Definition mode state
int fn_def_mode;    // 1 = collecting body lines
int fn_def_idx;     // function index being defined
int fn_def_line;    // next body line number

// Get pointer to function body line
char *fn_get_line(int fn, int line) {
    return fn_body + (fn * FN_BODY_MAX + line) * FN_BLINE;
}

// Store a body line
void fn_store_line(int fn, int line, char *src) {
    char *dst = fn_get_line(fn, line);
    int i = 0;
    while (src[i] && i < FN_BLINE - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

// Look up function by symbol index, returns fn index or -1
int fn_lookup(int sym) {
    int i = 0;
    while (i < fn_count) {
        if (fn_name_sym[i] == sym) return i;
        i++;
    }
    return -1;
}

// Reset function table
void fn_reset() {
    fn_count = 0;
    fn_def_mode = 0;
}

// Pre-scan labels in function body (sets symbol values for branch targets)
void fn_scan_labels(int fi) {
    int i = 0;
    while (i < fn_lines[fi]) {
        char *ln = fn_get_line(fi, i);
        if (is_upper(ln[0])) {
            int j = 1;
            while (is_alnum(ln[j])) j++;
            if (ln[j] == 58) {  // ':'
                int sym_idx = sym_lookup(ln, 0);
                if (sym_idx >= 0) {
                    sym_val[sym_idx] = arr_scalar(i + 1);  // 1-based line number
                    sym_set_flag[sym_idx] = 1;
                }
            }
        }
        i++;
    }
}

// Parse function header: "R <- FN X" (monadic) or "R <- X FN Y" (dyadic)
// s points past "del " to the header text
// fi is the function table slot to fill
// Returns 1 on success, 0 on error
int parse_fn_header(char *s, int fi) {
    int i = 0;

    // Skip leading spaces
    while (s[i] == 32) i++;

    // Read result variable name
    if (!is_upper(s[i])) return 0;
    int result_pos = i;
    while (is_alnum(s[i])) i++;
    int result_sym = sym_lookup(s, result_pos);
    if (result_sym < 0) return 0;

    // Skip spaces, expect "assign"
    while (s[i] == 32) i++;
    if (str_match(s, i, "assign") != 6) return 0;
    if (is_alnum(s[i + 6])) return 0;
    i = i + 6;
    while (s[i] == 32) i++;

    // Read remaining identifiers (2 = monadic, 3 = dyadic)
    int ids[3];
    int nids = 0;
    while (is_upper(s[i]) && nids < 3) {
        int id_pos = i;
        while (is_alnum(s[i])) i++;
        ids[nids] = sym_lookup(s, id_pos);
        if (ids[nids] < 0) return 0;
        nids++;
        while (s[i] == 32) i++;
    }

    if (nids == 1) {
        // Niladic: FN (no arguments)
        fn_name_sym[fi] = ids[0];
        fn_right_sym[fi] = -1;
        fn_left_sym[fi] = -1;
    } else if (nids == 2) {
        // Monadic: FN X
        fn_name_sym[fi] = ids[0];
        fn_right_sym[fi] = ids[1];
        fn_left_sym[fi] = -1;
    } else if (nids == 3) {
        // Dyadic: X FN Y
        fn_left_sym[fi] = ids[0];
        fn_name_sym[fi] = ids[1];
        fn_right_sym[fi] = ids[2];
    } else {
        return 0;
    }

    fn_result_sym[fi] = result_sym;
    fn_lines[fi] = 0;

    // Parse local variables: ;VAR1;VAR2 ...
    int lbase = fi * FN_LOCAL_MAX;
    int lcount = 0;
    while (s[i] == 59 && lcount < FN_LOCAL_MAX) {  // 59 = ';'
        i++;
        while (s[i] == 32) i++;
        if (!is_upper(s[i])) return 0;
        int lpos = i;
        while (is_alnum(s[i])) i++;
        int lsym = sym_lookup(s, lpos);
        if (lsym < 0) return 0;
        fn_local_sym[lbase + lcount] = lsym;
        lcount++;
        while (s[i] == 32) i++;
    }
    fn_local_count[fi] = lcount;

    return 1;
}
