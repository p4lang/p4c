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

#ifndef _P4_REDUNDANT_PARSERS_H
#define _P4_REDUNDANT_PARSERS_H

#include "ir/ir.h"
#include "typeMap.h"

namespace P4 {

/** Find parsers that have an unconditional "accept" in their start
 *  state, and put them in redundantParsers.
 */
class FindRedundantParsers : public Inspector {
    std::set<cstring> &redundantParsers;
    bool preorder(const IR::P4Parser *parser) override;
 public:
    FindRedundantParsers(std::set<cstring> &redundantParsers)
        : redundantParsers(redundantParsers) { }
};

/** Find .apply() calls on parsers that are on redundantParsers, and
 *  eliminate them.
 */
class EliminateSubparserCalls : public Transform {
    const std::set<cstring> &redundantParsers;
    TypeMap *typeMap;
    const IR::Node *postorder(IR::MethodCallStatement *methodCallStmt) override;
public:
    EliminateSubparserCalls(const std::set<cstring> &redundantParsers, TypeMap *typeMap)
        : redundantParsers(redundantParsers), typeMap(typeMap)
    { }
};

class RemoveRedundantParsers : public PassManager {
    std::set<cstring> redundantParsers;
public:
    RemoveRedundantParsers(TypeMap *typeMap)
        : PassManager {
                new FindRedundantParsers(redundantParsers),
                new EliminateSubparserCalls(redundantParsers, typeMap)
        } {
        setName("RemoveRedundantParsers");
    }
};

}

#endif
