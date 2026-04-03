Implement niladic (zero-argument) user-defined functions:
- Allow 'del R assign FN' with no argument parameter (just function name)
- Update parse_fn_header in fn.h to accept 1 identifier after assign (function name only)
- Callable as just FN (no argument needed)
- This removes the need for dummy arguments like 'Z assign RACE 0'
- Update horse race demos to use niladic RACE function
- Test: del R assign HELLO / quad assign 'hello world' / del / HELLO