#ifndef FRONTENDS_P4_TYPECHECKING_BINDVARIABLES_H_
#define FRONTENDS_P4_TYPECHECKING_BINDVARIABLES_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

// Insert explicit type specializations where they are missing
class DoBindTypeVariables : public Transform {
    IR::IndexedVector<IR::Node> *newTypes;
    TypeMap *typeMap;
    const IR::Type *getVarValue(const IR::Type_Var *var, const IR::Node *errorPosition) const;
    const IR::Node *insertTypes(const IR::Node *node);

 public:
    explicit DoBindTypeVariables(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoBindTypeVariables");
        newTypes = new IR::IndexedVector<IR::Node>();
    }

    /// Don't descend to unstructured annotations, the are not type checked, so we can't process
    /// them here either.
    const IR::Node *preorder(IR::Annotation *annotation) override;

    const IR::Node *postorder(IR::Expression *expression) override;
    const IR::Node *postorder(IR::Declaration_Instance *decl) override;
    const IR::Node *postorder(IR::MethodCallExpression *expression) override;
    const IR::Node *postorder(IR::ConstructorCallExpression *expression) override;
    const IR::Node *postorder(IR::P4Parser *parser) override { return insertTypes(parser); }
    const IR::Node *postorder(IR::P4Control *control) override { return insertTypes(control); }
};

class BindTypeVariables : public PassManager {
 public:
    explicit BindTypeVariables(TypeMap *typeMap) {
        CHECK_NULL(typeMap);
        passes.push_back(new ClearTypeMap(typeMap));
        passes.push_back(new TypeInference(typeMap, false));  // may insert casts
        passes.push_back(new DoBindTypeVariables(typeMap));
        passes.push_back(new ClearTypeMap(typeMap, true));
        setName("BindTypeVariables");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_TYPECHECKING_BINDVARIABLES_H_ */
