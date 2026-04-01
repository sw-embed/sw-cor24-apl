# COR24 APL Interpreter -- Architecture

## Overview

A minimal APL interpreter targeting the COR24 24-bit RISC ISA, written
in COR24 assembly (with possible C bootstrap via tc24r). Uses ASCII
surface syntax with lowercase reserved words for APL operators and
uppercase for user identifiers. Runs interactively via UART REPL on
the COR24 emulator.

## System Context

```
User (UART terminal)
  |
  v
+---------------------------+
| APL Interpreter           |
|  +---------------------+  |
|  | REPL                |  |
|  | Tokenizer/Lexer     |  |
|  | Parser (R-to-L)     |  |
|  | Evaluator           |  |
|  | Array Engine         |  |
|  | Memory Manager       |  |
|  +---------------------+  |
+---------------------------+
  |
  v
COR24 CPU (emulator or FPGA)
  - 24-bit registers (r0-r2, fp, sp, z, iv, ir)
  - 1 MB SRAM + 8 KB EBR stack
  - UART at 0xFF0100
```

## Target Platform Constraints

| Resource          | Specification                          |
|-------------------|----------------------------------------|
| Word size         | 24 bits                                |
| Registers         | 3 GP (r0-r2), fp, sp, z, iv, ir       |
| SRAM              | 1 MB (0x000000-0x0FFFFF)              |
| Stack (EBR)       | 8 KB (0xFEE000-0xFEFFFF)              |
| I/O               | UART at 0xFF0100, LED at 0xFF0000     |
| Arithmetic        | add, sub, mul (no hardware divide)     |
| Instruction sizes | 1, 2, or 4 bytes (variable-length)     |

## Component Architecture

### 1. REPL (Read-Eval-Print Loop)

- Reads a line from UART into an input buffer
- Passes buffer to tokenizer
- Displays result via UART
- Prompt: 6 spaces (traditional APL indent)
- Error messages: descriptive, printed before re-prompting

### 2. Tokenizer / Lexer

Scans input buffer left-to-right, producing a token stream.

**Token categories:**

| Category       | Pattern             | Examples              |
|----------------|---------------------|-----------------------|
| Number literal | `[0-9]+`            | `42`, `0`, `999`      |
| Negative num   | `_[0-9]+`           | `_3` (negative 3)     |
| Identifier     | `[A-Z][A-Z0-9_]*`  | `A`, `FOO`, `MAT1`   |
| Reserved word  | `[a-z][a-z]*`       | `rho`, `iota`, `take` |
| Operator       | single char         | `+`, `-`, `*`, `/`   |
| Assignment     | `<-`                | `A <- 5`              |
| Parens         | `(`, `)`            | grouping              |
| Slash-op       | `+/`, `*/`, etc.    | reduce operators      |
| Separator      | newline / EOL       | end of expression     |

**Reserved word table (initial):**

| Token     | Monadic meaning | Dyadic meaning | APL glyph |
|-----------|-----------------|----------------|-----------|
| `rho`     | shape-of        | reshape        | rho       |
| `iota`    | index generate  | index-of       | iota      |
| `take`    | take            | take           | up-arrow  |
| `drop`    | drop            | drop           | dn-arrow  |
| `rev`     | reverse         | rotate         | circle-bar|
| `cat`     | ravel           | catenate       | comma     |
| `gradeup` | grade up        | --             | delta-bar |
| `gradedn` | grade down      | --             | del-bar   |

### 3. Parser

APL has **no operator precedence** -- evaluation is strictly
**right-to-left**. This dramatically simplifies parsing.

**Grammar (simplified):**

```
line       = ident '<-' expr | expr
expr       = term | term op expr
term       = number | ident | '(' expr ')' | numbers
numbers    = number number+          (vector literal)
op         = '+' | '-' | '*' | reserved_word | ...
reduce_op  = op '/'
```

**Key rule:** When the parser sees `A op B op C`, it parses as
`A op (B op C)`. No precedence table needed.

**Implementation:** Recursive descent, scanning right-to-left within
an expression. Alternatively, build a list of tokens and process
from right end.

### 4. Evaluator

Tree-walking evaluator that processes the parsed expression:

- **Scalar ops:** Apply arithmetic element-wise
- **Monadic functions:** One argument (right side)
- **Dyadic functions:** Two arguments (left and right)
- **Reduce:** Apply binary op across a vector: `+/ 1 2 3` = 6
- **Assignment:** Bind result to name in symbol table

**Arity detection:** If a function token has a value to its left,
it is dyadic; otherwise monadic. This is determined at parse time
by context (does a value precede the operator?).

### 5. Array Engine

Core data structure -- every value is an array:

```
Array:
  rank:   uint8    (0=scalar, 1=vector, 2=matrix)
  shape:  uint24[4] (dimensions, max rank 4)
  size:   uint24   (total element count)
  data:   int24[]  (flat row-major storage)
```

In memory (24-bit words):

```
Offset 0: rank (packed with flags)
Offset 3: shape[0]
Offset 6: shape[1]
Offset 9: shape[2]
Offset 12: shape[3]
Offset 15: size
Offset 18: data[0]
Offset 21: data[1]
...
```

**Scalar:** rank=0, shape=[], size=1, data=[value]

**Vector:** rank=1, shape=[n], size=n, data=[v0,v1,...,vn-1]

**Matrix:** rank=2, shape=[r,c], size=r*c, data=[row-major]

**Conformability rules:**
- Scalar extends to match any shape (scalar extension)
- Dyadic ops require matching shapes (or one scalar)
- `rho` reshapes data to new shape (cyclic fill)

### 6. Symbol Table

Simple linear-scan table for variable bindings:

```
Entry:
  name:  char[16]   (null-terminated, uppercase)
  value: ptr -> Array
```

Maximum ~64 variables initially. Sufficient for interactive use.

### 7. Memory Manager

**Phase 1: Bump allocator**
- Heap grows upward from a base address
- No deallocation
- Reset on workspace clear (`)CLEAR`)

**Phase 2: Region-based**
- Per-expression temporary region
- Results promoted to workspace region
- Temporaries freed after each line

**Phase 3 (optional): Mark-sweep GC**
- Walk symbol table as roots
- Mark reachable arrays
- Sweep and compact

**Memory layout:**

```
0x000000  +------------------+
          | Code (interpreter)|
          +------------------+
~0x008000 | Static data       |
          | (token tables,    |
          |  reserved words)  |
          +------------------+
~0x010000 | Symbol table      |
          +------------------+
~0x012000 | Heap (arrays)     |
          |   grows upward    |
          |       ...         |
          +------------------+
0x080000  | APL image (batch) |
          | (loaded via       |
          |  --load-binary)   |
          +------------------+
0x09FF00  | Image pointer     |
          +------------------+
0x0FFFFF  | End of SRAM       |
          +------------------+
0xFEE000  | Stack (EBR)       |
          |   grows downward  |
0xFEFFFF  +------------------+
```

## Data Flow

```
Input: "A <- 2 3 rho iota 6"

1. Tokenize: [IDENT:A] [ASSIGN] [NUM:2] [NUM:3] [RHO] [IOTA] [NUM:6]

2. Parse (right-to-left):
   Assignment(A,
     Dyadic(RHO, Vector(2,3),
       Monadic(IOTA, Scalar(6))))

3. Evaluate (inside-out):
   iota 6        -> [0 1 2 3 4 5]
   2 3 rho [..] -> [[0 1 2] [3 4 5]]  (2x3 matrix)

4. Assign: A = result

5. Print:
   0 1 2
   3 4 5
```

## Error Handling

- **SYNTAX ERROR** -- malformed expression
- **LENGTH ERROR** -- non-conformable array shapes
- **RANK ERROR** -- operation not supported for given rank
- **DOMAIN ERROR** -- invalid argument (e.g., division by zero)
- **WS FULL** -- heap exhausted

Errors print a message and return to the REPL prompt. No exceptions
or unwinding -- just abandon the current expression.

## Implementation Language Decision

**Option A: COR24 Assembly (recommended for authenticity)**
- Follows the tradition of APL\360 (hand-written assembler)
- Full control over memory layout
- Matches tf24a (Forth) approach
- Harder to write but educational

**Option B: C via tc24r (recommended for productivity)**
- Faster development
- Proven toolchain (tc24r compiles to COR24 asm)
- Matches tml24c (Lisp) approach
- Easier to extend and debug

**Option C: Hybrid**
- Core interpreter loop in C
- Performance-critical array ops in assembly
- Best of both worlds

**Recommendation:** Start with C (tc24r) for the tokenizer, parser,
evaluator, and REPL. Implement array primitives (element-wise ops,
iota, rho) in assembly for performance. This mirrors the historical
pattern where later APL implementations moved from pure assembler
to C cores with assembly inner loops.

## Output Formatting

- Scalars: print value, newline
- Vectors: space-separated values, newline
- Matrices: one row per line, right-justified columns
- Column width: determined by widest element in each column
