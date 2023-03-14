#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_PATH_SELECTION_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_PATH_SELECTION_H_

#include <set>

namespace P4Tools::P4Testgen {

enum class PathSelectionPolicy {
    IncrementalStack,
    RandomAccessStack,
    GreedyPotential,
    LinearEnumeration,
    MaxCoverage,
    RandomAccessMaxCoverage,
    UnboundedRandomAccessStack
};

inline bool requiresLookahead(PathSelectionPolicy &pathSelectionPolicy) {
    static const std::set LOOKAHEAD_STRATEGYIES = {PathSelectionPolicy::GreedyPotential,
                                                   PathSelectionPolicy::RandomAccessMaxCoverage};
    return LOOKAHEAD_STRATEGYIES.find(pathSelectionPolicy) != LOOKAHEAD_STRATEGYIES.end();
}

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXPLORATION_STRATEGY_PATH_SELECTION_H_ */
