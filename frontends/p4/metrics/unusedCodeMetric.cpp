#include "unusedCodeMetric.h"

namespace P4 {

bool UnusedCodeMetricPass::scope_enter(cstring name) {
    scope.push_back(scope.empty() ? name : scope.back() + "."_cs + name);
    return true;
}

void UnusedCodeMetricPass::scope_leave() {
    if (!scope.empty()) scope.pop_back();
}

bool UnusedCodeMetricPass::preorder(const IR::Function *function) {
    currentInstancesCount.functions++;
    return scope_enter(function->getName().name);
}

bool UnusedCodeMetricPass::preorder(const IR::P4Control *control) {
    return scope_enter(control->getName().name);
}
bool UnusedCodeMetricPass::preorder(const IR::P4Parser *parser) {
    return scope_enter(parser->getName().name);
}
bool UnusedCodeMetricPass::preorder(const IR::IfStatement *stmt) {
    return scope_enter("if_"_cs + std::to_string(stmt->id));
}
bool UnusedCodeMetricPass::preorder(const IR::SwitchCase *caseNode) {
    return scope_enter("case_"_cs + std::to_string(caseNode->id));
}
void UnusedCodeMetricPass::postorder(const IR::P4Control * /*control*/) { scope_leave(); }
void UnusedCodeMetricPass::postorder(const IR::P4Parser * /*parser*/) { scope_leave(); }
void UnusedCodeMetricPass::postorder(const IR::Function * /*function*/) { scope_leave(); }
void UnusedCodeMetricPass::postorder(const IR::SwitchCase * /*case*/) { scope_leave(); }

void UnusedCodeMetricPass::postorder(const IR::IfStatement *stmt) {
    currentInstancesCount.conditionals++;
    if (stmt->ifFalse != nullptr) currentInstancesCount.conditionals++;
    scope_leave();
}

bool UnusedCodeMetricPass::preorder(const IR::P4Action *action) {
    if (action->getName().name == "NoAction"_cs) return false;
    cstring scopedName =
        scope.empty() ? action->getName().name : scope.back() + "."_cs + action->getName().name;

    if (isBefore)
        metrics.helperVars.beforeActions.push_back(scopedName);
    else
        metrics.helperVars.afterActions.push_back(scopedName);

    return true;
}

void UnusedCodeMetricPass::postorder(const IR::ParserState *state) {
    if (state->name != IR::ParserState::accept && state->name != IR::ParserState::reject) {
        currentInstancesCount.states++;
    }
}

void UnusedCodeMetricPass::postorder(const IR::Declaration_Variable *node) {
    cstring name = node->getName().name;
    cstring scopedName = scope.empty() ? name : scope.back() + "."_cs + name;

    if (isBefore)
        metrics.helperVars.beforeVariables.push_back(scopedName);
    else
        metrics.helperVars.afterVariables.push_back(scopedName);
}

void UnusedCodeMetricPass::postorder(const IR::Type_Enum * /*node*/) {
    currentInstancesCount.enums++;
}
void UnusedCodeMetricPass::postorder(const IR::Type_SerEnum * /*node*/) {
    currentInstancesCount.enums++;
}
void UnusedCodeMetricPass::postorder(const IR::Parameter * /*node*/) {
    currentInstancesCount.parameters++;
}
void UnusedCodeMetricPass::postorder(const IR::ReturnStatement * /*node*/) {
    currentInstancesCount.returns++;
}
void UnusedCodeMetricPass::postorder(const IR::P4Program * /*node*/) {
    isBefore ? recordBefore() : recordAfter();
}

void UnusedCodeMetricPass::recordBefore() {
    metrics.helperVars.interPassCounts = currentInstancesCount;
}

void UnusedCodeMetricPass::recordAfter() {
    metrics.unusedCodeInstances = metrics.helperVars.interPassCounts - currentInstancesCount;

    // Calculate the number of unused actions.
    for (const auto &beforeAction : metrics.helperVars.beforeActions) {
        bool found = std::any_of(
            metrics.helperVars.afterActions.begin(), metrics.helperVars.afterActions.end(),
            [&](const cstring &afterAction) {
                return afterAction.string().find(beforeAction.string()) != std::string::npos;
            });

        if (!found) metrics.unusedCodeInstances.actions++;
    }
    // Disregard actions that were inlined.
    metrics.unusedCodeInstances.actions =
        metrics.inlinedActions > metrics.unusedCodeInstances.actions
            ? metrics.unusedCodeInstances.actions
            : metrics.unusedCodeInstances.actions - metrics.inlinedActions;

    // Calculate the number of unused variables.
    for (const auto &beforeVar : metrics.helperVars.beforeVariables) {
        bool found =
            std::any_of(metrics.helperVars.afterVariables.begin(),
                        metrics.helperVars.afterVariables.end(), [&](const cstring &afterVar) {
                            return afterVar.string().find(beforeVar.string()) != std::string::npos;
                        });
        if (!found) metrics.unusedCodeInstances.variables++;
    }

    if (LOGGING(3)) {
        std::cout << "Original actions: \n";
        for (const auto &before : metrics.helperVars.beforeActions)
            std::cout << " - " << before << std::endl;

        std::cout << "Transformed actions: \n";
        for (const auto &after : metrics.helperVars.afterActions)
            std::cout << " - " << after << std::endl;

        std::cout << "Original variables: \n";
        for (const auto &before : metrics.helperVars.beforeVariables)
            std::cout << " - " << before << std::endl;

        std::cout << "Transformed variables: \n";
        for (const auto &after : metrics.helperVars.afterVariables)
            std::cout << " - " << after << std::endl;
    }
}

}  // namespace P4
