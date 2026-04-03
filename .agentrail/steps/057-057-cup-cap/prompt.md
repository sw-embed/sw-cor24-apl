Implement set operations:
- 'cup' keyword (APL ∪): monadic = unique (remove duplicates), dyadic = set union
- 'cap' keyword (APL ∩): dyadic = set intersection
- Add RES_CUP and RES_CAP to reserved words
- Implement for integer vectors and character vectors
- Monadic cup: return vector with duplicates removed, preserving first-occurrence order
- Dyadic cup: left argument unchanged, plus elements of right not in left
- Dyadic cap: elements of left that also appear in right
- Add .apl test file for GNU APL comparison and .a24 batch test
- Update encoding.md symbol mapping table