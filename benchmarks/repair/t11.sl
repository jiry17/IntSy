
(set-logic  LIA)
(
synth-fun func ( (x Bool) (y Bool)) Bool
	((Start Bool (
(and Start Start)
(or Start Start)
(not Start )
x y 


))

	)
)
(declare-var x Bool)
(declare-var y Bool)

(constraint  (= (func x y) (or (not x) (not y))))


(check-synth)