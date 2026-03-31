# Changelog

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
