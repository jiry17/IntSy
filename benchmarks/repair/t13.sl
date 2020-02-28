
(set-logic  LIA)
(
synth-fun func ( (x Bool)) Bool
	((Start Bool (
(not Start )
(=b Start Start)
x 


))

	)
)
(declare-var x Bool)

(constraint  (= (func x ) (not x)))


(check-synth)