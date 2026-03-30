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

Phase 1 in progress. Tokenizer complete -- lexes input into token
stream with NUM, IDENT, RESERVED, operators, ASSIGN, parens, EOL.
Reserved word lookup for `rho`, `iota`, `take`, `drop`, `rev`, `cat`.
Parser and evaluator are next. Implementation tracked via agentrail
saga (`cor24-apl-interpreter`, 26 steps across 6 phases).

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
