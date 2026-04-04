Implement outer product operator (APL ∘.f):
- Outer product: A ∘.f B applies f to every pair from A and B, producing a matrix
- 1 2 3 ∘.+ 10 20 -> 2x3 matrix [[11,21],[12,22],[13,23]]
- 1 2 3 ∘.* 1 2 3 -> multiplication table
- 1 2 3 ∘.= 1 2 3 -> identity matrix
- ASCII syntax: need to decide on keyword. Perhaps 'outer' as prefix operator: A outer.+ B
- Support all scalar dyadic functions: + - * / = != < > <= >= ceil floor and or
- Result shape is (rho A), (rho B) concatenated
- Add .apl and .a24 test files. Update encoding.md