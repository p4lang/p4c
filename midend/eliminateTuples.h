/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_ELIMINATETUPLES_H_
#define MIDEND_ELIMINATETUPLES_H_

#include <set>
#include <vector>

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/specializeGenericTypes.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

namespace P4 {

/**
 * Maintains for each type that may contain a tuple (or is a tuple) the
 * corresponding struct replacement.
 *
 * The shape of a tuple is its nesting structure, e.g. "(_,(_,_))" for
 * tuple<T, tuple<bit<8>, bool>>. A shape is generic if a tuple of that shape
 * contains a type variable, or is nested in such a tuple. A tuple of a generic
 * shape is replaced by a specialization of the generic struct of the shape.
 * Every other tuple is replaced by a struct.
 */
class ReplacementMap {
    /// Maps a tuple, or a type that contains one, to its replacement.
    ordered_map<const IR::Type *, const IR::Type *> replacement;
    /// Struct declarations to insert, in creation order.
    std::vector<const IR::Type_Struct *> declarations;
    std::set<const IR::Type_Struct *> inserted;
    ordered_set<cstring> genericShapes;
    /// The generic struct of each generic shape, created on first use.
    ordered_map<cstring, const IR::Type_Struct *> genericStructs;

    const IR::Type *convertType(const IR::Type *type);
    /// @returns the replacement of @p tuple.
    const IR::Type *convertTuple(const IR::Type_BaseList *tuple);
    /// @returns the specialization of the generic struct of @p shape for @p tuple.
    const IR::Type_Specialized *specializeGenericStruct(const IR::Type_BaseList *tuple,
                                                        cstring shape);
    /// @returns the generic struct of @p shape, with one type parameter per non-tuple component
    /// of @p tuple and of its nested tuples.
    const IR::Type_Struct *getGenericStruct(const IR::Type_BaseList *tuple, cstring shape);
    /// Marks the shape of @p tuple and the shapes of its nested tuples as generic.
    void addGenericShape(const IR::Type_BaseList *tuple);
    cstring shapeOf(const IR::Type_BaseList *tuple) const;
    /// Appends the non-tuple components of @p tuple and of its nested tuples to @p components.
    void flattenComponents(const IR::Type_BaseList *tuple,
                           std::vector<const IR::Type *> &components) const;
    /// @returns the canonical type of @p type, or @p type if the type map has none.
    const IR::Type *canonical(const IR::Type *type) const;

 public:
    NameGenerator &ng;
    P4::TypeMap *typeMap;

    ReplacementMap(NameGenerator &ng, TypeMap *typeMap) : ng(ng), typeMap(typeMap) {
        CHECK_NULL(typeMap);
    }
    /// Finds the generic shapes of @p program. Call this before the first getReplacement.
    void findGenericShapes(const IR::Node *program);
    /// @returns true if the program has a generic shape.
    bool hasGenericShapes() const { return !genericShapes.empty(); }
    const IR::Type *getReplacement(const IR::Type_BaseList *tt);
    IR::IndexedVector<IR::Node> *getNewReplacements();
};

/**
 * Implement a pass that convert each Tuple type into a Struct.
 *
 * The struct is inserted in the program replacing the tuple.
 * We make up field names for the struct fields.
 * Note that the replacement has to be recursive, since the tuple
 * could contain types that contain other tuples.
 *
 * Example:
 *   tuple<bit<32>, bit<32>> t;
 *
 * is replaced by
 *
 *   struct tuple_0 {
 *     bit<32> f0;
 *     bit<32> f1;
 *   }
 *   tuple_0 t;
 *
 * If a tuple contains a type variable, every tuple of the same shape is
 * replaced by a specialization of one generic struct. The generic declaration
 * and its concrete uses then have matching types:
 *
 *   extern E<T> { tuple<T, T> get(); }
 *   tuple<bit<32>, bit<32>> t = e.get();
 *
 * is replaced by
 *
 *   struct tuple_0<T0, T1> {
 *     T0 f0;
 *     T1 f1;
 *   }
 *   extern E<T> { tuple_0<T, T> get(); }
 *   tuple_0<bit<32>, bit<32>> t = e.get();
 *
 * SpecializeGenericTypes then replaces tuple_0<bit<32>, bit<32>> with a struct.
 *
 * Not supported: a type variable that is bound to a tuple. Type checking
 * rejects the resulting self-referential struct.
 *
 * Do not replace types within P4Lists.
 *
 *   @pre none
 *   @post ensure all tuples are replaced with structs or with
 *         specializations of generic structs.
 *         Notice that ListExpressions are not converted
 *         to StructExpressions; a subsequent type checking
 *         is needed for that.
 */
class DoReplaceTuples final : public Transform {
    ReplacementMap repl;
    MinimalNameGenerator ng;

 public:
    explicit DoReplaceTuples(TypeMap *typeMap) : repl(ng, typeMap) { setName("DoReplaceTuples"); }
    Visitor::profile_t init_apply(const IR::Node *node) override;
    /// @returns true if the program has a generic shape.
    bool hasGenericShapes() const { return repl.hasGenericShapes(); }
    const IR::Node *skip(const IR::Node *node) {
        prune();
        return node;
    }
    const IR::Node *postorder(IR::Type_BaseList *type) override;
    /// Inserts the new struct declarations in front of the top-level declaration @p before.
    const IR::Node *insertReplacements(const IR::Node *before);
    const IR::Node *postorder(IR::ArrayIndex *expression) override;
    /// Any top-level declaration can use a tuple, including parser, control, and package types.
    const IR::Node *postorder(IR::Node *node) override { return insertReplacements(node); }
    const IR::Node *preorder(IR::P4ValueSet *set) override {
        // Disable substitution of type parameters for value sets.
        // We want to keep these as tuples.
        return skip(set);
    }
    const IR::Node *preorder(IR::P4ListExpression *expression) override { return skip(expression); }
    const IR::Node *preorder(IR::Type_P4List *list) override { return skip(list); }
};

class EliminateTuples final : public PassManager {
 public:
    explicit EliminateTuples(TypeMap *typeMap, TypeChecking *typeChecking = nullptr,
                             TypeInference *typeInference = nullptr) {
        passes.push_back(typeChecking ? typeChecking : new TypeChecking(nullptr, typeMap));
        auto *replaceTuples = new DoReplaceTuples(typeMap);
        passes.push_back(replaceTuples);
        passes.push_back(new ClearTypeMap(typeMap));
        // We do a round of type-checking which may mutate the program.
        // This will convert some ListExpressions
        // into StructExpression where tuples were converted
        // to structs.
        passes.push_back(typeInference ? typeInference : new TypeInference(typeMap, false));
        // Replace the concrete specializations of the generic tuple structs with structs.
        passes.push_back(
            new PassIf([replaceTuples]() { return replaceTuples->hasGenericShapes(); },
                       {new SpecializeGenericTypes(typeMap), new TypeInference(typeMap, false)}));
        setName("EliminateTuples");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATETUPLES_H_ */
