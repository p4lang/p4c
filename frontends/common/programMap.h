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

#ifndef _FRONTENDS_COMMON_PROGRAMMAP_H_
#define _FRONTENDS_COMMON_PROGRAMMAP_H_

#include "ir/ir.h"

namespace P4 {

// Base class for various maps.
// A map is computed on a certain P4Program.
// If the program has not changed, the map is up-to-date.
class ProgramMap : public IHasDbPrint {
 protected:
    const IR::P4Program* program = nullptr;
    cstring mapKind;
    explicit ProgramMap(cstring kind) : mapKind(kind) {}
    virtual ~ProgramMap() {}

 public:
    // Check if map is up-to-date for the specified node; return true if it is
    bool checkMap(const IR::Node* node) const {
        if (node == program) {
            // program has not changed
            LOG2(mapKind << " is up-to-date");
            return true;
        } else {
            LOG2("Program has changed from " << dbp(program) << " to " << dbp(node));
        }
        return false;
    }
    void validateMap(const IR::Node* node) const {
        if (node == nullptr || !node->is<IR::P4Program>() || program == nullptr)
            return;
        if (program != node)
            BUG("Invalid map %1%: computed for %2%, used for %3%",
                mapKind, dbp(program), dbp(node));
    }
    void updateMap(const IR::Node* node) {
        if (node == nullptr || !node->is<IR::P4Program>())
            return;
        program = node->to<IR::P4Program>();
        LOG2(mapKind << " updated to " << dbp(node));
    }
};

}  // namespace P4

#endif /* _FRONTENDS_COMMON_PROGRAMMAP_H_ */
