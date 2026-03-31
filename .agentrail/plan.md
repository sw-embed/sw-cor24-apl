# COR24 APL Interpreter -- Implementation Plan

## Phase Overview

| Phase | Description                    | Deliverable                       |
|-------|-------------------------------|-----------------------------------|
| 0     | Project setup and toolchain   | Build system, hello-world on emu  |
| 1     | Scalar REPL                   | Read, eval, print scalar exprs    |
| 2     | Vector support                | 1D arrays, element-wise ops       |
| 3     | Core APL primitives           | iota, rho, reduce, take, drop     |
| 4     | Matrix support                | 2D arrays, formatted output       |
| 5     | Polish and testing            | Error handling, edge cases, docs  |
| 6     | Shared variable I/O           | □SVO, AP 242, MMIO indexing       |
| 7     | Bracket indexing & bitwise    | V[N], and/or/not                  |
| 8     | Control flow & functions      | →, labels, ∇ user functions       |
| 9     | Batch mode                    | APL image loading, headless exec  |

---

## Phase 0: Project Setup and Toolchain Validation

### Step 0.1: Repository structure
- Create `src/` directory for C source files
- Create `asm/` directory for hand-written assembly helpers
- Create `test/` directory for test scripts
- Create `Makefile` or build script for tc24r + cor24-run pipeline
- Verify tc24r can compile a trivial C program to COR24 assembly
- Verify cor24-run can assemble and execute the result

### Step 0.2: UART I/O bootstrap
- Write `uart_putchar()` and `uart_getchar()` in C (or asm)
- Write `uart_puts()` for string output
- Write `uart_getline()` with backspace support
- Test: print "COR24 APL v0.1" on startup, echo typed lines

### Step 0.3: Number I/O
- Write `print_int()` -- integer to decimal string via UART
- Write `parse_int()` -- decimal string to integer
- Handle negative numbers (underscore prefix `_N`)
- Test: type a number, see it echoed back

---

## Phase 1: Scalar REPL

### Step 1.1: Tokenizer
- Implement token types: NUM, PLUS, MINUS, STAR, SLASH, LPAREN,
  RPAREN, ASSIGN, IDENT, RESERVED, EOL
- Scan input buffer into token array
- Reserved word lookup table (rho, iota, etc. -- recognized but
  not yet functional)
- Test: tokenize "3 + 4" and dump tokens

### Step 1.2: Scalar parser
- Right-to-left recursive descent parser
- Handle: number literals, binary ops (+, -, *), parentheses
- Monadic minus (negate)
- Build AST nodes from pre-allocated pool
- Test: parse "2 + 3 * 4" -> correct R-to-L tree

### Step 1.3: Scalar evaluator
- Tree-walking evaluator for scalar expressions
- Arithmetic: add, subtract, multiply
- Division: software divide (COR24 has no div instruction)
- Modulo via software divide
- Test: "2 + 3 * 4" -> 14 (right-to-left: 2 + 12)

### Step 1.4: REPL loop
- Read line -> tokenize -> parse -> eval -> print result
- Error recovery: on any error, print message, discard line, re-prompt
- Test: interactive scalar calculator works

### Step 1.5: Variables
- Symbol table (64 entries, linear scan)
- Assignment: `A <- expr`
- Variable reference in expressions
- Test: `A <- 5` then `A + 3` -> 8

---

## Phase 2: Vector Support

### Step 2.1: Array data structure
- Heap allocation (bump allocator)
- Array header: rank, shape, size, data pointer
- `new_array(rank, shape)` allocator
- `array_get(arr, i)` and `array_set(arr, i, val)` accessors

### Step 2.2: Vector literals
- Parser recognizes adjacent numbers as vector: `1 2 3`
- Tokenizer produces sequence of NUM tokens
- Parser groups consecutive numbers into a vector node
- Test: `1 2 3` creates a 3-element vector

### Step 2.3: Element-wise operations
- Extend evaluator for vector + vector
- Scalar extension: scalar + vector, vector + scalar
- Conformability check (LENGTH ERROR on mismatch)
- All arithmetic ops: +, -, *, comparisons
- Test: `1 2 3 + 10 20 30` -> `11 22 33`

### Step 2.4: Vector output
- Print vectors: space-separated values on one line
- Right-justify numbers to consistent width
- Test: display various vectors correctly

---

## Phase 3: Core APL Primitives

### Step 3.1: iota
- Monadic: `iota N` generates vector 0 1 2 ... N-1
- Allocate N-element vector, fill with indices
- Test: `iota 5` -> `0 1 2 3 4`

### Step 3.2: rho (reshape and shape-of)
- Monadic: `rho A` returns shape as a vector
- Dyadic: `S rho A` reshapes A to shape S (with cyclic fill)
- Test: `2 3 rho iota 6` -> 2x3 matrix display
- Test: `rho 1 2 3` -> `3`

### Step 3.3: Reduce
- `+/`, `-/`, `*/` over vectors
- Right-to-left reduction (standard APL)
- Test: `+/ iota 10` -> `45`
- Test: `-/ 1 2 3` -> `1 - (2 - 3)` = `1 - (-1)` = `2`

### Step 3.4: take and drop
- Dyadic: `N take A` returns first N elements
- Dyadic: `N drop A` returns all but first N elements
- Negative N: take/drop from end
- Test: `3 take iota 10` -> `0 1 2`
- Test: `_2 take iota 5` -> `3 4`

### Step 3.5: rev (reverse) and cat (catenate/ravel)
- Monadic `rev`: reverse a vector
- Monadic `cat`: ravel (flatten to 1D)
- Dyadic `cat`: catenate two vectors
- Test: `rev iota 5` -> `4 3 2 1 0`
- Test: `(iota 3) cat (iota 3)` -> `0 1 2 0 1 2`

---

## Phase 4: Matrix Support

### Step 4.1: Matrix creation
- `rho` with 2-element shape creates matrix
- Internal storage: row-major flat array
- Test: `2 3 rho 1 2 3 4 5 6` creates 2x3 matrix

### Step 4.2: Matrix display
- One row per line
- Right-justified columns
- Pre-scan for column widths
- Test: matrices display with aligned columns

### Step 4.3: Matrix operations
- Element-wise ops on conformable matrices
- Scalar extension to matrix
- `rho` of matrix returns 2-element vector
- `cat` (ravel) on matrix returns vector
- Test: matrix + matrix, scalar + matrix

### Step 4.4: take/drop on matrices
- `N take M` takes first N rows
- `N drop M` drops first N rows
- Test: `1 take 2 3 rho iota 6` -> first row as matrix

---

## Phase 5: Polish and Testing

### Step 5.1: System commands
- `)CLEAR` -- reset heap and symbol table
- `)VARS` -- list defined variable names
- `)OFF` -- halt interpreter
- Test: each command works correctly

### Step 5.2: Error handling
- SYNTAX ERROR for malformed input
- LENGTH ERROR for non-conformable shapes
- RANK ERROR for unsupported rank combinations
- DOMAIN ERROR for division by zero
- WS FULL for heap exhaustion
- VALUE ERROR for undefined variables
- Test: each error type triggers correctly and REPL recovers

### Step 5.3: Edge cases
- Empty vectors (iota 0)
- Single-element arrays
- Very large arrays (stress test heap)
- Deeply nested parentheses
- Long input lines

### Step 5.4: Conformance test suite
- Comprehensive test script fed via cor24-run
- Expected output comparison
- Document any deviations from standard APL behavior

### Step 5.5: Documentation
- Update README.md with usage instructions
- Document supported operations and syntax
- Example session transcript
- Known limitations

---

## Phase 6: Shared Variable I/O

Hardware I/O via standard APL shared variable protocol (`□SVO`).
See `docs/apl-echo-example-plan.md` for full design rationale,
AP number scheme, and the target UART echo use case.

### Background

IBM 5100/5110/5120 and VS APL used `□SVO` (Shared Variable Offer)
to connect APL variables to auxiliary processors (APs) for device
I/O. We adopt this protocol with COR24-specific AP numbers in the
200+ user-defined range.

### AP Number Scheme

| AP  | Width       | Region | Address Range       |
|-----|-------------|--------|---------------------|
| 240 | byte        | SRAM   | `000000-0FFFFF`     |
| 241 | byte        | EBR    | `FEE000-FEFFFF`     |
| 242 | byte        | MMIO   | `FF0000-FFFFFF`     |
| 243-249 | byte    | —      | reserved future     |
| 250 | 24-bit word | SRAM   | `000000-0FFFFF`     |
| 251 | 24-bit word | EBR    | `FEE000-FEFFFF`     |
| 252-259 | word    | —      | reserved future     |

ASCII syntax: `□` maps to `q` prefix. `'MMIO' qSVO 242` in source.

### Step 6.1: `□SVO` system function
- Tokenizer recognizes `qSVO` as `TOK_QSVO`
- Dyadic: `'NAME' qSVO AP` -- returns coupling degree (0/1/2)
- Shared variable table: maps offered names to AP + region
- AP 242 handler: byte read/write to MMIO region (`FF0000+offset`)
- Active-low inversion on offset 0 (LED/switch convenience)
- Return 2 (coupled) for supported APs, 0 for unknown APs
- Test: `MMIORC <- 'MMIO' qSVO 242` returns `2`
- Test: `MMIORC <- 'X' qSVO 999` returns `0`

### Step 6.2: Shared variable indexed read/write
- `MMIO[N]` reads byte at `FF0000 + N`
- `MMIO[N] <- expr` writes byte at `FF0000 + N`
- 0-origin offsets regardless of `□IO` setting
- Test: `MMIO[0] <- 1` (LED on), `MMIO[0]` (read switch)
- Test: `MMIO[256]` (UART data), `MMIO[257]` (UART status)

### Step 6.3: Graceful degradation
- `→(MMIORC<2)/NODEV` pattern for portable code
- Programs skip hardware access on non-COR24 systems
- Test: verify branch-around works when AP returns 0

### Example Session
```
      MMIORC <- 'MMIO' qSVO 242
      MMIORC
  2
      MMIO[0] <- 1
      MMIO[0]
  1
      MMIO[257]
  2
```

---

## Phase 7: Bracket Indexing and Bitwise Operations

### Step 7.1: Bracket indexing on vectors
- `V[N]` reads element N from vector V
- `V[N] <- expr` writes element N
- 0-origin (matches our `iota` convention)
- Test: `A <- 10 20 30` then `A[1]` -> `20`

### Step 7.2: Bitwise operations
- `and` (bitwise AND): needed for UART status bit testing
- `or` (bitwise OR): general use
- `not` (bitwise NOT / complement): general use
- These are reserved words, same as `rho`, `iota`
- Test: `128 and 130` -> `128`
- Test: `3 or 4` -> `7`

---

## Phase 8: Control Flow and Functions

### Step 8.1: Branch and labels
- `→N` branches to line N
- `→LABEL` branches to labeled line
- `→(COND)/LABEL` conditional branch (standard APL idiom)
- `→0` exits current function or program
- `LABEL:` at start of line defines a branch target
- Test: loop that counts to 10

### Step 8.2: Multi-line program storage
- Line array stored in heap
- Direct entry: `[N] expr` syntax to define/edit lines
- `)LIST` system command to display program
- `)RUN` or `→1` to execute from line 1
- Test: enter and run a multi-line program

### Step 8.3: User-defined functions
- `del R <- FN X` ... `del` (ASCII for `∇`)
- Monadic and dyadic user functions
- Local variables
- Recursive calls
- Test: `del R <- DOUBLE X` / `R <- X + X` / `del`

---

## Phase 9: Batch Mode

### Step 9.1: APL image format
- Newline-separated lines, null-terminated in memory
- Loaded via `cor24-run --load-binary program.apl@0x080000`
- Image pointer set via `--patch`

### Step 9.2: Mode detection and batch reader
- Interpreter checks known address for non-zero image pointer
- Batch mode: read lines from SRAM, not UART
- UART is free for the APL program to use via `□SVO`
- Interactive mode: unchanged REPL behavior
- Test: load simple APL program, verify execution

### Step 9.3: UART echo demo
- Full target use case: UART echo with LED/switch
- See `docs/apl-echo-example-plan.md` for the program listing
- Test via `cor24-run --terminal` with piped input
