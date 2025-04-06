#include "unusedCodeMetric.h"

namespace P4 {


void UnusedCodeMetricPass::postorder(const IR::P4Action* node) {
    std::string parentName = "global";
    
    if (auto* parentCtrl = findContext<IR::P4Control>()) {
        parentName = parentCtrl->getName().name.string();
    } 
    else if (auto* parentParser = findContext<IR::P4Parser>()) {
        parentName = parentParser->getName().name.string();
    }

    std::string scopedActionName = parentName + "." + node->getName().name;

    if (isBefore) {
        metrics.helperVars.beforeActions.push_back(scopedActionName);
    } else {
        metrics.helperVars.afterActions.push_back(scopedActionName);
    }
}

void UnusedCodeMetricPass::postorder(const IR::ParserState* node) {
    if (node->name != IR::ParserState::accept && 
        node->name != IR::ParserState::reject) {
        currentInstancesCount.states++;
    }
}

void UnusedCodeMetricPass::postorder(const IR::Declaration_Variable* node) {
    std::string varName = node->getName().name.string();
    if (isBefore) {
        metrics.helperVars.beforeVariables.push_back(varName);
    } 
    else {
        metrics.helperVars.afterVariables.push_back(varName);
    }
}

void UnusedCodeMetricPass::recordBefore() {
    metrics.helperVars.interPassCounts = currentInstancesCount;
}

void UnusedCodeMetricPass::recordAfter() {
    metrics.unusedCodeInstances = metrics.helperVars.interPassCounts - currentInstancesCount;

    // Calculate the number of unused actions.
    for (const auto& beforeAction : metrics.helperVars.beforeActions) {
        bool found = std::any_of(metrics.helperVars.afterActions.begin(), metrics.helperVars.afterActions.end(),
            [&](const std::string& afterAction) {
                return afterAction.find(beforeAction) != std::string::npos;
            });
        
        if (!found) metrics.unusedCodeInstances.actions++;
    }
    // Disregard actions that were inlined.
    metrics.unusedCodeInstances.actions -= metrics.inlinedActionsNum;

    // Calculate the number of unused variables.
    for (const auto& beforeVar : metrics.helperVars.beforeVariables) {
        if (std::find(metrics.helperVars.afterVariables.begin(), metrics.helperVars.afterVariables.end(),beforeVar) 
            == metrics.helperVars.afterVariables.end()) 
        {
            metrics.unusedCodeInstances.variables++;
        }
    }
}

void UnusedCodeMetricPass::postorder(const IR::Type_Enum* /*node*/) { currentInstancesCount.enums++; }
void UnusedCodeMetricPass::postorder(const IR::Type_SerEnum* /*node*/) { currentInstancesCount.enums++; }
void UnusedCodeMetricPass::postorder(const IR::BlockStatement* /*node*/) { currentInstancesCount.blocks++; }
void UnusedCodeMetricPass::postorder(const IR::IfStatement* /*node*/) { currentInstancesCount.conditionals++; }
void UnusedCodeMetricPass::postorder(const IR::SwitchStatement* /*node*/) { currentInstancesCount.switches++; }
void UnusedCodeMetricPass::postorder(const IR::Function* /*node*/) { currentInstancesCount.functions++; }
void UnusedCodeMetricPass::postorder(const IR::Parameter* /*node*/) { currentInstancesCount.parameters++; }
void UnusedCodeMetricPass::postorder(const IR::ReturnStatement* /*node*/) { currentInstancesCount.returns++; }
void UnusedCodeMetricPass::postorder(const IR::P4Program* /*node*/) { isBefore ? recordBefore() : recordAfter(); }

}  // namespace P4