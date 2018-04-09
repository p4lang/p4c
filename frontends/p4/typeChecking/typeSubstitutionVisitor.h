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

#ifndef _TYPECHECKING_TYPESUBSTITUTIONVISITOR_H_
#define _TYPECHECKING_TYPESUBSTITUTIONVISITOR_H_

#include "ir/ir.h"
#include "typeSubstitution.h"

namespace P4 {

/**
 * See if a variable occurs in a Type.
 * If true, return null, else return the original type.
 */
class TypeOccursVisitor : public Inspector {
 public:
    const IR::ITypeVar* toFind;
    bool occurs;

    explicit TypeOccursVisitor(const IR::ITypeVar* toFind) : toFind(toFind), occurs(false)
    { setName("TypeOccurs"); }
    bool preorder(const IR::Type_Var* typeVariable) override;
    bool preorder(const IR::Type_InfInt* infint) override;
};

/* Replaces TypeVariables with other types */
class TypeVariableSubstitutionVisitor : public Transform {
 protected:
    const TypeVariableSubstitution* bindings;
    bool  replace;  // If true variables that map to variables are just replaced
                    // in the TypeParameterList of the replaced object; else they
                    // are removed.
    const IR::Node* replacement(IR::ITypeVar* typeVariable);
 public:
    explicit TypeVariableSubstitutionVisitor(const TypeVariableSubstitution *bindings,
                                             bool replace = false)
            : bindings(bindings), replace(replace) { setName("TypeVariableSubstitution"); }

    const IR::Node* preorder(IR::TypeParameters *tps) override;
    const IR::Node* preorder(IR::Type_Var* typeVariable) override
    { return replacement(typeVariable); }
    const IR::Node* preorder(IR::Type_InfInt* typeVariable) override
    { return replacement(typeVariable); }
};

/* Replaces TypeNames with other Types. */
class TypeNameSubstitutionVisitor : public Transform {
 protected:
    const TypeNameSubstitution* bindings;
 public:
    explicit TypeNameSubstitutionVisitor(const TypeNameSubstitution* bindings) :
            bindings(bindings) { setName("TypeNameSubstitution"); }
    const IR::Node* preorder(IR::Type_Name* typeName) override;
};

}  // namespace P4
#endif /* _TYPECHECKING_TYPESUBSTITUTIONVISITOR_H_ */
