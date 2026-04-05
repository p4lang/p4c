#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_FUZZER_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_FUZZER_H_

#include "backends/p4tools/modules/rtsmith/core/program_info.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/common/control_plane/bfruntime/bfruntime.pb.h"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

namespace P4::P4Tools::RtSmith {

using ProtobufMessagePtr = std::unique_ptr<google::protobuf::Message>;
using InitialConfig = std::vector<ProtobufMessagePtr>;
using UpdateSeries = std::vector<std::pair<uint64_t, ProtobufMessagePtr>>;

class RuntimeFuzzer {
 private:
    /// The program info of the target.
    std::reference_wrapper<const ProgramInfo> programInfo;

 protected:
    /// @returns the program info associated with the current target.
    [[nodiscard]] virtual const ProgramInfo &getProgramInfo() const { return programInfo; }

    /// A map of control plane objects and their current state.
    std::map<std::string, std::set<std::string>> currentState;

 public:
    explicit RuntimeFuzzer(const ProgramInfo &programInfo) : programInfo(programInfo) {}

    /// @brief Produce an `InitialConfig`, which is a vector of updates.
    /// @return A InitialConfig
    virtual InitialConfig produceInitialConfig() = 0;

    /// @brief Produce an `UpdateSeries`, which is a vector of indexed updates.
    /// @return A InitialConfig
    virtual UpdateSeries produceUpdateTimeSeries() = 0;

    /// Some Helper functions below

    /// @brief Check `value` to a string of length `bitwidth`.
    /// @param value
    /// @param bitwidth
    /// @return the result string.
    static std::string checkBigIntToString(const big_int &value, int bitwidth);

    /// @brief Produce bytes in form of std::string given bitwidth.
    /// @param bitwidth
    /// @return A random bytes of length bitwidth in form of std::string.
    static std::string produceBytes(int bitwidth);

    /// @brief Produce bytes in form of std::string given bitwidth.
    /// @param bitwidth
    /// @param value
    /// @return A bytes of value of length bitwidth in form of std::string.
    static std::string produceBytes(int bitwidth, const big_int &value);

    /// @brief Produce bytes within min and max in form of std::string given bitwidth.
    /// @param bitwidth
    /// @param min
    /// @param max
    /// @return A bytes of value of length bitwidth within min and max in form of std::string.
    static std::string produceBytes(int bitwidth, const big_int &min, const big_int &max);

    static bool tableHasFieldType(const p4::config::v1::Table &table,
                                  const p4::config::v1::MatchField::MatchType type);
};

class P4RuntimeFuzzer : public RuntimeFuzzer {
 public:
    explicit P4RuntimeFuzzer(const ProgramInfo &programInfo) : RuntimeFuzzer(programInfo) {}

    /// @brief Produce a FieldMatch_Exact with bitwidth
    /// @param bitwidth
    /// @return A FieldMatch_Exact
    virtual p4::v1::FieldMatch_Exact produceFieldMatch_Exact(int bitwidth);

    /// @brief Produce a FieldMatch_LPM with bitwidth
    /// @param bitwidth
    /// @return A FieldMatch_LPM
    virtual p4::v1::FieldMatch_LPM produceFieldMatch_LPM(int bitwidth);

    /// @brief Produce a FieldMatch_Ternary with bitwidth
    /// @param bitwidth
    /// @return A FieldMatch_Ternary
    virtual p4::v1::FieldMatch_Ternary produceFieldMatch_Ternary(int bitwidth);

    /// @brief Produce a FieldMatch_Range with bitwidth
    /// @param bitwidth
    /// @return A FieldMatch_Range
    virtual p4::v1::FieldMatch_Range produceFieldMatch_Range(int bitwidth);

    /// @brief Produce a FieldMatch_Optional with bitwidth
    /// @param bitwidth
    /// @return A FieldMatch_Optional
    virtual p4::v1::FieldMatch_Optional produceFieldMatch_Optional(int bitwidth);

    /// @brief Produce a param for an action in the table entry
    /// @param param
    /// @return An action param
    virtual p4::v1::Action_Param produceActionParam(const p4::config::v1::Action_Param &param);

    /// @brief Produce a random action selected for a table entry
    /// @param action_refs action reference options where we will randomly pick one from.
    /// @param actions actions that containing `action_refs` for finding the action.
    /// @return An `Action`
    virtual p4::v1::Action produceTableAction(
        const google::protobuf::RepeatedPtrField<p4::config::v1::ActionRef> &action_refs,
        const google::protobuf::RepeatedPtrField<p4::config::v1::Action> &actions);

    /// @brief Produce priority for an entry given match fields
    /// @param matchFields
    /// @return A 32-bit integer
    virtual uint32_t producePriority(
        const google::protobuf::RepeatedPtrField<p4::config::v1::MatchField> &matchFields);

    /// @brief Produce match field given match type
    /// @param match
    /// @return A `FieldMatch`
    virtual p4::v1::FieldMatch produceMatchField(p4::config::v1::MatchField &match);

    /// @brief Produce a `TableEntry` with id, match fields, priority and action
    /// @param table
    /// @param actions
    /// @return A `TableEntry`
    virtual p4::v1::TableEntry produceTableEntry(
        const p4::config::v1::Table &table,
        const google::protobuf::RepeatedPtrField<p4::config::v1::Action> &actions);

    /// @brief Produce a `WriteRequest` with a vector of `TableEntry`.
    /// @param isInitialConfig describes whether the write request is generated in the context of an
    /// initial configuration (no updates or deletes are used there).
    /// @return A `WriteRequest`
    std::unique_ptr<p4::v1::WriteRequest> produceWriteRequest(bool isInitialConfig);
};

}  // namespace P4::P4Tools::RtSmith

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_FUZZER_H_ */
