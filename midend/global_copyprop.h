#ifndef MIDEND_GLOBAL_COPYPROP_H_
#define MIDEND_GLOBAL_COPYPROP_H_

#include <map>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/safe_vector.h"

namespace P4 {
/**
Global copy propagation, currently only operationg on control blocks where it optimizes the bodies
of actions by propagating literal values for variables used in those actions. Pass is limited
to only optimizing actions called in the 'apply' body of the control block and has no effect
on actions found in tables.
This pass is designed as an extension of the LocalCopyPropagation pass, but the logic has been
separated into a standalone pass to avoid additionally complicating the LocalCopyProp pass.
GlobalCopyPropagation pass was made with the intent of being used together with the existing
LocalCopyPropagation pass, and therefore doesn't introduce some of the features of that pass.

The logic of this pass is divided into 2 passes, an Inspector pass that collects information
on the variables used in the program and their values at the time of the action call and a
Transformer pass that uses this information to edit the action bodies.
The nature of the below mentionied optimization is such that it requires retroactive
transformations to the action bodies, and this was the reason for having 2 separate passes.

The main situation that this pass optimizes is given below:
  ...
  control ing(out bit<16> y, ...) {
    bit<16> x;
    action do_action() {
      y = x;
    }

    apply {
      x = 16w5;
      do_action();
    }
  }
  ...

, and after optimization:
  ...
  control ing(out bit<16> y, ...) {
    action do_action() {
      y = 16w5;
    }

    apply {
      do_action();
    }
  }
  ...

  @pre  This pass should be run after the LocalizeAllActions frontend pass, which ensures that each
        action is invoked only once.
*/

/**
 * This pass operates on control blocks and collects information about the state of the
 * variables in that block and later distributes this information to the Transformer pass.
 * Information is represented as a map that contains literal values for variables that need to
 * be propagated, each action node has it's own map of information.
 */
class FindVariableValues final : public Inspector {
    ReferenceMap *refMap;
    TypeMap *typeMap;
    // Container for storing constant values for variables, is used as a representation of
    // the current state of the variables in the program. Keys are variable names and values
    // are pointers to 'Expression' nodes that represent a literal value for that variable.
    std::map<cstring, const IR::Expression *> &vars;
    // Container used to store information needed for later propagating by the Transformer pass.
    // Keys for outer map are pointers to action nodes whose bodies need to be rewritten by the
    // Transformer pass and values are maps that store literal values for variables.
    // This inner map uses the name of the variable as a key and the pointer to the 'Expression'
    // node, that represents a literal, as a value.
    std::map<const IR::Node *, std::map<cstring, const IR::Expression *> *> *actions;
    // Flag for controlling which IR nodes this pass operates on
    bool working = false;

    bool preorder(const IR::IfStatement *) override;
    bool preorder(const IR::SwitchStatement *) override;
    void postorder(const IR::P4Control *) override;
    bool preorder(const IR::P4Control *) override;
    bool preorder(const IR::P4Table *) override;
    bool preorder(const IR::P4Action *) override;
    bool preorder(const IR::AssignmentStatement *) override;
    void postorder(const IR::MethodCallExpression *) override;

 public:
    FindVariableValues(
        ReferenceMap *refMap, TypeMap *typeMap,
        std::map<const IR::Node *, std::map<cstring, const IR::Expression *> *> *acts)
        : refMap(refMap),
          typeMap(typeMap),
          vars(*new std::map<cstring, const IR::Expression *>),
          actions(acts) {}
};

/**
 * This pass operates on action bodies and is ran right after the Inspector pass.
 * It propagates the values for variables that got their values assigned to them
 * before the action call.
 */
class DoGlobalCopyPropagation final : public Transform {
    ReferenceMap *refMap;
    TypeMap *typeMap;
    // Container for storing constant values for variables, used as a representation of
    // the current state of the variables in the program. Keys are variable names and values
    // are pointers to 'Expression' nodes that represent a literal value for that variable.
    std::map<cstring, const IR::Expression *> *vars = nullptr;
    // Container used to store information needed for propagating.
    // Keys for outer map are pointers to action nodes whose bodies need to be rewritten by this
    // pass and values represent maps that store literal values for variables. This inner map uses
    // the name of the variable as a key and the pointer to the 'Expression' node, that represents a
    // literal, as a value.
    std::map<const IR::Node *, std::map<cstring, const IR::Expression *> *> *actions;
    // Flag for controlling which IR nodes this pass operates on
    bool performRewrite = false;

 public:
    explicit DoGlobalCopyPropagation(
        ReferenceMap *rM, TypeMap *tM,
        std::map<const IR::Node *, std::map<cstring, const IR::Expression *> *> *acts)
        : refMap(rM), typeMap(tM), actions(acts) {}

    // Returns the stored value for the used variable
    const IR::Expression *copyprop_name(cstring name);

    IR::IfStatement *preorder(IR::IfStatement *) override;
    const IR::Expression *postorder(IR::PathExpression *) override;
    const IR::Expression *preorder(IR::ArrayIndex *) override;
    const IR::Expression *preorder(IR::Member *) override;
    const IR::Node *preorder(IR::AssignmentStatement *) override;
    const IR::P4Action *preorder(IR::P4Action *) override;
    const IR::P4Action *postorder(IR::P4Action *) override;
    IR::MethodCallExpression *postorder(IR::MethodCallExpression *) override;
};

class GlobalCopyPropagation : public PassManager {
 public:
    GlobalCopyPropagation(ReferenceMap *rM, TypeMap *tM) {
        passes.push_back(new TypeChecking(rM, tM, true));
        auto acts = new std::map<const IR::Node *, std::map<cstring, const IR::Expression *> *>;
        passes.push_back(new FindVariableValues(rM, tM, acts));
        passes.push_back(new DoGlobalCopyPropagation(rM, tM, acts));
        setName("GlobalCopyPropagation");
    }
};

}  // namespace P4

#endif /* MIDEND_GLOBAL_COPYPROP_H_ */
