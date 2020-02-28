
(set-logic  LIA)
(
synth-fun func ( (x Int) (y Int)) Bool
	((Start Bool (
(>= IntExpr IntExpr)
(> IntExpr IntExpr)



))
(IntExpr Int (
(- IntExpr IntExpr)
(+ IntExpr IntExpr)
x y 
0 1
))

	)
)
(declare-var x Int)
(declare-var y Int)

(constraint  (= (func x y ) (>= y x)))


(check-synth)