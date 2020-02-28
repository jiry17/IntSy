
(set-logic  LIA)
(
synth-fun func ( (x Int) (y Int) (z Bool)) Bool
	((Start Bool (
(and Start Start)

(= IntExpr IntExpr)
(not Start )
(or Start Start)
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

(constraint  (= (func x y z ) (and z (= x y))))


(check-synth)