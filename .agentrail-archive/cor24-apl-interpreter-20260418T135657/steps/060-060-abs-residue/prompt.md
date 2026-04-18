Implement absolute value and residue (APL |):
- Monadic | (abs): absolute value. Keyword 'abs'. abs _5 -> 5, abs 3 -> 3
- Dyadic | (residue): modulo/remainder. Keyword 'residue'. 3 residue 7 -> 1, 5 residue 13 -> 3
- Element-wise on vectors and matrices with scalar extension
- Add RES_ABS and RES_RESIDUE to reserved words (or single RES with monadic/dyadic dispatch)
- Add .apl test file for GNU APL comparison and .a24 batch test
- Update encoding.md symbol mapping table