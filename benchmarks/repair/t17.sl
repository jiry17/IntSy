
(set-logic  LIA)
(
synth-fun f_255-8-255-24 ( (x Int) (y Int)) Bool
	((Start Bool (
(= IntExpr IntExpr)



))
(IntExpr Int (
(+ IntExpr IntExpr)
(- IntExpr IntExpr)
x y 
0 1
))

	)
)
(declare-var x Int)
(declare-var y Int)

(constraint  (= (f_255-8-255-24 x y ) (= (- y x) 1))) 


(check-synth)