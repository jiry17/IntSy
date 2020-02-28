
(set-logic  LIA)
(
synth-fun func ( (x Int) (y Int) (z Bool)) Bool
	((Start Bool (
(< IntExpr IntExpr)
(<= IntExpr IntExpr)
(and Start Start)
(> IntExpr IntExpr)
(>= IntExpr IntExpr)
z 


))
(IntExpr Int (
x y 
0 1
))

	)
)
(declare-var x Int)
(declare-var y Int)
(declare-var z Bool)

(constraint  (= (func x y z ) (and (<= x y) z)))


(check-synth)