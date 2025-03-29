#include "nestingDepthMetric.h"

namespace P4 {

bool NestingDepthMetricPass::increment(){
    currentDepth++;
    if (currentDepth > currentMax)
        currentMax = currentDepth;
    return true;
}

bool NestingDepthMetricPass::preorder(const IR::P4Parser* /*parser*/) {
    currentDepth = currentMax = 0;
    return true;
}

void NestingDepthMetricPass::postorder(const IR::P4Parser *parser) {
    metrics.nestingDepth.blockNestingDepth[parser->name.name.string()] = currentMax;
}

bool NestingDepthMetricPass::preorder(const IR::P4Control* /*control*/) {
    currentDepth = currentMax = 0;
    return true;
}

void NestingDepthMetricPass::postorder(const IR::P4Control *control) {
    metrics.nestingDepth.blockNestingDepth[control->name.name.string()] = currentMax;
}


bool NestingDepthMetricPass::preorder(const IR::ParserState* /*state*/){
    return increment();
}
void NestingDepthMetricPass::postorder(const IR::ParserState* /*state*/){
    currentDepth--;
}

bool NestingDepthMetricPass::preorder(const IR::SelectExpression* /*stmt*/) {
    return increment();
}

void NestingDepthMetricPass::postorder(const IR::SelectExpression* /*stmt*/) {
    currentDepth--;
}

bool NestingDepthMetricPass::preorder(const IR::BlockStatement* /*stmt*/) {
    return increment();
}

void NestingDepthMetricPass::postorder(const IR::BlockStatement* /*stmt*/) {
    currentDepth--;
}


void NestingDepthMetricPass::postorder(const IR::P4Program* /*program*/) {
    unsigned total = 0;
    unsigned count = 0;
    metrics.nestingDepth.maxNestingDepth = 0;

    for (const auto &entry : metrics.nestingDepth.blockNestingDepth) {
        total += entry.second;
        count++;
        if (entry.second > metrics.nestingDepth.maxNestingDepth)
            metrics.nestingDepth.maxNestingDepth = entry.second;
    }

    if (count > 0) {
        metrics.nestingDepth.avgNestingDepth = static_cast<double>(total) / count;
    } else {
        metrics.nestingDepth.avgNestingDepth = 0.0;
    }
}

}  // namespace P4