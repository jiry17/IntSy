
(set-logic  LIA)
(
synth-fun f_68-13-68-36 ( (x Int) (y Int) (z Bool)) Bool
	((Start Bool (
(not Start )
(= IntExpr IntExpr)
(and Start Start)
(or Start Start)
z 


))
(IntExpr Int (
x y 
0 55 1 56 8 10 9 46
))

	)
)
(declare-var x Int)
(declare-var y Int)
(declare-var z Bool)

(constraint   (= (f_68-13-68-36 x y z ) (not (or (= x 0) z))))


(check-synth)