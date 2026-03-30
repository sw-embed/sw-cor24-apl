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

Phase 2 complete. Phase 1 (scalar REPL) includes tokenizer, parser
(right-to-left recursive descent), tree-walking evaluator, and symbol
table. Phase 2 adds vector support: vector literals (stranding),
element-wise operations with scalar extension, conformability checks
(LENGTH ERROR), and right-justified vector output with consistent
column widths. Bump-allocated heap (4096 words) with per-iteration
reclamation for temporaries. Supports +, -, *, / (software divide),
parentheses, monadic negate (scalar and vector), integer literals with
APL underscore-negative convention, and variable assignment/reference
via `<-`. Errors: SYNTAX ERROR, DOMAIN ERROR (div by zero), VALUE
ERROR (undefined variable), LENGTH ERROR (vector size mismatch).
Implementation tracked via agentrail saga (`cor24-apl-interpreter`,
26 steps across 6 phases).

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
