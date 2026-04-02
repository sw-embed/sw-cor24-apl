# APL Samples

Sample APL programs for testing COR24 APL against GNU APL.

## Naming Convention

- `*.a24` — COR24 ASCII APL samples (keyword syntax, default 1-indexed iota)
- `*.apl` — GNU APL scripts (Unicode symbols, 1-indexed ⍳)
- `*.cor24` — COR24 APL input (ASCII keywords, default 1-indexed iota)
- `*.expected` — Expected output for COR24 tests

## Key Differences: COR24 vs GNU APL

| Feature | COR24 APL | GNU APL |
|---------|-----------|---------|
| iota | 1-indexed by default (`iota 5` → `1 2 3 4 5`), `qio <- 0` for 0-origin | 1-indexed (`⍳5` → `1 2 3 4 5`) |
| rho | `rho` keyword | `⍴` symbol |
| take | `take` keyword | `↑` symbol |
| drop | `drop` keyword | `↓` symbol |
| rev | `rev` keyword | `⌽` symbol |
| cat | `cat` keyword (ravel/catenate) | `,` symbol |
| reduce | `+/` `-/` `*/` | `+/` `-/` `×/` |
| negate | `_5` (underscore prefix) | `¯5` (high minus) |
| assign | `A <- 5` | `A ← 5` |
| quad out | `[] <- expr` | `⎕ ← expr` |

## Index Origin

COR24 APL defaults to 1-indexed iota (⎕IO=1), matching standard APL.
Use `qio <- 0` for 0-origin. GNU APL samples no longer need `⎕IO←0`
since both interpreters now default to 1-origin.

## Running

```bash
# Run all GNU APL reference tests:
./samples/test-gnu-apl.sh

# Run a specific GNU APL test:
./samples/test-gnu-apl.sh 09

# Run COR24 regression tests:
./samples/test-cor24.sh

# Run a specific COR24 test:
./samples/test-cor24.sh 09
```
