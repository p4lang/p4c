#include "parserMetrics.h"
#include "cyclomaticComplexity.h"
#include <iostream>

namespace P4 {

bool ParserMetricsPass::preorder(const IR::P4Parser *parser) {
    unsigned stateCount = 0;

    for (const auto &state : parser->states) {
        stateCount++;
        std::string stateName = state->name.name.string();

        CyclomaticComplexityCalculator calculator;
        state->apply(calculator);
        int complexity = calculator.getComplexity();

        metrics.StateComplexity[stateName] = complexity;
    }

    metrics.totalStates += stateCount;
    return false;
}

}  // namespace P4
