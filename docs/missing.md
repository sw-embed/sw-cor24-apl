# Missing Features and Deferred Work

## Planned (in saga)

### Step 059: Local Variables
The semicolon local-variable syntax (`del R assign FN X;LOCAL1;LOCAL2`)
is not implemented. All variables inside functions are currently global.
Only result, right arg, and left arg are saved/restored on the call stack.

## Deferred (requires hardware or library support)

### Domino (⌹) — Matrix Divide / Inverse
APL's domino operator provides:
- **Monadic**: matrix inverse (`⌹M`)
- **Dyadic**: linear equation solver (`B⌹A` solves Ax=B)
- Over-determined systems use least-squares fitting

**Requirements:**
- Floating-point arithmetic (software float library or FPU)
- Gaussian elimination or Householder transformation algorithm
- Inner product operator (`+.×`) for matrix multiplication
- Matrix transpose

**Status:** Deferred until a software floating-point library is available
for COR24. Integer-only matrix operations are insufficient for meaningful
linear algebra — domino results are almost always non-integer.

### Delay (⎕DL)
APL's `⎕DL N` pauses execution for N seconds. Previously implemented as
`qdl` using a calibrated spin loop, but removed because COR24-TB has no
hardware clock or timer. Will be re-added when the next hardware revision
(or emulator mode) provides a timer peripheral.

### Matrix Transpose
Monadic `⍉` (circle-backslash) transposes rows and columns. Not yet
implemented. Prerequisite for domino.

### Inner Product
The `f.g` operator (e.g., `+.×` for matrix multiplication) is needed
for linear algebra. Not yet implemented. Prerequisite for domino.
