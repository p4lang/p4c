/*
Copyright 2016 VMware, Inc.

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

#ifndef _MIDEND_SIMPLIFYKEY_H_
#define _MIDEND_SIMPLIFYKEY_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Policy used to decide whether a key expression is too complex.
 */
class KeyIsComplex {
 public:
    virtual ~KeyIsComplex() {}
    virtual bool isTooComplex(const IR::Expression* expression) const = 0;
};

/**
 * Policy that treats a key as complex if it's not an lvalue.
 */
class NonLeftValue : public KeyIsComplex {
 protected:
    TypeMap*      typeMap;
 public:
    explicit NonLeftValue(TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); }
    bool isTooComplex(const IR::Expression* expression) const {
        return !typeMap->isLeftValue(expression);
    }
};

/**
 * Policy that treats a key as complex if it's not either (1) an lvalue, or (2)
 * a call to isValid().
 */
class NonLeftValueOrIsValid : public NonLeftValue {
    ReferenceMap* refMap;
 public:
    NonLeftValueOrIsValid(ReferenceMap* refMap, TypeMap* typeMap)
        : NonLeftValue(typeMap), refMap(refMap)
    { CHECK_NULL(refMap); }
    bool isTooComplex(const IR::Expression* expression) const;
};

/**
 * Policy that treats a key as complex if it's not either (1) a simple lvalue,
 * or (2) a mask applied to a simple lvalue.
 */
class NonMaskLeftValue : public NonLeftValue {
 public:
    using NonLeftValue::NonLeftValue;
    bool isTooComplex(const IR::Expression* expression) const {
        if (auto mask = expression->to<IR::BAnd>()) {
            if (mask->right->is<IR::Constant>())
                expression = mask->left;
            else if (mask->left->is<IR::Constant>())
                expression = mask->right; }
        return NonLeftValue::isTooComplex(expression); }
};

class TableInsertions {
 public:
    std::vector<const IR::Declaration_Variable*> declarations;
    std::vector<const IR::AssignmentStatement*>  statements;
};

/**
 * Transform complex table keys into simpler expressions.
 *
 * \code{.cpp}
 *  table t {
 *    key = { h.a + 1 : exact; }
 *    ...
 *  }
 *  apply {
 *    t.apply();
 *  }
 * \endcode
 *
 * is transformed to
 *
 * \code{.cpp}
 *  bit<32> key_0;
 *  table t {
 *    key = { key_0 : exact; }
 *    ...
 *  }
 *  apply {
 *    key_0 = h.a + 1;
 *    t.apply();
 *  }
 * \endcode
 *
 * @pre none
 * @post all complex table key expressions are replaced with a simpler expression.
 */
class DoSimplifyKey : public Transform {
    ReferenceMap*       refMap;
    TypeMap*            typeMap;
    const KeyIsComplex* policy;
    std::map<const IR::P4Table*, TableInsertions*> toInsert;
 public:
    DoSimplifyKey(ReferenceMap* refMap, TypeMap* typeMap, const KeyIsComplex* policy) :
            refMap(refMap), typeMap(typeMap), policy(policy)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(policy); setName("DoSimplifyKey"); }
    const IR::Node* doStatement(const IR::Statement* statement, const IR::Expression* expression);

    // These should be all kinds of statements that may contain a table apply
    // after the program has been simplified
    const IR::Node* postorder(IR::MethodCallStatement* statement) override
    { return doStatement(statement, statement->methodCall); }
    const IR::Node* postorder(IR::IfStatement* statement) override
    { return doStatement(statement, statement->condition); }
    const IR::Node* postorder(IR::SwitchStatement* statement) override
    { return doStatement(statement, statement->expression); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override
    { return doStatement(statement, statement->right); }
    const IR::Node* postorder(IR::KeyElement* element) override;
    const IR::Node* postorder(IR::P4Table* table) override;
};

/**
 * This pass uses 'policy' to determine whether a key expression is too complex
 * and simplifies keys which are complex according to the policy.
 */
class SimplifyKey : public PassManager {
 public:
    SimplifyKey(ReferenceMap* refMap, TypeMap* typeMap, const KeyIsComplex* policy) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoSimplifyKey(refMap, typeMap, policy));
        setName("SimplifyKey");
    }
};

}  // namespace P4

#endif /* _MIDEND_SIMPLIFYKEY_H_ */
