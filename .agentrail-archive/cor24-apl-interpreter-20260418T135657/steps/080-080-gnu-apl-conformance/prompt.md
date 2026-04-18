Create GNU APL conformance comparison tool:
- Script that runs each .apl file through GNU APL and each .a24 file through COR24 emulator
- Compares outputs side by side
- Reports differences per test file
- Expected differences: GNU APL shows floating point, our interpreter is integer-only
- Document any APL conformance deviations found
- Fix any bugs discovered during comparison