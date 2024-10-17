/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/midend/type_checker.h"

namespace BFN {

// Overrides the setTypeType method in base classes.
const IR::Type* TypeInference::setTypeType(const IR::Type* type, bool learn) {
    if (done()) return type;
    const IR::Type* typeToCanonicalize;
    if (readOnly)
        typeToCanonicalize = getOriginal<IR::Type>();
    else
        typeToCanonicalize = type;
    auto canon = canonicalize(typeToCanonicalize);
    if (canon != nullptr) {
        // Learn the new type
        if (canon != typeToCanonicalize && learn) {
            TypeInference tc(typeMap, true);
            unsigned e = errorCount();
            (void)canon->apply(tc);
            if (errorCount() > e)
                return nullptr;
        }
        auto tt = new IR::Type_Type(canon);
        setType(getOriginal(), tt);
        setType(type, tt);
    }
    return canon;
}

/**
 * IR class to represent sign conversion from bit<n> to int<n>, or vice versa.
 */
const IR::Node* TypeInference::postorder(IR::BFN::ReinterpretCast *expression) {
    if (done()) return expression;
    const IR::Type* sourceType = getType(expression->expr);
    const IR::Type* castType = getTypeType(expression->destType);
    if (sourceType == nullptr || castType == nullptr)
        return expression;

    if (!castType->is<IR::Type_Bits>() &&
        !castType->is<IR::Type_Boolean>() &&
        !castType->is<IR::Type_Newtype>() &&
        !castType->is<IR::Type_SerEnum>()) {
        error("%1%: cast not supported", expression->destType);
        return expression;
    }
    setType(expression, castType);
    setType(getOriginal(), castType);
    return expression;
}

/**
 * IR class to represent the sign extension for int<n> type.
 */
const IR::Node* TypeInference::postorder(IR::BFN::SignExtend *expression) {
    if (done()) return expression;
    const IR::Type* sourceType = getType(expression->expr);
    const IR::Type* castType = getTypeType(expression->destType);
    if (sourceType == nullptr || castType == nullptr)
        return expression;
    if (!castType->is<IR::Type_Bits>()) {
        error("%1%: cast not supported", expression->destType);
        return expression;
    }
    setType(expression, castType);
    setType(getOriginal(), castType);
    return expression;
}

const IR::Node* TypeInference::postorder(IR::Member* expression) {
    if (done()) return expression;
    auto type = getType(expression->expr);
    if (type == nullptr)
        return expression;
    cstring member = expression->member.name;
    if (type->is<IR::Type_StructLike>()) {
        if (type->is<IR::Type_Header>() || type->is<IR::Type_HeaderUnion>()) {
            if (member == "$valid") {
                // Built-in method
                auto type = IR::Type::Bits::get(1);
                auto ctype = canonicalize(type);
                if (ctype == nullptr)
                    return expression;
                setType(getOriginal(), ctype);
                setType(expression, ctype);
                setLeftValue(expression);
                setLeftValue(getOriginal<IR::Expression>());
                return expression;
            }
        }
    }
    return P4::TypeInference::postorder(expression);
}

TypeInference* TypeInference::clone() const {
    return new TypeInference(typeMap, true); }

// it might be better to allow frontend TypeChecking class to use
// custom TypeInference pass.
TypeChecking::TypeChecking(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                           bool updateExpressions) :
                           P4::TypeChecking(refMap, typeMap, updateExpressions) {
    passes.clear();
    addPasses({
        new P4::ResolveReferences(refMap),
        new BFN::TypeInference(typeMap, true), /* extended P4::TypeInference */
        updateExpressions ? new P4::ApplyTypesToExpressions(typeMap) : nullptr,
        updateExpressions ? new P4::ResolveReferences(refMap) : nullptr });
    setStopOnError(true);
}

// similarly, it might be better to avoid code duplication here.
EvaluatorPass::EvaluatorPass(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
    evaluator = new P4::Evaluator(refMap, typeMap);
    passes.emplace_back(new BFN::TypeChecking(refMap, typeMap));
    passes.emplace_back(evaluator);
}



}  // namespace BFN
