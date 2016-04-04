/* -*-C++-*- */

#ifndef _EVALUATOR_SUBSTITUTEPARAMETERS_H_
#define _EVALUATOR_SUBSTITUTEPARAMETERS_H_

#include "ir/ir.h"
#include "ir/substitutionVisitor.h"
#include "../../common/resolveReferences/referenceMap.h"
#include "ir/parameterSubstitution.h"

namespace P4 {

class SubstituteParameters final : public IR::TypeVariableSubstitutionVisitor {
    ReferenceMap*          refMap;  // input
    IR::ParameterSubstitution* subst;   // input
 public:
    SubstituteParameters(ReferenceMap* refMap,
                         IR::ParameterSubstitution* subst,
                         IR::TypeVariableSubstitution* tvs) :
            TypeVariableSubstitutionVisitor(tvs), refMap(refMap), subst(subst) {
        CHECK_NULL(refMap); CHECK_NULL(subst); CHECK_NULL(tvs);
        visitDagOnce = true;
        LOG1("Will substitute " << std::endl << subst << bindings); }
    using IR::TypeVariableSubstitutionVisitor::postorder;
    const IR::Node* postorder(IR::PathExpression* expr) override;
    const IR::Node* postorder(IR::Type_Name* type) override;
};

}  // namespace P4

#endif /* _EVALUATOR_SUBSTITUTEPARAMETERS_H_ */
