/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_COMMON_PROGRAMMAP_H_
#define FRONTENDS_COMMON_PROGRAMMAP_H_

#include "ir/ir.h"
#include "lib/log.h"

namespace P4 {

// Base class for various maps.
// A map is computed on a certain P4Program.
// If the program has not changed, the map is up-to-date.
class ProgramMap : public IHasDbPrint {
 protected:
    const IR::P4Program *fake = new IR::P4Program();
    const IR::P4Program *program = nullptr;
    cstring mapKind;
    explicit ProgramMap(std::string_view kind) : mapKind(kind) {}
    virtual ~ProgramMap() {}

 public:
    // Check if map is up-to-date for the specified node; return true if it is
    bool checkMap(const IR::Node *node) const {
        if (node == program) {
            // program has not changed
            LOG2(mapKind << " is up-to-date");
            return true;
        } else {
            LOG2("Program has changed from " << dbp(program) << " to " << dbp(node));
        }
        return false;
    }
    void validateMap(const IR::Node *node) const {
        if (node == nullptr || !node->is<IR::P4Program>() || program == nullptr) return;
        if (program != node)
            BUG("Invalid map %1%: computed for %2%, used for %3%", mapKind, dbp(program),
                dbp(node));
    }
    void updateMap(const IR::Node *node) {
        if (node == nullptr || !node->is<IR::P4Program>()) return;
        program = node->to<IR::P4Program>();
        LOG2(mapKind << " updated to " << dbp(node));
    }
    void clear() {
        // This ensures that a clear map is never up-to-date,
        // since the 'fake' node cannot appear in a program.
        program = fake;
    }
};

}  // namespace P4

#endif /* FRONTENDS_COMMON_PROGRAMMAP_H_ */
