#include "nestingDepthMetric.h"

namespace P4 {

bool NestingDepthMetricPass::increment() {
    currentDepth++;
    if (currentDepth > currentMax) currentMax = currentDepth;
    return true;
}

void NestingDepthMetricPass::decrement() {
    if (currentDepth > 0) currentDepth--;
}

bool NestingDepthMetricPass::enter() {
    currentDepth = currentMax = 0;
    return true;
}

void NestingDepthMetricPass::logDepth(const cstring name) {
    metrics.blockNestingDepth[name] = currentMax;
}

bool NestingDepthMetricPass::preorder(const IR::P4Parser * /*parser*/) { return enter(); }
bool NestingDepthMetricPass::preorder(const IR::P4Control * /*control*/) { return enter(); }
bool NestingDepthMetricPass::preorder(const IR::Function * /*function*/) { return enter(); }
void NestingDepthMetricPass::postorder(const IR::P4Parser *parser) { logDepth(parser->name.name); }
void NestingDepthMetricPass::postorder(const IR::P4Control *control) {
    logDepth(control->name.name);
}
void NestingDepthMetricPass::postorder(const IR::Function *function) {
    logDepth(function->name.name);
}
bool NestingDepthMetricPass::preorder(const IR::ParserState * /*state*/) { return increment(); }
bool NestingDepthMetricPass::preorder(const IR::SelectExpression * /*stmt*/) { return increment(); }
bool NestingDepthMetricPass::preorder(const IR::BlockStatement * /*stmt*/) { return increment(); }
void NestingDepthMetricPass::postorder(const IR::ParserState * /*state*/) { decrement(); }
void NestingDepthMetricPass::postorder(const IR::SelectExpression * /*stmt*/) { decrement(); }
void NestingDepthMetricPass::postorder(const IR::BlockStatement * /*stmt*/) { decrement(); }

void NestingDepthMetricPass::postorder(const IR::P4Program * /*program*/) {
    unsigned total = 0;
    unsigned count = 0;
    metrics.maxNestingDepth = 0;

    for (const auto &entry : metrics.blockNestingDepth) {
        total += entry.second;
        count++;
        if (entry.second > metrics.maxNestingDepth) metrics.maxNestingDepth = entry.second;
    }

    if (count > 0) {
        metrics.avgNestingDepth = static_cast<double>(total) / count;
    } else {
        metrics.avgNestingDepth = 0.0;
    }
}

}  // namespace P4
