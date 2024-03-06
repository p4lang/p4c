#include "backends/p4tools/modules/smith/common/statementOrDeclaration.h"

#include <vector>

#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/util/util.h"

namespace P4Tools {

namespace P4Smith {

IR::StatOrDecl *statementOrDeclaration::gen_rnd(bool is_in_func = false) {
    std::vector<int64_t> percent = {PCT.STATEMENTORDECLARATION_VAR,
                                    PCT.STATEMENTORDECLARATION_CONSTANT,
                                    PCT.STATEMENTORDECLARATION_STATEMENT};
    auto val = randInt(percent);
    if (val == 0) {
        auto stmt = Declarations().genVariableDeclaration();
        if (!stmt) {
            BUG("Declaration in statementOrDeclaration should !be nullptr!");
        }
        return stmt;
    } else if (val == 1) {
        auto stmt = Declarations().genConstantDeclaration();
        if (!stmt) {
            BUG("Declaration in statementOrDeclaration should !be nullptr!");
        }
        return stmt;
    } else {
        auto genStmt = Statements();
        auto stmt = genStmt.genAssignmentOrMethodCallStatement(is_in_func);
        if (!stmt) {
            // it can happen that no statement can be generated
            // for example in functions without writable values
            // so declare a variable instead
            return Declarations().genVariableDeclaration();
        }
        return stmt;
    }
}
}  // namespace P4Smith

}  // namespace P4Tools
