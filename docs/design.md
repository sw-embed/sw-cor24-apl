# COR24 APL Interpreter -- Design Document

## Design Decisions

### D1: Implementation Language -- C via tc24r (Hybrid)

**Decision:** Write the interpreter in C, compiled with tc24r to
COR24 assembly. Add hand-written assembly for performance-critical
array inner loops.

**Rationale:**
- tc24r is proven (compiled tml24c Lisp successfully)
- C is faster to develop than pure assembly
- Assembly inner loops give performance where it matters (element-wise
  ops on large arrays)
- Historical precedent: later APL implementations were C + asm
- Can always hand-optimize hot paths after profiling

**Trade-off:** Less "authentic" than pure assembly (APL\360 was asm),
but far more productive. The Forth interpreter (tf24a) already
demonstrates pure assembly; this project demonstrates the C path.

### D2: ASCII Surface Syntax with Case Split

**Decision:** Lowercase reserved words for APL operators, uppercase
for user identifiers.

**Rationale:**
- Avoids namespace collision (IOTA vs user variable IOTA)
- No special keyboard or Unicode support needed
- Clean lexer split: `[a-z]+` = reserved, `[A-Z][A-Z0-9_]*` = ident
- Future: editor prettification (rho -> display as Greek rho)

**Examples:**
```
A assign iota 10
B assign 2 5 rho A
C assign +/ B
```

### D3: Right-to-Left Evaluation (No Precedence)

**Decision:** Strict right-to-left evaluation with no operator
precedence, matching standard APL semantics.

**Rationale:**
- This IS APL -- it is not optional
- Actually simplifies the parser (no precedence climbing)
- `2 + 3 * 4` evaluates as `2 + (3 * 4)` = `2 + 12` = `14`
  (same result by coincidence), but `2 * 3 + 4` = `2 * 7` = `14`
  (not `6 + 4 = 10`)

### D4: Integer-Only Arithmetic

**Decision:** 24-bit signed integers only. No floating point.

**Rationale:**
- COR24 has no FPU
- Software float adds significant complexity
- Integer APL is still useful and educational
- Historical: IBM 5100 APL was limited too
- Can add software float as a later phase

### D5: Maximum Rank 2

**Decision:** Support scalars, vectors, and matrices only.

**Rationale:**
- Covers the vast majority of introductory APL use
- Rank-3+ tensors add complexity to reshape, reduce, indexing
- Memory overhead of shape arrays stays small
- Can extend later without breaking existing code

### D6: Bump Allocator (Phase 1)

**Decision:** Start with a simple bump allocator. No GC.

**Rationale:**
- Simplest possible memory management
- Sufficient for interactive exploration sessions
- `)CLEAR` resets the heap entirely
- GC adds complexity that can be deferred
- tml24c's mark-sweep GC can be referenced later if needed

### D7: Negative Number Syntax

**Decision:** Use `_` (underscore) for negative literals, following
APL convention. Example: `_3` means -3.

**Rationale:**
- Standard APL uses high minus (overbar) for negative literals
- Underscore is the ASCII equivalent
- Distinguishes negative literal from subtraction operator
- `_3` is the number negative-three; `- 3` is monadic negate of 3
  (same result, different parse)

### D8: Reduce Syntax

**Decision:** `op/` where op is an arithmetic operator.

**Rationale:**
- Standard APL: `/` after an operator means reduce
- `+/ 1 2 3` sums the vector
- Parser treats `op/` as a single compound token (reduce-op)
- Initially only for `+`, `-`, `*`

## Internal Representation

### Token Structure

```c
typedef struct {
    int kind;       /* TOK_NUM, TOK_IDENT, TOK_RHO, TOK_PLUS, ... */
    int value;      /* numeric value for TOK_NUM */
    char name[16];  /* identifier name for TOK_IDENT */
} Token;
```

Token buffer: fixed array of 128 tokens per line (sufficient for
any reasonable interactive expression).

### Array Structure

```c
typedef struct {
    int rank;       /* 0, 1, or 2 */
    int shape0;     /* dim 0 (rows for matrix, length for vector) */
    int shape1;     /* dim 1 (cols for matrix, 0 otherwise) */
    int size;       /* total elements */
    int data[1];    /* flexible array member (C89 style) */
} Array;
```

**Note:** tc24r may not support flexible array members. Alternative:
store data pointer separately, or use fixed offset arithmetic.

Actual layout in heap memory:

```
+0:  rank
+3:  shape0
+6:  shape1
+9:  size
+12: data[0]
+15: data[1]
...
+12+3*(size-1): data[size-1]
```

Total bytes per array: 12 + 3 * size

### AST Node Structure

```c
typedef struct Node {
    int kind;          /* NODE_SCALAR, NODE_VECTOR, NODE_DYADIC, ... */
    int op;            /* operator token kind */
    int value;         /* for scalar literals */
    struct Node *left; /* left operand (NULL for monadic) */
    struct Node *right;/* right operand */
    char name[16];     /* for identifiers */
} Node;
```

Node pool: pre-allocated array of ~256 nodes. Sufficient for
expressions parsed one line at a time.

### Symbol Table

```c
typedef struct {
    char name[16];
    int used;       /* 0 = free slot */
    Array *value;
} Symbol;
```

Fixed array of 64 symbols. Linear scan for lookup (fast enough
for 64 entries on any architecture).

## Parsing Strategy

### Right-to-Left Parsing

APL's right-to-left evaluation suggests parsing the token stream
from right to left. Implementation approach:

1. Tokenize the full line left-to-right into a token array
2. Parse from the rightmost token leftward
3. Each call to `parse_expr()` consumes tokens from right to left

```
Tokens: A <- 2 3 rho iota 6
Index:  0  1  2 3  4    5  6

Parse from index 6:
  6 -> Scalar(6)
  5 -> Monadic(IOTA, Scalar(6))
  4,3,2 -> Dyadic(RHO, Vector(2,3), Monadic(IOTA, Scalar(6)))
  1,0 -> Assign(A, Dyadic(...))
```

### Monadic vs Dyadic Detection

A function/operator is **dyadic** if there is a value expression
to its left. It is **monadic** if:
- It is at the start of the expression
- It follows another operator
- It follows `<-`
- It follows `(`

This context-sensitivity is the trickiest part of the parser.

## Evaluation Strategy

### Element-wise Operations

For dyadic arithmetic (`A + B`):

```
if A is scalar and B is scalar:
    return scalar(A.data[0] op B.data[0])
if A is scalar:
    result = new_array(B.rank, B.shape)
    for i in 0..B.size: result.data[i] = A.data[0] op B.data[i]
if B is scalar:
    result = new_array(A.rank, A.shape)
    for i in 0..A.size: result.data[i] = A.data[i] op B.data[0]
else:
    assert A.shape == B.shape  (LENGTH ERROR if not)
    result = new_array(A.rank, A.shape)
    for i in 0..A.size: result.data[i] = A.data[i] op B.data[i]
```

### Reduce

```
+/ V  where V is a vector of size n:
    acc = V.data[n-1]
    for i in n-2..0: acc = V.data[i] + acc
    return scalar(acc)
```

Note: reduce goes right-to-left (standard APL).

### Iota

```
iota N:
    result = new_array(1, [N])
    for i in 0..N: result.data[i] = i
    return result
```

(Using 0-origin indexing. Traditional APL uses 1-origin by default
via the index origin quad-IO variable. We start with 0-origin
for simplicity and C-programmer familiarity.)

### Reshape (rho)

```
S rho D:
    total = product of S elements
    result = new_array(len(S), S, total)
    for i in 0..total:
        result.data[i] = D.data[i mod D.size]
    return result
```

## UART I/O

### Input

```c
int uart_getchar(void) {
    volatile int *status = (int *)0xFF0101;
    volatile int *data   = (int *)0xFF0100;
    while (!(*status & 1))  /* wait for RX ready */
        ;
    return *data & 0xFF;
}
```

Line buffer: 256 bytes. Read until newline. Support backspace
(delete last char, echo BS-SPACE-BS).

### Output

```c
void uart_putchar(int ch) {
    volatile int *status = (int *)0xFF0101;
    volatile int *data   = (int *)0xFF0100;
    while (*status & 0x80)  /* wait for TX not busy */
        ;
    *data = ch;
}
```

### Number Printing

Right-justify integers in a field width. For matrices, pre-scan
each column to determine maximum width, then format accordingly.

## Testing Approach

### Test Harness

Use cor24-run with UART I/O capture. Feed input expressions via
stdin, capture UART output, compare to expected.

### Test Cases (representative)

```
Input:                    Expected output:
3 + 4                     7
iota 5                    0 1 2 3 4
2 3 rho iota 6           0 1 2
                          3 4 5
+/ iota 10               45
rho 2 3 rho iota 6       2 3
A <- 10 20 30            10 20 30
A + A                     20 40 60
3 take iota 10            0 1 2
2 drop iota 5             2 3 4
rev iota 5                4 3 2 1 0
```

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| tc24r limitations (no flexible arrays, limited preprocessor) | Medium | Use explicit pointer arithmetic; test tc24r features early |
| 24-bit integer overflow in array indexing | Medium | Check size calculations; limit max array size |
| Heap fragmentation without GC | Low | Bump allocator + `)CLEAR` is sufficient for interactive use |
| Parser complexity for monadic/dyadic detection | Medium | Build test suite for edge cases early; keep grammar minimal |
| UART speed limits interactive feel | Low | COR24 emulator runs fast; real FPGA UART is 115200 baud |

### D9: Hardware I/O via `□SVO` Shared Variables

**Decision:** Use standard APL shared variable protocol (`□SVO`)
with COR24-specific auxiliary processor (AP) numbers for hardware
I/O. AP numbers in IBM's 200+ user-defined range.

**Rationale:**
- IBM 5100/5110/5120 and VS APL used `□SVO`/`□SVC` for device I/O
- `□SVO` is a recognized APL concept -- any APL programmer sees
  "shared variable" and understands device I/O intent
- Portable graceful degradation: `□SVO` returns coupling degree;
  code can branch around hardware access on non-COR24 systems
- AP 240-249 = byte access, AP 250-259 = word access, mapped to
  COR24 memory regions (SRAM, EBR, MMIO)
- User picks variable names (`'MMIO' □SVO 242`), not hardcoded

**AP scheme:**
- AP 240 = SRAM bytes, AP 241 = EBR bytes, AP 242 = MMIO bytes
- AP 250 = SRAM words, AP 251 = EBR words
- AP 243-249, 252-259 reserved for future (I2C, SPI, GPIO)

**Trade-off:** More complex than custom quad names (`qled`/`qsw`),
but portable, extensible, and APL-idiomatic. Considered and rejected
simpler alternatives: custom quad variables (non-portable, GNU APL
gives unhelpful errors), `□ARBIN`/`□ARBOUT` (less well-known),
PEEK/POKE (not APL-idiomatic at all).

**Skipped:** `□SVC` (access control -- no peer to sync with) and
`□SVQ` (query state -- user already has return code from `□SVO`).

### D10: Batch Mode (Headless APL Execution)

**Decision:** Two execution modes -- interactive (UART REPL) and
batch (APL image pre-loaded in SRAM). Batch mode frees UART for
the APL program's own use.

**Rationale:**
- Interactive mode uses UART for console I/O -- APL programs can't
  also use UART without conflict
- Batch mode: interpreter reads from SRAM, UART available to program
  via `□SVO` shared variables
- Mode selected by a memory flag set via `cor24-run --patch`
- Enables real embedded applications (UART echo, device control)

### D11: Control Flow (`→` Branch and Labels)

**Decision:** Standard APL branch (`→`) with line labels, plus
user-defined functions via `del` (ASCII `∇`).

**Rationale:**
- Required for any non-trivial APL program (loops, conditionals)
- Standard APL: `→(COND)/LABEL` conditional branch
- `del R <- FN X` ... `del` for function definition
- Without these, APL is limited to single-expression calculator
- Needed for the UART echo target use case

## Future Extensions (not in scope)

- User-defined functions (`del` / `nabla` editing)
- Outer product (`o.+`, `o.*`)
- Inner product (`+.*`)
- Scan operator (`+\`)
- Bracket indexing (`A[2;3]`)
- Character data and mixed arrays
- Floating-point via software library
- Workspace save/load
