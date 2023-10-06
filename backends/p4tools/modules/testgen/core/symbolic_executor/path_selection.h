#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_PATH_SELECTION_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_PATH_SELECTION_H_

#include <set>

namespace P4Tools::P4Testgen {

enum class PathSelectionPolicy {
    DepthFirst,
    RandomBacktrack,
    GreedyStmtCoverage,
};

inline bool requiresLookahead(PathSelectionPolicy &pathSelectionPolicy) {
    static const std::set LOOKAHEAD_STRATEGYIES = {PathSelectionPolicy::GreedyStmtCoverage};
    return LOOKAHEAD_STRATEGYIES.find(pathSelectionPolicy) != LOOKAHEAD_STRATEGYIES.end();
}

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SYMBOLIC_EXECUTOR_PATH_SELECTION_H_ */
