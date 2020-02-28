
(set-logic  LIA)
(
synth-fun func ( (x Int)) Bool
	((Start Bool (
(> IntExpr IntExpr)
(>= IntExpr IntExpr)



))
(IntExpr Int (
x 
0 1
))

	)
)
(declare-var x Int)

(constraint (= (func x ) (> x 1)))



(check-synth)