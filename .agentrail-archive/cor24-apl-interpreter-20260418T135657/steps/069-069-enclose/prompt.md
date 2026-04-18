Implement monadic enclose (APL ⊂):
- Monadic ⊂: wrap a value in a scalar box. Keyword 'enclose'. enclose 1 2 3 creates a scalar containing the vector 1 2 3
- This complements pick (⊃/disclose) which extracts from a box
- Useful for building nested arrays programmatically
- rho enclose 1 2 3 -> empty vector (scalar has no shape)
- pick enclose 1 2 3 -> 1 2 3 (round-trip)
- Add .apl and .a24 test files. Update encoding.md