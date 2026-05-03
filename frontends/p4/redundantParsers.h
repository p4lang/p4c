/*
 * Copyright 2022 Intel Corporation
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_REDUNDANTPARSERS_H_
#define FRONTENDS_P4_REDUNDANTPARSERS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"

namespace P4 {

/** Find parsers that have an unconditional "accept" in their start
 *  state, and put them in redundantParsers.
 */
class FindRedundantParsers : public Inspector {
    std::set<const IR::P4Parser *> &redundantParsers;
    bool preorder(const IR::P4Parser *parser) override;

 public:
    explicit FindRedundantParsers(std::set<const IR::P4Parser *> &redundantParsers)
        : redundantParsers(redundantParsers) {}
};

/** Find .apply() calls on parsers that are on redundantParsers, and
 *  eliminate them.
 */
class EliminateSubparserCalls : public Transform, public ResolutionContext {
    const std::set<const IR::P4Parser *> &redundantParsers;
    TypeMap *typeMap;
    const IR::Node *postorder(IR::MethodCallStatement *methodCallStmt) override;

 public:
    EliminateSubparserCalls(const std::set<const IR::P4Parser *> &redundantParsers,
                            TypeMap *typeMap)
        : redundantParsers(redundantParsers), typeMap(typeMap) {}
};

class RemoveRedundantParsers : public PassManager {
    std::set<const IR::P4Parser *> redundantParsers;

 public:
    RemoveRedundantParsers(TypeMap *typeMap, const RemoveUnusedPolicy &policy)
        : PassManager{new TypeChecking(nullptr, typeMap, true),
                      new FindRedundantParsers(redundantParsers),
                      new EliminateSubparserCalls(redundantParsers, typeMap),
                      new RemoveAllUnusedDeclarations(policy)} {
        setName("RemoveRedundantParsers");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_REDUNDANTPARSERS_H_ */
