; REQUIRES: unrestricted-mode
; COMMAND-LINE: --incremental --macros-quant --macros-quant-mode=all --check-unsat-cores
; EXPECT: sat
; EXPECT: (((f 0 7) 7) ((f 1 7) 99))
; EXPECT: unsat
; EXPECT: sat
; EXPECT: (((f 0 7) 8) ((f 1 7) 55))
; EXPECT: sat
; EXPECT: (((f 0 7) 9) ((f 1 7) 44))
; EXPECT: unsat
(set-logic ALL)
(set-option :produce-models true)
(set-option :global-declarations true)
(declare-fun f (Int Int) Int)
(push 1)
(assert (! (forall ((x Int)) (= (f 0 x) x)) :named first))
(assert (= (f 1 7) 99))
(check-sat)
(get-value ((f 0 7) (f 1 7)))
(push 1)
(assert (! (= (f 0 7) 8) :named bad_first))
(check-sat)
(pop 1)
(pop 1)
; Infer a different definition of the same slice after removing the first.
(push 1)
(assert (! (forall ((x Int)) (= (f 0 x) (+ x 1))) :named second))
(assert (= (f 1 7) 55))
(check-sat)
(get-value ((f 0 7) (f 1 7)))
(pop 1)
(reset-assertions)
; Re-infer again after reset, retaining declarations but no old definitions.
(assert (! (forall ((x Int)) (= (f 0 x) (+ x 2))) :named third))
(assert (= (f 1 7) 44))
(check-sat)
(get-value ((f 0 7) (f 1 7)))
(assert (! (= (f 0 7) 7) :named bad_third))
(check-sat)
