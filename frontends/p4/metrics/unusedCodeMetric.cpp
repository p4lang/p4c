#include "unusedCodeMetric.h"

namespace P4 {

void UnusedCodeMetricPass::scope_enter(std::string name){
    scope.push_back(scope.empty() ? name : scope.back() + "." + name);
}

void UnusedCodeMetricPass::scope_leave(){
    if (!scope.empty())
        scope.pop_back();
}

bool UnusedCodeMetricPass::preorder(const IR::P4Control* control) {
    scope_enter(control->getName().name.string());
    return true;
}

void UnusedCodeMetricPass::postorder(const IR::P4Control* /*control*/) {
    scope_leave();
}

bool UnusedCodeMetricPass::preorder(const IR::P4Parser* parser) {
    scope_enter(parser->getName().name.string());
    return true;
}

void UnusedCodeMetricPass::postorder(const IR::P4Parser* /*parser*/) {
    scope_leave();
}

bool UnusedCodeMetricPass::preorder(const IR::Function* function) {
    scope_enter(function->getName().name.string());
    currentInstancesCount.functions++;
    return true;
}

void UnusedCodeMetricPass::postorder(const IR::Function* /*function*/) {
    scope_leave();
}

bool UnusedCodeMetricPass::preorder(const IR::IfStatement* stmt) {
    scope_enter("if_" + std::to_string(stmt->id));
    return true;
}

void UnusedCodeMetricPass::postorder(const IR::IfStatement* stmt) {
    currentInstancesCount.conditionals++;
    if (stmt->ifFalse != nullptr) currentInstancesCount.conditionals++;
    scope_leave();
}

bool UnusedCodeMetricPass::preorder(const IR::SwitchCase* caseNode) {
    scope_enter("case_" + std::to_string(caseNode->id));
    return true;
}

void UnusedCodeMetricPass::postorder(const IR::SwitchCase* /*caseNode*/) {
    scope_leave();
}

bool UnusedCodeMetricPass::preorder(const IR::P4Action* action) {
    if (action->getName().name == "NoAction") return false;
    std::string scopedName = scope.empty() ? action->getName().name.string() : scope.back() + "." + action->getName().name.string();

    if (isBefore) {
        metrics.helperVars.beforeActions.push_back(scopedName);
    } else {
        metrics.helperVars.afterActions.push_back(scopedName);
    }
    return true;
}

void UnusedCodeMetricPass::postorder(const IR::ParserState* state) {
    if (state->name != IR::ParserState::accept && 
        state->name != IR::ParserState::reject) {
        currentInstancesCount.states++;
    }
}

void UnusedCodeMetricPass::postorder(const IR::Declaration_Variable* node) {

    std::string name = node->getName().name.string();
    std::string scopedName = scope.empty() ? name : scope.back() + "." + name;

    if (isBefore) {
        metrics.helperVars.beforeVariables.push_back(scopedName);
    } 
    else {
        metrics.helperVars.afterVariables.push_back(scopedName);
    }
}

void UnusedCodeMetricPass::postorder(const IR::Type_Enum* /*node*/) { currentInstancesCount.enums++; }
void UnusedCodeMetricPass::postorder(const IR::Type_SerEnum* /*node*/) { currentInstancesCount.enums++; }
void UnusedCodeMetricPass::postorder(const IR::Parameter* /*node*/) { currentInstancesCount.parameters++; }
void UnusedCodeMetricPass::postorder(const IR::ReturnStatement* /*node*/) { currentInstancesCount.returns++; }
void UnusedCodeMetricPass::postorder(const IR::P4Program* /*node*/) { isBefore ? recordBefore() : recordAfter(); }

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
    // Disregard actions and functions that were inlined.
    metrics.unusedCodeInstances.actions -= metrics.inlinedCode.actions;
    metrics.unusedCodeInstances.functions -= metrics.inlinedCode.functions;

    // Calculate the number of unused variables.
    for (const auto& beforeVar : metrics.helperVars.beforeVariables) {
        bool found = std::any_of(metrics.helperVars.afterVariables.begin(), metrics.helperVars.afterVariables.end(),
            [&](const std::string& afterVar) {
                return afterVar.find(beforeVar) != std::string::npos;
            });
        if (!found) metrics.unusedCodeInstances.variables++;
    }

    if(LOGGING(3)){
        std::cout<<"Original actions: \n";
        for (const auto& before : metrics.helperVars.beforeActions)
            std::cout<<" - "<< before << std::endl;
        
        std::cout<<"Transformed actions: \n";
        for (const auto& after : metrics.helperVars.afterActions)
            std::cout<<" - "<< after << std::endl;

        std::cout<<"Original variables: \n";
        for (const auto& before : metrics.helperVars.beforeVariables)
            std::cout<<" - "<< before << std::endl;

        std::cout<<"Transformed variables: \n";
        for (const auto& after : metrics.helperVars.afterVariables)
            std::cout<<" - "<< after << std::endl;
    }
}

}  // namespace P4