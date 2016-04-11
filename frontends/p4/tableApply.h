#ifndef _FRONTENDS_P4_TABLEAPPLY_H_
#define _FRONTENDS_P4_TABLEAPPLY_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"

// helps resolve complex expressions involving a table apply
// such as table.apply().action_run
// and table.apply().hit

namespace P4 {

// These
class TableApplySolver {
 public:
    static const IR::P4Table* isHit(const IR::Expression* expression,
                                    const P4::ReferenceMap* refMap,
                                    const P4::TypeMap* typeMap);
    static const IR::P4Table* isActionRun(const IR::Expression* expression,
                                          const P4::ReferenceMap* refMap,
                                          const P4::TypeMap* typeMap);
};

}  // namespace P4

#endif /* _FRONTENDS_P4_TABLEAPPLY_H_ */
