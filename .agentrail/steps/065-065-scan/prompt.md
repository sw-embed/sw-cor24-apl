Implement scan operator (APL f\ backslash):
- Scan is running reduce: +\ 1 2 3 4 -> 1 3 6 10
- Support same operators as reduce: +\ -\ *\ ceil\ floor\ and\ or\
- Tokenizer: detect OP followed by backslash (92) similar to how OP/ is detected for reduce
- Parser: create NODE_SCAN with val=operator, right=operand
- Evaluator: left-to-right accumulation for each prefix
- Add .apl and .a24 test files. Update encoding.md