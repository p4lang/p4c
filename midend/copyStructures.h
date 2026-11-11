/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_COPYSTRUCTURES_H_
#define MIDEND_COPYSTRUCTURES_H_

#include "frontends/p4/parserControlFlow.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

struct CopyStructuresConfig {
    /// Specific targets may allow functions or methods to return structs.
    /// Such methods will not be converted in this pass. Setting the
    /// errorOnMethodCall flag will produce an error message if such a
    /// method is encountered.
    bool errorOnMethodCall = true;

    /// Do not only copy normal structures but also perform copy assignments for headers.
    bool copyHeaders = false;
    /// Expand header union assignments into conditional copies of the valid member, or
    /// invalidate the destination if the source is invalid. Disabled by default so that
    /// targets can implement whole-union assignments directly.
    bool expandUnions = false;
    /// Also split up assignments of tuples.
    bool copyTuples = false;
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
 *   header assignments and tuple assignments are optionally converted in this pass, based
 *   on the configuration
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
    CopyStructuresConfig config;
    bool *expandedUnionInParser;

 public:
    explicit DoCopyStructures(TypeMap *typeMap, CopyStructuresConfig config,
                              bool *expandedUnionInParser = nullptr)
        : typeMap(typeMap), config(config), expandedUnionInParser(expandedUnionInParser) {
        CHECK_NULL(typeMap);
        setName("DoCopyStructures");
    }
    explicit DoCopyStructures(TypeMap *typeMap, bool errorOnMethodCall, bool copyHeaders = false,
                              bool copyTuples = true)
        : DoCopyStructures(typeMap, {.errorOnMethodCall = errorOnMethodCall,
                                     .copyHeaders = copyHeaders,
                                     .copyTuples = copyTuples}) {}
    // FIXME -- this should be a preorder so we can deal with nested structs directly,
    // but that fails because we depend on the typeMap which will be out of date after
    // expanding outer copies.  So we need to repeat this pass in a loop
    const IR::Node *postorder(IR::AssignmentStatement *statement) override;
    /// The caller must remove aliases before expanding a whole-union assignment.
    static const IR::Statement *expandHeaderUnion(const IR::AssignmentStatement *statement,
                                                  const IR::Type_HeaderUnion *headerUnion);
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

class CopyStructures : public PassManager {
    bool expandedUnionInParser = false;

 public:
    explicit CopyStructures(TypeMap *typeMap, CopyStructuresConfig config,
                            TypeChecking *typeChecking = nullptr) {
        CHECK_NULL(typeMap);
        setName("CopyStructures");
        if (typeChecking == nullptr) typeChecking = new TypeChecking(nullptr, typeMap);
        addPasses(
            {[this] { expandedUnionInParser = false; },
             new PassRepeated({typeChecking, new RemoveAliases(typeMap), typeChecking,
                               new DoCopyStructures(typeMap, config, &expandedUnionInParser)})});
        // Union copies introduce conditionals. Lower those in parsers before a later pass
        // such as ParsersUnroll requires states with straight-line bodies.
        if (config.expandUnions)
            addPasses({new PassIf([this] { return expandedUnionInParser; },
                                  {new RemoveParserIfs(typeMap)})});
    }
    explicit CopyStructures(TypeMap *typeMap, bool errorOnMethodCall = true,
                            bool copyHeaders = false, bool copyTuples = false,
                            TypeChecking *typeChecking = nullptr)
        : CopyStructures(typeMap,
                         {.errorOnMethodCall = errorOnMethodCall,
                          .copyHeaders = copyHeaders,
                          .copyTuples = copyTuples},
                         typeChecking) {}
};

}  // namespace P4

#endif /* MIDEND_COPYSTRUCTURES_H_ */
