#ifndef MIDEND_LOCAL_COPYPROP_H_
#define MIDEND_LOCAL_COPYPROP_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4 {

/* Local copy propagation and dead code elimination within a single action function.
 * - requires expression types be up-to-date with Typechecking(updateProgram = true)
 */
class LocalCopyPropagation : public ControlFlowVisitor, Transform, P4WriteContext {
    bool                        in_action = false;
    struct Local {
        bool                    live = false;
        const IR::Expression    *val = nullptr;
    };
    std::map<cstring, Local>    locals;
    LocalCopyPropagation *clone() const override { return new LocalCopyPropagation(*this); }
    void flow_merge(Visitor &) override;
    void dropLocalsUsing(cstring);

    IR::Declaration_Variable *postorder(IR::Declaration_Variable *) override;
    const IR::Expression *postorder(IR::PathExpression *) override;
    IR::AssignmentStatement *postorder(IR::AssignmentStatement *) override;
    IR::MethodCallExpression *postorder(IR::MethodCallExpression *) override;
    IR::Primitive *postorder(IR::Primitive *) override;
    IR::P4Action *preorder(IR::P4Action *) override;
    IR::P4Action *postorder(IR::P4Action *) override;
    class ElimDead;

    LocalCopyPropagation(const LocalCopyPropagation &) = default;
 public:
    LocalCopyPropagation() {}
};

}  // namespace P4

#endif /* MIDEND_LOCAL_COPYPROP_H_ */
