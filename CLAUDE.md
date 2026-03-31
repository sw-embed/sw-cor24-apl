# sw-cor24-apl -- Claude Instructions

## Project Overview

APL interpreter for the COR24 24-bit RISC ISA. Written in C (via
tc24r cross-compiler) with assembly helpers, targeting the COR24
emulator. Uses ASCII surface syntax with lowercase reserved words
(rho, iota, take, drop) and uppercase user identifiers.

See `docs/` for detailed documentation:
- `docs/prd.md` -- requirements
- `docs/architecture.md` -- system architecture
- `docs/design.md` -- design decisions
- `docs/plan.md` -- phased implementation plan
- `docs/research.txt` -- APL history and design research

## Build / Test

This project uses tc24r (C compiler) and cor24-run (emulator):

```bash
./build.sh              # compile only
./build.sh run          # compile and run on emulator
./build.sh run --terminal   # interactive REPL mode
./build.sh clean        # remove build artifacts
```

Toolchain repos (siblings under `~/github/sw-embed/`):
- `sw-cor24-tinyc` -- C compiler (tc24r)
- `sw-cor24-emulator` -- emulator + assembler (cor24-run)

## COR24 Quick Reference

- 24-bit RISC, 8 registers (r0-r2 GP, fp, sp, z, iv, ir)
- 1 MB SRAM + 8 KB EBR stack
- UART at 0xFF0100 (data), 0xFF0101 (status)
- No hardware divide -- use software division
- Variable-length instructions (1, 2, or 4 bytes)
- Calling convention: args on stack, return in r0, link in r1
