(set-logic BV)

(define-fun shr1 ((x (BitVec 64))) (BitVec 64) (bvlshr x #x0000000000000001))
(define-fun shr4 ((x (BitVec 64))) (BitVec 64) (bvlshr x #x0000000000000004))
(define-fun shr16 ((x (BitVec 64))) (BitVec 64) (bvlshr x #x0000000000000010))
(define-fun shl1 ((x (BitVec 64))) (BitVec 64) (bvshl x #x0000000000000001))
(define-fun if0 ((x (BitVec 64)) (y (BitVec 64)) (z (BitVec 64))) (BitVec 64) (ite (= x #x0000000000000001) y z))

(synth-fun f ( (x (BitVec 64))) (BitVec 64)
(

(Start (BitVec 64) (#x0000000000000000 #x0000000000000001 x (bvnot Start)
                    (shl1 Start)
 		    (shr1 Start)
		    (shr4 Start)
		    (shr16 Start)
		    (bvand Start Start)
		    (bvor Start Start)
		    (bvxor Start Start)
		    (bvadd Start Start)
		    (if0 Start Start Start)
 ))
)
)
(constraint (= (f #xD436DEE0903CEF48) #x5792423EDF86216F))
(constraint (= (f #x273D89A38D4F6398) #xB184ECB8E56138CF))
(constraint (= (f #xEB1F317A769EB128) #x29C19D0B12C29DAF))
(constraint (= (f #x55CAC42CF1CE8DEC) #x546A77A61C62E427))
(constraint (= (f #xBA390C7D2ED95BB8) #x8B8DE705A24D488F))
(constraint (= (f #x00000000001635A4) #xFFFFFFFFFFD394B7))
(constraint (= (f #x0000000000118C80) #xFFFFFFFFFFDCE6FF))
(constraint (= (f #x000000000013BEF8) #xFFFFFFFFFFD8820F))
(constraint (= (f #x0000000000115344) #xFFFFFFFFFFDD5977))
(constraint (= (f #x0000000000188B40) #xFFFFFFFFFFCEE97F))
(constraint (= (f #x703B635265B7A1EA) #x1F89395B3490BC2B))
(constraint (= (f #xA6F3B06C1F0C8106) #xB2189F27C1E6FDF3))
(constraint (= (f #x46EEED4D835459DE) #x72222564F9574C43))
(constraint (= (f #xC35A085D6EDD00E2) #x794BEF452245FE3B))
(constraint (= (f #x233A0AF84127B8F6) #xB98BEA0F7DB08E13))
(constraint (= (f #x00000000001F7CA2) #xFFFFFFFFFFC106BB))
(constraint (= (f #x0000000000146782) #xFFFFFFFFFFD730FB))
(constraint (= (f #x000000000019BA5E) #xFFFFFFFFFFCC8B43))
(constraint (= (f #x000000000017C676) #xFFFFFFFFFFD07313))
(constraint (= (f #x000000000019151E) #xFFFFFFFFFFCDD5C3))
(constraint (= (f #x11A4F088C45AF373) #x0000000000000001))
(constraint (= (f #xD69F896E22A81759) #x0000000000000001))
(constraint (= (f #xA819A60DEEC58929) #x0000000000000001))
(constraint (= (f #xC73C0006BCB95B2B) #x0000000000000001))
(constraint (= (f #x51377843F499167B) #x0000000000000001))
(constraint (= (f #x0000000000150F55) #x0000000000150F55))
(constraint (= (f #x00000000001ACC0F) #x00000000001ACC0F))
(constraint (= (f #x000000000012B50B) #x000000000012B50B))
(constraint (= (f #x0000000000199BD7) #x0000000000199BD7))
(constraint (= (f #x00000000001A9103) #x00000000001A9103))
(constraint (= (f #xE076CF4644921556) #x3F12617376DBD553))
(constraint (= (f #x56E6DF455B2C7B1B) #x0000000000000001))
(constraint (= (f #x1F4834AF897A705A) #xC16F96A0ED0B1F4B))
(constraint (= (f #xA262F970B74FBC03) #x0000000000000001))
(constraint (= (f #xA3C114F92F69CABC) #xB87DD60DA12C6A87))
(constraint (= (f #x8CBC8EA6BD2F3DAB) #x0000000000000001))
(constraint (= (f #x90D723043955837B) #x0000000000000001))
(constraint (= (f #x96F317B6C1C18D3E) #xD219D0927C7CE583))
(constraint (= (f #x1D0D400495A1E6A3) #x0000000000000001))
(constraint (= (f #xCC477D127335DD0D) #x0000000000000001))
(constraint (= (f #x000000000015BB95) #x000000000015BB95))
(constraint (= (f #x00000000001489F0) #xFFFFFFFFFFD6EC1F))
(constraint (= (f #x00000000001DB46E) #xFFFFFFFFFFC49723))
(check-synth)
