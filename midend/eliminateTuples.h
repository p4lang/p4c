/*
Copyright 2016 VMware, Inc.

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

#ifndef MIDEND_ELIMINATETUPLES_H_
#define MIDEND_ELIMINATETUPLES_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Maintains for each type that may contain a tuple (or is a tuple) the
 * corresponding struct replacement.
 */
class ReplacementMap {
    ordered_map<const IR::Type *, const IR::Type_Struct *> replacement;
    std::set<const IR::Type_Struct *> inserted;
    const IR::Type *convertType(const IR::Type *type);

 public:
    P4::NameGenerator *ng;
    P4::TypeMap *typeMap;

    ReplacementMap(NameGenerator *ng, TypeMap *typeMap) : ng(ng), typeMap(typeMap) {
        CHECK_NULL(ng);
        CHECK_NULL(typeMap);
    }
    const IR::Type_Struct *getReplacement(const IR::Type_BaseList *tt);
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
 * If some of the tuple type arguments are type variables
 * the tuple is left unchanged, as below:
 * struct S<T> { tuple<T> x; }
 * This decision may need to be revisited in the future.
 * Do not replace types within P4Lists either.
 *
 *   @pre none
 *   @post ensure all tuples are replaced with struct.
 *         Notice that ListExpressions are not converted
 *         to StructExpressions; a subsequent type checking
 *         is needed for that.
 */
class DoReplaceTuples final : public Transform {
    ReplacementMap *repl;

 public:
    DoReplaceTuples(ReferenceMap *refMap, TypeMap *typeMap)
        : repl(new ReplacementMap(refMap, typeMap)) {
        setName("DoReplaceTuples");
    }
    const IR::Node *skip(const IR::Node *node) {
        prune();
        return node;
    }
    const IR::Node *postorder(IR::Type_BaseList *type) override;
    const IR::Node *insertReplacements(const IR::Node *before);
    const IR::Node *postorder(IR::Type_Struct *type) override { return insertReplacements(type); }
    const IR::Node *postorder(IR::ArrayIndex *expression) override;
    const IR::Node *postorder(IR::Type_Typedef *type) override { return insertReplacements(type); }
    const IR::Node *postorder(IR::Type_Newtype *type) override { return insertReplacements(type); }
    const IR::Node *postorder(IR::P4Parser *parser) override { return insertReplacements(parser); }
    const IR::Node *postorder(IR::P4Control *control) override {
        return insertReplacements(control);
    }
    const IR::Node *postorder(IR::Method *method) override { return insertReplacements(method); }
    const IR::Node *postorder(IR::Type_Extern *ext) override { return insertReplacements(ext); }
    const IR::Node *postorder(IR::Declaration_Instance *decl) override {
        return insertReplacements(decl);
    }
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
    EliminateTuples(ReferenceMap *refMap, TypeMap *typeMap, TypeChecking *typeChecking = nullptr,
                    TypeInference *typeInference = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoReplaceTuples(refMap, typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        // We do a round of type-checking which may mutate the program.
        // This will convert some ListExpressions
        // into StructExpression where tuples were converted
        // to structs.
        passes.push_back(new ResolveReferences(refMap));
        if (!typeInference) typeInference = new TypeInference(refMap, typeMap, false);
        passes.push_back(typeInference);
        setName("EliminateTuples");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATETUPLES_H_ */
