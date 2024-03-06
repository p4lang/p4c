#include "backends/p4tools/modules/testgen/targets/pna/shared_table_stepper.h"

#include <optional>
#include <ostream>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/source_file.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"
#include "backends/p4tools/modules/testgen/lib/collect_coverable_nodes.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/pna/constants.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

const IR::Expression *SharedPnaTableStepper::computeTargetMatchType(
    const TableUtils::KeyProperties &keyProperties, TableMatchMap *matches,
    const IR::Expression *hitCondition) {
    const IR::Expression *keyExpr = keyProperties.key->expression;

    // TODO: We consider optional match types to be a no-op, but we could make them exact matches.
    if (keyProperties.matchType == PnaConstants::MATCH_KIND_OPT) {
        // We can recover from taint by simply not adding the optional match.
        // Create a new symbolic variable that corresponds to the key expression.
        const auto *ctrlPlaneKey =
            ControlPlaneState::getTableKey(properties.tableName, keyProperties.name, keyExpr->type);
        if (keyProperties.isTainted) {
            matches->emplace(keyProperties.name,
                             new Optional(keyProperties.key, ctrlPlaneKey, false));
        } else {
            const IR::Expression *keyExpr = keyProperties.key->expression;
            matches->emplace(keyProperties.name,
                             new Optional(keyProperties.key, ctrlPlaneKey, true));
            hitCondition = new IR::LAnd(hitCondition, new IR::Equ(keyExpr, ctrlPlaneKey));
        }
        return hitCondition;
    }
    // Action selector entries are not part of the match.
    if (keyProperties.matchType == PnaConstants::MATCH_KIND_SELECTOR) {
        SharedPnaProperties.actionSelectorKeys.emplace_back(keyExpr);
        return hitCondition;
    }
    // Ranges are not yet implemented for Pna STF tests.
    if (keyProperties.matchType == PnaConstants::MATCH_KIND_RANGE &&
        TestgenOptions::get().testBackend != "STF") {
        // We can recover from taint by matching on the entire possible range.
        const IR::Expression *minKey = nullptr;
        const IR::Expression *maxKey = nullptr;
        if (keyProperties.isTainted) {
            minKey = IR::getConstant(keyExpr->type, 0);
            maxKey = IR::getConstant(keyExpr->type, IR::getMaxBvVal(keyExpr->type));
            keyExpr = minKey;
        } else {
            std::tie(minKey, maxKey) = Bmv2ControlPlaneState::getTableRange(
                properties.tableName, keyProperties.name, keyExpr->type);
        }
        matches->emplace(keyProperties.name, new Range(keyProperties.key, minKey, maxKey));
        return new IR::LAnd(hitCondition, new IR::LAnd(new IR::LAnd(new IR::Lss(minKey, maxKey),
                                                                    new IR::Leq(minKey, keyExpr)),
                                                       new IR::Leq(keyExpr, maxKey)));
    }
    // If the custom match type does not match, delete to the core match types.
    return TableStepper::computeTargetMatchType(keyProperties, matches, hitCondition);
}

void SharedPnaTableStepper::evalTableActionProfile(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
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
        auto *actionProfile = new PnaDpdkActionProfile(*SharedPnaProperties.actionProfile);
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto *arguments = new IR::Vector<IR::Argument>();
        std::vector<ActionArg> ctrlPlaneArgs;
        for (const auto *parameter : parameters->parameters) {
            // Synthesize a symbolic variable here that corresponds to a control plane argument.
            const auto &actionArg = ControlPlaneState::getTableActionArgument(
                properties.tableName, actionName, parameter->name, parameter->type);
            arguments->push_back(new IR::Argument(actionArg));
            // We also track the argument we synthesize for the control plane.
            // Note how we use the control plane name for the parameter here.
            ctrlPlaneArgs.emplace_back(parameter, actionArg);
        }
        // Add the chosen action to the profile (it will create a new index.)
        // TODO: Should we check if we exceed the maximum number of possible profile entries?
        actionProfile->addToActionMap(actionName, ctrlPlaneArgs);
        // Update the action profile in the execution state.
        nextState.addTestObject("action_profile", actionProfile->getObjectName(), actionProfile);

        // We add the arguments to our action call, effectively creating a const entry call.
        auto *synthesizedAction = tableAction->clone();
        synthesizedAction->arguments = arguments;

        // Finally, add all the new rules to the execution state.
        const ActionCall ctrlPlaneActionCall(actionName, actionType, ctrlPlaneArgs);
        auto tableRule =
            TableRule(matches, TestSpec::LOW_PRIORITY, ctrlPlaneActionCall, TestSpec::TTL);
        auto *tableConfig = new TableConfig(table, {tableRule});

        // Add the action profile to the table.
        // This implies a slightly different implementation to usual control plane table behavior.
        tableConfig->addTableProperty("action_profile", actionProfile);
        nextState.addTestObject("tableconfigs", table->controlPlaneName(), tableConfig);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(
            new IR::MethodCallStatement(Util::SourceInfo(), synthesizedAction));
        // Some path selection strategies depend on looking ahead and collecting potential
        // nodes. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(*state);
            collector.updateNodeCoverage(actionType, coveredNodes);
        }

        nextState.set(getTableHitVar(table), IR::getBoolLiteral(true));
        nextState.set(getTableActionVar(table), getTableActionString(tableAction));
        std::stringstream tableStream;
        tableStream << "Table Branch: " << properties.tableName;
        tableStream << " Chosen action: " << actionName;
        nextState.add(*new TraceEvents::Generic(tableStream.str()));
        nextState.replaceTopBody(&replacements);
        getResult()->emplace_back(hitCondition, *state, nextState, coveredNodes);
    }
}

void SharedPnaTableStepper::evalTableActionSelector(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
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
        auto *actionProfile = new PnaDpdkActionProfile(
            SharedPnaProperties.actionSelector->getActionProfile()->getProfileDecl());
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto *arguments = new IR::Vector<IR::Argument>();
        std::vector<ActionArg> ctrlPlaneArgs;
        for (const auto *parameter : parameters->parameters) {
            // Synthesize a symbolic variable here that corresponds to a control plane argument.
            const auto &actionArg = ControlPlaneState::getTableActionArgument(
                properties.tableName, actionName, parameter->name, parameter->type);
            arguments->push_back(new IR::Argument(actionArg));
            // We also track the argument we synthesize for the control plane.
            // Note how we use the control plane name for the parameter here.
            ctrlPlaneArgs.emplace_back(parameter, actionArg);
        }
        // Add the chosen action to the profile (it will create a new index.)
        // TODO: Should we check if we exceed the maximum number of possible profile entries?
        actionProfile->addToActionMap(actionName, ctrlPlaneArgs);

        auto *actionSelector = new PnaDpdkActionSelector(
            SharedPnaProperties.actionSelector->getSelectorDecl(), actionProfile);

        // Update the action profile in the execution state.
        nextState.addTestObject("action_profile", actionProfile->getObjectName(), actionProfile);
        // Update the action selector in the execution state.
        nextState.addTestObject("action_selector", actionSelector->getObjectName(), actionSelector);

        // We add the arguments to our action call, effectively creating a const entry call.
        auto *synthesizedAction = tableAction->clone();
        synthesizedAction->arguments = arguments;

        // Finally, add all the new rules to the execution state.
        ActionCall ctrlPlaneActionCall(actionName, actionType, ctrlPlaneArgs);
        auto tableRule =
            TableRule(matches, TestSpec::LOW_PRIORITY, ctrlPlaneActionCall, TestSpec::TTL);
        auto *tableConfig = new TableConfig(table, {tableRule});

        // Add the action profile to the table. This signifies a slightly different implementation.
        tableConfig->addTableProperty("action_profile", actionProfile);
        // Add the action selector to the table. This signifies a slightly different implementation.
        tableConfig->addTableProperty("action_selector", actionSelector);

        nextState.addTestObject("tableconfigs", table->controlPlaneName(), tableConfig);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(
            new IR::MethodCallStatement(Util::SourceInfo(), synthesizedAction));
        // Some path selection strategies depend on looking ahead and collecting potential
        // nodes. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(*state);
            collector.updateNodeCoverage(actionType, coveredNodes);
        }

        nextState.set(getTableHitVar(table), IR::getBoolLiteral(true));
        nextState.set(getTableActionVar(table), getTableActionString(tableAction));

        std::stringstream tableStream;
        tableStream << "Table Branch: " << properties.tableName;
        tableStream << " Chosen action: " << actionName;
        nextState.add(*new TraceEvents::Generic(tableStream.str()));
        nextState.replaceTopBody(&replacements);
        getResult()->emplace_back(hitCondition, *state, nextState, coveredNodes);
    }
}

bool SharedPnaTableStepper::checkForActionProfile() {
    const auto *impl = table->properties->getProperty("implementation");
    if (impl == nullptr) {
        return false;
    }

    const auto *state = getExecutionState();
    const auto *implExpr = impl->value->checkedTo<IR::ExpressionValue>();
    const IR::IDeclaration *implDecl = nullptr;
    const IR::Type_Extern *implExtern = nullptr;
    if (const auto *implCall = implExpr->expression->to<IR::ConstructorCallExpression>()) {
        const auto *implDeclType = state->resolveType(implCall->constructedType);
        implExtern = implDeclType->checkedTo<IR::Type_Extern>();
        implDecl = implExtern;
    } else if (const auto *implPath = implExpr->expression->to<IR::PathExpression>()) {
        const auto *declInst = state->findDecl(implPath)->checkedTo<IR::Declaration_Instance>();
        const auto *implDeclType = state->resolveType(declInst->type);
        implExtern = implDeclType->checkedTo<IR::Type_Extern>();
        implDecl = declInst;
    } else {
        TESTGEN_UNIMPLEMENTED("Unimplemented action profile type %1%.",
                              implExpr->expression->node_type_name());
    }

    if (implExtern->name != "action_profile") {
        return false;
    }

    const auto *testObject =
        state->getTestObject("action_profile", implExtern->controlPlaneName(), false);
    if (testObject == nullptr) {
        // This means, for every possible control plane entry (and with that, new execution state)
        // add the generated action profile.
        SharedPnaProperties.addProfileToState = true;
        SharedPnaProperties.actionProfile = new PnaDpdkActionProfile(implDecl);
        return true;
    }
    SharedPnaProperties.actionProfile = testObject->checkedTo<PnaDpdkActionProfile>();
    SharedPnaProperties.addProfileToState = false;
    return true;
}

bool SharedPnaTableStepper::checkForActionSelector() {
    const auto *impl = table->properties->getProperty("implementation");
    if (impl == nullptr) {
        return false;
    }

    const auto *state = getExecutionState();
    const auto *selectorExpr = impl->value->checkedTo<IR::ExpressionValue>();
    const IR::IDeclaration *selectorDecl = nullptr;
    const IR::Type_Extern *selectorExtern = nullptr;
    if (const auto *implCall = selectorExpr->expression->to<IR::ConstructorCallExpression>()) {
        const auto *selectorDeclType = state->resolveType(implCall->constructedType);
        selectorExtern = selectorDeclType->checkedTo<IR::Type_Extern>();
        selectorDecl = selectorExtern;
    } else if (const auto *implPath = selectorExpr->expression->to<IR::PathExpression>()) {
        const auto *declInst = state->findDecl(implPath)->checkedTo<IR::Declaration_Instance>();
        const auto *selectorDeclType = state->resolveType(declInst->type);
        selectorExtern = selectorDeclType->checkedTo<IR::Type_Extern>();
        selectorDecl = declInst;
    } else {
        TESTGEN_UNIMPLEMENTED("Unimplemented action profile type %1%.",
                              selectorExpr->expression->node_type_name());
    }

    if (selectorExtern->name != "action_selector") {
        return false;
    }
    // Treat action selectors like action profiles for now.
    // The behavioral model P4Runtime is unclear how to configure action selectors.
    const auto *testObject =
        state->getTestObject("action_profile", selectorExtern->controlPlaneName(), false);
    if (testObject == nullptr) {
        // This means, for every possible control plane entry (and with that, new execution state)
        // add the generated action profile.
        SharedPnaProperties.addProfileToState = true;
        SharedPnaProperties.actionProfile = new PnaDpdkActionProfile(selectorDecl);
        return true;
    }
    SharedPnaProperties.actionProfile = testObject->checkedTo<PnaDpdkActionProfile>();
    SharedPnaProperties.addProfileToState = false;
    return true;
}

void SharedPnaTableStepper::checkTargetProperties(
    const std::vector<const IR::ActionListElement *> & /*tableActionList*/) {
    // Iterate over the table keys and check whether we can mitigate taint.
    for (auto keyProperties : properties.resolvedKeys) {
        const auto *keyElement = keyProperties.key;
        auto keyIsTainted =
            (keyProperties.isTainted &&
             (properties.tableIsImmutable || keyElement->matchType->toString() == "exact"));
        properties.tableIsTainted = properties.tableIsTainted || keyIsTainted;
        // If the key expression is tainted, do not bother resolving the remaining keys.
        if (properties.tableIsTainted) {
            ::warning("Key %1% of table %2% is tainted.", keyElement->expression, table);
            return;
        }
    }

    // Check whether the table has an action profile associated with it.
    if (checkForActionProfile()) {
        SharedPnaProperties.implementaton = TableImplementation::profile;
        return;
    }

    // Check whether the table has an action selector associated with it.
    if (checkForActionSelector()) {
        // TODO: This should be a selector. Implement.
        SharedPnaProperties.implementaton = TableImplementation::profile;
        return;
    }
}

void SharedPnaTableStepper::evalTargetTable(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    const auto *keys = table->getKey();
    // If we have no keys, there is nothing to match.
    if (keys == nullptr) {
        // Either override the default action or fall back to executing it.
        auto testBackend = TestgenOptions::get().testBackend;
        if (testBackend == "STF") {
            setTableDefaultEntries(tableActionList);
            return;
        }
        if (!properties.defaultIsImmutable) {
            ::warning(
                "Table %1%: Overriding default actions not supported for test back end %2%. "
                "Choosing default action",
                properties.tableName, testBackend);
        }
        addDefaultAction(std::nullopt);
        return;
    }

    // If the table is not constant, the default action can always be executed.
    // This is because we can simply not enter any table entry.
    std::optional<const IR::Expression *> tableMissCondition = std::nullopt;

    // If the table is not immutable, we synthesize control plane entries and follow the paths.
    if (properties.tableIsImmutable) {
        SharedPnaProperties.implementaton = TableImplementation::constant;
    }

    switch (SharedPnaProperties.implementaton) {
        case TableImplementation::selector: {
            // If an action selector is attached to the table, do not assume normal control plane
            // behavior.
            if (TestgenOptions::get().testBackend != "STF") {
                evalTableActionSelector(tableActionList);
            } else {
                // We can only generate profile entries for PTF and Protobuf tests.
                ::warning(
                    "Action selector control plane entries are not implemented. Using default "
                    "action.");
            }
            break;
        }
        case TableImplementation::profile: {
            // If an action profile is attached to the table, do not assume normal control plane
            // behavior.
            if (TestgenOptions::get().testBackend != "STF") {
                evalTableActionProfile(tableActionList);
            } else {
                // We can only generate profile entries for PTF and Protobuf tests.
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

SharedPnaTableStepper::SharedPnaTableStepper(SharedPnaExprStepper *stepper,
                                             const IR::P4Table *table)
    : TableStepper(stepper, table) {}

}  // namespace P4Tools::P4Testgen::Pna
