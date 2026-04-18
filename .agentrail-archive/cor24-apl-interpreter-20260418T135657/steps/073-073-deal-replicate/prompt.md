Implement deal and replicate:
- Dyadic ? (deal): N deal M returns N random integers from 1..M without replacement. Keyword 'deal'. 3 deal 10 -> e.g. 7 2 9 (three unique random numbers 1-10)
- Dyadic / (replicate): integer vector left arg replicates elements. 2 3 1 replicate 10 20 30 -> 10 10 20 20 20 30. Boolean compress is special case.
- Note: replicate generalizes existing compress (which only allows 0/1 left arg). Consider making compress handle integer left args
- Add .apl and .a24 test files. Update encoding.md