⍝ Horse Race -- Full Version (GNU APL)
⍝ 4 named horses race with track visualization
⍝ COR24 equivalent: horse-race.a24

⍝ NOTE: GNU APL uses ⎕IO←0 for 0-origin
⎕IO←0

NH←4
GOAL←15
POS←NH⍴0
RND←0
NAMES←'Thndr' 'Lghtn' 'Storm' 'Blaze'

∇ R←TRACK X
R←0
I←0
SHOW: R←(I⊃NAMES),'|',(X[I])⍴'#'
⎕←R
I←I+1
→(I<NH)/SHOW
∇

∇ R←RACE X
R←0
NEXT: RND←RND+1
⎕←'=== Round ',⍕RND
POS←POS+?NH⍴3
TRACK POS
LEAD←⌈/POS
⎕←'Leader at ',⍕LEAD
DONE←∨/POS≥GOAL
→(DONE=0)/NEXT
WIN←(POS≥GOAL)/⍳NH
NW←⍴WIN
⎕←'Race over!'
→(NW>1)/TIE
⎕←'Winner: ',(WIN[0]⊃NAMES)
→0
TIE: ⎕←(⍕NW),'-way tie!'
∇

⎕←'*** HORSE RACE ***'
RACE 0
)OFF
