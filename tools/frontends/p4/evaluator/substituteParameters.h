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

/* -*-C++-*- */

#ifndef _EVALUATOR_SUBSTITUTEPARAMETERS_H_
#define _EVALUATOR_SUBSTITUTEPARAMETERS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeSubstitutionVisitor.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/parameterSubstitution.h"

namespace P4 {

class SubstituteParameters : public TypeVariableSubstitutionVisitor {
 protected:
    // When a PathExpression is cloned, it is added to the RefMap.
    // It is set to point to the same declaration as the original path.
    // But running this pass may change some declaration nodes - so
    // in general the refMap won't be up-to-date at the end.
    ReferenceMap*              refMap;  // input and output
    const ParameterSubstitution* subst;   // input
 public:
    SubstituteParameters(ReferenceMap* refMap,
                         const ParameterSubstitution* subst,
                         const TypeVariableSubstitution* tvs) :
            TypeVariableSubstitutionVisitor(tvs), refMap(refMap), subst(subst) {
        CHECK_NULL(refMap); CHECK_NULL(subst); CHECK_NULL(tvs);
        visitDagOnce = true;
        setName("SubstituteParameters");
        LOG1("Will substitute " << std::endl << subst << bindings); }
    using TypeVariableSubstitutionVisitor::postorder;
    const IR::Node* postorder(IR::PathExpression* expr) override;
    const IR::Node* postorder(IR::Type_Name* type) override;
    const IR::Node* postorder(IR::This* t) override;
};

}  // namespace P4

#endif /* _EVALUATOR_SUBSTITUTEPARAMETERS_H_ */
