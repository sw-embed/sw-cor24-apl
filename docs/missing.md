────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯ why does RACE take an argument (that it does not use) I expect "RACE" enter to work.  Why does the example require "RACE 0" to work?
  Don't we support nilidac, monadic, and dyadic functions?  Also the original uses ;POS when declaring which is a local var, right?
  why do we not support that?  Are these issues addressed by pending steps in the saga?
─
Here's the answer:

  Why RACE 0 instead of just RACE?

  The COR24 interpreter only supports monadic (del R <- FN X) and dyadic (del R <- X FN Y) functions. Niladic functions are not
  implemented. The function header parser in fn.h expects exactly 2 or 3 uppercase identifiers after <- — 1 identifier (function name
  alone) is rejected. So the horse race demos use Z <- RACE 0 as a workaround, passing a dummy argument.

  Why no ;POS local variables?

  The semicolon local-variable syntax (∇ R ← RACE;POS;RND) is also not implemented. The header parser stops at the first non-uppercase
  character — it doesn't look for or handle semicolons. All variables inside functions are currently global — only the result, right
  arg, and left arg are saved/restored on the call stack.

  This means the horse race programs pollute the global workspace with POS, RND, LEAD, etc. It works, but it's not proper APL scoping.

  Is this in the saga?

  The sw-cor24-apl saga plan mentions "Local variables" and "Recursive calls" as planned features under the function definition phase,
  but both were left unimplemented — the saga completed without them. Niladic functions aren't mentioned at all in the plan. Both sagas
  (sw-cor24-apl and web-sw-cor24-apl) are marked complete.

  These would need new saga steps in sw-cor24-apl to address:
  1. Niladic functions — allow del R <- FN with no argument, callable as just FN
  2. Local variables — parse ;VAR1;VAR2 in headers, save/restore them on the call stack

  Want me to add these as planned steps to the sw-cor24-apl saga?

✻ Baked for 1m 47s

