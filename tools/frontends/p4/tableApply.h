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

#ifndef _FRONTENDS_P4_TABLEAPPLY_H_
#define _FRONTENDS_P4_TABLEAPPLY_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"

// helps resolve complex expressions involving a table apply
// such as table.apply().action_run
// and table.apply().hit

namespace P4 {

// These are used to figure out whether an expression has the form:
// table.apply().hit
// or
// table.apply().action_run
class TableApplySolver {
 public:
    static const IR::P4Table* isHit(const IR::Expression* expression,
                                    ReferenceMap* refMap, TypeMap* typeMap);
    static const IR::P4Table* isActionRun(const IR::Expression* expression,
                                          ReferenceMap* refMap, TypeMap* typeMap);
};

}  // namespace P4

#endif /* _FRONTENDS_P4_TABLEAPPLY_H_ */
