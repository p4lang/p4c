#include "backends/p4tools/modules/testgen/core/small_step/table_stepper.h"

#include <algorithm>
#include <map>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/multiprecision/number.hpp>
#include <boost/none.hpp>

#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/big_int_util.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"
#include "lib/safe_vector.h"

#include "backends/p4tools/modules/testgen/core/constants.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen {

const ExecutionState *TableStepper::getExecutionState() { return &stepper->state; }

const ProgramInfo *TableStepper::getProgramInfo() { return &stepper->programInfo; }

ExprStepper::Result TableStepper::getResult() { return stepper->result; }

const StateVariable &TableStepper::getTableActionVar(const IR::P4Table *table) {
    auto numActions = table->getActionList()->size();
    const auto *type = IR::getBitTypeToFit(numActions);
    return Utils::getZombieTableVar(type, table, "*action");
}

const StateVariable &TableStepper::getTableHitVar(const IR::P4Table *table) {
    return Utils::getZombieTableVar(IR::Type::Boolean::get(), table, "*hit");
}

const StateVariable &TableStepper::getTableKeyReadVar(const IR::P4Table *table, int keyIdx) {
    const auto *key = table->getKey()->keyElements.at(keyIdx);
    return Utils::getZombieTableVar(key->expression->type, table, "*keyRead", keyIdx);
}

const StateVariable &TableStepper::getTableReachedVar(const IR::P4Table *table) {
    return Utils::getZombieTableVar(IR::Type::Boolean::get(), table, "*reached");
}

bool TableStepper::compareLPMEntries(const IR::Entry *leftIn, const IR::Entry *rightIn,
                                     size_t lpmIndex) {
    // Get the entry key that matches with provided lpm index.
    // There should only be one and we only need to compare this precision.
    BUG_CHECK(
        lpmIndex < leftIn->keys->components.size() && lpmIndex < rightIn->keys->components.size(),
        "LPM index out of range.");
    const auto *left = leftIn->keys->components.at(lpmIndex);
    const auto *right = rightIn->keys->components.at(lpmIndex);

    // The expressions are equivalent, so no need to compare.
    if (left->equiv(*right)) {
        return true;
    }

    // DefaultExpression are least precise.
    if (left->is<IR::DefaultExpression>()) {
        return true;
    }
    if (right->is<IR::DefaultExpression>()) {
        return false;
    }
    // Constants are implicitly more precise than masks.
    if (left->is<IR::Constant>() && right->is<IR::Mask>()) {
        return true;
    }
    if (left->is<IR::Mask>() && right->is<IR::Constant>()) {
        return false;
    }

    const IR::Constant *leftMaskVal = nullptr;
    const IR::Constant *rightMaskVal = nullptr;
    if (const auto *leftMask = left->to<IR::Mask>()) {
        leftMaskVal = leftMask->right->checkedTo<IR::Constant>();
        // The other value must be a mask at this point.
        if (const auto *rightMask = right->checkedTo<IR::Mask>()) {
            rightMaskVal = rightMask->right->checkedTo<IR::Constant>();
        }
        return (leftMaskVal->value) >= (rightMaskVal->value);
    }

    BUG("Unhandled sort elements type: left: %1% - %2% \n right: %3% - %4% ", left,
        left->node_type_name(), right, right->node_type_name());
}

const IR::Expression *TableStepper::computeTargetMatchType(ExecutionState *nextState,
                                                           const KeyProperties &keyProperties,
                                                           TableMatchMap *matches,
                                                           const IR::Expression *hitCondition) {
    const IR::Expression *keyExpr = keyProperties.key->expression;
    // Create a new zombie constant that corresponds to the key expression.
    cstring keyName = properties.tableName + "_key_" + keyProperties.name;
    const auto ctrlPlaneKey = nextState->createZombieConst(keyExpr->type, keyName);

    if (keyProperties.matchType == P4Constants::MATCH_KIND_EXACT) {
        hitCondition = new IR::LAnd(hitCondition, new IR::Equ(keyExpr, ctrlPlaneKey));
        matches->emplace(keyProperties.name, new Exact(keyProperties.key, ctrlPlaneKey));
        return hitCondition;
    }
    if (keyProperties.matchType == P4Constants::MATCH_KIND_TERNARY) {
        cstring maskName = properties.tableName + "_mask_" + keyProperties.name;
        const IR::Expression *ternaryMask = nullptr;
        // We can recover from taint by inserting a ternary match that is 0.
        if (keyProperties.isTainted) {
            ternaryMask = IR::getConstant(keyExpr->type, 0);
            keyExpr = ternaryMask;
        } else {
            ternaryMask = nextState->createZombieConst(keyExpr->type, maskName);
        }
        matches->emplace(keyProperties.name,
                         new Ternary(keyProperties.key, ctrlPlaneKey, ternaryMask));
        return new IR::LAnd(hitCondition, new IR::Equ(new IR::BAnd(keyExpr, ternaryMask),
                                                      new IR::BAnd(ctrlPlaneKey, ternaryMask)));
    }
    if (keyProperties.matchType == P4Constants::MATCH_KIND_LPM) {
        const auto *keyType = keyExpr->type->checkedTo<IR::Type_Bits>();
        auto keyWidth = keyType->width_bits();
        cstring maskName = properties.tableName + "_lpm_prefix_" + keyProperties.name;
        const IR::Expression *maskVar = nextState->createZombieConst(keyExpr->type, maskName);
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

const IR::Expression *TableStepper::computeHit(ExecutionState *nextState, TableMatchMap *matches) {
    const IR::Expression *hitCondition = IR::getBoolLiteral(!properties.resolvedKeys.empty());
    for (auto keyProperties : properties.resolvedKeys) {
        hitCondition = computeTargetMatchType(nextState, keyProperties, matches, hitCondition);
    }
    return hitCondition;
}

void TableStepper::setTableAction(ExecutionState *nextState,
                                  const IR::MethodCallExpression *actionCall) {
    // Figure out the index of the selected action within the table's action list.
    // TODO: Simplify this. We really only need to work with indexes and names for the
    // respective table.
    const auto &actionList = table->getActionList()->actionList;
    size_t actionIdx = 0;
    for (; actionIdx < actionList.size(); ++actionIdx) {
        // Expect the expression within the ActionListElement to be a MethodCallExpression.
        const auto *expr = actionList.at(actionIdx)->expression;
        const auto *curCall = expr->to<IR::MethodCallExpression>();
        BUG_CHECK(curCall, "Action at index %1% for table %2% is not a MethodCallExpression: %3%",
                  actionIdx, table, expr);

        // Stop looping if the current action matches the selected action.
        if (curCall->method->equiv(*actionCall->method)) {
            break;
        }
    }

    BUG_CHECK(actionIdx < actionList.size(), "%1%: not a valid action for table %2%", actionCall,
              table);
    // Store the selected action.
    const auto &tableActionVar = getTableActionVar(table);
    nextState->set(tableActionVar, IR::getConstant(tableActionVar->type, actionIdx));
}

const IR::Expression *TableStepper::evalTableConstEntries() {
    const IR::Expression *tableMissCondition = IR::getBoolLiteral(true);

    const auto *keys = table->getKey();
    BUG_CHECK(keys != nullptr, "An empty key list should have been handled earlier.");

    const auto *entries = table->getEntries();
    // Sometimes, there are no entries. Just return.
    if (entries == nullptr) {
        return tableMissCondition;
    }

    auto entryVector = entries->entries;

    // Sort entries if one of the keys contains an LPM match.
    for (size_t idx = 0; idx < keys->keyElements.size(); ++idx) {
        const auto *keyElement = keys->keyElements.at(idx);
        if (keyElement->matchType->path->toString() == P4Constants::MATCH_KIND_LPM) {
            std::sort(entryVector.begin(), entryVector.end(), [idx](auto &&PH1, auto &&PH2) {
                return compareLPMEntries(std::forward<decltype(PH1)>(PH1),
                                         std::forward<decltype(PH2)>(PH2), idx);
            });
            break;
        }
    }

    for (const auto &entry : entryVector) {
        const auto *action = entry->getAction();
        BUG_CHECK(action && action->is<IR::MethodCallExpression>(),
                  "Unknown format of an action '%1%' in the table '%2%'", action, table);
        auto *nextState = new ExecutionState(stepper->state);
        const auto *tableAction = action->to<IR::MethodCallExpression>();
        BUG_CHECK(tableAction, "Invalid action '%1%' in the table '%2%'", action, table);
        // We need to set the table action in the state for eventual switch action_run hits.
        // We also will need it for control plane table entries.
        setTableAction(nextState, tableAction);
        // Compute the table key for a constant entry
        BUG_CHECK(keys->keyElements.size() == entry->keys->size(),
                  "The entry key list and key match list must be equal in size.");
        const IR::Expression *hitCondition = IR::getBoolLiteral(true);
        for (size_t idx = 0; idx < keys->keyElements.size(); ++idx) {
            const auto *key = keys->keyElements.at(idx);
            const auto *entryKey = entry->keys->components.at(idx);
            BUG_CHECK(key->expression, "Null expression %1% for matching in the table %2%", entry,
                      table);
            // These always match, so do not even consider them in the equation.
            if (entryKey->is<IR::DefaultExpression>()) {
                continue;
            }
            const IR::Expression *keyExpr = key->expression;
            if (const auto *rangeExpr = entryKey->to<IR::Range>()) {
                const auto *minKey = rangeExpr->left;
                const auto *maxKey = rangeExpr->right;
                hitCondition = new IR::LAnd(
                    hitCondition,
                    new IR::LAnd(new IR::Leq(minKey, keyExpr), new IR::Leq(keyExpr, maxKey)));
            } else if (const auto *maskExpr = entryKey->to<IR::Mask>()) {
                hitCondition = new IR::LAnd(
                    hitCondition, new IR::Equ(new IR::BAnd(keyExpr, maskExpr->right),
                                              new IR::BAnd(maskExpr->left, maskExpr->right)));
            } else {
                hitCondition = new IR::LAnd(hitCondition, new IR::Equ(keyExpr, entryKey));
            }
        }

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(new IR::MethodCallStatement(tableAction));
        nextState->set(getTableHitVar(table), IR::getBoolLiteral(true));
        nextState->set(getTableReachedVar(table), IR::getBoolLiteral(true));

        // Add some tracing information.
        std::stringstream tableStream;
        tableStream << "Constant Table Branch: " << properties.tableName;
        bool isFirstKey = true;
        const auto &keyElements = keys->keyElements;

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
        nextState->add(new TraceEvent::Generic(tableStream.str()));
        nextState->replaceTopBody(&replacements);
        // Update the default condition.
        // The default condition can only be triggered, if we do not hit this match.
        // We encode this constraint in this expression.
        stepper->result->emplace_back(new IR::LAnd(tableMissCondition, hitCondition),
                                      stepper->state, nextState);
        tableMissCondition = new IR::LAnd(new IR::LNot(hitCondition), tableMissCondition);
    }
    return tableMissCondition;
}

void TableStepper::setTableDefaultEntries(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    for (size_t idx = 0; idx < tableActionList.size(); idx++) {
        const auto *action = tableActionList.at(idx);
        // Grab the path from the method call.
        const auto *tableAction = action->expression->checkedTo<IR::MethodCallExpression>();
        // Try to find the action declaration corresponding to the path reference in the table.
        const auto *actionType = stepper->state.getActionDecl(tableAction->method);
        CHECK_NULL(actionType);

        auto *nextState = new ExecutionState(stepper->state);

        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto *arguments = new IR::Vector<IR::Argument>();
        std::vector<ActionArg> ctrlPlaneArgs;
        for (size_t argIdx = 0; argIdx < parameters->size(); ++argIdx) {
            const auto *parameter = parameters->getParameter(argIdx);
            // Synthesize a zombie constant here that corresponds to a control plane argument.
            // We get the unique name of the table coupled with the unique name of the action.
            // Getting the unique name is needed to avoid generating duplicate arguments.
            const auto &actionDataVar =
                Utils::getZombieTableVar(parameter->type, table, "*actionData", idx, argIdx);
            cstring paramName =
                properties.tableName + "_arg_" + actionName + std::to_string(argIdx);
            const auto &actionArg = nextState->createZombieConst(parameter->type, paramName);
            nextState->set(actionDataVar, actionArg);
            arguments->push_back(new IR::Argument(actionArg));
            // We also track the argument we synthesize for the control plane.
            // Note how we use the control plane name for the parameter here.
            ctrlPlaneArgs.emplace_back(parameter, actionArg);
        }
        const auto *ctrlPlaneActionCall = new ActionCall(actionType, ctrlPlaneArgs);

        // We add the arguments to our action call, effectively creating a const entry call.
        auto *synthesizedAction = tableAction->clone();
        synthesizedAction->arguments = arguments;

        // We need to set the table action in the state for eventual switch action_run hits.
        // We also will need it for control plane table entries.
        setTableAction(nextState, tableAction);

        // Finally, add all the new rules to the execution stepper->state.
        auto *tableConfig = new TableConfig(table, {});
        // Add the action selector to the table. This signifies a slightly different implementation.
        tableConfig->addTableProperty("overriden_default_action", ctrlPlaneActionCall);
        nextState->addTestObject("tableconfigs", properties.tableName, tableConfig);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(new IR::MethodCallStatement(synthesizedAction));

        nextState->set(getTableHitVar(table), IR::getBoolLiteral(false));
        nextState->set(getTableReachedVar(table), IR::getBoolLiteral(true));
        std::stringstream tableStream;
        tableStream << "Table Branch: " << properties.tableName;
        tableStream << "| Overriding default action: " << actionName;
        nextState->add(new TraceEvent::Generic(tableStream.str()));
        nextState->replaceTopBody(&replacements);
        stepper->result->emplace_back(nextState);
    }
}

void TableStepper::evalTableControlEntries(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    const auto *keys = table->getKey();
    BUG_CHECK(keys != nullptr, "An empty key list should have been handled earlier.");

    for (size_t idx = 0; idx < tableActionList.size(); idx++) {
        const auto *action = tableActionList.at(idx);
        // Grab the path from the method call.
        const auto *tableAction = action->expression->checkedTo<IR::MethodCallExpression>();
        // Try to find the action declaration corresponding to the path reference in the table.
        const auto *actionType = stepper->state.getActionDecl(tableAction->method);
        CHECK_NULL(actionType);

        auto *nextState = new ExecutionState(stepper->state);

        // First, we compute the hit condition to trigger this particular action call.
        TableMatchMap matches;
        const auto *hitCondition = computeHit(nextState, &matches);

        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto *arguments = new IR::Vector<IR::Argument>();
        std::vector<ActionArg> ctrlPlaneArgs;
        for (size_t argIdx = 0; argIdx < parameters->size(); ++argIdx) {
            const auto *parameter = parameters->getParameter(argIdx);
            // Synthesize a zombie constant here that corresponds to a control plane argument.
            // We get the unique name of the table coupled with the unique name of the action.
            // Getting the unique name is needed to avoid generating duplicate arguments.
            const auto &actionDataVar =
                Utils::getZombieTableVar(parameter->type, table, "*actionData", idx, argIdx);
            cstring paramName =
                properties.tableName + "_arg_" + actionName + std::to_string(argIdx);
            const auto &actionArg = nextState->createZombieConst(parameter->type, paramName);
            nextState->set(actionDataVar, actionArg);
            arguments->push_back(new IR::Argument(actionArg));
            // We also track the argument we synthesize for the control plane.
            // Note how we use the control plane name for the parameter here.
            ctrlPlaneArgs.emplace_back(parameter, actionArg);
        }
        ActionCall ctrlPlaneActionCall(actionType, ctrlPlaneArgs);

        // We add the arguments to our action call, effectively creating a const entry call.
        auto *synthesizedAction = tableAction->clone();
        synthesizedAction->arguments = arguments;

        // We need to set the table action in the state for eventual switch action_run hits.
        // We also will need it for control plane table entries.
        setTableAction(nextState, tableAction);

        // Finally, add all the new rules to the execution stepper->state.
        auto tableRule =
            TableRule(matches, TestSpec::LOW_PRIORITY, ctrlPlaneActionCall, TestSpec::TTL);
        auto *tableConfig = new TableConfig(table, {tableRule});
        nextState->addTestObject("tableconfigs", properties.tableName, tableConfig);

        // Update all the tracking variables for tables.
        std::vector<Continuation::Command> replacements;
        replacements.emplace_back(new IR::MethodCallStatement(synthesizedAction));

        nextState->set(getTableHitVar(table), IR::getBoolLiteral(true));
        nextState->set(getTableReachedVar(table), IR::getBoolLiteral(true));
        std::stringstream tableStream;
        tableStream << "Table Branch: " << properties.tableName;
        bool isFirstKey = true;
        const auto &keyElements = keys->keyElements;

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
        nextState->add(new TraceEvent::Generic(tableStream.str()));
        nextState->replaceTopBody(&replacements);
        stepper->result->emplace_back(hitCondition, stepper->state, nextState);
    }
}

void TableStepper::evalTaintedTable() {
    // If the table is not immutable, we just do not add any entry and execute the default action.
    if (!properties.tableIsImmutable) {
        addDefaultAction(boost::none);
        return;
    }
    std::vector<Continuation::Command> replacements;
    auto *nextState = new ExecutionState(stepper->state);

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
        BUG_CHECK(action && action->is<IR::MethodCallExpression>(),
                  "Unknown format of an action '%1%' in the table '%2%'", action, table);
        const auto *tableAction = action->to<IR::MethodCallExpression>();
        BUG_CHECK(tableAction, "Invalid action '%1%' in the table '%2%'", action, table);
        replacements.emplace_back(new IR::MethodCallStatement(tableAction));
    }
    // Since we do not know which table action was selected because of the tainted key, we also
    // set the selected action variable tainted.
    const auto &tableActionVar = getTableActionVar(table);
    nextState->set(tableActionVar,
                   stepper->programInfo.createTargetUninitialized(tableActionVar->type, true));
    // We do not know whether this table was hit or not.
    auto hitVar = getTableHitVar(table);
    nextState->set(hitVar, stepper->programInfo.createTargetUninitialized(hitVar->type, true));

    // Reset the property to its previous stepper->state.
    replacements.emplace_back(Continuation::PropertyUpdate("inUndefinedState", currentTaint));
    nextState->replaceTopBody(&replacements);
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

    const auto *state = getExecutionState();
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
        bool keyHasTaint = state->hasTaint(keyElement->expression);

        // Initialize the standard keyProperties.
        KeyProperties keyProperties(keyElement, fieldName, keyIdx, keyMatchType, keyHasTaint);
        properties.resolvedKeys.emplace_back(keyProperties);
    }
    return false;
}

void TableStepper::checkTableIsImmutable() {
    bool isConstant = false;
    const auto *entriesAnnotation = table->properties->getProperty("entries");
    if (entriesAnnotation != nullptr) {
        isConstant = entriesAnnotation->isConstant;
    }
    // Also check if the table is invisible to the control plane.
    // This also implies that it cannot be modified.
    properties.tableIsImmutable = isConstant || table->getAnnotation("hidden") != nullptr;
    const auto *defaultAction = table->properties->getProperty("default_action");
    CHECK_NULL(defaultAction);
    properties.defaultIsImmutable = defaultAction->isConstant;
}

std::vector<const IR::ActionListElement *> TableStepper::buildTableActionList() {
    std::vector<const IR::ActionListElement *> tableActionList;
    const auto *actionList = table->getActionList();
    if (actionList == nullptr) {
        return tableActionList;
    }
    for (size_t idx = 0; idx < actionList->size(); idx++) {
        const auto *action = actionList->actionList.at(idx);
        if (action->getAnnotation("defaultonly") != nullptr) {
            continue;
        }
        // Check some properties of the list.
        CHECK_NULL(action->expression);
        action->expression->checkedTo<IR::MethodCallExpression>();
        tableActionList.emplace_back(action);
    }
    return tableActionList;
}

void TableStepper::addDefaultAction(boost::optional<const IR::Expression *> tableMissCondition) {
    const auto *defaultAction = table->getDefaultAction();
    CHECK_NULL(defaultAction);
    BUG_CHECK(defaultAction->is<IR::MethodCallExpression>(),
              "Unknown format of default action in the table '%1%'", table);
    const auto *tableAction = defaultAction->to<IR::MethodCallExpression>();
    BUG_CHECK(tableAction, "Invalid action default action in the table '%1%'", table);
    auto *nextState = new ExecutionState(stepper->state);
    // We need to set the table action in the state for eventual switch action_run hits.
    // We also will need it for control plane table entries.
    setTableAction(nextState, tableAction);
    const auto *actionPath = tableAction->method->to<IR::PathExpression>();
    BUG_CHECK(actionPath, "Unknown formation of action '%1%' in table %2%", tableAction, table);

    std::vector<Continuation::Command> replacements;
    std::stringstream tableStream;
    tableStream << "Table Branch: " << properties.tableName;
    tableStream << " Choosing default action: " << actionPath;
    nextState->add(new TraceEvent::Generic(tableStream.str()));
    replacements.emplace_back(new IR::MethodCallStatement(tableAction));
    nextState->set(getTableHitVar(table), IR::getBoolLiteral(false));
    nextState->set(getTableReachedVar(table), IR::getBoolLiteral(true));
    nextState->replaceTopBody(&replacements);
    stepper->result->emplace_back(tableMissCondition, stepper->state, nextState);
}

void TableStepper::checkTargetProperties(
    const std::vector<const IR::ActionListElement *> & /*tableActionList*/) {}

void TableStepper::evalTargetTable(
    const std::vector<const IR::ActionListElement *> &tableActionList) {
    // If the table is not constant, the default action can always be executed.
    // This is because we can simply not enter any table entry.
    boost::optional<const IR::Expression *> tableMissCondition = boost::none;
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
    // Set the appropriate properties when the table is immutable, meaning it has constant entries.
    checkTableIsImmutable();
    // Resolve any non-symbolic table keys. The function returns true when a key needs replacement.
    if (resolveTableKeys()) {
        return false;
    }
    // Gather the list of executable actions. This does not include default actions, for example.
    const auto tableActionList = buildTableActionList();

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
}

}  // namespace P4Tools::P4Testgen
