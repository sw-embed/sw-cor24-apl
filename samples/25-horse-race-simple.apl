⍝ Horse Race -- Simple Version (GNU APL)
⍝ 4 horses race to a finish line
⍝ COR24 equivalent: horse-race-simple.a24

⍝ NOTE: GNU APL uses ⎕IO←0 for 0-origin
⎕IO←0

⍝ Seed not portable — COR24 uses qrl←42
⍝ Random results will differ from COR24

NH←4
GOAL←10
POS←NH⍴0
RND←0

∇ R←RACE X
R←0
NEXT: RND←RND+1
⎕←'Round ',⍕RND
POS←POS+?NH⍴3
⎕←POS
DONE←∨/POS≥GOAL
→(DONE=0)/NEXT
WIN←(POS≥GOAL)/⍳NH
⎕←'Winner: horse ',⍕1+WIN[0]
∇

⎕←'--- Horse Race ---'
RACE 0
⎕←'Final positions:'
⎕←POS
)OFF
