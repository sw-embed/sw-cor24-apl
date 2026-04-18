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

## Build / Test

This project uses tc24r (C compiler) and cor24-run (emulator):

```bash
./build.sh              # compile only
./build.sh run          # compile and run on emulator
./build.sh run --terminal   # interactive REPL mode
./build.sh clean        # remove build artifacts
```

Toolchain repos (siblings under `~/github/sw-embed/`):
- `sw-cor24-tinyc` -- C compiler (tc24r)
- `sw-cor24-emulator` -- emulator + assembler (cor24-run)

## CRITICAL: Git Branching Workflow (devgroup policy)

This clone is downstream of a coordinator-gated integration model:

- `main` and `dev` are coordinator-only. **Never commit to them
  directly, and never `git push`.** The coordinator (mike) relays
  ready branches into `dev` and pushes.
- Do all work on `feature/<topic>` or `fix/<topic>` branches, based
  on local `dev` (which tracks the integration branch).
- When work is ready for integration, rename the branch to
  `pr/<topic>` so the coordinator's scan picks it up.
- The ref name is the contract -- no PR API, no JSON, no tickets.

### Helpers (on `PATH` via `$SCRIPTROOT`)

```bash
dg-new-feature <topic>   # switch dev, fetch, create feature/<topic>
dg-new-fix <topic>       # same flavor, fix/<topic>
dg-mark-pr               # rename current feature/* (or fix/*) -> pr/*
dg-list-pr               # list pr/* branches
dg-policy                # reprint this policy
onboarding               # environment / state summary
```

### Rules

- **Never `git push`** -- the coordinator handles all pushes.
- Base new branches on local `dev`, not `main`.
- Do not rewrite history on `dev` or `main`. Rebase is fine on your
  own `feature/*` before marking `pr/*`.
- After the coordinator integrates `pr/<topic>`, re-fetch and fast-
  forward local `dev` before starting the next branch.

Full policy:
`/disk1/github/softwarewrighter/devgroup/docs/branching-pr-strategy.md`

## CRITICAL: AgentRail Session Protocol (MUST follow exactly)

Each AgentRail step maps to one `feature/<slug>` (or `fix/<slug>`)
branch. Create the branch BEFORE doing the work, and rename it to
`pr/<slug>` AFTER `agentrail complete`.

### 1. START (do this FIRST, before anything else)
```bash
agentrail next
```
Read the output carefully. It contains your current step, prompt,
plan context, and any relevant skills/trajectories.

### 2. BRANCH (create a work branch for the step)
```bash
dg-new-feature <slug>    # or dg-new-fix <slug> for a bug fix
```
Use the step's slug (e.g. `matrix-take-drop`) as the topic. This
switches to `dev`, fetches, and creates `feature/<slug>`.

### 3. BEGIN (tell AgentRail the step is started)
```bash
agentrail begin
```

### 4. WORK (do what the step prompt says)
Do NOT ask "want me to proceed?". The step prompt IS your instruction.
Execute it directly.

### 5. COMMIT (after the work is done)
Commit your code changes with git on the `feature/<slug>` branch.
Do NOT push -- the coordinator handles pushes.

### 6. COMPLETE (after committing)
```bash
agentrail complete --summary "what you accomplished" \
  --reward 1 \
  --actions "tools and approach used"
```
- If the step failed: `--reward -1 --failure-mode "what went wrong"`
- If the saga is finished: add `--done`

### 7. MARK PR (signal ready-to-merge)
```bash
dg-mark-pr               # renames feature/<slug> -> pr/<slug>
```

### 8. STOP (after mark-pr, DO NOT continue working)
Do NOT make further code changes after `dg-mark-pr`. Any changes
after complete/mark-pr are outside the step's recorded scope.
Future work belongs in the NEXT step on a NEW branch.

Before starting the next step, fast-forward local `dev`:
```bash
git switch dev && git fetch --all --prune && git merge --ff-only
```

## Key Rules

- **Never push** -- coordinator-only
- **Never commit to main or dev** -- always work on feature/* or fix/*
- **Do NOT skip steps** -- the next session depends on accurate tracking
- **Do NOT ask for permission** -- the step prompt is the instruction
- **Do NOT continue working** after `dg-mark-pr`
- **Commit before complete** -- always commit first, then record completion
- **Mark PR after complete** -- rename feature/* -> pr/* last

## Useful Commands

```bash
agentrail status          # Current saga state
agentrail history         # All completed steps
agentrail plan            # View the plan
agentrail next            # Current step + context
```

## COR24 Quick Reference

- 24-bit RISC, 8 registers (r0-r2 GP, fp, sp, z, iv, ir)
- 1 MB SRAM + 8 KB EBR stack
- UART at 0xFF0100 (data), 0xFF0101 (status)
- No hardware divide -- use software division
- Variable-length instructions (1, 2, or 4 bytes)
- Calling convention: args on stack, return in r0, link in r1
