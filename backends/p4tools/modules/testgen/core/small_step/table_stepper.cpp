#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"

#include <algorithm>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/number.hpp>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/constants.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"
#include "lib/source_file.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"
#include "backends/p4tools/modules/testgen/lib/collect_coverable_nodes.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

const ExecutionState *TableStepper::getExecutionState() { return &stepper->state; }

const ProgramInfo *TableStepper::getProgramInfo() { return &stepper->programInfo; }

ExprStepper::Result TableStepper::getResult() { return stepper->result; }

const IR::StateVariable &TableStepper::getTableStateVariable(const IR::Type *type,
                                                             const IR::P4Table *table, cstring name,
                                                             std::optional<int> idx1_opt,
                                                             std::optional<int> idx2_opt) {
    // Mash the table name, the given name, and the optional indices together.
    // XXX To be nice, we should probably build a PathExpression, but that's annoying to do, and we
    // XXX can probably get away with this.
    std::stringstream out;
    out << table->name.toString() << "." << name;
    if (idx1_opt.has_value()) {
        out << "." << idx1_opt.value();
    }
    if (idx2_opt.has_value()) {
        out << "." << idx2_opt.value();
    }

    return ToolsVariables::getStateVariable(type, out.str());
}

const IR::StateVariable &TableStepper::getTableActionVar(const IR::P4Table *table) {
    auto numActions = table->getActionList()->size();
    size_t max = 255;
    BUG_CHECK(numActions < max, "Number of actions in the table (%1%) exceeds the maximum of %2%.",
              numActions, max);
    return getTableStateVariable(IR::getBitType(8), table, "*action");
}

const IR::StateVariable &TableStepper::getTableResultVar(const IR::P4Table *table) {
    return getTableStateVariable(IR::Type::Boolean::get(), table, "*result");
}

const IR::StateVariable &TableStepper::getTableHitVar(const IR::P4Table *table) {
    return getTableStateVariable(IR::Type::Boolean::get(), table, "*hit");
}

const IR::Expression *TableStepper::computeTargetMatchType(
    const TableUtils::KeyProperties &keyProperties, TableMatchMap *matches,
    const IR::Expression *hitCondition) {
    const IR::Expression *keyExpr = keyProperties.key->expression;
    // Create a new variable constant that corresponds to the key expression.
    const auto *ctrlPlaneKey =
        ControlPlaneState::getTableKey(properties.tableName, keyProperties.name, keyExpr->type);

    if (keyProperties.matchType == P4Constants::MATCH_KIND_EXACT) {
        hitCondition = new IR::LAnd(hitCondition, new IR::Equ(keyExpr, ctrlPlaneKey));
        matches->emplace(keyProperties.name, new Exact(keyProperties.key, ctrlPlaneKey));
        return hitCondition;
    }
    if (keyProperties.matchType == P4Constants::MATCH_KIND_TERNARY) {
        const IR::Expression *ternaryMask = nullptr;
        // We can recover from taint by inserting a ternary match that is 0.
        if (keyProperties.isTainted) {
            ternaryMask = IR::getConstant(keyExpr->type, 0);
            keyExpr = ternaryMask;
        } else {
            ternaryMask = ControlPlaneState::getTableTernaryMask(properties.tableName,
                                                                 keyProperties.name, keyExpr->type);
        }
        matches->emplace(keyProperties.name,
                         new Ternary(keyProperties.key, ctrlPlaneKey, ternaryMask));
        return new IR::LAnd(hitCondition, new IR::Equ(new IR::BAnd(keyExpr, ternaryMask),
                                                      new IR::BAnd(ctrlPlaneKey, ternaryMask)));
    }
    if (keyProperties.matchType == P4Constants::MATCH_KIND_LPM) {
        const auto *keyType = keyExpr->type->checkedTo<IR::Type_Bits>();
        auto keyWidth = keyType->width_bits();
        const IR::Expression *maskVar = ControlPlaneState::getTableMatchLpmPrefix(
            properties.tableName, keyProperties.name, keyExpr->type);
        // The maxReturn is the maximum vale for the given bit width. This value is shifted by
        // the mask variable to create a mask (and with that, a prefix).
        auto maxReturn = IR::getMaxBvVal(keyWidth);
        auto *prefix = new IR::Sub(IR::getConstant(keyType, keyWidth), maskVar);
        const IR::Expression *lpmMask = nullptr;
        // We can recover from taint by inserting a ternary match that is 0.
        if (keyProperties.isTainted) {
            lpmMask = IR::getConstant(keyExpr->type, 0);
            maskVar = lpmMask;
            keyExpr = lpmMask;
        } else {
            lpmMask = new IR::Shl(IR::getConstant(keyType, maxReturn), prefix);
        }
        matches->emplace(keyProperties.name, new LPM(keyProperties.key, ctrlPlaneKey, maskVar));
        return new IR::LAnd(
            hitCondition,
            new IR::LAnd(
                // This is the actual LPM match under the shifted mask (the prefix).
                new IR::Leq(maskVar, IR::getConstant(keyType, keyWidth)),
                // The mask variable shift should not be larger than the key width.
                new IR::Equ(new IR::BAnd(keyExpr, lpmMask), new IR::BAnd(ctrlPlaneKey, lpmMask))));
    }

    TESTGEN_UNIMPLEMENTED("Match type %s not implemented for table keys.", keyProperties.matchType);
}

const IR::Expression *TableStepper::computeHit(TableMatchMap *matches) {
    const IR::Expression *hitCondition = IR::getBoolLiteral(!properties.resolvedKeys.empty());
    for (auto keyProperties : properties.resolvedKeys) {
        hitCondition = computeTargetMatchType(keyProperties, matches, hitCondition);
    }
    return hitCondition;
}

const IR::StringLiteral *TableStepper::getTableActionString(
    const IR::MethodCallExpression *actionCall) {
    cstring actionName = actionCall->method->toString();
    return new IR::StringLiteral(IR::Type_String::get(), actionName);
}

const IR::Expression *TableStepper::evalTableConstEntries() {
    const IR::Expression *tableMissCondition = IR::getBoolLiteral(true);

    const auto *key = table->getKey();
    BUG_CHECK(key != nullptr, "An empty key list should have been handled earlier.");

    const auto *entries = table->getEntries();
    // Sometimes, there are no entries. Just return.
    if (entries == nullptr) {
        return tableMissCondition;
    }

    auto entryVector = entries->entries;

    // Sort entries if one of the key contains an LPM match.
    for (size_t idx = 0; idx < key->keyElements.size(); ++idx) {
        const auto *keyElement = key->keyElements.at(idx);
        if (keyElement->matchType->path->toString() == P4Constants::MATCH_KIND_LPM) {
            std::sort(entryVector.begin(), entryVector.end(), [idx](auto &&PH1, auto &&PH2) {
                return TableUtils::compareLPMEntries(std::forward<decltype(PH1)>(PH1),
                                                     std::forward<decltype(PH2)>(PH2), idx);
            });
            break;
        }
    }

    for (const auto *entry : entryVector) {
        const auto *action = entry->getAction();
        const auto *tableAction = action->checkedTo<IR::MethodCallExpression>();
        const auto *actionType = stepper->state.getP4Action(tableAction);
        auto &nextState = stepper->state.clone();
        nextState.markVisited(entry);
        // Compute the table key for a constant entry
        const auto *hitCondition = TableUtils::computeEntryMatch(*table, *entry, *key);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(new IR::MethodCallStatement(Util::SourceInfo(), tableAction));
        nextState.set(getTableHitVar(table), IR::getBoolLiteral(true));
        nextState.set(getTableActionVar(table), getTableActionString(tableAction));

        // Some path selection strategies depend on looking ahead and collecting potential
        // nodes. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(stepper->state);
            collector.updateNodeCoverage(actionType, coveredNodes);
        }

        // Add some tracing information.
        std::stringstream tableStream;
        tableStream << "Constant Table Branch: " << properties.tableName;
        bool isFirstKey = true;
        const auto &keyElements = key->keyElements;

        for (const auto *keyElement : keyElements) {
            if (isFirstKey) {
                tableStream << " | Key(s): ";
            } else {
                tableStream << ", ";
            }
            tableStream << keyElement->expression;
            isFirstKey = false;
        }
        tableStream << " | Chosen action: " << tableAction->toString();
        const auto *args = tableAction->arguments;
        bool isFirstArg = true;
        for (const auto *arg : *args) {
            if (isFirstArg) {
                tableStream << " | Arg(s): ";
            } else {
                tableStream << ", ";
            }
            tableStream << arg->expression;
            isFirstArg = false;
        }
        nextState.add(*new TraceEvents::Generic(tableStream.str()));
        nextState.replaceTopBody(&replacements);
        // Update the default condition.
        // The default condition can only be triggered, if we do not hit this match.
        // We encode this constraint in this expression.
        stepper->result->emplace_back(new IR::LAnd(tableMissCondition, hitCondition),
                                      stepper->state, nextState, coveredNodes);
        tableMissCondition = new IR::LAnd(new IR::LNot(hitCondition), tableMissCondition);
    }
    return tableMissCondition;
}

void TableStepper::setTableDefaultEntries(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    for (const auto *action : tableActionList) {
        const auto *tableAction = action->expression->checkedTo<IR::MethodCallExpression>();
        const auto *actionType = stepper->state.getP4Action(tableAction);

        auto &nextState = stepper->state.clone();

        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto *arguments = new IR::Vector<IR::Argument>();
        std::vector<ActionArg> ctrlPlaneArgs;
        for (const auto *parameter : *parameters) {
            // Synthesize a variable constant here that corresponds to a control plane argument.
            const auto &actionArg = ControlPlaneState::getTableActionArgument(
                properties.tableName, actionName, parameter->name, parameter->type);

            arguments->push_back(new IR::Argument(actionArg));
            // We also track the argument we synthesize for the control plane.
            // Note how we use the control plane name for the parameter here.
            ctrlPlaneArgs.emplace_back(parameter, actionArg);
        }
        const auto *ctrlPlaneActionCall = new ActionCall(actionType, ctrlPlaneArgs);

        // We add the arguments to our action call, effectively creating a const entry call.
        auto *synthesizedAction = tableAction->clone();
        synthesizedAction->arguments = arguments;

        // Finally, add all the new rules to the execution stepper->state.
        auto *tableConfig = new TableConfig(table, {});
        // Add the action selector to the table. This signifies a slightly different implementation.
        tableConfig->addTableProperty("overriden_default_action", ctrlPlaneActionCall);
        nextState.addTestObject("tableconfigs", properties.tableName, tableConfig);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(
            new IR::MethodCallStatement(Util::SourceInfo(), synthesizedAction));
        // Some path selection strategies depend on looking ahead and collecting potential
        // nodes. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(stepper->state);
            collector.updateNodeCoverage(actionType, coveredNodes);
        }
        nextState.set(getTableHitVar(table), IR::getBoolLiteral(false));
        nextState.set(getTableActionVar(table), getTableActionString(tableAction));
        std::stringstream tableStream;
        tableStream << "Table Branch: " << properties.tableName;
        tableStream << "| Overriding default action: " << actionName;
        nextState.add(*new TraceEvents::Generic(tableStream.str()));
        nextState.replaceTopBody(&replacements);
        stepper->result->emplace_back(std::nullopt, stepper->state, nextState, coveredNodes);
    }
}

void TableStepper::evalTableControlEntries(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    const auto *key = table->getKey();
    BUG_CHECK(key != nullptr, "An empty key list should have been handled earlier.");

    // First, we compute the hit condition to trigger this particular action call.
    TableMatchMap matches;
    const auto *hitCondition = computeHit(&matches);

    // Now we iterate over all table actions and create a path per table action.
    for (const auto *action : tableActionList) {
        // Create an execution state per action.
        auto &nextState = stepper->state.clone();
        // Grab the path from the method call.
        const auto *tableAction = action->expression->checkedTo<IR::MethodCallExpression>();
        // Try to find the action declaration corresponding to the path reference in the table.
        const auto *actionType = stepper->state.getP4Action(tableAction);

        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto *arguments = new IR::Vector<IR::Argument>();
        std::vector<ActionArg> ctrlPlaneArgs;
        for (const auto *parameter : *parameters) {
            // Synthesize a variable constant here that corresponds to a control plane argument.
            const auto &actionArg = ControlPlaneState::getTableActionArgument(
                properties.tableName, actionName, parameter->name, parameter->type);
            arguments->push_back(new IR::Argument(actionArg));
            // We also track the argument we synthesize for the control plane.
            // Note how we use the control plane name for the parameter here.
            ctrlPlaneArgs.emplace_back(parameter, actionArg);
        }
        ActionCall ctrlPlaneActionCall(actionType, ctrlPlaneArgs);

        // We add the arguments to our action call, effectively creating a const entry call.
        auto *synthesizedAction = tableAction->clone();
        synthesizedAction->arguments = arguments;

        // Finally, add all the new rules to the execution stepper->state.
        auto tableRule =
            TableRule(matches, TestSpec::LOW_PRIORITY, ctrlPlaneActionCall, TestSpec::TTL);
        auto *tableConfig = new TableConfig(table, {tableRule});
        nextState.addTestObject("tableconfigs", properties.tableName, tableConfig);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(
            new IR::MethodCallStatement(Util::SourceInfo(), synthesizedAction));
        // Some path selection strategies depend on looking ahead and collecting potential
        // nodes. If that is the case, apply the CoverableNodesScanner visitor.
        P4::Coverage::CoverageSet coveredNodes;
        if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
            auto collector = CoverableNodesScanner(stepper->state);
            collector.updateNodeCoverage(actionType, coveredNodes);
        }

        nextState.set(getTableHitVar(table), IR::getBoolLiteral(true));
        nextState.set(getTableActionVar(table), getTableActionString(tableAction));

        std::stringstream tableStream;
        tableStream << "Table Branch: " << properties.tableName;
        bool isFirstKey = true;
        const auto &keyElements = key->keyElements;

        for (const auto *keyElement : keyElements) {
            if (isFirstKey) {
                tableStream << " | Key(s): ";
            } else {
                tableStream << ", ";
            }
            tableStream << keyElement->expression;
            isFirstKey = false;
        }
        tableStream << "| Chosen action: " << actionName;
        nextState.add(*new TraceEvents::Generic(tableStream.str()));
        nextState.replaceTopBody(&replacements);
        stepper->result->emplace_back(hitCondition, stepper->state, nextState, coveredNodes);
    }
}

void TableStepper::evalTaintedTable() {
    // If the table is not immutable, we just do not add any entry and execute the default action.
    if (!properties.tableIsImmutable) {
        addDefaultAction(std::nullopt);
        return;
    }
    std::vector<Continuation::Command> replacements;
    auto &nextState = stepper->state.clone();

    // If the table is immutable, we execute all the constant entries in its list.
    // We get the current value of the inUndefinedState property.
    auto currentTaint = stepper->state.getProperty<bool>("inUndefinedState");
    replacements.emplace_back(Continuation::PropertyUpdate("inUndefinedState", true));

    const auto *entries = table->getEntries();
    // Sometimes, there are no entries. Just return.
    if (entries == nullptr) {
        return;
    }
    auto entryVector = entries->entries;

    for (const auto &entry : entryVector) {
        const auto *action = entry->getAction();
        const auto *tableAction = action->checkedTo<IR::MethodCallExpression>();
        replacements.emplace_back(new IR::MethodCallStatement(Util::SourceInfo(), tableAction));
    }
    // Since we do not know which table action was selected because of the tainted key, we also
    // set the selected action variable tainted.
    const auto &tableActionVar = getTableActionVar(table);
    nextState.set(tableActionVar,
                  stepper->programInfo.createTargetUninitialized(tableActionVar->type, true));
    // We do not know whether this table was hit or not.
    auto hitVar = getTableHitVar(table);
    nextState.set(hitVar, stepper->programInfo.createTargetUninitialized(hitVar->type, true));

    // Reset the property to its previous stepper->state.
    replacements.emplace_back(Continuation::PropertyUpdate("inUndefinedState", currentTaint));
    nextState.replaceTopBody(&replacements);
    stepper->result->emplace_back(nextState);
}

bool TableStepper::resolveTableKeys() {
    auto propertyIdx = -1;
    const IR::Key *key = nullptr;
    for (propertyIdx = 0; propertyIdx < static_cast<int>(table->properties->properties.size());
         ++propertyIdx) {
        const auto *property = table->properties->properties.at(propertyIdx);
        if (property->name == "key") {
            key = property->value->checkedTo<IR::Key>();
            break;
        }
    }

    if (key == nullptr) {
        return false;
    }

    auto keyElements = key->keyElements;
    for (size_t keyIdx = 0; keyIdx < keyElements.size(); ++keyIdx) {
        const auto *keyElement = keyElements.at(keyIdx);
        const auto *keyExpr = keyElement->expression;
        if (!SymbolicEnv::isSymbolicValue(keyExpr)) {
            // Resolve all keys in the table.
            const auto *const p4Table = table;
            ExprStepper::stepToSubexpr(
                keyExpr, stepper->result, stepper->state,
                [p4Table, propertyIdx, keyIdx](const Continuation::Parameter *v) {
                    // We have to clone a whole bunch of nodes first.
                    auto *clonedTable = p4Table->clone();
                    auto *clonedTableProperties = p4Table->properties->clone();
                    auto *properties = &clonedTableProperties->properties;
                    auto *propertyValue = properties->at(propertyIdx)->clone();
                    const auto *key = clonedTable->getKey();
                    CHECK_NULL(key);
                    auto *newKey = key->clone();
                    auto *newKeyElement = newKey->keyElements[keyIdx]->clone();
                    // Now bubble them up in reverse order.
                    // Replace the single (!) key in the table.
                    newKeyElement->expression = v->param;
                    newKey->keyElements[keyIdx] = newKeyElement;
                    propertyValue->value = newKey;
                    (*properties)[propertyIdx] = propertyValue;
                    clonedTable->properties = clonedTableProperties;
                    return Continuation::Return(clonedTable);
                });
            return true;
        }

        const auto *nameAnnot = keyElement->getAnnotation("name");
        // Some hidden tables do not have any key name annotations.
        BUG_CHECK(nameAnnot != nullptr || properties.tableIsImmutable,
                  "Non-constant table key without an annotation");
        cstring fieldName;
        if (nameAnnot != nullptr) {
            fieldName = nameAnnot->getName();
        }
        // It is actually possible to use a variety of types as key.
        // So we have to stay generic and produce a corresponding variable.
        cstring keyMatchType = keyElement->matchType->toString();
        // We can recover from taint for some match types, which is why we track taint.
        bool keyHasTaint = Taint::hasTaint(keyElement->expression);

        // Initialize the standard keyProperties.
        TableUtils::KeyProperties keyProperties(keyElement, fieldName, keyIdx, keyMatchType,
                                                keyHasTaint);
        properties.resolvedKeys.emplace_back(keyProperties);
    }
    return false;
}

void TableStepper::addDefaultAction(std::optional<const IR::Expression *> tableMissCondition) {
    const auto *defaultAction = table->getDefaultAction();
    const auto *tableAction = defaultAction->checkedTo<IR::MethodCallExpression>();
    const auto *actionType = stepper->state.getP4Action(tableAction);
    auto &nextState = stepper->state.clone();
    const auto *actionPath = tableAction->method->to<IR::PathExpression>();
    BUG_CHECK(actionPath, "Unknown formation of action '%1%' in table %2%", tableAction, table);

    std::vector<Continuation::Command> replacements;
    std::stringstream tableStream;
    tableStream << "Table Branch: " << properties.tableName;
    tableStream << " Choosing default action: " << actionPath;
    nextState.add(*new TraceEvents::Generic(tableStream.str()));
    replacements.emplace_back(new IR::MethodCallStatement(Util::SourceInfo(), tableAction));
    // Some path selection strategies depend on looking ahead and collecting potential
    // nodes.
    P4::Coverage::CoverageSet coveredNodes;
    if (requiresLookahead(TestgenOptions::get().pathSelectionPolicy)) {
        auto collector = CoverableNodesScanner(stepper->state);
        collector.updateNodeCoverage(actionType, coveredNodes);
    }
    nextState.set(getTableHitVar(table), IR::getBoolLiteral(false));
    nextState.set(getTableActionVar(table), getTableActionString(tableAction));

    nextState.replaceTopBody(&replacements);
    stepper->result->emplace_back(tableMissCondition, stepper->state, nextState, coveredNodes);
}

void TableStepper::checkTargetProperties(
    const std::vector<const IR::ActionListElement *> & /*tableActionList*/) {}

void TableStepper::evalTargetTable(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    // If the table is not constant, the default action can always be executed.
    // This is because we can simply not enter any table entry.
    std::optional<const IR::Expression *> tableMissCondition = std::nullopt;
    // If the table is not immutable, we synthesize control plane entries and follow the paths.
    if (properties.tableIsImmutable) {
        // If the entries properties is constant it means the entries are fixed.
        // We cannot add or remove table entries.
        tableMissCondition = evalTableConstEntries();
    } else {
        evalTableControlEntries(tableActionList);
    }

    // Add the default action.
    addDefaultAction(tableMissCondition);
}

bool TableStepper::eval() {
    // Resolve any non-symbolic table keys. The function returns true when a key needs replacement.
    if (resolveTableKeys()) {
        return false;
    }
    // Gather the list of executable actions. This does not include default actions, for example.
    const auto tableActionList = TableUtils::buildTableActionList(*table);

    checkTargetProperties(tableActionList);

    // If the table key is tainted, the control plane entries do not really matter.
    // Assume that any action can be executed.
    // Important: This should follow the immutability check.
    // This is because the taint behavior may differ with constant entries.
    if (properties.tableIsTainted) {
        evalTaintedTable();
        return false;
    }

    evalTargetTable(tableActionList);

    return false;
}

TableStepper::TableStepper(ExprStepper *stepper, const IR::P4Table *table)
    : stepper(stepper), table(table) {
    properties.tableName = table->controlPlaneName();
    for (size_t index = 0; index < table->getActionList()->size(); index++) {
        const auto *action = table->getActionList()->actionList.at(index);
        properties.actionIdMap.emplace(action->controlPlaneName(), index);
    }

    // Set the appropriate properties when the table is immutable, meaning it has constant entries.
    TableUtils::checkTableImmutability(*table, properties);

    // If the table is in the set of entities to skip, we set it immutable.
    // P4Testgen will not add a control plane entry for this table.
    auto &skipped = TestgenOptions::get().skippedControlPlaneEntities;
    if (skipped.find(properties.tableName) != skipped.end()) {
        properties.tableIsImmutable = true;
    }
}

}  // namespace P4Tools::P4Testgen
