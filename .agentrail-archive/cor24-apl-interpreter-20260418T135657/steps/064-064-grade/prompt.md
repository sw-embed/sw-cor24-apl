Implement grade up and grade down (APL ⍋/⍒):
- Monadic ⍋ (grade up): returns indices that would sort ascending. Keyword 'gradeup'. gradeup 30 10 20 -> 2 3 1 (1-origin)
- Monadic ⍒ (grade down): returns indices that would sort descending. Keyword 'gradedown'. gradedown 30 10 20 -> 1 3 2 (1-origin)
- Respects quad-origin setting
- Implementation: simple insertion sort or selection sort on indices (O(n^2) is fine for embedded)
- Works on integer vectors. Character vector sorting by ASCII value is a bonus
- Add .apl and .a24 test files. Update encoding.md