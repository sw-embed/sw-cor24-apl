# Character Encoding and APL Symbol Representation

## Decision Summary

COR24 APL uses **ASCII-only source encoding** with **lowercase English
keywords** for APL primitives. Unicode APL glyphs are a **display-layer
concern** handled by editor ligatures (e.g., Emacs prettify-symbols-mode),
not by the interpreter or wire protocol.

This follows the precedent set by the IBM 5100 (internal EBCDIC-like
encoding, custom display font) and many ASCII APL systems of the 1970s-80s.

---

## Why Not Unicode?

### COR24 hardware constraints

- **UART is 8-bit.** One byte per transfer. UTF-8 requires 1-4 bytes
  per codepoint, so multi-byte accumulator logic would be needed on
  both TX and RX paths.

- **COR24 words are 24-bit.** A single word *can* hold any Unicode
  codepoint (max U+10FFFF = 21 bits), but UTF-8 *encoding* is
  variable-width. Storing codepoints as fixed 24-bit values wastes
  memory for ASCII text. Storing UTF-8 bytes breaks element indexing
  (`3 take 'cafe\u0301'` would split a combining sequence).

- **No OS, no locale.** There is no libc, no iconv, no Unicode
  normalization library. Every byte of UTF-8 handling would be
  hand-written in COR24 assembly or C compiled by tc24r.

### String operation correctness

APL string operations assume character-addressable arrays:

```
      rho 'hello'
5
      3 take 'hello'
hel
```

With UTF-8 byte storage, `rho` would return byte count (not character
count), and `take`/`drop`/indexing would split multi-byte sequences.
With codepoint storage, every character operation works correctly, but
UART I/O needs encode/decode at every boundary.

**Conclusion:** The complexity cost of Unicode is not justified for an
embedded interpreter with no display hardware beyond a serial terminal.

---

## Encoding Layers

| Layer | Encoding | Notes |
|-------|----------|-------|
| Source code (files) | ASCII (7-bit) | Keywords, operators, identifiers |
| String data (heap) | 8-bit (one byte per element) | Printable ASCII; Latin-1 possible later |
| UART wire protocol | 8-bit | Raw bytes, no framing |
| Editor display | Unicode ligatures | `rho` -> `⍴`, `[]` -> `⎕`, `<-` -> `←` |
| Emulator terminal | UTF-8 | Terminal handles rendering |

---

## ASCII-to-APL Symbol Mapping

### Operators and Primitives

| ASCII source | APL glyph | Unicode | Meaning |
|-------------|-----------|---------|---------|
| `assign` | `←` | U+2190 | Assignment |
| `rho` | `⍴` | U+2374 | Shape / reshape |
| `iota` | `⍳` | U+2373 | Index generator |
| `take` | `↑` | U+2191 | Take first/last N |
| `drop` | `↓` | U+2193 | Drop first/last N |
| `rev` | `⌽` | U+233D | Reverse |
| `cat` | `,` | U+002C | Catenate / ravel |
| `ceil` | `⌈` | U+2308 | Maximum |
| `floor` | `⌊` | U+230A | Minimum |
| `and` | `∧` | U+2227 | Logical/bitwise AND |
| `or` | `∨` | U+2228 | Logical/bitwise OR |
| `not` | `~` | U+007E | Logical/bitwise NOT |
| `pick` | `⊃` | U+2283 | Pick from nested |
| `compress` | `/` | U+002F | Boolean compress |
| `del` | `∇` | U+2207 | Function definition |
| `roll` | `?` | U+003F | Random number |
| `fmt` | `⍕` | U+2355 | Format (number to string) |
| `comment` | `⍝` | U+235D | Comment (rest of line) |
| `cup` | `∪` | U+222A | Unique / union |
| `cap` | `∩` | U+2229 | Intersection |
| `+/` | `+/` | — | Plus-reduce |
| `-/` | `-/` | — | Minus-reduce |
| `*/` | `×/` | — | Times-reduce |

### Quad (System) Variables and Functions

| ASCII source | APL glyph | Unicode | Meaning |
|-------------|-----------|---------|---------|
| `quad` | `⎕` | U+2395 | Bare quad (I/O) |
| `quad-origin` | `⎕IO` | — | Index origin |
| `quad-seed` | `⎕RL` | — | Random link (seed) |
| `qsvo` | `⎕SVO` | — | Shared variable offer |

### Identifiers

| Convention | Example | Rule |
|-----------|---------|------|
| User variables | `A`, `MATRIX1` | Uppercase start, alphanumeric |
| Reserved words | `rho`, `iota` | Lowercase, from lookup table |
| System names | `[]IO`, `[]RL` | `[]` prefix + uppercase name |

---

## The `[]` Quad Convention

### Why `[]` for quad?

The `⎕` (quad) character is U+2395, requiring 3 bytes in UTF-8. Since
our source encoding is ASCII, we need an ASCII representation. `[]`
(empty square brackets) is the standard ASCII transliteration used by
many historical APL systems:

- APL\360 terminals used a special APL typeball; ASCII terminals
  substituted `[]` for quad
- APL*PLUS/PC used `[]` in its ASCII mode
- Many APL-to-ASCII conversion tools use this mapping

### No conflict with bracket indexing

In standard APL, bracket indexing always contains an expression:

```
      A[3]        -> index expression inside brackets
      A[2;3]      -> multi-axis index
      M[1 2;]     -> row selection with empty axis
```

**Empty brackets `[]` have no meaning in standard APL indexing.**
The parser distinguishes them trivially:

### Quad variable syntax

Quad and its system variables are lowercase keywords:

```
      quad-origin assign 0       set index origin
      quad-seed assign 12345     set random seed
      quad assign 42             bare quad output (print 42)
```

The tokenizer matches compound keywords (`quad-origin`, `quad-seed`)
before the bare `quad` keyword, using longest-match-first ordering.
The hyphen is part of the keyword, not the minus operator.

---

## Display Ligatures (Editor Layer)

The recommended approach for human-readable APL display is an Emacs
`prettify-symbols-mode` configuration (or equivalent for other editors):

```elisp
;; cor24-apl-mode ligatures (display only, source stays ASCII)
(defun cor24-apl-ligatures ()
  (setq prettify-symbols-alist
        '(("rho"   . ?⍴)
          ("iota"  . ?⍳)
          ("take"  . ?↑)
          ("drop"  . ?↓)
          ("rev"   . ?⌽)
          ("ceil"  . ?⌈)
          ("floor" . ?⌊)
          ("and"   . ?∧)
          ("or"    . ?∨)
          ("not"   . ?~)
          ("pick"  . ?⊃)
          ("del"   . ?∇)
          ("roll"  . ??)
          ("assign" . ?←)
          ("comment" . ?⍝)
          ("quad"  . ?⎕)
          ("quad-origin" . ?⎕IO)
          ("quad-seed" . ?⎕RL)))
  (prettify-symbols-mode 1))
```

With this mode active, the source:

```
A assign 3 4 rho iota 12
quad assign +/ A
```

Displays as:

```
A ← 3 4 ⍴ ⍳ 12
⎕ ← +/ A
```

The file on disk remains pure ASCII. The ligature rendering is
bidirectional -- editing through a ligature modifies the underlying
ASCII tokens.

---

## Future Considerations

### Latin-1 extension

If character data beyond ASCII-127 is ever needed (e.g., accented
characters for European text), the interpreter can extend to Latin-1
(ISO 8859-1) without any architectural change -- it is still one byte
per character, one COR24 word per character element.

### Terminal encoding

The COR24 emulator's `--terminal` mode bridges stdin/stdout to UART.
The host terminal typically speaks UTF-8. Since all interpreter output
is ASCII, this works transparently. If Latin-1 output were added, the
terminal would need to be configured for the correct encoding (or a
translation layer added to the emulator).

### Source file portability

Because source files are pure ASCII, they are valid UTF-8, valid
Latin-1, valid ASCII -- universally portable with no BOM, no encoding
declaration, and no ambiguity.
