Fix bracket indexing on lazy iota arrays:
- (iota 999999)[5] should return 5 (1-origin) without materializing the full vector
- Currently bracket indexing may not handle ARR_IOTA type correctly
- The bracket index read path in eval.h (NODE_SVO_READ for regular vectors) needs to check arr_type and use arr_get which already handles lazy iota
- Verify: V assign iota 999999 then V[1] V[100] V[999999] all work
- Add test cases to batch-iota-test.a24