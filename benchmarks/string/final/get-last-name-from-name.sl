; https=//exceljet.net/formula/get-last-name-from-name
(set-logic SLIA)
(synth-fun f ((_arg_0 String)) String 
 ( (Start String (ntString)) 
 (ntString String (
	_arg_0
	"" " "
	(str.++ ntString ntString) 
	(str.replace ntString ntString ntString) 
	(str.at ntString ntInt)
	(int.to.str ntInt)
	(str.substr ntString ntInt ntInt)
)) 
 (ntInt Int (
	
	1 0 -1
	(+ ntInt ntInt)
	(- ntInt ntInt)
	(str.len ntString)
	(str.to.int ntString)
	(str.indexof ntString ntString ntInt)
)) 
 (ntBool Bool (
	
	true false
	(= ntInt ntInt)
	(str.prefixof ntString ntString)
	(str.suffixof ntString ntString)
	(str.contains ntString ntString)
)) ))
(constraint (= (f "Park Kim") "Kim"))
(constraint (= (f "Lee Kim") "Kim"))
(constraint (= (f "Kim Lee") "Lee"))
(check-synth)
(define-fun f1 ((_arg_0 String)) String (str.substr _arg_0 (+ (str.indexof _arg_0 " " 1) 1) (str.len _arg_0)))
