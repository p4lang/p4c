#include "inlinedActionsMetric.h"

namespace P4 {

void InlinedActionsMetricPass::postorder(const IR::BlockStatement *block) {
    const auto *annotation = block->getAnnotation(IR::Annotation::inlinedFromAnnotation);
    if (annotation == nullptr) return;

    if (std::holds_alternative<IR::Vector<IR::Expression>>(annotation->body)) {
        const auto &exprs = std::get<IR::Vector<IR::Expression>>(annotation->body);
        if (!exprs.empty()) {
            const auto *stringLiteral = exprs.at(0)->to<IR::StringLiteral>();
            if (stringLiteral != nullptr) {
                // Only count unique actions.
                if (actions.insert(stringLiteral->value.string()).second) metrics.inlinedActions++;
            }
        }
    }
}

}  // namespace P4
