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

#ifndef MIDEND_COPYSTRUCTURES_H_
#define MIDEND_COPYSTRUCTURES_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

struct CopyStructuresConfig {
    /// Specific targets may allow functions or methods to return structs.
    /// Such methods will not be converted in this pass. Setting the
    /// errorOnMethodCall flag will produce an error message if such a
    ///  method is encountered.
    bool errorOnMethodCall = true;

    /// Do not only copy normal structures but also perform copy assignments for headers.
    bool copyHeaders = false;
    /// Also expand header union assignments.
    /// TODO: This is only necessary because the copy structure pass does not correctly expand
    /// header unions for some back ends.
    bool expandUnions = true;
};

/**
 * Convert assignments between structures to assignments between fields
 *
 * Examples:
 *   struct src {
 *     bit<32> hdr;
 *   }
 *   struct dst {
 *     bit<32> hdr;
 *   }
 *   action test() {
 *     dst = src;
 *   }
 *
 *   is replaced by
 *
 *   action test() {
 *     dst.hdr = src.hdr;
 *   }
 *
 *   Further, struct initialization is converted to assignment on struct fields
 *
 *   Note, header assignments are not converted in this pass.
 *
 * @pre none
 * @post
 *  - struct to struct copy is replaced by field assignment
 *  - struct initialization is replaced by field assignment
 *
 */
class DoCopyStructures : public Transform {
    /// The type map.
    TypeMap *typeMap;

    /// Configuration options.
    CopyStructuresConfig _config;

 public:
    explicit DoCopyStructures(TypeMap *typeMap, CopyStructuresConfig config)
        : typeMap(typeMap), _config(config) {
        CHECK_NULL(typeMap);
        setName("DoCopyStructures");
    }
    const IR::Node *postorder(IR::AssignmentStatement *statement) override;
};

/**
 * Analyzes an assignment between structures.  If the RHS expression
 * refers to any of the fields in the LHS then it introduces an
 * additional copy operation.  For example:
 * struct S {
 *    bit<32> a; bit<32> b;
 * }
 * S s;
 * s = { s.b, s.a };
 *
 * is replaced by:
 *
 * S tmp;
 * tmp = { s.b, s.a };
 * s = tmp;
 *
 * @pre none
 * @post no structure assignment refers on the RHS to fields that appear in the LHS.
 */
class RemoveAliases : public Transform {
    MinimalNameGenerator nameGen;
    TypeMap *typeMap;

    IR::IndexedVector<IR::Declaration> declarations;

 public:
    explicit RemoveAliases(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("RemoveAliases");
    }
    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = Transform::init_apply(node);
        node->apply(nameGen);

        return rv;
    }

    const IR::Node *postorder(IR::AssignmentStatement *statement) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::P4Control *control) override;
};

class CopyStructures : public PassRepeated {
 public:
    explicit CopyStructures(TypeMap *typeMap, CopyStructuresConfig config,
                            TypeChecking *typeChecking = nullptr) {
        CHECK_NULL(typeMap);
        setName("CopyStructures");
        if (typeChecking == nullptr) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.emplace_back(typeChecking);
        passes.emplace_back(new RemoveAliases(typeMap));
        passes.emplace_back(typeChecking);
        passes.emplace_back(new DoCopyStructures(typeMap, config));
    }
};

}  // namespace P4

#endif /* MIDEND_COPYSTRUCTURES_H_ */
