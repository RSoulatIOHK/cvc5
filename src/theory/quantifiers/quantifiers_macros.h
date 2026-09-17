/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Utility for detecting quantifier macro definitions.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__QUANTIFIERS__QUANTIFIERS_MACROS_H
#define CVC5__THEORY__QUANTIFIERS__QUANTIFIERS_MACROS_H

#include <map>
#include <memory>
#include <vector>

#include "expr/node.h"
#include "proof/lazy_proof.h"
#include "proof/trust_node.h"
#include "smt/env_obj.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class QuantifiersRegistry;

/**
 * A utility for inferring macros from quantified formulas. This can be seen as
 * a method for putting quantified formulas in solved form, e.g.
 *   forall x. P(x) ---> P = (lambda x. true)
 */
class QuantifiersMacros : protected EnvObj
{
 public:
  QuantifiersMacros(Env& env, QuantifiersRegistry& qr);
  ~QuantifiersMacros() {}
  /**
   * Infer a macro from a quantified formula forall vars. n = ndef, where n
   * applies U to distinct bound variables and possibly ground arguments.
   * Returns a solved equality U = lambda args. body for a full definition,
   * or an equivalent existential definition (encoded as negated forall)
   * for a partial definition. Returns null if no legal macro is found.
   *
   * @param lit The asserted quantified formula
   * @param reqGround Whether the definition must have no quantified subterms
   */
  Node solve(Node lit, bool reqGround = false);
  /**
   * Skolemize a partial definition returned by solve and justify its equality
   * from tin. Full definitions do not use this method: their substitution is
   * justified by the existing addSubstitutionSolved mechanism.
   */
  TrustNode skolemizeMacro(Node definition, TrustNode tin);

 private:
  /**
   * Return true if n is an APPLY_UF whose arguments are distinct bound
   * variables or ground terms independent of its operator.
   */
  bool isMacroApplyUf(Node n);
  /**
   * Returns true if n contains op, or if n contains a quantified formula
   * as a subterm and reqGround is true.
   */
  bool containsBadOp(Node n, Node op, bool reqGround);
  /**
   * Return true if n preserves trigger variables in quantified formula q, that
   * is, triggers can be inferred containing all variables in q in term n.
   */
  bool preservesTriggerVariables(Node q, Node n);
  /**
   * From n, get a list of possible subterms of n that could be the head of a
   * macro definition.
   */
  void getMacroCandidates(Node n,
                          std::vector<Node>& candidates,
                          std::map<Node, bool>& visited);
  /**
   * Solve n in literal lit, return n' such that n = n' is equivalent to lit
   * if possible, or null otherwise.
   */
  Node solveInEquality(Node n, Node lit);
  /**
   * Called when we have inferred a quantified formula is of the form
   *   forall x1 ... xn. n = ndef
   * where n applies U to the variables and possibly ground arguments.
   * Returns a solved equality, or an equivalent existential definition
   * (encoded as a negated forall) if ground arguments restrict the function
   * to a slice. Returns null if the definition has free bound variables.
   */
  Node solveEq(Node n, Node ndef);
  /**
   * Returns the macro fdef, which originated from lit. This method is for
   * debugging.
   */
  Node returnMacro(Node fdef, Node lit) const;
  /** Reference to the quantifiers registry */
  QuantifiersRegistry& d_qreg;
  /** Lazily allocated, context-dependent proofs for partial definitions only.
   */
  std::unique_ptr<LazyCDProof> d_partialProof;
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif /*CVC5__THEORY__QUANTIFIERS__QUANTIFIER_MACROS_H */
