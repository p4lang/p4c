#ifndef TESTGEN_LIB_TEST_SPEC_H_
#define TESTGEN_LIB_TEST_SPEC_H_

#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include <boost/optional/optional.hpp>
#include <boost/variant/variant.hpp>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools {

namespace P4Testgen {

/* =========================================================================================
 *  Abstract Test Object Class
 * ========================================================================================= */

class TestObject {
 public:
    /// Tries to convert the test object to type T. Returns a nullptr if the cast fails.
    template <typename T>
    const T* to() const {
        return dynamic_cast<const T*>(this);
    }

    /// Performs a checked cast. A BUG occurs if the cast fails.
    template <typename T>
    const T* checkedTo() const {
        auto* result = to<T>();
        BUG_CHECK(result, "Cast failed: %1%. Object has label %2%", this, getObjectName());
        return result;
    }

    /// @returns the string name of this particular test object.
    virtual cstring getObjectName() const = 0;

    virtual ~TestObject() = default;

    /// @returns a version of the test object where all expressions are resolved and symbolic
    /// variables are substituted according to the mapping present in the @param model.
    virtual const TestObject* evaluate(const Model& model) const = 0;
};

/* =========================================================================================
 *  Packet
 * ========================================================================================= */

class Packet : public TestObject {
 private:
    /// The port associated with this packets.
    int port;

    /// The actual content of the packet.
    const IR::Expression* payload;

    /// The mask of this packet.
    const IR::Expression* payloadIgnoreMask;

 public:
    Packet(int port, const IR::Expression* payload, const IR::Expression* payloadIgnoreMask);

    const Packet* evaluate(const Model& model) const override;

    cstring getObjectName() const override;

    /// @returns the port that is associated with this packet.
    int getPort() const;

    /// @returns the payload of the packet, which, at this point, needs to be a constant.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedPayload() const;

    /// @returns the don't care mask of the packet, which, at this point, needs to be a constant.
    /// If a bit is set in the @payloadIgnoreMask, the corresponding bit in @payload
    /// is ignored.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedPayloadMask() const;
};

/* =========================================================================================
 *  Table Test Objects
 * ========================================================================================= */

class ActionArg : public TestObject {
 private:
    /// The original parameter associated with this object.
    const IR::Parameter* param;

    /// Value being supplied.
    const IR::Expression* value;

 public:
    ActionArg(const IR::Parameter* param, const IR::Expression* value);

    const ActionArg* evaluate(const Model& model) const override;

    cstring getObjectName() const override;

    const IR::Parameter* getActionParam() const;

    /// @returns the parameter name associated with the action argument.
    cstring getActionParamName() const;

    /// @returns input argument value, which at this point needs to be a constant.
    /// If the value is a bool, it is converted into a constant.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedValue() const;
};

class ActionCall : public TestObject {
 private:
    /// The name of the action. If not otherwise specified, this is the control plane name.
    cstring identifier;

    /// The action that is associated with this object.
    const IR::P4Action* action;

    /// Action arguments.
    const std::vector<ActionArg> args;

 public:
    ActionCall(cstring identifier, const IR::P4Action* action, std::vector<ActionArg> args);

    ActionCall(const IR::P4Action* action, std::vector<ActionArg> args);

    const ActionCall* evaluate(const Model& model) const override;

    cstring getObjectName() const override;

    /// @returns the name of the action that is being called. If not otherwise specified, this is
    /// the control plane name.
    cstring getActionName() const;

    /// @returns the action that is associated with this object.
    const IR::P4Action* getAction() const;

    /// @returns the arguments of this particular call.
    const std::vector<ActionArg>* getArgs() const;
};

class TableMatch : public TestObject {
 private:
    /// The key associated with this object.
    const IR::KeyElement* key;

 public:
    explicit TableMatch(const IR::KeyElement* key);

    /// @returns the key associated with this object.
    const IR::KeyElement* getKey() const;
};

class Ternary : public TableMatch {
 private:
    /// The actual match value.
    const IR::Expression* value;

    /// The mask is applied using binary and operator when matching with a key.
    const IR::Expression* mask;

 public:
    explicit Ternary(const IR::KeyElement* key, const IR::Expression* value,
                     const IR::Expression* mask);

    const Ternary* evaluate(const Model& model) const override;

    cstring getObjectName() const override;

    /// @returns the value of the ternary object, which is matched with the key. At this point the
    /// value needs to be a constant.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedValue() const;

    /// @returns the mask of the ternary object. At this point the
    /// value needs to be a constant.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedMask() const;
};

class LPM : public TableMatch {
 private:
    /// How value the key is matched with.
    const IR::Expression* value;

    /// How much of the value to match.
    const IR::Expression* prefixLength;

 public:
    explicit LPM(const IR::KeyElement* key, const IR::Expression* value,
                 const IR::Expression* prefixLength);

    const LPM* evaluate(const Model& model) const override;

    cstring getObjectName() const override;

    /// @returns the value of the LPM object, which is matched with the key. At this point the
    /// value needs to be a constant.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedValue() const;

    /// @returns the prefix of LPM, which describes how many bits of the value are matched. At this
    /// point the prefix is expected to be a constant.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedPrefixLength() const;
};

class Range : public TableMatch {
 private:
    /// The inclusive start of the range.
    const IR::Expression* low;

    /// The inclusive end of the range.
    const IR::Expression* high;

 public:
    explicit Range(const IR::KeyElement* key, const IR::Expression* low,
                   const IR::Expression* high);

    const Range* evaluate(const Model& model) const override;

    cstring getObjectName() const override;

    /// @returns the inclusive start of the range. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedLow() const;

    /// @returns the inclusive end of the range. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedHigh() const;
};

class Exact : public TableMatch {
 private:
    /// The value the key is matched with.
    const IR::Expression* value;

 public:
    explicit Exact(const IR::KeyElement* key, const IR::Expression* value);

    const Exact* evaluate(const Model& model) const override;

    cstring getObjectName() const override;

    /// @returns the match value. It is expected to be a constant at this point.
    /// A BUG is thrown otherwise.
    const IR::Constant* getEvaluatedValue() const;
};

using FieldMatch = boost::variant<Exact, Ternary, LPM, Range>;

class TableRule : public TestObject {
 private:
    /// Each element is an API name paired with a match rule.
    const std::map<cstring, const FieldMatch> matches;

    /// The priority of this entry. This is required for STF back ends when matching ternary.
    int priority;
    /// The action that is called when the key matches.
    const ActionCall action;
    /// The expiry time of the entry. We currently set this to not expire.
    int ttl;

 public:
    TableRule(std::map<cstring, const FieldMatch> matches, int priority, ActionCall action,
              int ttl);

    const TableRule* evaluate(const Model& model) const override;

    cstring getObjectName() const override;

    /// @returns the list of keys that need to match to execute the action.
    const std::map<cstring, const FieldMatch>* getMatches() const;

    /// @returns the priority of this entry.
    int getPriority() const;

    /// @returns action that is called when the key matches.
    const ActionCall* getActionCall() const;

    /// @returns the time-to-live of this particular entry.
    int getTTL() const;
};

class TableConfig : public TestObject {
 private:
    /// The original table associated with this object.
    const IR::P4Table* table;

    /// Vector of TableRules.
    const std::vector<TableRule> rules;

    /// A map of table properties. For example, an action profile may be part of a table property.
    std::map<cstring, const TestObject*> tableProperties;

 public:
    explicit TableConfig(const IR::P4Table* table, std::vector<TableRule> rules);

    explicit TableConfig(const IR::P4Table* table, std::vector<TableRule> rules,
                         std::map<cstring, const TestObject*> tableProperties);

    const IR::P4Table* getTable() const;

    cstring getObjectName() const override;

    const TableConfig* evaluate(const Model& model) const override;

    /// @returns the table rules of this table.
    const std::vector<TableRule>* getRules() const;

    /// @returns the properties associated with this table.
    const std::map<cstring, const TestObject*>* getProperties() const;

    /// @returns a particular property based on @param propertyName.
    /// If @param checked is true, this will throw a BUG if @param propertyName is not in the list.
    const TestObject* getProperty(cstring propertyName, bool checked) const;

    /// Add a table property to the table.
    void addTableProperty(cstring propertyName, const TestObject* property);
};

/* =========================================================================================
 * Test Specification
 * ========================================================================================= */

class TestSpec {
 private:
    /// The input packet of the test.
    const Packet ingressPacket;

    /// The expected output packet of this test.
    const boost::optional<Packet> egressPacket;

    /// The traces that have been collected while the interpreter stepped through the program.
    const std::vector<gsl::not_null<const TraceEvent*>> traces;

    /// A map of table properties. For example, tables objects or action profiles.
    std::map<cstring, std::map<cstring, const TestObject*>> testObjects;

 public:
    TestSpec(Packet ingressPacket, boost::optional<Packet> egressPacket,
             std::vector<gsl::not_null<const TraceEvent*>> traces);

    /// Add a test object to the test specification with @param category as the object category
    /// ("tables", "registers") and objectLabel as the label
    void addTestObject(cstring category, cstring objectLabel, const TestObject* object);

    /// @returns a test object for the given category and objectlabel.
    /// Returns a BUG if checked is true and the object is not found.
    const TestObject* getTestObject(cstring category, cstring objectLabel, bool checked) const;

    /// @returns the test object using the provided category and object label. If
    /// @param checked is enabled, a BUG is thrown if the object label does not exist.
    /// Also casts the test object to the specified type. If the type does not match, a BUG is
    /// thrown.
    template <class T>
    T* getTestObject(cstring category, cstring objectLabel, bool checked) const {
        const auto* testObject = getTestObject(category, objectLabel, checked);
        return testObject->checkedTo<T>();
    }

    /// @returns the map of test objects for a given category.
    std::map<cstring, const TestObject*> getTestObjectCategory(cstring category) const;

    /// @returns the input packet for this test.
    const Packet* getIngressPacket() const;

    /// @returns the list of tables that need to be configured for this test.
    const std::map<cstring, const TableConfig>* getTables() const;

    /// @returns the expected output packet for this test.
    boost::optional<const Packet*> getEgressPacket() const;

    /// @returns the list of traces that has been executed
    const std::vector<gsl::not_null<const TraceEvent*>>* getTraces() const;

    /// Priority definitions for LPM and ternary entries.
    static constexpr int NO_PRIORITY = -1;
    static constexpr int LOW_PRIORITY = 1;
    static constexpr int HIGH_PRIORITY = 100;
    static constexpr int TTL = 0;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_LIB_TEST_SPEC_H_ */
