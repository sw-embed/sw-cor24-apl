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

## CRITICAL: AgentRail Session Protocol (MUST follow exactly)

### 1. START (do this FIRST, before anything else)
```bash
agentrail next
```
Read the output carefully. It contains your current step, prompt,
plan context, and any relevant skills/trajectories.

### 2. BEGIN (immediately after reading the next output)
```bash
agentrail begin
```

### 3. WORK (do what the step prompt says)
Do NOT ask "want me to proceed?". The step prompt IS your instruction.
Execute it directly.

### 4. COMMIT (after the work is done)
Commit your code changes with git. Use `/mw-cp` for the checkpoint
process (pre-commit checks, docs, detailed commit, push).

### 5. COMPLETE (LAST thing, after committing)
```bash
agentrail complete --summary "what you accomplished" \
  --reward 1 \
  --actions "tools and approach used"
```
- If the step failed: `--reward -1 --failure-mode "what went wrong"`
- If the saga is finished: add `--done`

### 6. STOP (after complete, DO NOT continue working)
Do NOT make further code changes after running `agentrail complete`.
Any changes after complete are untracked and invisible to the next
session. Future work belongs in the NEXT step, not this one.

## Key Rules

- **Do NOT skip steps** -- the next session depends on accurate tracking
- **Do NOT ask for permission** -- the step prompt is the instruction
- **Do NOT continue working** after `agentrail complete`
- **Commit before complete** -- always commit first, then record completion

## Useful Commands

```bash
agentrail status          # Current saga state
agentrail history         # All completed steps
agentrail plan            # View the plan
agentrail next            # Current step + context
```

## Build / Test

This project uses tc24r (C compiler) and cor24-run (emulator):

```bash
# Compile C to COR24 assembly (tc24r location TBD -- check stable repos)
# Assemble and run on emulator
cor24-run <assembled-output>
```

Stable toolchain code is in the original repos under:
- `~/github/sw-vibe-coding/tc24r` -- C compiler
- `~/github/sw-embed/sw-cor24-emulator` -- emulator + assembler

The `~/github/sw-embed/sw-cor24-project` repo is an in-progress
refactoring -- use the original repos above for stable references.

## COR24 Quick Reference

- 24-bit RISC, 8 registers (r0-r2 GP, fp, sp, z, iv, ir)
- 1 MB SRAM + 8 KB EBR stack
- UART at 0xFF0100 (data), 0xFF0101 (status)
- No hardware divide -- use software division
- Variable-length instructions (1, 2, or 4 bytes)
- Calling convention: args on stack, return in r0, link in r1
