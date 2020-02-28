
(set-logic  LIA)
(
synth-fun func ( (a Int) (b Int) (y Int) (x Int)) Bool
	((Start Bool (
(and Start Start)
(= IntExpr IntExpr)
(or Start Start)
(not Start )



))
(IntExpr Int (
a b y x 
0 1
))

	)
)
(declare-var a Int)
(declare-var b Int)
(declare-var y Int)
(declare-var x Int)

(constraint  (= (= y a) (func a b y x)))


(check-synth)