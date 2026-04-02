# APL Samples

Sample APL programs for testing COR24 APL against GNU APL.

## Naming Convention

- `*.a24` — COR24 ASCII APL samples (keyword syntax, 0-indexed iota)
- `*.apl` — GNU APL scripts (Unicode symbols, 1-indexed ⍳)
- `*.cor24` — COR24 APL input (ASCII keywords, 0-indexed iota)
- `*.expected` — Expected output for COR24 tests

## Key Differences: COR24 vs GNU APL

| Feature | COR24 APL | GNU APL |
|---------|-----------|---------|
| iota | 0-indexed (`iota 5` → `0 1 2 3 4`) | 1-indexed (`⍳5` → `1 2 3 4 5`) |
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

COR24 APL currently uses 0-indexed iota (⎕IO←0). GNU APL samples
include `⎕IO←0` to match. A future step will add `[]IO` support
with default 1 (standard APL) and `[]IO <- 0` option.

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
