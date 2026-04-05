Fix dyadic cat on boxed arrays:
- Currently cat of two boxed vectors outputs raw heap indices instead of concatenating properly
- (enclose 'short') cat enclose 'longer' should produce a 2-element boxed vector
- Cat of boxed vector with boxed vector: concatenate elements
- Cat of boxed vector with simple value: enclose the simple value and append
- This enables programmatic nested array construction beyond parser stranding
- Test with enclose+cat combinations
- Add .a24 test file