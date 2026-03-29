# COR24 APL Interpreter -- Product Requirements Document

## Purpose

Implement a minimal APL interpreter for the COR24 24-bit RISC
architecture, running on the COR24 emulator via UART. This continues
the pattern of building language implementations for COR24 (after
tc24r C compiler, tf24a Forth, and tml24c Lisp).

## Goals

1. **Working APL REPL** over UART on the COR24 emulator
2. **ASCII surface syntax** using lowercase reserved words (rho, iota,
   take, drop) to avoid collision with uppercase user identifiers
3. **Integer-only arithmetic** (24-bit signed) initially
4. **Arrays up to rank 2** (scalars, vectors, matrices)
5. **Core APL semantics** including right-to-left evaluation, scalar
   extension, and monadic/dyadic operator overloading
6. **Educational value** -- clean, readable implementation that
   demonstrates APL concepts on a minimal architecture

## Non-Goals (explicitly deferred)

- Floating-point arithmetic
- Ranks above 2 (tensors)
- User-defined functions (dfns/tradfns)
- Nested arrays
- Complex numbers
- File I/O beyond UART
- Editor/IDE integration (Emacs prettification is separate)
- Full APL2/ISO 13751 compliance
- Performance optimization beyond basic correctness

## Target Users

- The developer (learning APL implementation internals)
- Anyone exploring COR24 as a platform for language runtimes
- Educational use demonstrating APL on minimal hardware

## Functional Requirements

### FR-1: Interactive REPL

- Read one line of input from UART
- Tokenize, parse, evaluate, and print result
- Display errors with category (SYNTAX, LENGTH, RANK, DOMAIN, WS FULL)
- Prompt with 6-space indent (traditional APL style)
- Support workspace clear command: `)CLEAR`
- Support variable listing: `)VARS`

### FR-2: Data Types

- **Integers:** 24-bit signed (-8388608 to 8388607)
- **Arrays:** rank 0 (scalar), rank 1 (vector), rank 2 (matrix)
- **Maximum rank:** 2 (initially)
- **Maximum total elements:** limited by available heap (~900 KB)

### FR-3: Lexical Rules

- **Identifiers:** `[A-Z][A-Z0-9_]*` (uppercase only)
- **Reserved words:** `[a-z][a-z]*` (lowercase only)
- **Numbers:** `[0-9]+` with `_` prefix for negative (`_3` = -3)
- **Assignment:** `<-`
- **Operators:** `+ - * /` (and `/` as reduce when following an op)
- **Comments:** lamp symbol -- use `;;` or `NB.` (to be decided)

### FR-4: Arithmetic Operations

| Operator | Monadic      | Dyadic        |
|----------|-------------|---------------|
| `+`      | identity    | addition      |
| `-`      | negate      | subtraction   |
| `*`      | signum      | multiplication|
| `/`      | (reduce)    | (reserved)    |
| `=`      | --          | equals (0/1)  |
| `<`      | --          | less than     |
| `>`      | --          | greater than  |
| `\|`     | magnitude   | residue (mod) |

All operations are element-wise on arrays with scalar extension.

### FR-5: Array Operations

| Token   | Monadic       | Dyadic            |
|---------|---------------|--------------------|
| `rho`   | shape-of      | reshape            |
| `iota`  | index generate| index-of (phase 2) |
| `take`  | --            | take first/last N  |
| `drop`  | --            | drop first/last N  |
| `rev`   | reverse       | rotate (phase 2)   |
| `cat`   | ravel (to 1D) | catenate           |

### FR-6: Reduce Operator

- Syntax: `op/` applied to a vector
- Examples: `+/ 1 2 3` = 6, `*/ 1 2 3 4` = 24
- Supported for: `+`, `-`, `*`, `=`, `<`, `>`

### FR-7: Vector Literals

- Adjacent numbers form a vector: `1 2 3` is a 3-element vector
- Parenthesized expressions can appear in vectors: `(2+3) 4 5`

### FR-8: Assignment and Variables

- `NAME <- expr` binds result to NAME
- Variables persist until `)CLEAR`
- Referencing undefined variable: VALUE ERROR

### FR-9: Output Formatting

- Scalars: value followed by newline
- Vectors: space-separated, one line
- Matrices: one row per line, right-justified columns
- Negative numbers: display with leading `_` (APL high minus)

### FR-10: System Commands

- `)CLEAR` -- reset workspace (free all variables)
- `)VARS` -- list defined variable names
- `)OFF` -- halt the interpreter

## Quality Requirements

### QR-1: Correctness

- All core operations must handle scalar extension correctly
- Right-to-left evaluation must be strict (no precedence)
- Reshape must cycle data when source is shorter than target shape
- Empty arrays (iota 0) must be handled gracefully

### QR-2: Memory

- Interpreter code: target under 32 KB
- Must function with arrays totaling up to ~900 KB of heap
- Clean error on heap exhaustion (WS FULL), not crash

### QR-3: Usability

- Error messages must identify the error category
- REPL must recover from errors without restart
- Input editing: backspace support at minimum

## Test Strategy

- **Unit tests:** Each primitive operation tested with scalars,
  vectors, and matrices (run on emulator via cor24-run)
- **Integration tests:** Multi-operation expressions
- **Conformance tests:** Known APL expressions with expected output
- **Edge cases:** Empty arrays, single-element arrays, max-size arrays,
  division by zero, workspace full

## Success Criteria

1. Can evaluate `A <- iota 12` and display `0 1 2 3 4 5 6 7 8 9 10 11`
2. Can evaluate `3 4 rho A` and display a 3x4 matrix
3. Can evaluate `+/ iota 100` and display `4950`
4. Can evaluate `B <- 2 3 rho 10 20 30 40 50 60` and `rho B` displays `2 3`
5. Recovers from errors and continues accepting input
6. Runs on cor24-run with UART I/O
