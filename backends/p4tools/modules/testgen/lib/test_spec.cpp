#include "backends/p4tools/modules/testgen/lib/test_spec.h"

#include <list>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

namespace P4Tools::P4Testgen {

/* =========================================================================================
 *  Test Specification Objects
 * ========================================================================================= */

Packet::Packet(int port, const IR::Expression *payload, const IR::Expression *payloadIgnoreMask)
    : port(port), payload(payload), payloadIgnoreMask(payloadIgnoreMask) {}

cstring Packet::getObjectName() const { return "Packet"; }

int Packet::getPort() const { return port; }

const IR::Constant *Packet::getEvaluatedPayloadMask() const {
    const auto *constant = payloadIgnoreMask->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              payloadIgnoreMask->type->node_type_name(), getObjectName());
    return constant;
}

const IR::Constant *Packet::getEvaluatedPayload() const {
    const auto *constant = payload->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              payload->type->node_type_name(), getObjectName());
    return constant;
}

const Packet *Packet::evaluate(const Model &model, bool doComplete) const {
    const auto *newPayload = model.evaluate(payload, true);
    const auto *newPayloadIgnoreMask = model.evaluate(payloadIgnoreMask, doComplete);
    return new Packet(port, newPayload, newPayloadIgnoreMask);
}

/* =========================================================================================
 *  Table Test Objects
 * ========================================================================================= */

ActionArg::ActionArg(const IR::Parameter *param, const IR::Expression *value)
    : param(param), value(value) {}

const IR::Parameter *ActionArg::getActionParam() const { return param; }

cstring ActionArg::getActionParamName() const { return param->controlPlaneName(); }

const IR::Constant *ActionArg::getEvaluatedValue() const {
    if (const auto *boolVal = value->to<IR::BoolLiteral>()) {
        return IR::convertBoolLiteral(boolVal);
    }
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              value->type->node_type_name(), getObjectName());
    return constant;
}

cstring ActionArg::getObjectName() const { return "ActionArg"; }

const ActionArg *ActionArg::evaluate(const Model &model, bool doComplete) const {
    const auto &newValue = model.evaluate(value, doComplete);
    return new ActionArg(param, newValue);
}

ActionCall::ActionCall(cstring identifier, const IR::P4Action *action, std::vector<ActionArg> args)
    : identifier(identifier), action(action), args(std::move(args)) {}

ActionCall::ActionCall(const IR::P4Action *action, std::vector<ActionArg> args)
    : identifier(action->controlPlaneName()), action(action), args(std::move(args)) {}

cstring ActionCall::getActionName() const { return identifier; }

const IR::P4Action *ActionCall::getAction() const { return action; }

const ActionCall *ActionCall::evaluate(const Model &model, bool doComplete) const {
    std::vector<ActionArg> evaluatedArgs;
    evaluatedArgs.reserve(args.size());
    for (const auto &actionArg : args) {
        evaluatedArgs.emplace_back(*actionArg.evaluate(model, doComplete));
    }
    return new ActionCall(identifier, action, evaluatedArgs);
}

cstring ActionCall::getObjectName() const { return "ActionCall"; }

const std::vector<ActionArg> *ActionCall::getArgs() const { return &args; }

TableMatch::TableMatch(const IR::KeyElement *key) : key(key) {}

const IR::KeyElement *TableMatch::getKey() const { return key; }

Ternary::Ternary(const IR::KeyElement *key, const IR::Expression *value, const IR::Expression *mask)
    : TableMatch(key), value(value), mask(mask) {}

const IR::Constant *Ternary::getEvaluatedValue() const {
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              value->type->node_type_name(), getObjectName());
    return constant;
}

const IR::Constant *Ternary::getEvaluatedMask() const {
    const auto *constant = mask->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              mask->type->node_type_name(), getObjectName());
    return constant;
}

const Ternary *Ternary::evaluate(const Model &model, bool doComplete) const {
    const auto *evaluatedValue = model.evaluate(value, doComplete);
    const auto *evaluatedMask = model.evaluate(mask, doComplete);
    return new Ternary(getKey(), evaluatedValue, evaluatedMask);
}

cstring Ternary::getObjectName() const { return "Ternary"; }

LPM::LPM(const IR::KeyElement *key, const IR::Expression *value, const IR::Expression *prefixLength)
    : TableMatch(key), value(value), prefixLength(prefixLength) {}

const IR::Constant *LPM::getEvaluatedValue() const {
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              value->type->node_type_name(), getObjectName());
    return constant;
}

const IR::Constant *LPM::getEvaluatedPrefixLength() const {
    const auto *constant = prefixLength->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              prefixLength->type->node_type_name(), getObjectName());
    return constant;
}

const LPM *LPM::evaluate(const Model &model, bool doComplete) const {
    const auto *evaluatedValue = model.evaluate(value, doComplete);
    const auto *evaluatedPrefixLength = model.evaluate(prefixLength, doComplete);
    return new LPM(getKey(), evaluatedValue, evaluatedPrefixLength);
}

cstring LPM::getObjectName() const { return "LPM"; }

Exact::Exact(const IR::KeyElement *key, const IR::Expression *val) : TableMatch(key), value(val) {}

const IR::Constant *Exact::getEvaluatedValue() const {
    const auto *constant = value->to<IR::Constant>();
    BUG_CHECK(constant,
              "Variable is not a constant. It has type %1% instead. Has the test object %2% "
              "been evaluated?",
              value->type->node_type_name(), getObjectName());
    return constant;
}

const Exact *Exact::evaluate(const Model &model, bool doComplete) const {
    const auto *evaluatedValue = model.evaluate(value, doComplete);
    return new Exact(getKey(), evaluatedValue);
}

cstring Exact::getObjectName() const { return "Exact"; }

TableRule::TableRule(TableMatchMap matches, int priority, ActionCall action, int ttl)
    : matches(std::move(matches)), priority(priority), action(std::move(action)), ttl(ttl) {}

const TableMatchMap *TableRule::getMatches() const { return &matches; }

int TableRule::getPriority() const { return priority; }

const ActionCall *TableRule::getActionCall() const { return &action; }

int TableRule::getTTL() const { return ttl; }

cstring TableRule::getObjectName() const { return "TableRule"; }

const TableRule *TableRule::evaluate(const Model &model, bool doComplete) const {
    TableMatchMap evaluatedMatches;
    for (const auto &matchTuple : matches) {
        auto name = matchTuple.first;
        const auto &match = matchTuple.second;
        // This is a lambda function that applies the visitor to each variant.
        const auto *evaluatedMatch = match->evaluate(model, doComplete)->checkedTo<TableMatch>();
        evaluatedMatches[name] = evaluatedMatch;
    }
    const auto *evaluatedAction = action.evaluate(model, doComplete);
    return new TableRule(evaluatedMatches, priority, *evaluatedAction, ttl);
}

const IR::P4Table *TableConfig::getTable() const { return table; }

cstring TableConfig::getObjectName() const { return "TableConfig"; }

TableConfig::TableConfig(const IR::P4Table *table, std::vector<TableRule> rules)
    : table(table), rules(std::move(rules)) {}

TableConfig::TableConfig(const IR::P4Table *table, std::vector<TableRule> rules,
                         TestObjectMap tableProperties)
    : table(table), rules(std::move(rules)), tableProperties(std::move(tableProperties)) {}

const std::vector<TableRule> *TableConfig::getRules() const { return &rules; }

const TestObjectMap *TableConfig::getProperties() const { return &tableProperties; }

const TestObject *TableConfig::getProperty(cstring propertyName, bool checked) const {
    auto it = tableProperties.find(propertyName);
    if (it != tableProperties.end()) {
        return it->second;
    }
    if (checked) {
        BUG("Unable to find test object with the label %1%. ", propertyName);
    }
    return nullptr;
}

void TableConfig::addTableProperty(cstring propertyName, const TestObject *property) {
    tableProperties[propertyName] = property;
}

const TableConfig *TableConfig::evaluate(const Model &model, bool doComplete) const {
    std::vector<TableRule> evaluatedRules;
    for (const auto &rule : rules) {
        const auto *evaluatedRule = rule.evaluate(model, doComplete);
        evaluatedRules.emplace_back(*evaluatedRule);
    }
    TestObjectMap evaluatedProperties;
    for (const auto &propertyTuple : tableProperties) {
        auto name = propertyTuple.first;
        const auto *property = propertyTuple.second;
        // This is a lambda function that applies the visitor to each variant.
        const auto *evaluatedProperty = property->evaluate(model, doComplete);
        evaluatedProperties.emplace(name, evaluatedProperty);
    }
    return new TableConfig(table, evaluatedRules, evaluatedProperties);
}

/* =========================================================================================
 * Test Specification Object
 * ========================================================================================= */

TestSpec::TestSpec(Packet ingressPacket, std::optional<Packet> egressPacket,
                   std::vector<std::reference_wrapper<const TraceEvent>> traces)
    : ingressPacket(std::move(ingressPacket)),
      egressPacket(std::move(egressPacket)),
      traces(std::move(traces)) {}

std::optional<const Packet *> TestSpec::getEgressPacket() const {
    if (egressPacket) {
        return &(*egressPacket);
    }
    return std::nullopt;
}

const Packet *TestSpec::getIngressPacket() const { return &ingressPacket; }

const std::vector<std::reference_wrapper<const TraceEvent>> *TestSpec::getTraces() const {
    return &traces;
}

void TestSpec::addTestObject(cstring category, cstring objectLabel, const TestObject *object) {
    testObjects[category][objectLabel] = object;
}

const TestObject *TestSpec::getTestObject(cstring category, cstring objectLabel,
                                          bool checked) const {
    auto testObjectCategory = getTestObjectCategory(category);
    auto it = testObjectCategory.find(objectLabel);
    if (it != testObjectCategory.end()) {
        return it->second;
    }
    if (checked) {
        BUG("Unable to find test object with the label %1% in the category %2%. ", objectLabel,
            category);
    }
    return nullptr;
}

TestObjectMap TestSpec::getTestObjectCategory(cstring category) const {
    auto it = testObjects.find(category);
    if (it != testObjects.end()) {
        return it->second;
    }
    return {};
}

}  // namespace P4Tools::P4Testgen
