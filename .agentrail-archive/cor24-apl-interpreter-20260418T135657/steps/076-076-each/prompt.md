Implement each operator (APL ¨):
- Each applies a function per element of a nested/boxed array
- Syntax: f each V where f is a monadic function and V is a boxed vector
- Example: rho each 'hello' 'world' 'hi' -> 5 5 2 (length of each string)
- Example: rev each 'abc' 'def' -> 'cba' 'fed'
- Need to decide ASCII syntax — perhaps 'each' as a modifier keyword
- Result is a boxed vector of individual results
- Add .apl and .a24 test files. Update encoding.md