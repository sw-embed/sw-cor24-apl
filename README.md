# sw-cor24-apl

APL interpreter for the COR24 24-bit RISC architecture.

## Overview

A minimal APL interpreter targeting the COR24 ISA, written in C
(compiled via tc24r) with assembly helpers. Runs interactively
over UART on the COR24 emulator.

Uses ASCII surface syntax with lowercase reserved words (`rho`,
`iota`, `take`, `drop`) for APL operators and uppercase for user
identifiers.

## Documentation

- [Product Requirements](docs/prd.md)
- [Architecture](docs/architecture.md)
- [Design Decisions](docs/design.md)
- [Implementation Plan](docs/plan.md)
- [APL Research Notes](docs/research.txt)

## Building

Requires `tc24r` (C compiler) and `cor24-run` (emulator) on PATH.

```bash
./build.sh              # compile only
./build.sh run          # compile and run on emulator
./build.sh run --terminal   # interactive REPL mode
./build.sh clean        # remove build artifacts
```

## Status

Phase 4 in progress. Phase 1 (scalar REPL) includes tokenizer, parser
(right-to-left recursive descent), tree-walking evaluator, and symbol
table. Phase 2 adds vector support: vector literals (stranding),
element-wise operations with scalar extension, conformability checks
(LENGTH ERROR), and right-justified vector output with consistent
column widths. Phase 3 adds core APL primitives: monadic `iota N`
(generates vector 0..N-1), monadic `rho` (shape-of: returns array
dimensions), and dyadic `rho` (reshape with cyclic fill, supports
vector and matrix results). Parser extended with `NODE_DYAD` AST type
for dyadic primitive functions (rho, take, drop, cat). Bump-allocated
heap (4096 words) with per-iteration reclamation for temporaries.
Phase 3.3 adds reduce operators (`+/`, `-/`, `*/`) with right-to-left
reduction over vectors (e.g., `-/ 1 2 3` = `1-(2-3)` = 2). Scalar
passthrough for reduce on scalars. Phase 3.4 adds dyadic `take` and
`drop` with negative-N support (`_2 take iota 5` -> `3 4`,
`2 drop iota 5` -> `2 3 4`). Phase 3.5 adds monadic `rev` (reverse vector), monadic `cat` (ravel/flatten
to 1D), and dyadic `cat` (catenate arrays). Phase 4.1 adds matrix creation and display: `2 3 rho iota 6` creates
a 2x3 matrix displayed one row per line with right-justified columns.
Monadic `rho` on matrices returns 2-element shape vector. Monadic `cat`
(ravel) flattens matrices to 1D vectors. Phase 4.3 adds element-wise
operations on matrices (matrix+matrix, scalar+matrix, matrix*scalar),
matrix negate, and conformability checks for mismatched matrix shapes.
Phase 4.4 adds `take` and `drop` on matrices operating on rows
(`1 take M` = first row, `_1 drop M` = all but last row). Quad output (`[] <- expr`)
provides explicit I/O following IBM 5100 conventions. Supports +, -, *,
/ (software
divide), parentheses, monadic negate (scalar and vector), integer
literals with APL underscore-negative convention, and variable
assignment/reference via `<-`. Errors:
SYNTAX ERROR, DOMAIN ERROR (div by zero), VALUE ERROR (undefined
variable), LENGTH ERROR (vector size mismatch). Implementation tracked
via agentrail saga (`cor24-apl-interpreter`).

## Example (target syntax)

```
      A <- iota 12
0 1 2 3 4 5 6 7 8 9 10 11
      3 4 rho A
0  1  2  3
4  5  6  7
8  9  10 11
      +/ A
66
```

## License

See [LICENSE](LICENSE).
