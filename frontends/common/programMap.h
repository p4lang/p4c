#ifndef _FRONTENDS_COMMON_PROGRAMMAP_H_
#define _FRONTENDS_COMMON_PROGRAMMAP_H_

#include "ir/ir.h"

namespace P4 {

// Base class for various maps.
// A map is computed on a certain P4Program.
// If the program has not changed, the map is up-to-date.
class ProgramMap {
 protected:
    const IR::P4Program* program = nullptr;
    cstring mapKind;
    ProgramMap(cstring kind) : mapKind(kind) {}
    virtual ~ProgramMap() {}
 public:
    // Check if map is up-to-date for the specified node; return true if it is
    bool checkMap(const IR::Node* node) const {
        if (!node->is<IR::P4Program>() || program == nullptr)
            return false;
        if (node->to<IR::P4Program>() == program) {
            // program has not changed
            LOG2(mapKind << " is up-to-date");
            return true;
        } else {
            LOG2("Program has changed from " << dbp(program) << " to " << dbp(node));
        }
        return false;
    }
    void validateMap(const IR::Node* node) const {
        if (!node->is<IR::P4Program>() || program == nullptr)
            return;
        if (program != node->to<IR::P4Program>())
            BUG("Invalid map %1%: computed for %2%, used for %3%",
                mapKind, dbp(program), dbp(node));
    }
    void updateMap(const IR::Node* node) {
        if (!node->is<IR::P4Program>())
            return;
        program = node->to<IR::P4Program>();
        LOG2(mapKind << " updated to " << dbp(node));
    }
};

}

#endif /* _FRONTENDS_COMMON_PROGRAMMAP_H_ */
