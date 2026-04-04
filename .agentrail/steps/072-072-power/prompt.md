Implement integer exponentiation (APL * dyadic for integers):
- Dyadic *: base raised to integer exponent. Keyword 'power'. 2 power 10 -> 1024. 3 power 0 -> 1
- Currently * is multiply. Need to add 'power' as a new reserved word (not replacing *)
- Handle: negative exponents -> 0 (integer division), 0 power 0 -> 1
- Element-wise on vectors with scalar extension
- Add .apl and .a24 test files. Update encoding.md