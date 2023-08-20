/*
Copyright 2020 VMware, Inc.

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

#ifndef FRONTENDS_P4_SPECIALIZEGENERICTYPES_H_
#define FRONTENDS_P4_SPECIALIZEGENERICTYPES_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

struct TypeSpecialization : public IHasDbPrint {
    /// Name to use for specialized type.
    cstring name;
    /// Type that is being specialized
    const IR::Type_Specialized *specialized;
    /// Declaration of specialized type, which will be replaced
    const IR::Type_Declaration *declaration;
    /// New synthesized type (created later)
    const IR::Type_StructLike *replacement;
    /// Insertion point
    const IR::Node *insertion;
    /// Save here the canonical types of the type arguments of 'specialized'.
    /// The typeMap will be cleared, so we cannot look them up later.
    const IR::Vector<IR::Type> *argumentTypes;

    TypeSpecialization(cstring name, const IR::Type_Specialized *specialized,
                       const IR::Type_Declaration *decl, const IR::Node *insertion,
                       const IR::Vector<IR::Type> *argTypes)
        : name(name),
          specialized(specialized),
          declaration(decl),
          replacement(nullptr),
          insertion(insertion),
          argumentTypes(argTypes) {
        CHECK_NULL(specialized);
        CHECK_NULL(decl);
        CHECK_NULL(insertion);
        CHECK_NULL(argTypes);
    }
    void dbprint(std::ostream &out) const override {
        out << "Specializing:" << dbp(specialized) << " from " << dbp(declaration) << " as "
            << dbp(replacement) << " inserted at " << dbp(insertion);
    }
};

struct TypeSpecializationMap : public IHasDbPrint {
    ReferenceMap *refMap;
    TypeMap *typeMap;
    // The map can have multiple keys pointing to the same value
    ordered_map<const IR::Type_Specialized *, TypeSpecialization *> map;
    // Keep track of the values in the above map which are already
    // inserted in the program.
    std::set<TypeSpecialization *> inserted;

    void add(const IR::Type_Specialized *t, const IR::Type_StructLike *decl,
             const IR::Node *insertion);
    TypeSpecialization *get(const IR::Type_Specialized *t) const;
    bool same(const TypeSpecialization *left, const IR::Type_Specialized *right) const;
    void dbprint(std::ostream &out) const override {
        for (auto it : map) {
            out << dbp(it.first) << " => " << it.second << std::endl;
        }
    }
    IR::Vector<IR::Node> *getSpecializations(const IR::Node *insertionPoint) {
        IR::Vector<IR::Node> *result = nullptr;
        for (auto s : map) {
            if (inserted.find(s.second) != inserted.end()) continue;
            if (s.second->insertion == insertionPoint) {
                if (result == nullptr) result = new IR::Vector<IR::Node>();
                LOG2("Will insert " << dbp(s.second->replacement) << " before "
                                    << dbp(insertionPoint));
                result->push_back(s.second->replacement);
                inserted.emplace(s.second);
            }
        }
        return result;
    }
};

/**
 * Find all generic type instantiations and their type arguments.
 */
class FindTypeSpecializations : public Inspector {
    TypeSpecializationMap *specMap;

 public:
    explicit FindTypeSpecializations(TypeSpecializationMap *specMap) : specMap(specMap) {
        CHECK_NULL(specMap);
        setName("FindTypeSpecializations");
    }

    void postorder(const IR::Type_Specialized *type) override;
};

/**
 * For each generic type that is specialized with concrete arguments create a
 * specialized type version and insert it in the program.
 */
class CreateSpecializedTypes : public Transform {
    TypeSpecializationMap *specMap;

 public:
    explicit CreateSpecializedTypes(TypeSpecializationMap *specMap) : specMap(specMap) {
        CHECK_NULL(specMap);
        setName("CreateSpecializedTypes");
    }

    const IR::Node *insert(const IR::Node *before);
    const IR::Node *postorder(IR::Type_Declaration *type) override;
    const IR::Node *postorder(IR::Declaration *decl) override { return insert(decl); }
};

/**
 * Replace all uses of a specialized type with concrete arguments
 * with the specialized version produced in the specialization map
 */
class ReplaceTypeUses : public Transform {
    TypeSpecializationMap *specMap;

 public:
    explicit ReplaceTypeUses(TypeSpecializationMap *specMap) : specMap(specMap) {
        setName("ReplaceTypeUses");
        CHECK_NULL(specMap);
    }
    const IR::Node *postorder(IR::Type_Specialized *type) override;
    const IR::Node *postorder(IR::StructExpression *expresison) override;
};

/**
 * @brief Specializes each generic type by substituting type parameters.
 *
 * For example:
 *
```
struct S<T> { T data; }
S<bit<32>> s;
```
 *
 * generates the following code:
 *
```
struct S0 { bit<32> data; }
S0 s;
```
 */
class SpecializeGenericTypes : public PassRepeated {
    TypeSpecializationMap specMap;

 public:
    SpecializeGenericTypes(ReferenceMap *refMap, TypeMap *typeMap) {
        passes.emplace_back(new PassRepeated({
            new TypeChecking(refMap, typeMap),
            new FindTypeSpecializations(&specMap),
            new CreateSpecializedTypes(&specMap),
            // The previous pass has mutated some struct types
            new ClearTypeMap(typeMap, true),
        }));
        passes.emplace_back(new TypeChecking(refMap, typeMap));
        passes.emplace_back(new ReplaceTypeUses(&specMap));
        // The previous pass has invalidated the types of struct expressions
        passes.emplace_back(new ClearTypeMap(typeMap, true));
        specMap.refMap = refMap;
        specMap.typeMap = typeMap;
        setName("SpecializeGenericTypes");
    }
};

/// Removes all structs or stacks that are generic
//  Note: this pass is currently not used, but some
//  back-ends may find it useful.
class RemoveGenericTypes : public Transform {
 public:
    const IR::Node *postorder(IR::Type_StructLike *type) override {
        if (!type->typeParameters->empty()) return nullptr;
        return type;
    }
    const IR::Node *postorder(IR::Type_Stack *type) override {
        if (type->elementType->is<IR::Type_Specialized>()) return nullptr;
        return type;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_SPECIALIZEGENERICTYPES_H_ */
