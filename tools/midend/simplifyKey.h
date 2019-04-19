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
 * Policy used to decide whether a key expression is simple enough
 * to be implemented directly.
 */
class KeyIsSimple {
 public:
    virtual ~KeyIsSimple() {}
    virtual bool isSimple(const IR::Expression* expression, const Visitor::Context*) = 0;
};

/**
 * Policy that treats a key as simple if it contains just
 * PathExpression, member and ArrayIndex operations.
 */
class IsLikeLeftValue : public KeyIsSimple, public Inspector {
 protected:
    TypeMap* typeMap;
    bool     simple;

 public:
    IsLikeLeftValue()
    { setName("IsLikeLeftValue"); }

    void postorder(const IR::Expression*) override {
        // all other expressions are complicated
        simple = false;
    }
    void postorder(const IR::Member*) override {}
    void postorder(const IR::PathExpression*) override {}
    void postorder(const IR::ArrayIndex*) override {}
    profile_t init_apply(const IR::Node* root) override {
        simple = true;
        return Inspector::init_apply(root); }

    bool isSimple(const IR::Expression* expression, const Visitor::Context*) override {
        (void)expression->apply(*this);
        return simple;
    }
};

/**
 * Policy that treats a key as simple if it a call to isValid().
 */
class IsValid : public KeyIsSimple {
    ReferenceMap* refMap;
    TypeMap* typeMap;

 public:
    IsValid(ReferenceMap* refMap, TypeMap* typeMap) : refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    bool isSimple(const IR::Expression* expression, const Visitor::Context*);
};

/**
 * Policy that treats a key as simple if it is a mask applied to an lvalue
 * or just an lvalue.
 */
class IsMask : public IsLikeLeftValue {
 public:
    bool isSimple(const IR::Expression* expression, const Visitor::Context* ctxt) {
        if (auto mask = expression->to<IR::BAnd>()) {
            if (mask->right->is<IR::Constant>())
                expression = mask->left;
            else if (mask->left->is<IR::Constant>())
                expression = mask->right; }
        return IsLikeLeftValue::isSimple(expression, ctxt); }
};

/**
    A KeyIsSimple policy formed by combining two
    other policies with a logical 'or'.
*/
class OrPolicy : public KeyIsSimple {
    KeyIsSimple* left;
    KeyIsSimple* right;

 public:
    OrPolicy(KeyIsSimple* left, KeyIsSimple* right) : left(left), right(right)
    { CHECK_NULL(left); CHECK_NULL(right); }
    bool isSimple(const IR::Expression* expression, const Visitor::Context* ctxt) {
        return left->isSimple(expression, ctxt) || right->isSimple(expression, ctxt);
    }
};

class TableInsertions {
 public:
    std::vector<const IR::Declaration_Variable*> declarations;
    std::vector<const IR::AssignmentStatement*> statements;
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
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    KeyIsSimple*  key_policy;
    std::map<const IR::P4Table*, TableInsertions*> toInsert;

 public:
    DoSimplifyKey(ReferenceMap* refMap, TypeMap* typeMap, KeyIsSimple* key_policy)
        : refMap(refMap), typeMap(typeMap), key_policy(key_policy)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(key_policy); setName("DoSimplifyKey"); }
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
    SimplifyKey(ReferenceMap* refMap, TypeMap* typeMap, KeyIsSimple* key_policy,
                TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoSimplifyKey(refMap, typeMap, key_policy));
        setName("SimplifyKey");
    }
};

}  // namespace P4

#endif /* _MIDEND_SIMPLIFYKEY_H_ */
