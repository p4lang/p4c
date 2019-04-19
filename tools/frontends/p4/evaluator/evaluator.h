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

#ifndef _EVALUATOR_EVALUATOR_H_
#define _EVALUATOR_EVALUATOR_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

class IHasBlock {
 public:
    virtual IR::ToplevelBlock* getToplevelBlock() = 0;
};

class Evaluator final : public Inspector, public IHasBlock {
    const ReferenceMap*      refMap;
    const TypeMap*           typeMap;
    std::vector<IR::Block*>  blockStack;
    IR::ToplevelBlock*       toplevelBlock;

 protected:
    void pushBlock(IR::Block* block);
    void popBlock(IR::Block* block);

 public:
    Evaluator(const ReferenceMap* refMap, const TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), toplevelBlock(nullptr)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("Evaluator"); visitDagOnce = false; }
    IR::ToplevelBlock* getToplevelBlock() override { return toplevelBlock; }

    IR::Block* currentBlock() const;
    /// Map a node to value.  The value can be nullptr, e.g., for an
    /// optional parameter.
    void setValue(const IR::Node* node, const IR::CompileTimeValue* constant);
    /// True if the node is mapped to a value, even if the value is
    /// nullptr.
    bool hasValue(const IR::Node* node) const;
    const IR::CompileTimeValue* getValue(const IR::Node* node) const;

    /// Evaluates the arguments and returns a vector of parameter values
    /// ordered in the parameter order.
    std::vector<const IR::CompileTimeValue*>*
            evaluateArguments(const IR::ParameterList* parameters,
                              const IR::Vector<IR::Argument>* arguments,
                              IR::Block* context);

    profile_t init_apply(const IR::Node* node) override;

    // The traversal order is controlled very explicitly
    bool preorder(const IR::P4Program* program) override;
    bool preorder(const IR::Declaration_Constant* decl) override;
    bool preorder(const IR::P4Table* table) override;
    bool preorder(const IR::Declaration_Instance* inst) override;
    bool preorder(const IR::ConstructorCallExpression* inst) override;
    bool preorder(const IR::MethodCallExpression* expr) override;
    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::Property* prop) override;
    bool preorder(const IR::Member* expression) override;
    bool preorder(const IR::ListExpression* expression) override;
    bool preorder(const IR::Constant* expression) override
    { setValue(expression, expression); return false; }
    bool preorder(const IR::BoolLiteral* expression) override
    { setValue(expression, expression); return false; }
    bool preorder(const IR::ListCompileTimeValue* expression) override
    { setValue(expression, expression); return false; }
    bool preorder(const IR::Declaration_ID* expression) override
    { setValue(expression, expression); return false; }

    const IR::Block* processConstructor(const IR::Node* node,
                                        const IR::Type* type, const IR::Type* instanceType,
                                        const IR::Vector<IR::Argument>* arguments);
};

// A pass which "evaluates" the program
class EvaluatorPass final : public PassManager, public IHasBlock {
    P4::Evaluator* evaluator;
 public:
    IR::ToplevelBlock* getToplevelBlock() override { return evaluator->getToplevelBlock(); }
    EvaluatorPass(ReferenceMap* refMap, TypeMap* typeMap);
};

}  // namespace P4

#endif /* _EVALUATOR_EVALUATOR_H_ */
