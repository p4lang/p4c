/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef MIDEND_LOCAL_COPYPROP_H_
#define MIDEND_LOCAL_COPYPROP_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4 {

/* Local copy propagation and dead code elimination within a single action function.
   This pass is designed to be run after all declarations have received unique
   internal names.  This is important because the locals map uses only the
   declaration name, and not the full path.
   Requires expression types be stored inline in the expression
   (obtained by running Typechecking(updateProgram = true)).
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

    const IR::Node *postorder(IR::Declaration_Variable *) override;
    const IR::Expression *postorder(IR::PathExpression *) override;
    IR::AssignmentStatement *postorder(IR::AssignmentStatement *) override;
    IR::MethodCallExpression *postorder(IR::MethodCallExpression *) override;
    IR::Primitive *postorder(IR::Primitive *) override;
    IR::P4Action *preorder(IR::P4Action *) override;
    IR::P4Action *postorder(IR::P4Action *) override;
    class ElimDead;

    LocalCopyPropagation(const LocalCopyPropagation &) = default;
 public:
    LocalCopyPropagation() { visitDagOnce = false; setName("LocalCopyPropagation"); }
};

}  // namespace P4

#endif /* MIDEND_LOCAL_COPYPROP_H_ */
