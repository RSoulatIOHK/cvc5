; REQUIRES: unrestricted-mode
; COMMAND-LINE: --incremental --finite-model-find --macros-quant --macros-quant-mode=all
; EXPECT: sat
; EXPECT: sat
; EXPECT: sat
(set-logic ALL)
(declare-fun f ((_ BitVec 2) (_ BitVec 2)) (_ BitVec 2))
(push 1)
; Repeated variables constrain the diagonal only.
(assert (forall ((x (_ BitVec 2))) (= (f x x) x)))
(assert (= (f #b00 #b01) #b11))
(check-sat)
(pop 1)
(push 1)
; A compound argument containing the bound variable is not a ground slice.
(assert (forall ((x (_ BitVec 2))) (= (f x (bvnot x)) x)))
(assert (= (f #b00 #b00) #b11))
(check-sat)
(pop 1)
(push 1)
; A ground argument depending on the function is not an independent slice.
(assert (forall ((x (_ BitVec 2))) (= (f (f #b00 #b00) x) #b01)))
(assert (= (f #b00 #b00) #b01))
(assert (= (f #b10 #b00) #b11))
(check-sat)
(pop 1)
