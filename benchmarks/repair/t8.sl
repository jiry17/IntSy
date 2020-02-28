
(set-logic  LIA)
(
synth-fun func ( (x Int) (y Int) (z Int) (w Int)) Bool
	((Start Bool (
(< IntExpr IntExpr)
(and Start Start)
(= IntExpr IntExpr)
(not Start )
(<= IntExpr IntExpr)
(or Start Start)



))
(IntExpr Int (
x y z w 
0 1
))

	)
)
(declare-var x Int)
(declare-var y Int)
(declare-var z Int)
(declare-var w Int)

(constraint  (=  (< z x) (func x y z w )))


(check-synth)