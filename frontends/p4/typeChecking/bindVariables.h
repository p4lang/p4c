#ifndef _FRONTENDS_P4_TYPECHECKING_BINDVARIABLES_H_
#define _FRONTENDS_P4_TYPECHECKING_BINDVARIABLES_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4 {

// Insert explicit type specializations where they are missing
class BindTypeVariables : public Transform {
    IR::IndexedVector<IR::Node> *newTypes;
    TypeMap                     *typeMap;
    const IR::Type* getVarValue(const IR::Type_Var* var, const IR::Node* errorPosition) const;
    const IR::Node* insertTypes(const IR::Node* node);
 public:
    explicit BindTypeVariables(TypeMap* typeMap) :
            typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("BindTypeVariables");
        newTypes = new IR::IndexedVector<IR::Node>(); }
    const IR::Node* postorder(IR::Expression* expression) override;
    const IR::Node* postorder(IR::Declaration_Instance* decl) override;
    const IR::Node* postorder(IR::MethodCallExpression* expression) override;
    const IR::Node* postorder(IR::ConstructorCallExpression* expression) override;
    const IR::Node* postorder(IR::P4Parser* parser) override
    { return insertTypes(parser); }
    const IR::Node* postorder(IR::P4Control* control) override
    { return insertTypes(control); }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_TYPECHECKING_BINDVARIABLES_H_ */
