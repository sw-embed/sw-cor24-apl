Implement inner product operator (APL f.g):
- Inner product: A f.g B generalizes matrix multiplication
- A +.* B is standard matrix multiply (sum of products)
- A or.and B is boolean matrix multiply
- ASCII syntax: A +.* B or similar dot notation
- For vector-vector: result is f/ A g B (reduce of pairwise)
- For matrix-vector and matrix-matrix: standard linear algebra rules
- Prerequisite for many APL idioms
- Add .apl and .a24 test files. Update encoding.md