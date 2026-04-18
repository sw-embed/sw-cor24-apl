Implement monadic transpose (APL ⍉):
- Monadic ⍉: swap rows and columns of a matrix. Keyword 'transpose'. transpose 2 3 rho iota 6 produces 3 2 matrix with transposed elements
- On vectors: no-op (returns same vector)
- On scalars: no-op
- Allocate new matrix with swapped dimensions, copy elements
- Add .apl and .a24 test files. Update encoding.md