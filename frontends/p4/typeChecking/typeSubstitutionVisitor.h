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

#ifndef FRONTENDS_P4_TYPECHECKING_TYPESUBSTITUTIONVISITOR_H_
#define FRONTENDS_P4_TYPECHECKING_TYPESUBSTITUTIONVISITOR_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/log.h"
#include "typeSubstitution.h"

namespace P4 {

/**
 * See if a variable occurs in a Type.
 * If true, return null, else return the original type.
 */
class TypeOccursVisitor : public Inspector {
 public:
    const IR::ITypeVar *toFind;
    bool occurs;

    explicit TypeOccursVisitor(const IR::ITypeVar *toFind) : toFind(toFind), occurs(false) {
        setName("TypeOccurs");
    }
    bool preorder(const IR::Type_Var *typeVariable) override;
    bool preorder(const IR::Type_InfInt *infint) override;
};

/* Replaces TypeVariables with other types */
class TypeVariableSubstitutionVisitor : public Transform {
 protected:
    const TypeVariableSubstitution *bindings;
    bool replace;      // If true variables that map to variables are just replaced
                       // in the TypeParameterList of the replaced object; else they
                       // are removed.
    bool cloneInfInt;  // If true, ensure that Type_InfInt nodes are replaced with
                       // new ones as fresh type variables
    const IR::Node *replacement(const IR::ITypeVar *original, const IR::Node *node);

 public:
    explicit TypeVariableSubstitutionVisitor(const TypeVariableSubstitution *bindings,
                                             bool replace = false, bool cloneInfInt = false)
        : bindings(bindings), replace(replace), cloneInfInt(cloneInfInt) {
        setName("TypeVariableSubstitution");
    }

    const IR::Node *preorder(IR::TypeParameters *tps) override;
    const IR::Node *preorder(IR::Type_Any *tv) override {
        return replacement(getOriginal<IR::Type_Any>(), tv);
    }
    const IR::Node *preorder(IR::Type_Var *tv) override {
        return replacement(getOriginal<IR::Type_Var>(), tv);
    }
    const IR::Node *preorder(IR::Type_InfInt *ti) override {
        const auto *n = cloneInfInt ? IR::Type_InfInt::get() : ti;
        return replacement(getOriginal<IR::Type_InfInt>(), n);
    }
};

class TypeSubstitutionVisitor : public TypeVariableSubstitutionVisitor {
    TypeMap *typeMap;

 public:
    TypeSubstitutionVisitor(TypeMap *typeMap, TypeVariableSubstitution *ts)
        : TypeVariableSubstitutionVisitor(ts), typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("TypeSubstitutionVisitor");
    }
    const IR::Node *postorder(IR::PathExpression *path) override {
        // We want fresh nodes for variables, etc.
        return new IR::PathExpression(path->path->clone());
    }
    const IR::Node *postorder(IR::Type_Name *type) override {
        auto actual = typeMap->getTypeType(getOriginal<IR::Type_Name>(), true);
        if (auto tv = actual->to<IR::ITypeVar>()) {
            LOG3("Replacing " << tv);
            return replacement(tv, type);
        }
        return type;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_TYPECHECKING_TYPESUBSTITUTIONVISITOR_H_ */
