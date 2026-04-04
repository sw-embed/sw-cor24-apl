Implement encode and decode (APL ⊤/⊥):
- Dyadic ⊤ (encode): represent number in mixed radix. Keyword 'encode'. 2 2 2 encode 5 -> 1 0 1 (binary). 24 60 60 encode 3661 -> 1 1 1 (1h 1m 1s)
- Dyadic ⊥ (decode): evaluate in mixed radix. Keyword 'decode'. 2 2 2 decode 1 0 1 -> 5. 24 60 60 decode 1 1 1 -> 3661
- These are pure integer operations, very useful for base conversion and time/date encoding
- Add .apl and .a24 test files. Update encoding.md