; REQUIRES: unrestricted-mode
; COMMAND-LINE: --incremental --finite-model-find --macros-quant --macros-quant-mode=all
; EXPECT: sat
; EXPECT: unsat
(set-logic ALL)
(declare-fun p ((_ BitVec 2) (_ BitVec 2)) Bool)
(assert (forall ((x (_ BitVec 2))) (not (p x #b00))))
(assert (p #b00 #b01))
(check-sat)
(assert (p #b01 #b00))
(check-sat)
