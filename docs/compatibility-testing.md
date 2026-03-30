# Compatibility Testing

## Goal

Verify that COR24 APL produces the same results as a reference APL
implementation for a shared subset of the language. This catches
semantic bugs (wrong evaluation order, off-by-one in indexing, incorrect
reduce direction) that unit tests might miss.

---

## Reference Implementation

**GNU APL** is the primary reference. It is freely available, runs on
Linux/macOS, and implements ISO 13751 (Programming Language APL,
Extended). Install via:

```bash
# macOS
brew install gnu-apl

# Ubuntu/Debian
apt install apl
```

We do not have access to an IBM 5100 or other IBM APL implementation
for live comparison. IBM APL conventions (especially around quad
variables) are validated against **historical documentation and program
listings only**, not a running system.

---

## APL Program Sources for Test Cases

Beyond hand-written test cases, look for working APL programs from
online sources to use as compatibility tests:

- **Rosetta Code** (rosettacode.org) -- hundreds of APL solutions for
  well-defined problems with expected outputs. Good source for
  non-trivial programs that exercise multiple language features.
- **APL Wiki** (aplwiki.com) -- idiom lists, code examples, and
  explanations of standard APL behavior.
- **Dyalog APL problem sets** -- annual programming competitions with
  solutions and expected outputs.
- **tryapl.org** -- Dyalog's online REPL; useful for quick verification
  of individual expressions.
- **GNU APL documentation and test suite** -- ships with its own
  conformance tests.

When adapting programs from these sources:
1. Transliterate Unicode APL glyphs to our ASCII keyword syntax
2. Remove features we don't support (floating point, complex numbers,
   higher-rank arrays, operators beyond reduce)
3. Verify the original produces expected output in GNU APL first
4. Then run the transliterated version on COR24 APL

---

## Syntax Translation

COR24 APL uses ASCII keywords where GNU APL uses Unicode glyphs.
Test files must be maintained in two forms:

### GNU APL form (`test.apl`)
```apl
A ← ⍳10
⎕ ← +/A
⎕ ← 2 3⍴⍳6
⎕ ← 3↑A
⎕ ← ¯2↑A
```

### COR24 form (`test.cor24.apl`)
```
A <- iota 10
[] <- +/ A
[] <- 2 3 rho iota 6
[] <- 3 take A
[] <- _2 take A
```

### Translation rules

| GNU APL | COR24 APL | Notes |
|---------|-----------|-------|
| `←` | `<-` | Assignment |
| `⍴` | `rho` | Shape/reshape |
| `⍳` | `iota` | Index generator |
| `↑` | `take` | Take |
| `↓` | `drop` | Drop |
| `⌽` | `rev` | Reverse |
| `,` | `cat` | Catenate (dyadic only) |
| `⌈` | `ceil` | Maximum |
| `⌊` | `floor` | Minimum |
| `⎕←` | `[] <-` | Quad output |
| `¯` | `_` | Negative sign |
| `∧` | `and` | Logical AND |
| `∨` | `or` | Logical OR |
| `~` | `not` | Logical NOT |
| `?` | `roll` | Random |
| `⍕` | `fmt` | Format |

A transliteration script could automate this, but manual translation
is fine for now given the small test corpus.

---

## Test Harness

### Running GNU APL in batch mode

```bash
# Feed APL script to GNU APL, capture output
echo ')OFF' >> test.apl
apl --script -f test.apl > expected.txt 2>/dev/null
```

### Running COR24 APL in batch mode

```bash
# Convert newlines to \n escapes for --uart-input
INPUT=$(cat test.cor24.apl | sed 's/$/\\n/' | tr -d '\n')
cor24-run --run build/apl.s --uart-input "$INPUT" \
    -t 60 -s 1000000 2>&1 \
    | grep 'UART output' -A 1000 \
    | tail -n +2 \
    > actual.txt
```

### Comparing results

```bash
diff expected.txt actual.txt
```

Output formatting may differ (whitespace, column alignment), so a
normalized comparison may be needed:

```bash
# Strip leading whitespace and blank lines for comparison
normalize() { sed 's/^[[:space:]]*//' | grep -v '^$'; }
diff <(normalize < expected.txt) <(normalize < actual.txt)
```

---

## Known Differences from GNU APL

Document deviations here as they are discovered.

| Feature | GNU APL | COR24 APL | Reason |
|---------|---------|-----------|--------|
| Index origin | `⎕IO` default 1 | Always 0 | Design decision D3; C convention |
| Number format | Float (`3.0`) | Integer only | No FPU on COR24 |
| Negative sign | `¯` (U+00AF) | `_` (underscore) | ASCII encoding |
| Max rank | Unlimited | 2 (matrix) | Memory constraints |
| Quad syntax | `⎕` (U+2395) | `[]` | ASCII encoding |
| Empty vector display | (blank line) | (blank line) | Should match |
| Division | Float result | Integer truncation | No FPU |
| System commands | `)OFF`, etc. | Same names | Should match |

### Index origin impact

GNU APL defaults to `⎕IO←1` (1-origin). COR24 APL uses 0-origin.
This affects:

- `⍳N` / `iota N`: GNU gives `1 2 ... N`, COR24 gives `0 1 ... N-1`
- `A[1]`: GNU gives first element, COR24 gives second
- Bracket axis specifications

**When writing comparison tests**, either:
1. Set `⎕IO←0` at the start of the GNU APL script, or
2. Account for the offset in expected output

```apl
⍝ GNU APL: set 0-origin to match COR24
⎕IO←0
⎕←⍳5
0 1 2 3 4
```

---

## Test Categories

### Category 1: Arithmetic (scalar)
```
3 + 4           -> 7
2 * 3 + 4       -> 14  (right-to-left!)
10 - 3          -> 7
_5 + 3          -> _2
```

### Category 2: Vector operations
```
1 2 3 + 10 20 30   -> 11 22 33
5 + 1 2 3           -> 6 7 8
iota 5               -> 0 1 2 3 4
rho 1 2 3            -> 3
```

### Category 3: Reduce
```
+/ 1 2 3 4 5      -> 15
-/ 1 2 3           -> 2
*/ 1 2 3 4         -> 24
+/ iota 10         -> 45
```

### Category 4: Take and drop
```
3 take iota 10     -> 0 1 2
_2 take iota 5     -> 3 4
2 drop iota 5      -> 2 3 4
_1 drop iota 5     -> 0 1 2 3
```

### Category 5: Reshape
```
3 rho 7            -> 7 7 7
5 rho 1 2 3        -> 1 2 3 1 2
rho iota 0         -> 0
```

### Category 6: Assignment and variables
```
A <- 5
A + 3              -> 8
B <- iota 5
+/ B               -> 10
```

### Category 7: Quad output
```
[] <- 42           -> 42  (prints and returns)
[] <- 1 2 3        -> 1 2 3
```

---

## Automation (Future)

When the test corpus grows, automate with a script:

```bash
#!/bin/bash
# test/run-compat.sh
PASS=0; FAIL=0
for test in test/compat/*.cor24.apl; do
    base="${test%.cor24.apl}"
    expected="$base.expected"
    actual=$(run_cor24 "$test")
    if diff -q <(echo "$actual") "$expected" > /dev/null 2>&1; then
        PASS=$((PASS + 1))
    else
        echo "FAIL: $test"
        diff <(echo "$actual") "$expected"
        FAIL=$((FAIL + 1))
    fi
done
echo "$PASS passed, $FAIL failed"
```

This infrastructure should be built when step 025 (conformance test
suite) is reached in the saga.
