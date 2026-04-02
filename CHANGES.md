# Changelog

## Step 049: Format Operator and .a24 Extension (2026-04-01)

- Implemented `fmt` monadic operator (`RES_FMT = 14`) for integer-to-string conversion
- `fmt N` converts scalar integer to character vector: `fmt 42` → `'42'`
- `fmt V` converts vector to space-separated string: `fmt 1 2 3` → `'1 2 3'`
- Handles negative numbers with underscore prefix: `fmt _7` → `'_7'`
- Key use case: `'ROUND ' cat fmt N` for string concatenation in horse race
- Renamed COR24 ASCII dialect samples from `.apl` to `.a24` extension
  - `.apl` reserved for GNU APL Unicode reference files
  - `.a24` for COR24 keyword-syntax files (samples, batch tests)
- Updated build.sh, test-cor24.sh, docs/batch-mode.md, docs/compatibility-testing.md
- Added `samples/23-fmt.a24` test file
- Horse race prerequisite: enables formatted output like `'Score: ' cat fmt 123`

## Step 048: PRNG and Roll Operator (2026-04-01)

- Implemented LCG pseudo-random number generator for 24-bit COR24 architecture
- LCG constants: a=1664525, c=12345, modulus 2^24 (natural overflow)
- Added `roll` reserved word (`RES_ROLL = 13`): monadic random number generation
- `roll N` returns random integer 1..N (APL 1-origin convention)
- Element-wise on vectors: `roll 6 6 6 6` returns 4 random dice rolls
- Added `qrl` quad variable (`TOK_QRL = 27`): read/write PRNG seed
- `qrl <- 42` sets deterministic seed, `qrl` reads current seed state
- Auto-seeding from UART keystroke counter (`io_key_count`) on first use
- Performance: uses bit extraction (`>> 8` + 12-bit mask) before modulo to
  avoid slow repeated-subtraction division on large values
- `NODE_QRL` and `NODE_QRL_ASSIGN` AST nodes in parser
- DOMAIN ERROR on `roll 0` or negative argument
- Added `samples/22-roll.apl` and `samples/batch-roll.apl` tests
- Deterministic: same seed produces same sequence (verified by re-seeding)
- Horse race prerequisite: enables random dice rolls for horse movement

## Step 047: Pick Operator (2026-04-01)

- Added `RES_PICK = 12` reserved word and `pick` keyword in tokenizer
- `pick` is dyadic: `I pick V` returns the I-th element from vector V
- Works on boxed vectors: `2 pick 'cat' 'dog' 'fish'` → `fish`
- Works on simple vectors: `1 pick 10 20 30` → `20`
- Preserves element type (char scalars from char vectors, heap refs from boxed)
- Bounds checking: INDEX ERROR on out-of-range, RANK ERROR on non-vector right arg
- Added `samples/21-pick.apl` and `samples/batch-pick.apl` tests (8 cases, all pass)
- All existing batch tests pass (0 regressions)
- Horse race prerequisite: enables indexing into vector-of-strings by horse number

## Step 046: Nested/Boxed Arrays (2026-04-01)

- Added `ARR_BOXED = 2` type constant for arrays whose elements are heap indices
- String stranding in parser: adjacent string literals (`'A' 'B' 'C'`) create
  a boxed vector where each element points to its character array on the heap
- `print_array()` handles boxed type: prints each element on its own line,
  delegating recursively for string vs numeric elements
- `rho` works on boxed vectors (returns element count, e.g. `rho 'a' 'b' 'c'` → 3)
- Assignment and variable reference work: `A <- 'hello' 'world'` stores boxed vector
- Added `samples/20-nested-arrays.apl` test (7 test cases, all pass)
- All existing batch tests pass (0 regressions)
- Horse race prerequisite: enables vector-of-strings for horse names

## Step 045: String Operations (2026-04-01)

- Dyadic `rho` now preserves character type: `5 rho '#'` produces `#####`
- Dyadic `cat` preserves character type when both operands are char:
  `'hi' cat ' world'` produces `hi world`
- Monadic `cat` (ravel) preserves character type on flatten
- Added `samples/19-string-ops.apl` (GNU APL) and `samples/27-string-ops.cor24` tests
- Tests: `N rho char`, cyclic fill (`3 rho 'ab'` → `aba`), `rho` of char result,
  catenation of strings, empty string cat
- All 31 tests pass (0 regressions)
- Horse race prerequisite: enables building commentary strings via rho/cat

## Step 044: Character Data Type (2026-04-01)

- Extended array header from 3 to 4 words: added type field (0=numeric, 1=char)
- `ARR_HDR` changed from 3 to 4; added `ARR_NUM`, `ARR_CHAR` constants
- Added `arr_type()` and `arr_set_type()` accessors in arr.h
- Parser: TOK_STRING now creates char vector literals (rank-1, type=char)
  with ASCII codes stored as data elements
- Preserved qsvo compatibility: `'NAME' qsvo AP` still resolves as identifier
- Updated `print_array()` in eval.h: char arrays print as raw characters
- Replaced inline result printing in main.c with `print_array()` call
- Added `can_start_expr` support for TOK_STRING in expression contexts
- Added `samples/18-char-type.apl` and `samples/26-char-type.cor24` tests
- `rho` on char vectors returns numeric length (correct APL behavior)
- All 30 tests pass (0 regressions from ARR_HDR change)
- Horse race prerequisite: enables string display for race commentary

## Step 043: Boolean Compress (2026-04-01)

- Implemented dyadic `compress` reserved word for boolean selection
- `RES_COMPRESS` added to tokenizer with `lookup_reserved` entry
- Added to `is_dyadic_res()` in parser for dyadic dispatch
- Evaluator: `MASK compress VECTOR` selects elements where mask is 1
- Supports vectors and scalars; LENGTH ERROR on size mismatch, RANK ERROR on rank > 1
- Empty result when all-zero mask (e.g., `0 0 0 compress 4 5 6` returns empty vector)
- Classic APL filter pattern works: `(3 = iota 5) compress iota 5` returns `3`
- Added `samples/17-compress.apl` (GNU APL) and `samples/batch-compress.apl`
- All 6 test cases pass including composed expression filtering
- Horse race prerequisite: enables selecting horses that meet criteria

## Step 042: Max/Min (ceil/floor) Primitives (2026-04-01)

- Implemented dyadic `ceil` (max) and `floor` (min) reserved words
- `RES_CEIL`, `RES_FLOOR` in tokenizer with `lookup_reserved` entries
- `TOK_CEIL`, `TOK_FLOOR` internal pseudo-tokens for reduce node values
- Evaluator: `eval_binop_scalar` handles max/min; `NODE_DYAD` dispatches
  ceil/floor with full scalar extension (scalar-scalar, vector-vector,
  scalar-vector, vector-scalar)
- Reduce: `ceil/` (max-reduce) and `floor/` (min-reduce) via parser
  detecting `TOK_RES` + `TOK_SLASH` for ceil/floor reserved words
- Added `samples/16-max-min.apl` (GNU APL) and `samples/batch-max-min.apl`
- All 14 test cases pass: dyadic, element-wise, scalar extension, reduce
- Horse race prerequisite: enables finding max/min of race positions

## Step 041: Comparison Operators (2026-04-01)

- Implemented comparison operators: `=`, `!=`, `<`, `>`, `<=`, `>=`
- Return integer 0 or 1 (APL boolean convention)
- Element-wise on vectors with scalar extension
- Tokenizer: `TOK_EQ`, `TOK_NE`, `TOK_LT`, `TOK_GT`, `TOK_LE`, `TOK_GE`
- Parser: `is_binop()` extended for comparison tokens
- Evaluator: `apply_binop()` handles all six comparisons
- Added `samples/15-comparison.apl` (GNU APL) and `samples/batch-comparison.apl`
- Output matches GNU APL exactly for all test cases
- Horse race prerequisite: enables `→(MMIORC<2)/NODEV` graceful degradation pattern

## Phase 9.3: UART Echo Demo (2026-04-01)

- Created `samples/uart-echo.apl`: full UART echo program using shared variables
- Program couples MMIO via `'MMIO' qsvo 242`, polls UART RX status, reads
  chars, toggles LED based on switch state, waits for TX not busy, echoes char
- Scalar-to-scalar reassignment optimization in eval: reuses existing heap slot
  instead of allocating, preventing WS FULL in tight program loops
- Program-mode heap reclamation: reclaims RHS temporaries after scalar
  reassignment when the variable's heap slot was reused
- String literal tokenizer (`TOK_STRING`): single-quoted `'ABC'` for qsvo names
- Parser support for string literals as identifiers in qsvo expressions
- Updated qsvo samples to use `'MMIO' qsvo 242` syntax (old bare-ident still works)
- Verified: echo works with `--terminal`, TX busy polling works with
  `--uart-never-ready`, LED toggles correctly with `--switch on/off`

## Phase 9.2: Batch Mode Detection and Reader (2026-04-01)

- Implemented batch mode: at startup, reads 24-bit image pointer at 0x09FF00
- If non-zero, reads APL lines from SRAM image instead of UART
- `batch_getline()` reads newline-separated lines from memory, null-terminated
- End of image switches to interactive REPL mode (unless `)OFF` halts first)
- UART remains free for APL program use via shared variables (□SVO)
- Interactive mode completely unchanged when no image loaded
- Added batch test infrastructure to `test-cor24.sh`
- New batch test samples: `batch-hello`, `batch-vars`, `batch-functions`

## Phase 9.1: APL Image Format (2026-04-01)

- Defined APL image format: newline-separated lines, null-terminated in SRAM
- Image pointer at 0x09FF00, image data at 0x080000
- Updated `build.sh` with `--batch <file.apl>` option for loading images
- Uses `cor24-run --load-binary` and `--patch` to load image and set pointer
- Updated memory layout in architecture.md to show image region
- Created `docs/batch-mode.md` with format specification
- Added `samples/batch-hello.apl` sample image
- Constants `APL_IMAGE_PTR` and `APL_IMAGE_BASE` defined in main.c

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
