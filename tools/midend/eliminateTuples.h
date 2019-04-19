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

#ifndef _MIDEND_ELIMINATETUPLES_H_
#define _MIDEND_ELIMINATETUPLES_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

/**
 * Maintains for each type that may contain a tuple (or be a tuple) the
 * corresponding struct replacement.
*/
class ReplacementMap {
    ordered_map<const IR::Type*, const IR::Type_Struct*> replacement;
    std::set<const IR::Type_Struct*> inserted;
    const IR::Type* convertType(const IR::Type* type);
 public:
    P4::NameGenerator* ng;
    P4::TypeMap* typeMap;

    ReplacementMap(NameGenerator* ng, TypeMap* typeMap) : ng(ng), typeMap(typeMap)
    { CHECK_NULL(ng); CHECK_NULL(typeMap); }
    const IR::Type_Struct* getReplacement(const IR::Type_Tuple* tt);
    IR::IndexedVector<IR::Node>* getNewReplacements();
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
 *     bit<32> field;
 *     bit<32> field_0;
 *   }
 *   tuple_0 t;
 *
 *   @pre none
 *   @post ensure all tuples are replaced with struct.
*/
class DoReplaceTuples final : public Transform {
    ReplacementMap* repl;

 public:
    explicit DoReplaceTuples(ReplacementMap* replMap) : repl(replMap)
    { CHECK_NULL(repl); setName("DoReplaceTuples"); }
    const IR::Node* postorder(IR::Type_Tuple* type) override;
    const IR::Node* insertReplacements(const IR::Node* before);
    const IR::Node* postorder(IR::Type_Struct* type) override
    { return insertReplacements(type); }
    const IR::Node* postorder(IR::Type_Typedef* type) override
    { return insertReplacements(type); }
    const IR::Node* postorder(IR::Type_Newtype* type) override
    { return insertReplacements(type); }
    const IR::Node* postorder(IR::P4Parser* parser) override
    { return insertReplacements(parser); }
    const IR::Node* postorder(IR::P4Control* control) override
    { return insertReplacements(control); }
    const IR::Node* postorder(IR::Method* method) override
    { return insertReplacements(method); }
    const IR::Node* postorder(IR::Type_Extern* ext) override
    { return insertReplacements(ext); }
    const IR::Node* postorder(IR::Declaration_Instance* decl) override
    { return insertReplacements(decl); }
    const IR::Node* preorder(IR::P4ValueSet* set) override
    // Disable substitution of type parameters for value sets.
    // We want to keep these as tuples.
    { prune(); return set; }
};

class EliminateTuples final : public PassManager {
 public:
    EliminateTuples(ReferenceMap* refMap, TypeMap* typeMap,
            TypeChecking* typeChecking = nullptr,
            TypeInference* typeInference = nullptr) {
        auto repl = new ReplacementMap(refMap, typeMap);
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoReplaceTuples(repl));
        passes.push_back(new ClearTypeMap(typeMap));
        // We do a round of type-checking which may mutate the program.
        // This will convert some ListExpressions
        // into StructInitializerExpression where tuples were converted
        // to structs.
        passes.push_back(new ResolveReferences(refMap));
        if (!typeInference)
            typeInference = new TypeInference(refMap, typeMap, false);
        passes.push_back(typeInference);
        setName("EliminateTuples");
    }
};

}  // namespace P4

#endif /* _MIDEND_ELIMINATETUPLES_H_ */
