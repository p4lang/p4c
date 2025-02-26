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

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/// @brief  Set of type declaration names that must be defined before the type can be inserted.
using InsertionSet = absl::flat_hash_set<P4::cstring>;

struct TypeSpecialization : public IHasDbPrint {
    /// Name to use for specialized type.
    cstring name;
    /// Type that is being specialized
    const IR::Type_Specialized *specialized;
    /// Declaration of specialized type, which will be replaced
    const IR::Type_Declaration *declaration;
    /// New synthesized type (created later)
    const IR::Type_StructLike *replacement;
    /// Insertion point, the specialization will be inserted after all the top-level nodes in this
    /// set.
    InsertionSet insertion;
    /// Save here the canonical types of the type arguments of 'specialized'.
    /// The typeMap will be cleared, so we cannot look them up later.
    const IR::Vector<IR::Type> *argumentTypes;
    /// was this specialization already inserted?
    bool inserted = false;

    TypeSpecialization(cstring name, const IR::Type_Specialized *specialized,
                       const IR::Type_Declaration *decl, const InsertionSet &insertion,
                       const IR::Vector<IR::Type> *argTypes)
        : name(name),
          specialized(specialized),
          declaration(decl),
          replacement(nullptr),
          insertion(insertion),
          argumentTypes(argTypes) {
        CHECK_NULL(specialized);
        CHECK_NULL(decl);
        CHECK_NULL(argTypes);
    }
    void dbprint(std::ostream &out) const override {
        out << "Specializing:" << dbp(specialized) << " from " << dbp(declaration) << " as "
            << dbp(replacement) << " inserted at ";
        format_container(out, insertion, '(', ')');
    }
};

/// @brief A signature of a *concrete* specialization. None of the parameters can be type variables
/// or generic types, therefore the specialization can always be identified globally the the names
/// of types.
struct SpecSignature {
    /// @brief  Name of the type declaration of the base (unspecialized) type (i.e. the
    /// struct/header)
    cstring baseType;
    /// @brief String representation of the type argument names.
    safe_vector<cstring> arguments;

    /// @brief Get a candidate name for the instantiation.
    std::string name() const;

    /// @brief Order for the sake of std::map only, the requirements are:
    /// - it is a total order;
    /// - no-argument specialization is always ordered before any other specializations of the type
    ///   (for the sake of lower_bound searches).
    /// Otherwise, correctness does not depend on the order.
    bool operator<(const SpecSignature &other) const {
        return std::tie(baseType, arguments) < std::tie(other.baseType, other.arguments);
    }

    /// @brief Get a specialization signature if it is valid (i.e. the type is specialized only by
    /// concrete non-generic types).
    static std::optional<SpecSignature> get(const IR::Type_Specialized *spec);
};

std::string toString(const SpecSignature &);

struct TypeSpecializationMap : IHasDbPrint {
    TypeMap *typeMap;
    ordered_map<SpecSignature, TypeSpecialization> map;

    void add(const IR::Type_Specialized *t, const IR::Type_StructLike *decl,
             NameGenerator *nameGen);
    const TypeSpecialization *get(const IR::Type_Specialized *t) const;
    void dbprint(std::ostream &out) const override {
        for (auto it : map) {
            out << toString(it.first) << " => " << it.second << std::endl;
        }
    }

    /// @brief Get a single specialization that is already available (i.e. it does not require any
    /// additional definitions) and mark it as inserted. Returns nullptr if none such exists.
    [[nodiscard]] const IR::Type_Declaration *nextAvailable();
    /// @brief  Mark the @p tdec as already present and therefore remove it from required
    /// definitions for specializations.
    void markDefined(const IR::Type_Declaration *tdec);

 private:
    friend class CreateSpecializedTypes;
    void fillInsertionSet(const IR::Type_StructLike *decl, InsertionSet &insertion);
};

/**
 * Find all generic type instantiations and their type arguments.
 */
class FindTypeSpecializations : public Inspector, ResolutionContext {
    TypeSpecializationMap *specMap;
    MinimalNameGenerator nameGen;

 public:
    explicit FindTypeSpecializations(TypeSpecializationMap *specMap) : specMap(specMap) {
        CHECK_NULL(specMap);
        setName("FindTypeSpecializations");
    }

    void postorder(const IR::Type_Specialized *type) override;
    profile_t init_apply(const IR::Node *node) override;
};

/**
 * For each generic type that is specialized with concrete arguments create a
 * specialized type version and insert it in the program.
 */
class CreateSpecializedTypes : public Modifier, public ResolutionContext {
    TypeSpecializationMap *specMap;

 public:
    explicit CreateSpecializedTypes(TypeSpecializationMap *specMap) : specMap(specMap) {
        CHECK_NULL(specMap);
        setName("CreateSpecializedTypes");
    }

    void postorder(IR::Type_Declaration *type) override;
    void postorder(IR::P4Program *prog) override;
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
    explicit SpecializeGenericTypes(TypeMap *typeMap) {
        passes.emplace_back(new PassRepeated({
            new TypeChecking(nullptr, typeMap),
            new FindTypeSpecializations(&specMap),
            new CreateSpecializedTypes(&specMap),
            // The previous pass has mutated some struct types
            new ClearTypeMap(typeMap, true),
        }));
        passes.emplace_back(new TypeChecking(nullptr, typeMap));
        passes.emplace_back(new ReplaceTypeUses(&specMap));
        // The previous pass has invalidated the types of struct expressions
        passes.emplace_back(new ClearTypeMap(typeMap, true));
        specMap.typeMap = typeMap;
        setName("SpecializeGenericTypes");
        setStopOnError(true);
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
