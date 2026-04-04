Implement dyadic rotate (APL ⌽ with left argument):
- Dyadic ⌽: N rotate V rotates vector V left by N positions. Keyword: dyadic use of existing 'rev'. 2 rev 1 2 3 4 5 -> 3 4 5 1 2. Negative N rotates right.
- On matrices: rotates each row
- Monadic rev (reverse) already works. Parser must distinguish: left arg present = rotate, no left arg = reverse
- Add .apl and .a24 test files. Update encoding.md