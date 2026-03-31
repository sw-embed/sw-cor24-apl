# Changelog

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
