// COR24 APL Interpreter -- Symbol Table
// Linear-scan symbol table for variable storage.
// Variable names are uppercase (A-Z, alphanumeric).
// Names stored in a flat buffer with NUL terminators.

#pragma once

#define SYM_MAX   64    // max number of variables
#define SYM_NAMES 256   // name buffer size

// Symbol storage
int sym_val[SYM_MAX];       // value for each symbol
int sym_set_flag[SYM_MAX];  // 1 if assigned, 0 if undefined
int sym_name_off[SYM_MAX];  // offset into sym_name_buf
int sym_count;               // number of symbols defined
char sym_name_buf[SYM_NAMES]; // flat name storage
int sym_name_pos;            // next free position in name buffer

// Compare identifier at line[pos..] with name at sym_name_buf[off..].
// Returns 1 if identical, 0 otherwise.
int sym_name_eq(char *line, int pos, int off) {
    int i = 0;
    while (sym_name_buf[off + i]) {
        if (line[pos + i] != sym_name_buf[off + i]) return 0;
        i++;
    }
    // Ensure the identifier in line also ends here
    if (is_alnum(line[pos + i])) return 0;
    return 1;
}

// Copy identifier from line[pos..] into name buffer at current position.
// Returns the offset where the name was stored.
int sym_name_copy(char *line, int pos) {
    int off = sym_name_pos;
    while (is_alnum(line[pos])) {
        sym_name_buf[sym_name_pos] = line[pos];
        sym_name_pos++;
        pos++;
    }
    sym_name_buf[sym_name_pos] = 0;
    sym_name_pos++;
    return off;
}

// Look up or create a symbol for the identifier at line[pos..].
// Returns symbol index (0..SYM_MAX-1), or -1 if table full.
int sym_lookup(char *line, int pos) {
    int i = 0;
    while (i < sym_count) {
        if (sym_name_eq(line, pos, sym_name_off[i])) return i;
        i++;
    }
    // Not found -- create new entry
    if (sym_count >= SYM_MAX) return -1;
    if (sym_name_pos >= SYM_NAMES - 16) return -1;
    int idx = sym_count;
    sym_name_off[idx] = sym_name_copy(line, pos);
    sym_val[idx] = 0;
    sym_set_flag[idx] = 0;
    sym_count++;
    return idx;
}
