#ifndef _EVALUATOR_BLOCKMAP_H_
#define _EVALUATOR_BLOCKMAP_H_

#include <vector>
#include <iostream>

#include "lib/enumerator.h"
#include "lib/exceptions.h"
#include "frontends/common/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4 {

// Describes the program structure as a set of compile-time blocks.
class BlockMap final {
    friend class Evaluator;
 public:
    const IR::ToplevelBlock* toplevelBlock;
    const IR::P4Program* program;
    ReferenceMap* refMap;
    TypeMap* typeMap;

    BlockMap(ReferenceMap* refMap, TypeMap* typeMap) :
            toplevelBlock(nullptr), program(nullptr), refMap(refMap), typeMap(typeMap) {}
    const IR::PackageBlock* getMain() const;
    const IR::Block* getBlockBoundToParameter(const IR::InstantiatedBlock* block,
                                              cstring paramName) const;

    void setProgram(const IR::P4Program* prog) { program = prog; }
    void dbprint(std::ostream& out) const
    { toplevelBlock->dbprint_recursive(out); }
};

}  // namespace P4

#endif /* _EVALUATOR_BLOCKMAP_H_ */
