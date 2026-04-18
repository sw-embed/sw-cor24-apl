Implement multi-axis take and drop on matrices:
- 2 3 take M: take first 2 rows and 3 columns from matrix M
- _1 3 take M: take last 1 row, first 3 columns
- 1 2 drop M: drop first row and first 2 columns
- Negative values take/drop from the end
- Left arg must be 2-element vector for matrices, scalar for vectors (existing)
- Test with 3 3 rho iota 9 and various take/drop shapes
- Add .apl and .a24 test files