# Changelog

## Phase 8.3: User-Defined Functions (2026-04-01)

- Implemented `del` keyword (ASCII for ∇) to define user functions
- Monadic functions: `del R <- FN X` ... `del` (e.g., DOUBLE)
- Dyadic functions: `del R <- X FN Y` ... `del` (e.g., ADD)
- Recursive functions supported with call stack depth up to 8 levels
- Local variables: result, right arg, left arg saved/restored per call frame
- AST and token state saved/restored around function body execution
  to support function calls within larger expressions
- Labels and `goto` branching work within function bodies
- Quad output (`[] <- expr`) works inside functions for explicit output
- `goto 0` exits function early (returns result if set)
- Function redefinition supported (re-entering `del` with same name overwrites)
- `)FNS` system command lists defined function names
- `)CLEAR` resets function table along with variables and heap
- New files: `src/fn.h` (function table, header parser, label scanner)
- New AST node: `NODE_FNCALL` with monadic/dyadic dispatch in parser
- `eval_fncall()` separated from `eval()` to reduce stack frame size
- Added test sample 25-user-functions: DOUBLE, ADD, FACT (recursive), nested calls
- All 26 tests pass

## Phase 8.2: Multi-line Program Storage (2026-04-01)

- Implemented `[N] expr` syntax to store program lines at specific line numbers
- `)LIST` displays all stored program lines with `[N]` prefix
- `)RUN` executes stored program from line 1 with pre-scanned labels
- `)ERASE` clears all stored program lines
- Labels pre-scanned before execution so forward branches resolve correctly
- Errors during program execution display offending line number (e.g., `SYNTAX ERROR [3]`)
- `goto 0` in program mode returns to REPL (not interpreter exit)
- Sparse line numbering supported (gaps skipped during execution)
- Line replacement by re-entering same line number
- Converted branch/label tests (22, 23) from immediate-mode auto-store to `[N]` program entry
- Added test sample 24-multiline: stored program with loop, )LIST, )RUN, )ERASE
- All 25 tests pass

## Phase 8.1: Branch and Labels (2026-03-31)

- Implemented `goto` reserved word (ASCII surface syntax for APL →)
- Unconditional branch: `goto LABEL` branches to labeled line
- Conditional branch: `goto (expr)/LABEL` branches if expr is nonzero
- `goto 0` exits the interpreter (APL →0 convention)
- Label definition: `LABEL:` at start of line sets label to current line number
- Labels are stored as regular variables (value = line number), following APL convention
- Program line buffer (64 lines) stores input for backward branching (loops)
- PC-based execution: replays stored lines when branching backward
- Forward branches require label to be defined first (pre-scan not yet implemented)
- Added test sample 22-branch-labels: loop counting 1 to 10 with backward goto
- Added test sample 23-goto-variants: sum accumulation loop, goto 0 exit
- All 24 tests pass

## Phase 7.2: Bitwise Operations (2026-03-31)

- Implemented `and` (bitwise AND), `or` (bitwise OR), `not` (bitwise NOT)
- Reserved words like `rho`, `iota` — lowercase syntax
- Dyadic: `128 and 130` returns 128, `3 or 4` returns 7
- Monadic: `not 255` returns complement (_256 on 24-bit)
- Element-wise on vectors with scalar extension:
  `1 2 3 or 4` returns `5 6 7`, `15 and 3 7 15` returns `3 7 15`
- Needed for UART status bit testing (`MMIO[257] and 2` checks TX ready)
- Added test sample 21-bitwise with expected output (22 tests total)

## Phase 7.1: Bracket Indexing on Vectors (2026-03-31)

- Implemented `V[N]` read and `V[N] <- expr` write on regular vectors
- 0-origin indexing: `A <- 10 20 30` then `A[1]` returns 20
- Works in expression context: `A[0] + A[2]`, `B <- A[1] + 5`
- In-place mutation: `A[1] <- 99` modifies the vector
- Out-of-bounds index produces LENGTH ERROR (clean fail)
- Unified bracket syntax: same `IDENT[N]` for both SVO and vector access
  - SVO variables (coupled via qsvo) route to MMIO hardware
  - Regular variables route to vector element access
  - Uncoupled SVO variables still produce VALUE ERROR (graceful degradation preserved)
- Added test sample 20-bracket-index with expected output (21 tests total)

## Phase 6.6: Graceful Degradation (2026-03-31)

- Validated graceful degradation pattern for portable APL code
- Coupling degree return (2=coupled, 0=unsupported) enables AP detection
- Arithmetic on coupling degree works: `RC - 2` yields 0 when coupled
- Uncoupled variables produce VALUE ERROR on indexed access (clean fail)
- Re-coupling after failed offer works correctly
- Full `→(MMIORC<2)/NODEV` branch-around requires comparison ops (step 041)
  and branch/labels (step 035) — building blocks validated here
- Added test sample 19-graceful-degrade with expected output (20 tests total)

## Phase 6.5: Shared Variable Indexed Read/Write (2026-03-31)

- Implemented bracket-indexed read/write on shared variables
- `MMIO[N]` reads byte at `0xFF0000+N` after coupling via `qsvo`
- `MMIO[N] <- expr` writes byte at `0xFF0000+N`
- 0-origin offsets regardless of future `□IO` setting
- Works in expression context: `A <- MMIO[257]`, `MMIO[257] + 10`
- Uncoupled variables produce VALUE ERROR on indexed access
- Added `TOK_LBRAK`/`TOK_RBRAK` tokens for bracket syntax
- New AST nodes: `NODE_SVO_READ`, `NODE_SVO_WRITE`
- Added test sample 18-svo-index with expected output (19 tests total)

## Phase 6.4: Shared Variable Offer — qsvo (2026-03-31)

- Implemented `qsvo` (□SVO) system function for shared variable coupling
- Dyadic syntax: `MMIO qsvo 242` couples variable MMIO to AP 242 (MMIO region)
- Returns coupling degree: 2 for supported APs (242), 0 for unknown APs
- Shared variable table (`svo_ap[]`) tracks AP coupling per symbol
- AP 242 maps to COR24 MMIO byte region (0xFF0000+offset)
- Result usable in expressions: `MMIORC <- MMIO qsvo 242`
- Added "CPU halted" filter to test runner for clean `)OFF` handling
- Added test sample 17-qsvo with expected output (18 tests total)

## Phase 6.3: Quad Expression Contexts (2026-03-31)

- Verified quad variables work in arbitrary expression contexts
- Cross-variable: `qled <- qsw` copies switch state to LED
- Toggle pattern: `qled <- 1 - qled` flips LED state
- Arithmetic: `qsw + qled`, `qled + 100`, `qled * 3`
- Variable capture: `A <- qled + qsw`, `B <- qled * 3`
- Self-assignment: `qled <- qled` (identity through shadow register)
- Added test sample 16-quad-exprs with expected output

## Phase 6.2: Quad Switch (2026-03-31)

- Added `qsw` quad system variable for switch S2 hardware I/O (read-only)
- `qsw` reads switch state: 1=pressed, 0=released (active-low inversion)
- Assignment to `qsw` produces SYNTAX ERROR (read-only variable)
- Works in expressions: `qsw + 5`, `A <- qsw`
- Added test sample 15-qsw with expected output

## Phase 6.1: Quad LED (2026-03-30)

- Added `qled` quad system variable for LED D2 hardware I/O
- `qled` reads current LED state (0=off, 1=on), `qled <- 1` turns LED on
- Active-low inversion handled transparently (hardware 0xFF0000 is active-low)
- Shadow register for read-back (hardware LED register is write-only)
- Fixed build.sh include path for renamed tinyc repo (sw-cor24-x-tinyc)
- Added test sample 14-qled with expected output

## Fork Migration (2026-03-30)

- Forked from `softwarewrighter/a24a` to `sw-embed/sw-cor24-apl`
- Added `scripts/build.sh` wrapper for ecosystem consistency
- Updated `build.sh` include path to reference `sw-cor24-tinyc`
- Removed legacy `.agentrail` metadata
- Cleaned up untracked working files
