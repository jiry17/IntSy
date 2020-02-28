
(set-logic  LIA)
(
synth-fun func ( (x Int)) Bool
	((Start Bool (
(> IntExpr IntExpr)
(and Start Start)
(or Start Start)



))
(IntExpr Int (
x 
0 1
))

	)
)
(declare-var x Int)

(constraint  (= (func x) (> x 0)))


(check-synth)