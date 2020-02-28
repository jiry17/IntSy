
(set-logic  LIA)
(
synth-fun func ( (x Int)) Bool
	((Start Bool (
(< IntExpr IntExpr)
(and Start Start)
(or Start Start)
(>= IntExpr IntExpr)



))
(IntExpr Int (
x 
0 1 99
))

	)
)
(declare-var x Int)

(constraint  (= (func x ) (and (>= x 0) (<= x 99))))


(check-synth)