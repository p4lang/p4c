#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_STATEMENTORDECLARATION_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_STATEMENTORDECLARATION_H_

#include "ir/ir.h"

namespace P4Tools {

namespace P4Smith {

class statementOrDeclaration {
 public:
    const char *types[4] = {"variableDeclaration", "constantDeclaration", "statement",
                            "instantiation"};
    statementOrDeclaration() {}
    ~statementOrDeclaration() {}

    static IR::StatOrDecl *gen_rnd(bool if_in_func);
};
}  // namespace P4Smith

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_STATEMENTORDECLARATION_H_ */
