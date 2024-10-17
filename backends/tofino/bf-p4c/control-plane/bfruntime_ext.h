/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_CONTROL_PLANE_BFRUNTIME_EXT_H_
#define EXTENSIONS_BF_P4C_CONTROL_PLANE_BFRUNTIME_EXT_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "barefoot/p4info.pb.h"
#pragma GCC diagnostic pop
#include "bf-p4c/device.h"
#include "bfruntime.h"
#include "bf-utils/include/dynamic_hash/bfn_hash_algorithm.h"

namespace BFN {

namespace BFRT {

using namespace BFN::BFRT;

/// This class is in charge of translating the P4Info Protobuf message used in
/// the context of P4Runtime to the BF-RT info JSON used by the BF-RT API. It
/// supports both the "standard" P4Info message (the one used for v1model & PSA)
/// and P4Info with Tofino-specific extensions (the one used for TNA and JNA
/// programs).
/// TODO: In theory the BF-RT info JSON generated for a PSA program
/// (standard P4Info) and the one generated for the translated version of the
/// program (P4Info with Tofino extensions) should be exactly the same since
/// there is no loss of information and the names should remain the
/// same. Whether this is true in practice remains to be seen. We can change our
/// approach fairly easily if needed.
class BFRuntimeSchemaGenerator : public BFRuntimeGenerator {
 public:
    explicit BFRuntimeSchemaGenerator(const p4configv1::P4Info& p4info)
        : BFRuntimeGenerator(p4info) { }

    /// Generates the schema as a Json object for the provided P4Info instance.
    const Util::JsonObject* genSchema() const override;

 private:
    // TODO: these values may need to be available to the BF-RT
    // implementation as well, if they want to expose them as enums.

    // To avoid potential clashes with P4 names, we prefix the names of "fixed"
    // data field with a '$'. For example, for TD_DATA_ACTION_MEMBER_ID, we
    // use the name $ACTION_MEMBER_ID.
    enum BFRuntimeDataFieldIds : P4Id {
        // ids for fixed data fields must not collide with the auto-generated
        // ids for P4 fields (e.g. match key fields). Snapshot tables include
        // ALL the fields defined in the P4 program so we need to ensure that
        // this BF_RT_DATA_START offset is quite large.
        // PSA Ids are between TD_DATA_START = (1 << 16) and TD_DATA_END,
        BF_RT_DATA_START = TD_DATA_END,

        BF_RT_DATA_MATCH_PRIORITY,

        BF_RT_DATA_LPF_INDEX,
        BF_RT_DATA_WRED_INDEX,

        BF_RT_DATA_ACTION_MEMBER_ID,
        BF_RT_DATA_SELECTOR_GROUP_ID,
        BF_RT_DATA_ACTION_MEMBER_STATUS,
        BF_RT_DATA_MAX_GROUP_SIZE,
        BF_RT_DATA_ADT_OFFSET,

        BF_RT_DATA_LPF_SPEC_TYPE,
        BF_RT_DATA_LPF_SPEC_GAIN_TIME_CONSTANT_NS,
        BF_RT_DATA_LPF_SPEC_DECAY_TIME_CONSTANT_NS,
        BF_RT_DATA_LPF_SPEC_OUT_SCALE_DOWN_FACTOR,

        BF_RT_DATA_WRED_SPEC_TIME_CONSTANT_NS,
        BF_RT_DATA_WRED_SPEC_MIN_THRESH_CELLS,
        BF_RT_DATA_WRED_SPEC_MAX_THRESH_CELLS,
        BF_RT_DATA_WRED_SPEC_MAX_PROBABILITY,

        BF_RT_DATA_PORT_METADATA_PORT,
        BF_RT_DATA_PORT_METADATA_DEFAULT_FIELD,

        BF_RT_DATA_HASH_ALGORITHM_ROTATE,
        BF_RT_DATA_HASH_ALGORITHM_SEED,
        BF_RT_DATA_HASH_ALGORITHM_MSB,
        BF_RT_DATA_HASH_ALGORITHM_EXTEND,
        BF_RT_DATA_HASH_ALGORITHM_PRE_DEFINED,
        BF_RT_DATA_HASH_ALGORITHM_USER_DEFINED,
        BF_RT_DATA_HASH_VALUE,
        BF_RT_DATA_HASH_CONFIGURE_START_BIT,
        BF_RT_DATA_HASH_CONFIGURE_LENGTH,
        BF_RT_DATA_HASH_CONFIGURE_ORDER,

        BF_RT_DATA_SNAPSHOT_START_STAGE,
        BF_RT_DATA_SNAPSHOT_END_STAGE,
        BF_RT_DATA_SNAPSHOT_ENABLE,
        BF_RT_DATA_SNAPSHOT_TRIGGER_STATE,
        BF_RT_DATA_SNAPSHOT_TIMER_ENABLE,
        BF_RT_DATA_SNAPSHOT_TIMER_TRIGGER,
        BF_RT_DATA_SNAPSHOT_TIMER_VALUE_USECS,
        BF_RT_DATA_SNAPSHOT_FIELD_INFO,
        BF_RT_DATA_SNAPSHOT_CONTROL_INFO,
        BF_RT_DATA_SNAPSHOT_TABLE_INFO,
        BF_RT_DATA_SNAPSHOT_METER_ALU_INFO,
        BF_RT_DATA_SNAPSHOT_STAGE_ID,
        BF_RT_DATA_SNAPSHOT_PREV_STAGE_TRIGGER,
        BF_RT_DATA_SNAPSHOT_INGRESS_TRIGGER_MODE,
        BF_RT_DATA_SNAPSHOT_LOCAL_STAGE_TRIGGER,
        BF_RT_DATA_SNAPSHOT_NEXT_TABLE_ID,
        BF_RT_DATA_SNAPSHOT_NEXT_TABLE_NAME,
        BF_RT_DATA_SNAPSHOT_ENABLED_NEXT_TABLES,
        BF_RT_DATA_SNAPSHOT_GLOBAL_EXECUTE_TABLES,
        BF_RT_DATA_SNAPSHOT_ENABLED_GLOBAL_EXECUTE_TABLES,
        BF_RT_DATA_SNAPSHOT_LONG_BRANCH_TABLES,
        BF_RT_DATA_SNAPSHOT_ENABLED_LONG_BRANCH_TABLES,
        BF_RT_DATA_SNAPSHOT_DEPARSER_ERROR,
        BF_RT_DATA_SNAPSHOT_TABLE_ID,
        BF_RT_DATA_SNAPSHOT_TABLE_NAME,
        BF_RT_DATA_SNAPSHOT_METER_TABLE_NAME,
        BF_RT_DATA_SNAPSHOT_METER_ALU_OPERATION_TYPE,
        BF_RT_DATA_SNAPSHOT_MATCH_HIT_HANDLE,
        BF_RT_DATA_SNAPSHOT_TABLE_HIT,
        BF_RT_DATA_SNAPSHOT_TABLE_INHIBITED,
        BF_RT_DATA_SNAPSHOT_TABLE_EXECUTED,
        BF_RT_DATA_SNAPSHOT_LIVENESS_FIELD_NAME,
        BF_RT_DATA_SNAPSHOT_LIVENESS_VALID_STAGES,
        BF_RT_DATA_SNAPSHOT_THREAD,
        BF_RT_DATA_SNAPSHOT_INGRESS_CAPTURE,
        BF_RT_DATA_SNAPSHOT_EGRESS_CAPTURE,
        BF_RT_DATA_SNAPSHOT_GHOST_CAPTURE,

        BF_RT_DATA_PARSER_INSTANCE,
        BF_RT_DATA_PARSER_NAME,

        BF_RT_DATA_DEBUG_CNT_TABLE_NAME,
        BF_RT_DATA_DEBUG_CNT_TYPE,
        BF_RT_DATA_DEBUG_CNT_VALUE,
    };
    // Externs only for TNA architectures
    struct Lpf;
    struct Wred;
    struct PortMetadata;
    struct Snapshot;
    struct DynHash;
    struct ParserChoices;
    struct RegisterParam;
    struct ValueSet;
    struct ActionSelector;

    static void addLpfDataFields(Util::JsonArray* dataJson);
    static void addWredDataFields(Util::JsonArray* dataJson);

    void addDebugCounterTable(Util::JsonArray* tablesJson) const;
    void addDynHash(Util::JsonArray* tablesJson, const DynHash& dynHash) const;
    void addDynHashAlgorithm(Util::JsonArray* tablesJson, const DynHash& dynHash) const;
    void addDynHashCompute(Util::JsonArray* tablesJson, const DynHash& dynHash) const;
    void addDynHashConfig(Util::JsonArray* tablesJson, const DynHash& dynHash) const;
    void addLpf(Util::JsonArray* tablesJson, const Lpf& lpf) const;
    void addParserChoices(Util::JsonArray* tablesJson, const ParserChoices& parserChoices) const;
    void addPortMetadata(Util::JsonArray* tablesJson, const PortMetadata& portMetadata) const;
    void addPortMetadataDefault(Util::JsonArray* tablesJson) const;
    void addPortMetadataExtern(Util::JsonArray* tablesJson) const;
    void addRegisterParam(Util::JsonArray* tablesJson, const RegisterParam&) const;
    /// Add register parameter data fields to the JSON data array for a BFRT table. Field
    /// ids are assigned incrementally starting at @p idOffset, which is 1 by
    /// default.
    void addRegisterParamDataFields(Util::JsonArray* dataJson,
                                    const RegisterParam& register_param_,
                                    P4Id idOffset = 1) const;
    void addSnapshot(Util::JsonArray* tablesJson, const Snapshot& snapshot) const;
    void addSnapshotLiveness(Util::JsonArray* tablesJson, const Snapshot& snapshot) const;
    void addTNAExterns(Util::JsonArray* tablesJson,
                                    Util::JsonArray* learnFiltersJson) const;
    void addWred(Util::JsonArray* tablesJson, const Wred& wred) const;
    void addDirectResources(const p4configv1::Table& table, Util::JsonArray* dataJson,
            Util::JsonArray* operationsJson, Util::JsonArray* attributesJson,
            P4Id maxActionParamId) const override;
    void addValueSet(Util::JsonArray* tablesJson, const ValueSet& valueSet) const;
    void addActionSelectorCommon(Util::JsonArray* tablesJson,
                                 const ActionSelector& actionProf) const;
    void addActionSelectorGetMemberCommon(Util::JsonArray* tablesJson,
                                    const ActionSelector& actionProf) const;
    void addActionProfs(Util::JsonArray* tablesJson) const override;
    bool addActionProfIds(const p4configv1::Table& table,
                            Util::JsonObject* tableJson) const override;

    std::optional<bool> actProfHasSelector(P4Id actProfId) const override;

    std::optional<Lpf> getDirectLpf(P4Id lpfId) const;
    std::optional<Wred> getDirectWred(P4Id wredId) const;
    std::optional<Counter> getDirectCounter(P4Id counterId) const override;
    std::optional<Meter> getDirectMeter(P4Id meterId) const override;
    std::optional<Register> getDirectRegister(P4Id registerId) const;

    static std::optional<ActionProf>
    fromTNAActionProfile(const p4configv1::P4Info& p4info,
            const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::ActionProfile actionProfile;
        if (!externInstance.info().UnpackTo(&actionProfile)) {
            error("Extern instance %1% does not pack an ActionProfile object", pre.name());
            return std::nullopt;
        }
        auto tableIds = collectTableIds(
            p4info, actionProfile.table_ids().begin(), actionProfile.table_ids().end());
        return ActionProf{pre.name(), pre.id(), actionProfile.size(), tableIds,
                          transformAnnotations(pre)};
    };

    static std::optional<Counter>
    fromTNACounter(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::Counter counter;
        if (!externInstance.info().UnpackTo(&counter)) {
            error("Extern instance %1% does not pack a Counter object", pre.name());
            return std::nullopt;
        }
        auto unit = static_cast<Counter::Unit>(counter.spec().unit());
        return Counter{pre.name(), pre.id(), counter.size(), unit, transformAnnotations(pre)};
    }

    static std::optional<Counter>
    fromTNADirectCounter(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::DirectCounter counter;
        if (!externInstance.info().UnpackTo(&counter)) {
            error("Extern instance %1% does not pack a DirectCounter object", pre.name());
            return std::nullopt;
        }
        auto unit = static_cast<Counter::Unit>(counter.spec().unit());
        return Counter{pre.name(), pre.id(), 0, unit, transformAnnotations(pre)};
    }

    static std::optional<Meter>
    fromTNAMeter(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::Meter meter;
        if (!externInstance.info().UnpackTo(&meter)) {
            error("Extern instance %1% does not pack a Meter object", pre.name());
            return std::nullopt;
        }
        auto unit = static_cast<Meter::Unit>(meter.spec().unit());
        auto type = static_cast<Meter::Type>(meter.spec().type());
        return Meter{pre.name(), pre.id(), meter.size(), unit, type, transformAnnotations(pre)};
    }

    static std::optional<Meter>
    fromTNADirectMeter(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::DirectMeter meter;
        if (!externInstance.info().UnpackTo(&meter)) {
            error("Extern instance %1% does not pack a Meter object", pre.name());
            return std::nullopt;
        }
        auto unit = static_cast<Meter::Unit>(meter.spec().unit());
        auto type = static_cast<Meter::Type>(meter.spec().type());
        return Meter{pre.name(), pre.id(), 0, unit, type, transformAnnotations(pre)};
    }

    static std::optional<Digest>
    fromTNADigest(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::Digest digest;
        if (!externInstance.info().UnpackTo(&digest)) {
            error("Extern instance %1% does not pack a Digest object", pre.name());
            return std::nullopt;
        }
        return Digest{pre.name(), pre.id(), digest.type_spec(), transformAnnotations(pre)};
    }

    static std::optional<Register>
    fromTNARegister(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::Register register_;
        if (!externInstance.info().UnpackTo(&register_)) {
            error("Extern instance %1% does not pack a Register object", pre.name());
            return std::nullopt;
        }
        return Register{pre.name(), register_.data_field_name(), pre.id(),
                register_.size(), register_.type_spec(), transformAnnotations(pre)};
    }

    static std::optional<Register>
    fromTNADirectRegister(const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        ::barefoot::DirectRegister register_;
        if (!externInstance.info().UnpackTo(&register_)) {
            error("Extern instance %1% does not pack a Register object", pre.name());
            return std::nullopt;
        }
        return Register{pre.name(), register_.data_field_name(), pre.id(), 0,
                register_.type_spec(), transformAnnotations(pre)};
    }
};

}  // namespace BFRT

}  // namespace BFN

#endif  // EXTENSIONS_BF_P4C_CONTROL_PLANE_BFRUNTIME_EXT_H_
