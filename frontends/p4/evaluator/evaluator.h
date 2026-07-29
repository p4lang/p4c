/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_EVALUATOR_EVALUATOR_H_
#define FRONTENDS_P4_EVALUATOR_EVALUATOR_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {

class IHasBlock {
 public:
    virtual IR::ToplevelBlock *getToplevelBlock() const = 0;
};

class Evaluator final : public Inspector, public IHasBlock {
    const ReferenceMap *refMap;
    const TypeMap *typeMap;
    std::vector<IR::MutablePtr<IR::Block>> blockStack;
    IR::MutablePtr<IR::ToplevelBlock> toplevelBlock;

 protected:
    void pushBlock(IR::Block *block);
    void popBlock(IR::Block *block);

 public:
    Evaluator(const ReferenceMap *refMap, const TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap), toplevelBlock(nullptr) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        setName("Evaluator");
        visitDagOnce = false;
    }
    IR::ToplevelBlock *getToplevelBlock() const override { return toplevelBlock; }

    IR::Block *currentBlock() const;
    /// Map a node to value.  The value can be nullptr, e.g., for an
    /// optional parameter.
    void setValue(const IR::Node *node, const IR::CompileTimeValue *constant);
    /// True if the node is mapped to a value, even if the value is
    /// nullptr.
    bool hasValue(const IR::Node *node) const;
    IR::Ptr<IR::CompileTimeValue> getValue(const IR::Node *node) const;

    /// Evaluates the arguments and returns a vector of parameter values
    /// ordered in the parameter order.
    std::vector<IR::Ptr<IR::CompileTimeValue>> *evaluateArguments(
        const IR::ParameterList *parameters, const IR::Vector<IR::Argument> *arguments,
        IR::Block *context);

    profile_t init_apply(const IR::Node *node) override;

    // The traversal order is controlled very explicitly
    bool preorder(const IR::P4Program *program) override;
    bool preorder(const IR::Declaration_Constant *decl) override;
    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::Declaration_Instance *inst) override;
    bool preorder(const IR::ConstructorCallExpression *inst) override;
    bool preorder(const IR::MethodCallExpression *expr) override;
    bool preorder(const IR::PathExpression *expression) override;
    bool preorder(const IR::Property *prop) override;
    bool preorder(const IR::Member *expression) override;
    bool preorder(const IR::ListExpression *expression) override;
    bool preorder(const IR::P4ListExpression *expression) override;
    bool preorder(const IR::StructExpression *expression) override;
    bool preorder(const IR::Constant *expression) override {
        setValue(expression, expression);
        return false;
    }
    bool preorder(const IR::StringLiteral *expression) override {
        setValue(expression, expression);
        return false;
    }
    bool preorder(const IR::BoolLiteral *expression) override {
        setValue(expression, expression);
        return false;
    }
    bool preorder(const IR::ListCompileTimeValue *expression) override {
        setValue(expression, expression);
        return false;
    }
    bool preorder(const IR::StructCompileTimeValue *expression) override {
        setValue(expression, expression);
        return false;
    }
    bool preorder(const IR::Declaration_ID *expression) override {
        setValue(expression, expression);
        return false;
    }

    IR::Ptr<IR::Block> processConstructor(const IR::Node *node, const IR::Type *type,
                                          const IR::Type *instanceType,
                                          const IR::Vector<IR::Argument> *arguments);
};

/// A pass which "evaluates" the program, creating Blocks for all
/// high-level constructs.
class EvaluatorPass final : public PassManager, public IHasBlock {
    P4::Evaluator *evaluator;
    std::unique_ptr<ReferenceMap> selfRefMap;

 public:
    IR::ToplevelBlock *getToplevelBlock() const override { return evaluator->getToplevelBlock(); }
    EvaluatorPass(ReferenceMap *refMap, TypeMap *typeMap);
    explicit EvaluatorPass(TypeMap *typeMap) : EvaluatorPass(nullptr, typeMap) {}
};

}  // namespace P4

#endif /* FRONTENDS_P4_EVALUATOR_EVALUATOR_H_ */
