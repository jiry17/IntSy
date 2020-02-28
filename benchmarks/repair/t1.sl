
(set-logic  LIA)
(
synth-fun func ( (x Int) (y Int) (z Int) (w Int) (a Int) (b Int) (c Int) (d Int) (e Int) (f Int) (g Int) (h Int)) Bool
	((Start Bool (
(= IntExpr IntExpr)
(not Start )



))
(IntExpr Int (
x y z w a b c d e f g h 
0 1
))

	)
)
(declare-var x Int)
(declare-var y Int)
(declare-var z Int)
(declare-var w Int)
(declare-var a Int)
(declare-var b Int)
(declare-var c Int)
(declare-var d Int)
(declare-var e Int)
(declare-var f Int)
(declare-var g Int)
(declare-var h Int)

(constraint  (= (func x y z w a b c d e f g h ) (= g d)))


(check-synth)