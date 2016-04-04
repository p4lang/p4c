#ifndef _IR_SUBSTITUTIONVISITOR_H_
#define _IR_SUBSTITUTIONVISITOR_H_

#include "ir/ir.h"
#include "substitution.h"

namespace IR {

/**
 * See if a variable occurs in a Type.
 * If true, return null, else return the original type.
 */
class TypeOccursVisitor : public Inspector {
 public:
    const IR::Type_VarBase* toFind;
    bool occurs;

    explicit TypeOccursVisitor(const IR::Type_VarBase* toFind) : toFind(toFind), occurs(false) {}
    bool preorder(const IR::Type_VarBase* typeVariable) override;
};

/* Replaces TypeVariables with other types. */
class TypeVariableSubstitutionVisitor : public Transform {
 protected:
    const TypeVariableSubstitution* bindings;
 public:
    explicit TypeVariableSubstitutionVisitor(TypeVariableSubstitution *bindings)
            : bindings(bindings) {}

    using Transform::preorder;
    const IR::Node* preorder(IR::TypeParameters *tps) override;
    const IR::Node* preorder(IR::Type_VarBase* typeVariable) override;
};

/* Replaces TypeNames with other Types. */
class TypeNameSubstitutionVisitor : public Transform {
 protected:
    const TypeNameSubstitution* bindings;
 public:
    explicit TypeNameSubstitutionVisitor(TypeNameSubstitution* bindings) :
            bindings(bindings) {}

    using Transform::preorder;
    const IR::Node* preorder(IR::Type_Name* typeName) override;
};

}  // namespace IR
#endif /* _IR_SUBSTITUTIONVISITOR_H_ */
