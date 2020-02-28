
(set-logic  LIA)
(
synth-fun func ( (x Int) (y Int)) Bool
	((Start Bool (
(< IntExpr IntExpr)
(<= IntExpr IntExpr)



))
(IntExpr Int (
x y 
0 1
))

	)
)
(declare-var x Int)
(declare-var y Int)

(constraint  (= (func x y ) (<= 1 (- y x))))
(check-synth)