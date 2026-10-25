#include "frontends/p4/metrics/parserMetrics.h"

#include "frontends/p4/metrics/cyclomaticComplexity.h"

namespace P4 {

bool ParserMetricsPass::preorder(const IR::P4Parser *parser) {
    unsigned stateCount = 0;

    for (const auto &state : parser->states) {
        stateCount++;
        cstring stateName = state->name.name;

        CyclomaticComplexityCalculator calculator;
        state->apply(calculator);
        int complexity = calculator.getComplexity();

        metrics.StateComplexity[stateName] = complexity;
    }

    metrics.totalStates += stateCount;
    return false;
}

}  // namespace P4
