Implement execute (APL ⍎):
- Monadic ⍎: evaluate a character vector as APL code and return the result
- Keyword 'execute'. execute '1 + 2' -> 3
- Internally: tokenize the string, parse it, eval the AST, return result
- Must handle errors gracefully (DOMAIN ERROR for invalid expressions)
- Works with variables in current scope
- Add .apl and .a24 test files. Update encoding.md