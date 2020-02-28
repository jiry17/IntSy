
(set-logic  LIA)
(
synth-fun func ( (x Int) (y Int) (a Int) (b Int)) Bool
	((Start Bool (
(= IntExpr IntExpr)
(not Start )
(> IntExpr IntExpr)
(>= IntExpr IntExpr)



))
(IntExpr Int (
x y a b 
0 1
))

	)
)
(declare-var x Int)
(declare-var y Int)
(declare-var a Int)
(declare-var b Int)

(constraint  (= (func x y a b ) (> x b)))

(check-synth)