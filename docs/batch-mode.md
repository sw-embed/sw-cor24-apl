# APL Image Format and Batch Mode

## Overview

APL programs can be loaded as "images" into SRAM for batch
(non-interactive) execution. The interpreter detects the image at
startup and reads lines from memory instead of UART.

## Image Format

An APL image is a plain text file containing one APL expression per
line, separated by newlines (0x0A). The file is null-terminated in
memory (the emulator's `--load-binary` zero-fills surrounding memory).

```
line 1\n
line 2\n
...
last line\n
\0
```

Lines are processed exactly as if typed at the REPL prompt: expressions
are evaluated and results printed, system commands are executed, and
function definitions are collected.

The image should end with `)OFF` to halt the interpreter cleanly.
Without `)OFF`, execution continues into REPL mode after the image
is exhausted (useful for setup scripts that prepare a workspace).

## Memory Layout

| Address    | Purpose                                    |
|------------|--------------------------------------------|
| 0x09FF00   | Image pointer (24-bit): points to image data, or 0 for interactive mode |
| 0x080000   | Default image data base address            |

The image pointer at `0x09FF00` is checked at startup. If non-zero,
the interpreter reads APL lines from the address it points to. If
zero (default), the interpreter runs in normal interactive REPL mode.

The image data occupies SRAM from the base address upward. Maximum
image size is limited by available SRAM (roughly 512 KB with the
default base at 0x080000).

## Loading an Image

Use `cor24-run` flags to load the image and set the pointer:

```bash
cor24-run --run build/apl.s \
  --load-binary program.a24@0x080000 \
  --patch 0x09FF00=0x080000
```

Or use the build script shorthand:

```bash
./build.sh run --batch program.a24
```

## Example

File `hello.a24`:
```
1 + 1
iota 5
+/ iota 10
)OFF
```

Run:
```bash
./build.sh run --batch hello.a24
```

Output:
```
COR24 APL v0.1
  2
 0 1 2 3 4
  45
```

## Design Notes

- The image pointer is stored high in SRAM (0x09FF00) to avoid
  conflict with the heap, which grows upward from ~0x012000.
- The image data at 0x080000 is above the heap's practical reach
  for typical APL programs.
- The format is deliberately simple: raw text, no headers, no
  metadata. This makes images easy to create with any text editor.
- The interpreter reads the image sequentially, advancing a pointer
  through SRAM. Each newline terminates a line. A null byte (0x00)
  terminates the image.
