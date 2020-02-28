
(set-logic  LIA)
(
synth-fun f_35-7-35-46 ( (x Int) (y Int) (z Int)) Bool
	((Start Bool (
(>= IntExpr IntExpr)
(> IntExpr IntExpr)



))
(IntExpr Int (
(- IntExpr IntExpr)
(+ IntExpr IntExpr)
x y z 
0 1
))

	)
)
(declare-var x Int)
(declare-var y Int)
(declare-var z Int)

(constraint  (= (f_35-7-35-46 x y z ) (>= x (+ y z) )))


(check-synth)