Rename quad syntax to use descriptive keyword names:
- Replace [] and qout with 'quad' keyword for bare quad I/O (APL ⎕)
- Replace qio with 'quad-origin' for index origin (APL ⎕IO)
- Replace qrl with 'quad-seed' for random link seed (APL ⎕RL)
- Remove qdl (delay) entirely — COR24-TB has no hardware clock; defer to future hardware revision
- Remove qled and qsw temporary placeholders — these will be replaced by shared variable MMIO (qsvo AP 242)
- Update tokenizer, parser, evaluator, function definitions, all .a24 samples, and docs
- Update encoding.md symbol mapping table and ligature config
- Test all existing samples still work with new syntax