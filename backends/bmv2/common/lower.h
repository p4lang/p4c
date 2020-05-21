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

#ifndef BACKENDS_BMV2_COMMON_LOWER_H_
#define BACKENDS_BMV2_COMMON_LOWER_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace BMV2 {

/**
  This pass rewrites expressions which are not supported natively on BMv2.
*/
class LowerExpressions : public Transform {
    P4::TypeMap* typeMap;
    // Cannot shift with a value larger than 8 bits
    const int maxShiftWidth = 8;

    const IR::Expression* shift(const IR::Operation_Binary* expression) const;
 public:
    explicit LowerExpressions(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("LowerExpressions"); }

    const IR::Node* postorder(IR::Expression* expression) override;
    const IR::Node* postorder(IR::Shl* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Shr* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Cast* expression) override;
    const IR::Node* postorder(IR::Neg* expression) override;
    const IR::Node* postorder(IR::Slice* expression) override;
    const IR::Node* postorder(IR::Concat* expression) override;
    const IR::Node* preorder(IR::P4Table* table) override
    { prune(); return table; }  // don't simplify expressions in table
};

/**
Policy which selects the control blocks where remove
complex expression is applied.
*/
class RemoveComplexExpressionsPolicy {
 public:
    virtual ~RemoveComplexExpressionsPolicy() {}
    /**
       If the policy returns true the control block is processed,
       otherwise it is left unchanged.
    */
    virtual bool convert(const IR::P4Control* control) const = 0;
};

/**
BMv2 does not support complex expressions for a select
or as arguments to external functions.
Such expressions are lifted into a temporaries.
Also, convert a statement like lookahead<T>() into
tmp = lookahead<T>();
*/
class RemoveComplexExpressions : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    RemoveComplexExpressionsPolicy* policy;
    IR::IndexedVector<IR::Declaration> newDecls;
    IR::IndexedVector<IR::StatOrDecl>  assignments;

    const IR::PathExpression* createTemporary(const IR::Expression* expression);
    const IR::Expression* simplifyExpression(const IR::Expression* expression, bool force);
    const IR::Vector<IR::Expression>* simplifyExpressions(
        const IR::Vector<IR::Expression>* vec, bool force = false);
    const IR::Vector<IR::Argument>* simplifyExpressions(
        const IR::Vector<IR::Argument>* vec);
    const IR::IndexedVector<IR::NamedExpression>* simplifyExpressions(
        const IR::IndexedVector<IR::NamedExpression>* vec);

    const IR::Node* simpleStatement(IR::Statement* statement);

 public:
    RemoveComplexExpressions(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                             RemoveComplexExpressionsPolicy* policy = nullptr) :
            refMap(refMap), typeMap(typeMap), policy(policy) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("RemoveComplexExpressions"); }
    const IR::Node* postorder(IR::SelectExpression* expression) override;
    const IR::Node* preorder(IR::ParserState* state) override
    { assignments.clear(); return state; }
    const IR::Node* postorder(IR::ParserState* state) override {
        state->components.append(assignments);
        return state;
    }
    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    const IR::Node* preorder(IR::P4Parser* parser) override
    { newDecls.clear(); return parser; }
    const IR::Node* postorder(IR::P4Parser* parser) override {
        parser->parserLocals.append(newDecls);
        return parser;
    }
    const IR::Node* preorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Control* control) override {
        if (newDecls.size() != 0) {
            // prepend declarations
            newDecls.append(control->controlLocals);
            control->controlLocals = newDecls;
        }
        return control;
    }
    const IR::Node* postorder(IR::Statement* statement) override;
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_LOWER_H_ */
