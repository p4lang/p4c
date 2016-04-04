#ifndef _FRONTENDS_P4V1_TYPECHECK_H_
#define _FRONTENDS_P4V1_TYPECHECK_H_

#include "ir/ir.h"

/* This is the P4 v1.0/v1.1 typechecker/type inference algorithm */
class TypeCheck : public PassManager {
    std::map<const IR::Node *, const IR::Type *>        actionArgUseTypes;
    int                                                 iterCounter = 0;
    class Pass1;
    class Pass2;
    class Pass3;
 public:
    TypeCheck();
    const IR::Node *apply_visitor(const IR::Node *, const char *) override;
};

#endif /* _FRONTENDS_P4V1_TYPECHECK_H_ */
