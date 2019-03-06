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

#ifndef _FRONTENDS_P4_TYPEMAP_H_
#define _FRONTENDS_P4_TYPEMAP_H_

#include "ir/ir.h"
#include "frontends/common/programMap.h"
#include "frontends/p4/typeChecking/typeSubstitution.h"

namespace P4 {
/**
Maps nodes to their canonical types.
Not all Node objects have types.
The type of each node is maintained in the typeMap.
Objects that have a type in the map:
- all Expression objects (includes Literals)
- function parameters
- variables and constant declarations
- functors (control, parser, etc.)
- functions
- interfaces
- enum fields (pointing to the enclosing enum)
- error (pointing to the error type)
- type declarations - map name to the actual type
*/
class TypeMap final : public ProgramMap {
 protected:
    // We want to have the same canonical type for two
    // different tuples or stacks with the same signature.
    std::vector<const IR::Type*> canonicalTuples;
    std::vector<const IR::Type*> canonicalStacks;

    // Map each node to its canonical type
    std::map<const IR::Node*, const IR::Type*> typeMap;
    // All left-values in the program.
    std::set<const IR::Expression*> leftValues;
    // All compile-time constants.  A compile-time constant
    // is not necessarily a constant - it could be a directionless
    // parameter as well.
    std::set<const IR::Expression*> constants;
    // For each type variable in the program the actual
    // type that is substituted for it.
    TypeVariableSubstitution allTypeVariables;

    // checks some preconditions before setting the type
    void checkPrecondition(const IR::Node* element, const IR::Type* type) const;

 public:
    TypeMap() : ProgramMap("TypeMap") {}

    bool contains(const IR::Node* element) { return typeMap.count(element) != 0; }
    void setType(const IR::Node* element, const IR::Type* type);
    const IR::Type* getType(const IR::Node* element, bool notNull = false) const;
    // unwraps a TypeType into its contents
    const IR::Type* getTypeType(const IR::Node* element, bool notNull) const;
    void dbprint(std::ostream& out) const;
    void clear();
    bool isLeftValue(const IR::Expression* expression) const
    { return leftValues.count(expression) > 0; }
    bool isCompileTimeConstant(const IR::Expression* expression) const;
    size_t size() const
    { return typeMap.size(); }

    void setLeftValue(const IR::Expression* expression);
    void setCompileTimeConstant(const IR::Expression* expression);
    void addSubstitutions(const TypeVariableSubstitution* tvs);
    const IR::Type* getSubstitution(const IR::Type_Var* var)
    { return allTypeVariables.lookup(var); }
    const TypeVariableSubstitution* getSubstitutions() const { return &allTypeVariables; }

    /// Check deep structural equivalence; defined between canonical types only.
    static bool equivalent(const IR::Type* left, const IR::Type* right);
    /// This is the same as equivalence, but it also allows some legal
    /// implicit conversions, such as a tuple type to a struct type, which
    /// is used when initializing a struct with a list expression.
    static bool implicitlyConvertibleTo(const IR::Type* from, const IR::Type* to);

    // Used for tuples and stacks only
    const IR::Type* getCanonical(const IR::Type* type);
    /// The minimum width in bits of this type.  If the width is not
    /// well-defined this will report an error and return -1.
    int minWidthBits(const IR::Type* type, const IR::Node* errorPosition);
};
}  // namespace P4

#endif /* _FRONTENDS_P4_TYPEMAP_H_ */
