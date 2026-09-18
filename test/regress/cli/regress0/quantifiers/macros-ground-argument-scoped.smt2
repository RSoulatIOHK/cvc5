; REQUIRES: unrestricted-mode
; COMMAND-LINE: --incremental --finite-model-find --macros-quant --macros-quant-mode=all
; EXPECT: sat
; EXPECT: unsat
; EXPECT: sat
(set-logic ALL)
(declare-sort Function 0)
(declare-fun apply (Function Int) Int)
(declare-const a Function)
(push 1)
(assert (forall ((x Int)) (= (apply a x) x)))
(check-sat)
(assert (= (apply a 0) 2))
(check-sat)
(pop 1)
(assert (= (apply a 0) 2))
(check-sat)
