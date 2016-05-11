#ifndef _EVALUATOR_EVALUATOR_H_
#define _EVALUATOR_EVALUATOR_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

class Evaluator final : public Inspector {
    ReferenceMap*            refMap;
    TypeMap*                 typeMap;
    std::vector<IR::Block*>  blockStack;
    IR::ToplevelBlock*       toplevelBlock;

 protected:
    void pushBlock(IR::Block* block);
    void popBlock(IR::Block* block);

 public:
    Evaluator(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), toplevelBlock(nullptr)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("Evaluator"); }
    IR::ToplevelBlock* getToplevelBlock() { return toplevelBlock; }

    IR::Block* currentBlock() const;
    void setValue(const IR::Node* node, const IR::CompileTimeValue* constant);
    const IR::CompileTimeValue* getValue(const IR::Node* node) const;

    std::vector<const IR::CompileTimeValue*>*
            evaluateArguments(const IR::Vector<IR::Expression>* arguments,
                              IR::Block* context);

    profile_t init_apply(const IR::Node* node) override;

    // The traversal order is controlled very explicitly
    bool preorder(const IR::P4Program* program) override;
    bool preorder(const IR::Declaration_Constant* decl) override;
    bool preorder(const IR::P4Table* table) override;
    bool preorder(const IR::Declaration_Instance* inst) override;
    bool preorder(const IR::ConstructorCallExpression* inst) override;
    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::TableProperty* prop) override;
    bool preorder(const IR::Member* expression) override;
    bool preorder(const IR::Constant* expression) override
    { setValue(expression, expression); return false; }
    bool preorder(const IR::BoolLiteral* expression) override
    { setValue(expression, expression); return false; }
    bool preorder(const IR::Declaration_ID* expression) override
    { setValue(expression, expression); return false; }

    const IR::Block* processConstructor(const IR::Node* node,
                                        const IR::Type* type, const IR::Type* instanceType,
                                        const IR::Vector<IR::Expression>* arguments);
};

// A pass which "evaluates" the program
class EvaluatorPass final : public PassManager {
    P4::Evaluator* evaluator;
 public:
    IR::ToplevelBlock* getToplevelBlock() { return evaluator->getToplevelBlock(); }
    EvaluatorPass(ReferenceMap* refMap, TypeMap* typeMap, bool anyOrder);
};

}  // namespace P4

#endif /* _EVALUATOR_EVALUATOR_H_ */
