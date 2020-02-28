; https=//stackoverflow.com/questions/26262269/how-to-find-all-the-strings-in-a-cell-that-contains-a-given-substring-in-excel
(set-logic SLIA)
(synth-fun f ((_arg_0 String)) String 
 ( (Start String (ntString)) 
 (ntString String (
	_arg_0
	"" " " "in"
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
(constraint (= (f "india china japan") "india china"))
(constraint (= (f "indonesia korea") "indonesia"))
(check-synth)
(define-fun f1 ((_arg_0 String)) String (str.substr _arg_0 0 (str.indexof _arg_0 " " (str.len (str.substr _arg_0 (+ (str.indexof _arg_0 " " 0) 1) (str.len _arg_0))))))
