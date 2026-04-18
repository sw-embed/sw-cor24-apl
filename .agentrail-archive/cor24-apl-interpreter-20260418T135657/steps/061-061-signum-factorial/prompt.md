Implement signum and factorial/binomial (APL × monadic, APL !):
- Monadic × (signum): returns -1, 0, or 1. Keyword 'signum'. signum _5 -> _1, signum 0 -> 0, signum 7 -> 1
- Monadic ! (factorial): n factorial. Keyword 'factorial'. factorial 5 -> 120, factorial 0 -> 1
- Dyadic ! (binomial): C(n,k) combinations. Keyword 'binomial'. 2 binomial 5 -> 10
- Element-wise on vectors with scalar extension
- Add .apl and .a24 test files. Update encoding.md