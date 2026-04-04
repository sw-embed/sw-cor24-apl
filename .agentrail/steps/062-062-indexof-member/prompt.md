Implement dyadic iota (index-of) and membership (APL ∊):
- Dyadic iota (index-of): A iota B returns index of B in A. If not found, returns 1+rho A (or rho A if 0-origin). Keyword: dyadic use of existing 'iota'. 10 20 30 iota 20 -> 2 (1-origin)
- Membership (∊): A member B returns 1 where elements of A appear in B. Keyword 'member'. 1 2 3 4 member 2 4 6 -> 0 1 0 1
- Both work on integer and character vectors
- Add .apl and .a24 test files. Update encoding.md