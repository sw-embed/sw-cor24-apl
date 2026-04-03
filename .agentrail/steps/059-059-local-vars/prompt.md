Implement local variables in user-defined functions:
- Support semicolon syntax in function headers: del R assign FN X;LOCAL1;LOCAL2
- Save local variable values on function entry, restore on exit
- Uses the existing call stack mechanism (extend sym save/restore)
- Local variables start as undefined (VALUE ERROR if read before set)
- Test with recursive functions that use local state
- Update docs/missing.md to remove these items from gap list