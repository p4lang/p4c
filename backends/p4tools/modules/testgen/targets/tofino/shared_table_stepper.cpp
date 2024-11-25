/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#include "backends/p4tools/modules/testgen/targets/tofino/shared_table_stepper.h"

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <boost/none.hpp>

#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir-generated.h"
#include "ir/irutils.h"
#include "lib/error.h"
#include "lib/null.h"
#include "shared_program_info.h"

#include "backends/p4tools/modules/testgen/lib/collect_coverable_nodes.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/tofino/constants.h"

namespace P4::P4Tools::P4Testgen::Tofino {

const IR::Expression *TofinoTableStepper::computeTargetMatchType(
    const TableUtils::KeyProperties &keyProperties, TableMatchMap *matches,
    const IR::Expression *hitCondition) {
    const IR::Expression *keyExpr = keyProperties.key->expression;

    // Ranges are not yet implemented for Tofino STF tests.
    if (keyProperties.matchType == SharedTofinoConstants::MATCH_KIND_RANGE &&
        TestgenOptions::get().testBackend == "PTF") {
        cstring minName = properties.tableName + "_range_min_" + keyProperties.name;
        cstring maxName = properties.tableName + "_range_max_" + keyProperties.name;
        // We can recover from taint by matching on the entire possible range.
        const IR::Expression *minKey = nullptr;
        const IR::Expression *maxKey = nullptr;
        if (keyProperties.isTainted) {
            minKey = IR::Constant::get(keyExpr->type, 0);
            maxKey = IR::Constant::get(keyExpr->type, IR::getMaxBvVal(keyExpr->type));
            keyExpr = minKey;
        } else {
            minKey = ToolsVariables::getSymbolicVariable(keyExpr->type, minName);
            maxKey = ToolsVariables::getSymbolicVariable(keyExpr->type, maxName);
        }
        matches->emplace(keyProperties.name, new Range(keyProperties.key, minKey, maxKey));
        return new IR::LAnd(hitCondition, new IR::LAnd(new IR::LAnd(new IR::Lss(minKey, maxKey),
                                                                    new IR::Leq(minKey, keyExpr)),
                                                       new IR::Leq(keyExpr, maxKey)));
    }
    // Action selector entries are not part of the match.
    if (keyProperties.matchType == SharedTofinoConstants::MATCH_KIND_SELECTOR) {
        tofinoProperties.actionSelectorKeys.emplace_back(keyExpr);
        return hitCondition;
    }
    // Atcam partition indices are not yet implemented but do not influence the table semantics.
    if (keyProperties.matchType == SharedTofinoConstants::MATCH_KIND_ATCAM) {
        ::warning("Key match type atcam partition index not fully implemented.");
        return hitCondition;
    }

    // If the custom match type does not match, delete to the core match types.
    return TableStepper::computeTargetMatchType(keyProperties, matches, hitCondition);
}

void TofinoTableStepper::evalTableActionProfile(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    const auto *keys = table->getKey();
    // If we have no keys, there is nothing to match.
    if (keys == nullptr) {
        return;
    }
    const auto *state = getExecutionState();

    // First, we compute the hit condition to trigger this particular action call.
    TableMatchMap matches;
    const auto *hitCondition = computeHit(&matches);

    // Now we iterate over all table actions and create a path per table action.
    for (const auto *action : tableActionList) {
        // Grab the path from the method call.
        const auto *tableAction = action->expression->checkedTo<IR::MethodCallExpression>();
        // Try to find the action declaration corresponding to the path reference in the table.
        const auto *actionType = state->getP4Action(tableAction);

        auto &nextState = state->clone();
        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();
        // Copy the previous action profile.
        auto *actionProfile = new TofinoActionProfile(*tofinoProperties.actionProfile);
        // The entry we are inserting using an index instead of the action name.
        cstring actionIndex = std::to_string(actionProfile->getActionMapSize());
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto *arguments = new IR::Vector<IR::Argument>();
        std::vector<ActionArg> ctrlPlaneArgs;
        for (size_t argIdx = 0; argIdx < parameters->size(); ++argIdx) {
            const auto *parameter = parameters->getParameter(argIdx);
            // Synthesize a symbolic variable here that corresponds to a control plane argument.
            // We get the unique name of the table coupled with the unique name of the action.
            // Getting the unique name is needed to avoid generating duplicate arguments.
            cstring keyName =
                properties.tableName + "_param_" + actionName + std::to_string(argIdx);
            const auto &actionArg = ToolsVariables::getSymbolicVariable(parameter->type, keyName);
            arguments->push_back(new IR::Argument(actionArg));
            // We also track the argument we synthesize for the control plane.
            // Note how we use the control plane name for the parameter here.
            ctrlPlaneArgs.emplace_back(parameter, actionArg);
        }
        // Add the chosen action to the profile (it will create a new index.)
        // TODO: Should we check if we exceed the maximum number of possible profile entries?
        actionProfile->addToActionMap(actionName, ctrlPlaneArgs);
        // Update the action profile in the execution state.
        nextState.addTestObject("action_profile"_cs, actionProfile->getObjectName(), actionProfile);

        // We add the arguments to our action call, effectively creating a const entry call.
        auto *synthesizedAction = tableAction->clone();
        synthesizedAction->arguments = arguments;

        // Finally, add all the new rules to the execution state.
        ActionCall ctrlPlaneActionCall(actionIndex, actionType, {});
        auto tableRule =
            TableRule(matches, TestSpec::LOW_PRIORITY, ctrlPlaneActionCall, TestSpec::TTL);
        auto *tableConfig = new TableConfig(table, {tableRule});

        // Add the action profile to the table. This signifies a slightly different implementation.
        tableConfig->addTableProperty("action_profile"_cs, actionProfile);
        nextState.addTestObject("tableconfigs"_cs, table->controlPlaneName(), tableConfig);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(
            new IR::MethodCallStatement(Util::SourceInfo(), synthesizedAction));
        // Some path selection strategies depend on looking ahead and collecting potential
        // statements. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(*state);
            collector.updateNodeCoverage(actionType, coveredNodes);
        }

        nextState.set(getTableHitVar(table), IR::BoolLiteral::get(true));
        nextState.set(getTableActionVar(table), getTableActionString(tableAction));
        std::stringstream tableStream;
        tableStream << "Table Branch: " << properties.tableName;
        tableStream << " Chosen action: " << actionName;
        nextState.add(*new TraceEvents::Generic(tableStream.str()));
        nextState.replaceTopBody(&replacements);
        getResult()->emplace_back(hitCondition, *state, nextState, coveredNodes);
    }
}

void TofinoTableStepper::evalTableActionSelector(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    const auto *keys = table->getKey();
    // If we have no keys, there is nothing to match.
    if (keys == nullptr) {
        return;
    }
    const auto *state = getExecutionState();

    // First, we compute the hit condition to trigger this particular action call.
    TableMatchMap matches;
    const auto *hitCondition = computeHit(&matches);

    // Now we iterate over all table actions and create a path per table action.
    for (const auto *action : tableActionList) {
        // Grab the path from the method call.
        const auto *tableAction = action->expression->checkedTo<IR::MethodCallExpression>();
        // Try to find the action declaration corresponding to the path reference in the table.
        const auto *actionType = state->getP4Action(tableAction);

        auto &nextState = state->clone();
        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();

        // Copy the previous action profile.
        auto *actionProfile = new TofinoActionProfile(
            tofinoProperties.actionSelector->getActionProfile()->getProfileDecl());
        // The entry we are inserting using an index instead of the action name.
        cstring actionIndex = std::to_string(actionProfile->getActionMapSize());
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto *arguments = new IR::Vector<IR::Argument>();
        std::vector<ActionArg> ctrlPlaneArgs;
        for (size_t argIdx = 0; argIdx < parameters->size(); ++argIdx) {
            const auto *parameter = parameters->getParameter(argIdx);
            // Synthesize a symbolic variable here that corresponds to a control plane argument.
            // We get the unique name of the table coupled with the unique name of the action.
            // Getting the unique name is needed to avoid generating duplicate arguments.
            cstring keyName =
                properties.tableName + "_param_" + actionName + std::to_string(argIdx);
            const auto &actionArg = ToolsVariables::getSymbolicVariable(parameter->type, keyName);
            arguments->push_back(new IR::Argument(actionArg));
            // We also track the argument we synthesize for the control plane.
            // Note how we use the control plane name for the parameter here.
            ctrlPlaneArgs.emplace_back(parameter, actionArg);
        }
        // Add the chosen action to the profile (it will create a new index.)
        // TODO: Should we check if we exceed the maximum number of possible profile entries?
        actionProfile->addToActionMap(actionName, ctrlPlaneArgs);

        auto *actionSelector = new TofinoActionSelector(
            tofinoProperties.actionSelector->getSelectorDecl(), actionProfile);

        // Update the action profile in the execution state.
        nextState.addTestObject("action_profile"_cs, actionProfile->getObjectName(), actionProfile);
        // Update the action selector in the execution state.
        nextState.addTestObject("action_selector"_cs, actionSelector->getObjectName(),
                                actionSelector);

        // We add the arguments to our action call, effectively creating a const entry call.
        auto *synthesizedAction = tableAction->clone();
        synthesizedAction->arguments = arguments;

        // Finally, add all the new rules to the execution state.
        ActionCall ctrlPlaneActionCall(actionIndex, actionType, {});
        auto tableRule =
            TableRule(matches, TestSpec::LOW_PRIORITY, ctrlPlaneActionCall, TestSpec::TTL);
        auto *tableConfig = new TableConfig(table, {tableRule});

        // Add the action profile to the table. This signifies a slightly different implementation.
        tableConfig->addTableProperty("action_profile"_cs, actionProfile);
        // Add the action selector to the table. This signifies a slightly different implementation.
        tableConfig->addTableProperty("action_selector"_cs, actionSelector);

        nextState.addTestObject("tableconfigs"_cs, table->controlPlaneName(), tableConfig);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(
            new IR::MethodCallStatement(Util::SourceInfo(), synthesizedAction));
        // Some path selection strategies depend on looking ahead and collecting potential
        // statements. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(*state);
            collector.updateNodeCoverage(actionType, coveredNodes);
        }

        nextState.set(getTableHitVar(table), IR::BoolLiteral::get(true));
        nextState.set(getTableActionVar(table), getTableActionString(tableAction));
        std::stringstream tableStream;
        tableStream << "Table Branch: " << properties.tableName;
        tableStream << " Chosen action: " << actionName;
        nextState.add(*new TraceEvents::Generic(tableStream.str()));
        nextState.replaceTopBody(&replacements);
        getResult()->emplace_back(hitCondition, *state, nextState);
    }
}

bool TofinoTableStepper::checkForActionProfile() {
    const auto *impl = table->properties->getProperty("implementation");
    if (impl == nullptr) {
        return false;
    }

    const auto *state = getExecutionState();

    const auto *implExpr = impl->value->checkedTo<IR::ExpressionValue>();
    const auto *implPath = implExpr->expression->checkedTo<IR::PathExpression>();
    const auto *implDecl = state->findDecl(implPath)->checkedTo<IR::Declaration_Instance>();
    const auto *implDeclType = implDecl->type;
    if (const auto *typeName = implDeclType->to<IR::Type_Name>()) {
        implDeclType = state->resolveType(typeName);
    }
    const auto *implExtern = implDeclType->checkedTo<IR::Type_Extern>();
    if (implExtern->name != "ActionProfile") {
        return false;
    }

    tofinoProperties.implementaton = TableImplementation::profile;

    const auto *testObject =
        state->getTestObject("action_profile"_cs, implDecl->controlPlaneName(), false);
    if (testObject == nullptr) {
        tofinoProperties.addProfileToState = true;
        tofinoProperties.actionProfile = new TofinoActionProfile(implDecl);
        return true;
    }
    tofinoProperties.actionProfile = testObject->checkedTo<TofinoActionProfile>();
    tofinoProperties.addProfileToState = false;
    return true;
}

bool TofinoTableStepper::checkForActionSelector() {
    const auto *impl = table->properties->getProperty("implementation");
    if (impl == nullptr) {
        return false;
    }

    const auto *state = getExecutionState();
    const auto *selectorExpr = impl->value->checkedTo<IR::ExpressionValue>();
    const auto *selectorPath = selectorExpr->expression->checkedTo<IR::PathExpression>();
    const auto *selectorDecl = state->findDecl(selectorPath)->checkedTo<IR::Declaration_Instance>();
    const auto *selectorDeclType = selectorDecl->type;
    if (const auto *typeName = selectorDeclType->to<IR::Type_Name>()) {
        selectorDeclType = state->resolveType(typeName);
    }
    const auto *selectorExtern = selectorDeclType->checkedTo<IR::Type_Extern>();
    if (selectorExtern->name != "ActionSelector") {
        return false;
    }

    // There are deprecated action selector declarations we do not support.
    // These have less than 5 arguments.
    const auto *selectorArgs = selectorDecl->arguments;
    if (selectorArgs->size() < 5) {
        ::warning(
            "Only action selectors with at least 5 parameters are supported. "
            "Using default action for selector table.");
        tofinoProperties.implementaton = TableImplementation::skip;
        return false;
    }

    const auto *actionProfilePath =
        selectorArgs->at(0)->expression->checkedTo<IR::PathExpression>();
    const auto *actionProfileDecl =
        state->findDecl(actionProfilePath)->checkedTo<IR::Declaration_Instance>();
    const auto *testObject =
        state->getTestObject("action_profile"_cs, actionProfileDecl->controlPlaneName(), false);
    const TofinoActionProfile *actionProfile = nullptr;

    if (testObject == nullptr) {
        // This means, for every possible control plane entry (and with that, new execution state)
        // add the generated action profile.
        tofinoProperties.addProfileToState = true;
        actionProfile = new TofinoActionProfile(actionProfileDecl);
    } else {
        actionProfile = testObject->checkedTo<TofinoActionProfile>();
    }
    CHECK_NULL(actionProfile);

    tofinoProperties.actionSelector = new TofinoActionSelector(selectorDecl, actionProfile);
    return true;
}

void TofinoTableStepper::checkTargetProperties(
    const std::vector<const IR::ActionListElement *> & /*tableActionList*/) {
    // Iterate over the table keys and check whether we can mitigate taint.
    for (auto keyProperties : properties.resolvedKeys) {
        const auto *keyElement = keyProperties.key;
        auto keyIsTainted =
            keyProperties.isTainted &&
            (properties.tableIsImmutable || keyElement->matchType->toString() == "exact");
        properties.tableIsTainted = properties.tableIsTainted || keyIsTainted;
        // If the key expression is tainted, do not bother resolving the remaining keys.
        if (properties.tableIsTainted) {
            ::warning("Key %1% of table %2% is tainted.", keyElement->expression, table);
            return;
        }
    }

    // Check whether the table has an action profile associated with it.
    if (checkForActionProfile()) {
        tofinoProperties.implementaton = TableImplementation::profile;
        return;
    }

    // Check whether the table has an action selector associated with it.
    if (checkForActionSelector()) {
        tofinoProperties.implementaton = TableImplementation::selector;
        return;
    }
}

void TofinoTableStepper::evalTargetTable(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    const auto *keys = table->getKey();

    const auto *progInfo = getProgramInfo()->checkedTo<TofinoSharedProgramInfo>();
    if (progInfo->isMultiPipe() && TestgenOptions::get().testBackend == "PTF") {
        properties.tableName =
            getExecutionState()->getProperty<cstring>("pipe_name"_cs) + "." + properties.tableName;
    }

    // If we have no keys, there is nothing to match.
    if (keys == nullptr) {
        // Either override the default action or fall back to executing it.
        // Some test frameworks do not support overriding a default action if there is only one.
        // We can not use tableActionList.size() because it does not include the default action.
        auto canOverride = !properties.defaultIsImmutable && !tableActionList.empty();
        // There is a special case where we should not override.
        // That is when we have only one action and that action is already set as default.
        // TODO: Find a simpler way to express this?
        canOverride =
            canOverride && (tableActionList.size() != 1 ||
                            !tableActionList.at(0)->expression->equiv(*table->getDefaultAction()));
        if (canOverride) {
            setTableDefaultEntries(tableActionList);
        } else {
            addDefaultAction(std::nullopt);
        }
        return;
    }

    // If the table is not constant, the default action can always be executed.
    // This is because we can simply not enter any table entry.
    std::optional<const IR::Expression *> tableMissCondition = std::nullopt;
    // If the table is not immutable, we synthesize control plane entries and follow the paths.
    if (properties.tableIsImmutable) {
        tofinoProperties.implementaton = TableImplementation::constant;
    }

    switch (tofinoProperties.implementaton) {
        case TableImplementation::selector: {
            // If an action selector is attached to the table, do not assume normal control plane
            // behavior.
            if (TestgenOptions::get().testBackend == "PTF") {
                evalTableActionSelector(tableActionList);
            } else {
                // We can only generate profile entries for PTF tests.
                ::warning(
                    "Action selector control plane entries are not implemented. Using default "
                    "action.");
            }
            break;
        }
        case TableImplementation::profile: {
            // If an action profile is attached to the table, do not assume normal control plane
            // behavior.
            if (TestgenOptions::get().testBackend == "PTF") {
                evalTableActionProfile(tableActionList);
            } else {
                // We can only generate profile entries for PTF tests.
                ::warning(
                    "Action profile control plane entries are not implemented. Using default "
                    "action.");
            }
            break;
        };
        case TableImplementation::skip: {
            break;
        };
        case TableImplementation::constant: {
            // If the entries properties is constant it means the entries are fixed.
            // We cannot add or remove table entries.
            tableMissCondition = evalTableConstEntries();
            break;
        };
        default: {
            evalTableControlEntries(tableActionList);
        }
    }

    // Add the default action.
    addDefaultAction(tableMissCondition);
}

TofinoTableStepper::TofinoTableStepper(SharedTofinoExprStepper *stepper, const IR::P4Table *table)
    : TableStepper(stepper, table) {}

}  // namespace P4::P4Tools::P4Testgen::Tofino
