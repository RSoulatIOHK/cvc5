/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Utility for quantifiers macro definitions.
 */

#include "theory/quantifiers/quantifiers_macros.h"

#include "expr/node_algorithm.h"
#include "expr/skolem_manager.h"
#include "options/quantifiers_options.h"
#include "theory/arith/arith_msum.h"
#include "theory/quantifiers/ematching/pattern_term_selector.h"
#include "theory/quantifiers/quantifiers_registry.h"
#include "theory/quantifiers/skolemize.h"
#include "theory/quantifiers/term_database.h"
#include "theory/quantifiers/term_util.h"
#include "theory/rewriter.h"

using namespace cvc5::internal::kind;

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

QuantifiersMacros::QuantifiersMacros(Env& env, QuantifiersRegistry& qr)
    : EnvObj(env), d_qreg(qr)
{
}

Node QuantifiersMacros::solve(Node lit, bool reqGround)
{
  Trace("macros-debug") << "QuantifiersMacros::solve " << lit << std::endl;
  if (lit.getKind() != Kind::FORALL)
  {
    return Node::null();
  }
  Node body = lit[1];
  bool pol = body.getKind() != Kind::NOT;
  Node n = pol ? body : body[0];
  NodeManager* nm = nodeManager();
  if (n.getKind() == Kind::APPLY_UF)
  {
    // predicate case
    if (isMacroApplyUf(n))
    {
      Node op = n.getOperator();
      Node n_def = nm->mkConst(pol);
      Node fdef = solveEq(n, n_def);
      Assert(!fdef.isNull());
      return returnMacro(fdef, lit);
    }
  }
  else if (pol && n.getKind() == Kind::EQUAL)
  {
    // literal case
    Trace("macros-debug") << "Check macro literal : " << n << std::endl;
    std::map<Node, bool> visited;
    std::vector<Node> candidates;
    for (const Node& nc : n)
    {
      getMacroCandidates(nc, candidates, visited);
    }
    for (const Node& m : candidates)
    {
      Node op = m.getOperator();
      Trace("macros-debug") << "Check macro candidate : " << m << std::endl;
      // get definition and condition
      Node n_def = solveInEquality(m, n);  // definition for the macro
      if (n_def.isNull())
      {
        continue;
      }
      Trace("macros-debug")
          << m << " is possible macro in " << lit << std::endl;
      Trace("macros-debug")
          << "  corresponding definition is : " << n_def << std::endl;
      visited.clear();
      // cannot contain a defined operator
      if (!containsBadOp(n_def, op, reqGround))
      {
        Trace("macros-debug")
            << "...does not contain bad (recursive) operator." << std::endl;
        // must be ground UF term if mode is GROUND_UF
        if (options().quantifiers.macrosQuantMode
                != options::MacrosQuantMode::GROUND_UF
            || preservesTriggerVariables(lit, n_def))
        {
          Trace("macros-debug")
              << "...respects ground-uf constraint." << std::endl;
          Node fdef = solveEq(m, n_def);
          if (!fdef.isNull())
          {
            return returnMacro(fdef, lit);
          }
        }
      }
    }
  }
  return Node::null();
}

bool QuantifiersMacros::containsBadOp(Node n, Node op, bool reqGround)
{
  std::unordered_set<TNode> visited;
  std::unordered_set<TNode>::iterator it;
  std::vector<TNode> visit;
  TNode cur;
  visit.push_back(n);
  do
  {
    cur = visit.back();
    visit.pop_back();
    it = visited.find(cur);
    if (it == visited.end())
    {
      visited.insert(cur);
      if (cur.isClosure() && reqGround)
      {
        return true;
      }
      else if (cur == op)
      {
        return true;
      }
      else if (cur.hasOperator() && cur.getOperator() == op)
      {
        return true;
      }
      visit.insert(visit.end(), cur.begin(), cur.end());
    }
  } while (!visit.empty());
  return false;
}

bool QuantifiersMacros::preservesTriggerVariables(Node q, Node n)
{
  Assert(q.getKind() == Kind::FORALL)
      << "Expected quantified formula, got " << q;
  Node icn = d_qreg.substituteBoundVariablesToInstConstants(n, q);
  Trace("macros-debug2") << "Get free variables in " << icn << std::endl;
  std::vector<Node> var;
  quantifiers::TermUtil::computeInstConstContainsForQuant(q, icn, var);
  Trace("macros-debug2") << "Get trigger variables for " << icn << std::endl;
  std::vector<Node> trigger_var;
  inst::PatternTermSelector::getTriggerVariables(
      d_env.getOptions(), icn, q, trigger_var);
  Trace("macros-debug2") << "Done." << std::endl;
  // only if all variables are also trigger variables
  return trigger_var.size() >= var.size();
}

bool QuantifiersMacros::isMacroApplyUf(Node n)
{
  Assert(n.getKind() == Kind::APPLY_UF);
  TypeNode tno = n.getOperator().getType();
  std::map<Node, bool> vars;
  // Allow distinct bound variables and independent ground arguments.
  for (size_t i = 0, nchild = n.getNumChildren(); i < nchild; i++)
  {
    if (n[i].getKind() != Kind::BOUND_VARIABLE)
    {
      // A ground argument restricts the definition to a slice of the
      // function. solveEq preserves other inputs with a fresh remainder.
      if (expr::hasBoundVar(n[i]) || containsBadOp(n[i], n.getOperator(), true))
      {
        return false;
      }
      continue;
    }
    if (n[i].getType() != tno[i])
    {
      return false;
    }
    if (vars.find(n[i]) == vars.end())
    {
      vars[n[i]] = true;
    }
    else
    {
      return false;
    }
  }
  return true;
}

void QuantifiersMacros::getMacroCandidates(Node n,
                                           std::vector<Node>& candidates,
                                           std::map<Node, bool>& visited)
{
  if (visited.find(n) == visited.end())
  {
    visited[n] = true;
    if (n.getKind() == Kind::APPLY_UF)
    {
      if (isMacroApplyUf(n))
      {
        candidates.push_back(n);
      }
    }
    else if (n.getKind() == Kind::ADD)
    {
      for (size_t i = 0; i < n.getNumChildren(); i++)
      {
        getMacroCandidates(n[i], candidates, visited);
      }
    }
    else if (n.getKind() == Kind::MULT)
    {
      // if the LHS is a constant
      if (n.getNumChildren() == 2 && n[0].isConst())
      {
        getMacroCandidates(n[1], candidates, visited);
      }
    }
    else if (n.getKind() == Kind::NOT)
    {
      getMacroCandidates(n[0], candidates, visited);
    }
  }
}

Node QuantifiersMacros::solveInEquality(Node n, Node lit)
{
  if (lit.getKind() == Kind::EQUAL)
  {
    // return the opposite side of the equality if defined that way
    for (int i = 0; i < 2; i++)
    {
      if (lit[i] == n)
      {
        return lit[i == 0 ? 1 : 0];
      }
      else if (lit[i].getKind() == Kind::NOT && lit[i][0] == n)
      {
        return lit[i == 0 ? 1 : 0].negate();
      }
    }
    std::map<Node, Node> msum;
    if (ArithMSum::getMonomialSumLit(lit, msum))
    {
      Node veq_c;
      Node val;
      int res = ArithMSum::isolate(n, msum, veq_c, val, Kind::EQUAL);
      if (res != 0 && veq_c.isNull())
      {
        return val;
      }
    }
  }
  Trace("macros-debug") << "Cannot find for " << lit << " " << n << std::endl;
  return Node::null();
}

Node QuantifiersMacros::solveEq(Node n, Node ndef)
{
  Assert(n.getKind() == Kind::APPLY_UF);
  NodeManager* nm = nodeManager();
  Trace("macros-debug") << "Add macro eq for " << n << std::endl;
  Trace("macros-debug") << "  def: " << ndef << std::endl;
  std::vector<Node> vars;
  std::vector<Node> fvars;
  std::vector<Node> subs;
  std::vector<Node> guards;
  for (const Node& nc : n)
  {
    Node v = NodeManager::mkBoundVar(nc.getType());
    fvars.push_back(v);
    if (nc.getKind() == Kind::BOUND_VARIABLE)
    {
      vars.push_back(nc);
      subs.push_back(v);
    }
    else
    {
      guards.push_back(v.eqNode(nc));
    }
  }
  Node fdef =
      ndef.substitute(vars.begin(), vars.end(), subs.begin(), subs.end());
  Node remainder;
  if (!guards.empty())
  {
    // This is an equisatisfiable extension: the remainder can match the
    // original function outside the constrained slice. Never replace the
    // whole function by a body that only defines one slice.
    remainder = NodeManager::mkBoundVar(n.getOperator().getType());
    std::vector<Node> args = {remainder};
    args.insert(args.end(), fvars.begin(), fvars.end());
    Node fallback = nm->mkNode(Kind::APPLY_UF, args);
    fdef = nm->mkNode(Kind::ITE, nm->mkAnd(guards), fdef, fallback);
  }
  fdef =
      nm->mkNode(Kind::LAMBDA, nm->mkNode(Kind::BOUND_VAR_LIST, fvars), fdef);
  // If the definition has a free variable, it is malformed. This can happen
  // if the right hand side of a macro definition contains a variable not
  // contained in the left hand side
  Node closed = remainder.isNull()
                    ? fdef
                    : nm->mkNode(Kind::FORALL,
                                 nm->mkNode(Kind::BOUND_VAR_LIST, remainder),
                                 n.getOperator().eqNode(fdef));
  if (expr::hasFreeVar(closed))
  {
    return Node::null();
  }
  TNode op = n.getOperator();
  TNode fdeft = fdef;
  AssertEqual(op.getType(), fdef.getType());
  Node eq = op.eqNode(fdef);
  if (!remainder.isNull())
  {
    // The input is equivalent to exists remainder. eq: in the forward
    // direction the original function witnesses remainder; in the reverse
    // direction, apply eq to the arguments of the original quantified body.
    // Use negated forall to match the standard skolemization proof rule.
    return nm
        ->mkNode(Kind::FORALL,
                 nm->mkNode(Kind::BOUND_VAR_LIST, remainder),
                 eq.notNode())
        .notNode();
  }
  return eq;
}

Node QuantifiersMacros::returnMacro(Node fdef, Node lit) const
{
  Trace("macros") << "* Inferred macro " << fdef << " from " << lit
                  << std::endl;
  return fdef;
}

TrustNode QuantifiersMacros::skolemizeMacro(Node definition, TrustNode tin)
{
  Assert(definition.getKind() == Kind::NOT);
  Node q = definition[0];
  Assert(q.getKind() == Kind::FORALL && q[0].getNumChildren() == 1);
  Node k = Skolemize::getSkolemConstant(q, 0);
  Node eq = q[1][0].substitute(TNode(q[0][0]), TNode(k));
  ProofGenerator* pg = nullptr;
  if (d_env.isProofProducing() && tin.getGenerator() != nullptr)
  {
    if (d_partialProof == nullptr)
    {
      d_partialProof.reset(new LazyCDProof(
          d_env, nullptr, userContext(), "QuantifiersMacros::partial"));
    }
    // Only partial definitions need this extension. Full macros retain the
    // existing addSubstitutionSolved path, including its checked transforms.
    // The trusted equivalence relates the input to an existential definition;
    // standard skolemization then justifies choosing the remainder function.
    Node lit = tin.getProven();
    Node equiv = lit.eqNode(definition);
    d_partialProof->addTrustedStep(equiv, TrustId::SUBS_EQ, {}, {});
    d_partialProof->addLazyStep(lit, tin.getGenerator());
    d_partialProof->addStep(
        definition, ProofRule::EQ_RESOLVE, {lit, equiv}, {});
    Node nn = eq.notNode().notNode();
    d_partialProof->addStep(nn, ProofRule::SKOLEMIZE, {definition}, {});
    d_partialProof->addStep(eq, ProofRule::NOT_NOT_ELIM, {nn}, {});
    pg = d_partialProof.get();
  }
  return TrustNode::mkTrustLemma(eq, pg);
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
