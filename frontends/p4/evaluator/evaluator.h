#ifndef _EVALUATOR_EVALUATOR_H_
#define _EVALUATOR_EVALUATOR_H_

#include "ir/ir.h"
#include "../../common/typeMap.h"
#include "../../common/resolveReferences/resolveReferences.h"
#include "blockMap.h"

namespace P4 {

class Evaluator final : public Inspector {
    BlockMap*                blockMap;
    std::vector<IR::Block*>  blockStack;

 public:
    explicit Evaluator(BlockMap* blockMap) : blockMap(blockMap) {}
    BlockMap* getBlockMap() { return blockMap; }

    IR::Block* currentBlock() const;
    void setValue(const IR::Node* node, const IR::CompileTimeValue* constant);
    const IR::CompileTimeValue* getValue(const IR::Node* node) const;

    std::vector<const IR::CompileTimeValue*>*
            evaluateArguments(const IR::Vector<IR::Expression>* arguments);

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
 private:
    ReferenceMap *refMap;
    TypeMap      *typeMap;
    BlockMap     *blockMap;

 public:
    BlockMap* getBlockMap() { return blockMap; }
    explicit EvaluatorPass(bool anyOrder);
};

}  // namespace P4

#endif /* _EVALUATOR_EVALUATOR_H_ */
