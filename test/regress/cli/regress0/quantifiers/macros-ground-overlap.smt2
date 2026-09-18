; REQUIRES: unrestricted-mode
; COMMAND-LINE: --incremental --finite-model-find --macros-quant --macros-quant-mode=all
; EXPECT: sat
; EXPECT: unsat
; EXPECT: sat
(set-logic ALL)
(declare-fun f ((_ BitVec 2) (_ BitVec 2)) (_ BitVec 2))
(declare-const a (_ BitVec 2))
(declare-const b (_ BitVec 2))
(assert (= a b))
(assert (forall ((x (_ BitVec 2))) (= (f a x) x)))
(assert (forall ((y (_ BitVec 2))) (= (f b y) y)))
(check-sat)
(push 1)
(assert (forall ((z (_ BitVec 2))) (= (f b z) (bvnot z))))
(check-sat)
(pop 1)
(check-sat)
