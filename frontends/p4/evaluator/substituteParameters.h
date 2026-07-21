/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* -*-C++-*- */

#ifndef FRONTENDS_P4_EVALUATOR_SUBSTITUTEPARAMETERS_H_
#define FRONTENDS_P4_EVALUATOR_SUBSTITUTEPARAMETERS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/typeChecking/typeSubstitutionVisitor.h"
#include "ir/ir.h"

namespace P4 {

class SubstituteParameters : public TypeVariableSubstitutionVisitor, public ResolutionContext {
 protected:
    const DeclarationLookup *refMap;     // input
    const ParameterSubstitution *subst;  // input
 public:
    SubstituteParameters(const DeclarationLookup *refMap, const ParameterSubstitution *subst,
                         const TypeVariableSubstitution *tvs)
        : TypeVariableSubstitutionVisitor(tvs), refMap(refMap), subst(subst) {
        CHECK_NULL(subst);
        CHECK_NULL(tvs);
        visitDagOnce = true;
        setName("SubstituteParameters");
        LOG1("Will substitute " << std::endl << subst << bindings);
    }
    using TypeVariableSubstitutionVisitor::postorder;
    const IR::Node *postorder(IR::PathExpression *expr) override;
    const IR::Node *postorder(IR::Type_Name *type) override;
    const IR::Node *postorder(IR::This *t) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_EVALUATOR_SUBSTITUTEPARAMETERS_H_ */
