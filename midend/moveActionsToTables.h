#ifndef _MIDEND_MOVEACTIONSTOTABLES_H_
#define _MIDEND_MOVEACTIONSTOTABLES_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// Convert direct action calls to table invocations.
// control c() {
//   action x(in bit b) { ... }
//   apply { x(e); }
// }
// turns into
// control c() { 
//   action x(in bit b) { ... }
//   table _tmp() {
//     actions = { x; }
//     const default_action = x(e);
//   }
//   apply { _tmp.apply(); }
// }
class MoveActionsToTables : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;

 public:
    MoveActionsToTables(ReferenceMap* refMap, TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap) {}
};

}  // namespace P4

#endif /* _MIDEND_MOVEACTIONSTOTABLES_H_ */
