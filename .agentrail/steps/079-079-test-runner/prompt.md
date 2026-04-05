Create automated test runner script:
- Shell script that runs all batch-*.a24 samples and compares against expected output
- For each sample, store expected output in a companion .expected file (or inline in the script)
- Report: PASS/FAIL per sample, total count, any diffs on failure
- Run as: ./test.sh (or ./build.sh test)
- Capture the UART output line from cor24-run and compare
- Should be fast enough to run as a pre-commit check
- Also run horse race demos (just check for WINNER line, not exact output due to PRNG)