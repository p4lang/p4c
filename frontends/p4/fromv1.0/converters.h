#ifndef _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_
#define _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_

#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"
#include "programStructure.h"

namespace P4V1 {

// Converts expressions from v1.0 to v1.2
// However, the type in each expression is still a v1.0 type.
class ExpressionConverter : public Transform {
 protected:
    ProgramStructure* structure;
    P4::P4CoreLibrary &p4lib;
 public:
    bool replaceNextWithLast;  // if true p[next] becomes p.last
    explicit ExpressionConverter(ProgramStructure* structure)
            : structure(structure), p4lib(P4::P4CoreLibrary::instance),
              replaceNextWithLast(false) {}
    const IR::Type* getFieldType(const IR::Type_StructLike* ht, cstring fieldName);
    const IR::Node* postorder(IR::Constant* expression) override;
    const IR::Node* postorder(IR::Member* field) override;
    const IR::Node* postorder(IR::FieldList* fl) override;
    const IR::Node* postorder(IR::Mask* expression) override;
    const IR::Node* postorder(IR::ActionArg* arg) override;
    const IR::Node* postorder(IR::Primitive* primitive) override;
    const IR::Node* postorder(IR::NamedRef* ref) override;
    const IR::Node* postorder(IR::ConcreteHeaderRef* nhr) override;
    const IR::Node* postorder(IR::HeaderStackItemRef* ref) override;
    const IR::Expression* convert(const IR::Node* node) {
        auto result = node->apply(*this);
        return result->to<IR::Expression>();
    }
};

class StatementConverter : public ExpressionConverter {
    std::map<cstring, cstring> *renameMap;
 public:
    StatementConverter(ProgramStructure* structure, std::map<cstring, cstring> *renameMap)
            : ExpressionConverter(structure), renameMap(renameMap) {}

    const IR::Node* preorder(IR::Apply* apply) override;
    const IR::Node* preorder(IR::Primitive* primitive) override;
    const IR::Node* preorder(IR::If* cond) override;
    const IR::Statement* convert(const IR::Vector<IR::Expression>* toConvert);

    const IR::Statement* convert(const IR::Node* node) {
        auto conv = node->apply(*this);
        auto result = conv->to<IR::Statement>();
        BUG_CHECK(result != nullptr, "Conversion of %1% did not produce a statement", node);
        return result;
    }
};

// Is fed a P4 v1.0 program and outputs an equivalent P4 v1.2 program
class Converter : public PassManager {
    ProgramStructure structure;

 public:
    Converter();
    void loadModel() { structure.loadModel(); }
    Visitor::profile_t init_apply(const IR::Node* node) override;
};

}  // namespace P4V1

#endif /* _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_ */
