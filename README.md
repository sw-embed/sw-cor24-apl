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

The `cor24-apl-interpreter` agentrail saga is complete (81 steps,
archived under `.agentrail-archive/`). The interpreter runs on the
COR24 emulator in both interactive (UART REPL) and batch (APL image)
modes. Supported features by area:

### Core language

- Right-to-left recursive descent parser; tree-walking evaluator
  split into per-node-type functions to keep frames shallow.
- Bump-allocated heap (4096 words) with per-iteration reclamation
  for temporaries.
- Integer literals with APL underscore-negative convention, `<-`
  assignment, symbol table, software divide.
- Error types: SYNTAX, DOMAIN, VALUE, LENGTH, RANK, WS FULL. REPL
  recovers cleanly on error.

### Data types

- Integer scalars and vectors (literal stranding).
- Character vectors / string literals.
- Rank-2 matrices with right-justified column display.
- Nested / boxed arrays.
- Lazy `iota` arrays for large ranges.

### Primitives

- Shape: `iota`, monadic/dyadic `rho`, `rev`, ravel and dyadic
  `cat`, `enclose`, monadic transpose.
- Selection: `take` / `drop` (scalar and multi-axis matrix forms,
  negative counts), bracket indexing (`V[i]`, vector index),
  `pick`, compress, replicate, `without`, `member`, dyadic-`iota`
  index-of, grade up / grade down, dyadic `rotate`.
- Arithmetic: `+ - * /`, integer exponent, abs/residue (`|`),
  signum, factorial / binomial (`!`), ceil/floor (dyadic max/min).
- Comparison and logic: `= != < <= > >=`, bitwise `and or not`.
- Higher-order: reduce (`f/`, including `or/` and `and/`),
  scan (`f\`), each (`f-each`), outer product (`.f`),
  inner product (`f.g`).
- Numeric conversion: encode / decode, `deal`, `roll` (PRNG),
  `fmt` (integer-to-string), `execute`.
- Set operations: `cup` (union / unique), `cap` (intersection).

### Control flow and functions

- `goto LABEL`, `goto (expr)/LABEL`, `goto 0`; `LABEL:` prefix
  labels stored as variables holding line numbers.
- Multi-line program buffer; `[N] expr` editing.
- User-defined monadic, dyadic, and niladic functions via
  `del R <- FN X ... del`; local variables via `;` syntax.
- `#` line comments to end of line.

### System interface

- Quad variables: `qio` (index origin), `qled`, `qsw`, `qrl`
  (PRNG seed), `qdl` (millisecond delay), `qout` (explicit print).
- `qsvo` shared-variable offer (AP 242 pattern) couples APL
  variables to hardware regions; indexed read/write on coupled
  variables; coupling-degree checks enable graceful degradation.
- System commands: `)CLEAR`, `)VARS`, `)OFF`.

### Batch mode

- APL image format (newline-separated, null-terminated),
  loaded via `cor24-run --load-binary program.apl@0x080000`
  with a `--patch` pointer.
- Batch / interactive mode auto-detected. UART stays free for
  the APL program via `qsvo`.
- Reference demo: UART echo implemented as a pure APL program.

### Tooling

- Automated batch test runner with expected-output comparison.
- GNU APL conformance comparison harness.

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

## Links

- Blog: [Software Wrighter Lab](https://software-wrighter-lab.github.io/)
- Discord: [Join the community](https://discord.com/invite/Ctzk5uHggZ)
- YouTube: [Software Wrighter](https://www.youtube.com/@SoftwareWrighter)

## License

Copyright (c) 2026 Michael A. Wright. MIT-licensed; see [LICENSE](LICENSE).
