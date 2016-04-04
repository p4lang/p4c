#ifndef _FRONTENDS_P4_CLONER_H_
#define _FRONTENDS_P4_CLONER_H_

#include "ir/ir.h"

namespace P4 {

// Inserting an IR dag in several places does not work,
// because PathExpressions must be unique.  The Cloner takes
// care of this.
class Cloner : public Transform {
 public:
    const IR::Node* clone(const IR::Node* node)
    { return node->apply(*this); }
    const IR::Node* postorder(IR::PathExpression* path) override
    { return new IR::PathExpression(path->path->clone()); }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_CLONER_H_ */
