; REQUIRES: unrestricted-mode
; COMMAND-LINE: --incremental --macros-quant --macros-quant-mode=all --produce-unsat-cores --check-unsat-cores
; EXPECT: unsat
; EXPECT: sat
(set-logic ALL)
(set-option :global-declarations true)
(declare-fun f (Int Int) Int)
(declare-const a Int)
(assert (! (forall ((x Int)) (= (f a x) x)) :named slice))
(assert (! (= (f a 0) 1) :named contradiction))
(check-sat)
(reset-assertions)
(assert (= (f a 0) 1))
(check-sat)
