/*
Copyright 2022 Intel Corporation

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

#ifndef FRONTENDS_P4_REDUNDANTPARSERS_H_
#define FRONTENDS_P4_REDUNDANTPARSERS_H_

#include <set>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"

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
class EliminateSubparserCalls : public Transform {
    const std::set<const IR::P4Parser *> &redundantParsers;
    ReferenceMap *refMap;
    TypeMap *typeMap;
    const IR::Node *postorder(IR::MethodCallStatement *methodCallStmt) override;

 public:
    EliminateSubparserCalls(const std::set<const IR::P4Parser *> &redundantParsers,
                            ReferenceMap *refMap, TypeMap *typeMap)
        : redundantParsers(redundantParsers), refMap(refMap), typeMap(typeMap) {}
};

class RemoveRedundantParsers : public PassManager {
    std::set<const IR::P4Parser *> redundantParsers;

 public:
    RemoveRedundantParsers(ReferenceMap *refMap, TypeMap *typeMap)
        : PassManager{new TypeChecking(refMap, typeMap, true),
                      new FindRedundantParsers(redundantParsers),
                      new EliminateSubparserCalls(redundantParsers, refMap, typeMap)} {
        setName("RemoveRedundantParsers");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_REDUNDANTPARSERS_H_ */
