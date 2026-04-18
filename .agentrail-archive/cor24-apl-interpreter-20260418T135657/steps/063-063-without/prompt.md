Implement dyadic without/except (APL ~ dyadic):
- Dyadic ~: A without B removes elements of A that appear in B. Keyword 'without'. 1 2 3 4 5 without 2 4 -> 1 3 5
- Works on integer and character vectors
- Note: monadic 'not' already exists for logical NOT. The parser must distinguish monadic not from dyadic without based on context (left argument present = dyadic without)
- Add .apl and .a24 test files. Update encoding.md