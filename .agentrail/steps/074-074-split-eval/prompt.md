Reduce eval() frame size to increase recursion depth:
- Split the monolithic eval() function (~2000 lines) into per-node-type functions
- Each node type handler becomes its own function: eval_num, eval_binop, eval_monad, eval_dyad, eval_reduce, eval_scan, eval_outer, eval_inner, etc.
- The main eval() becomes a small dispatch function calling these
- Goal: reduce C stack frame from ~530 bytes to ~100 bytes per eval call
- This should increase effective recursion depth from 3 to 6-8 levels
- Test: FACT 5 and FACT 6 should work after this change
- Verify all existing batch samples still pass