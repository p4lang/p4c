#ifndef DPDK_CONTROL_PLANE_BFRUNTIME_EXT_H_
#define DPDK_CONTROL_PLANE_BFRUNTIME_EXT_H_

#include "control-plane/bfruntime.h"
#include "p4/config/dpdk/p4info.pb.h"

namespace P4 {

namespace BFRT {

static Util::JsonObject* makeTypeString(boost::optional<cstring> defaultValue = boost::none) {
    auto* typeObj = new Util::JsonObject();
    typeObj->emplace("type", "string");
    if (defaultValue != boost::none)
        typeObj->emplace("default_value", *defaultValue);
    return typeObj;
}

static P4Id makeActSelectorId(P4Id implementationId) {
    return makeBFRuntimeId(implementationId, ::dpdk::P4Ids::ACTION_SELECTOR);
}

/// This class is in charge of translating the P4Info Protobuf message used in
/// the context of P4Runtime to the BF-RT info JSON used by the BF-RT API.
class BFRuntimeSchemaGenerator : public BFRuntimeGenerator {
 public:
    explicit BFRuntimeSchemaGenerator(const p4configv1::P4Info& p4info)
        : BFRuntimeGenerator(p4info) { }

    /// Generates the schema as a Json object for the provided P4Info instance.
    const Util::JsonObject* genSchema() const override;

 private:
    // TODO(antonin): these values may need to be available to the BF-RT
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

        BF_RT_DATA_ACTION_MEMBER_ID,
        BF_RT_DATA_SELECTOR_GROUP_ID,
        BF_RT_DATA_ACTION_MEMBER_STATUS,
        BF_RT_DATA_MAX_GROUP_SIZE,
        BF_RT_DATA_HASH_VALUE,
    };
    // Externs only for DPDK backend
    struct ActionSelector;

    void addDPDKExterns(Util::JsonArray* tablesJson,
                                    Util::JsonArray* learnFiltersJson) const;
    void addActionSelectorCommon(Util::JsonArray* tablesJson,
                                 const ActionSelector& actionProf) const;
    void addActionSelectorGetMemberCommon(Util::JsonArray* tablesJson,
                                    const ActionSelector& actionProf) const;
    void addActionProfs(Util::JsonArray* tablesJson) const override;
    bool addActionProfIds(const p4configv1::Table& table,
                            Util::JsonObject* tableJson) const override;
    void addMatchActionData(const p4configv1::Table& table,
            Util::JsonObject* tableJson, Util::JsonArray* dataJson,
            P4Id maxActionParamId) const ;

    boost::optional<bool> actProfHasSelector(P4Id actProfId) const override;

    static boost::optional<ActionProf>
    fromDPDKActionProfile(const p4configv1::P4Info& p4info,
            const p4configv1::ExternInstance& externInstance) {
        const auto& pre = externInstance.preamble();
        p4configv1::ActionProfile actionProfile;
        if (!externInstance.info().UnpackTo(&actionProfile)) {
            ::error("Extern instance %1% does not pack an ActionProfile object", pre.name());
            return boost::none;
        }
        auto tableIds = collectTableIds(
            p4info, actionProfile.table_ids().begin(), actionProfile.table_ids().end());
        return ActionProf{pre.name(), pre.id(), actionProfile.size(), tableIds,
                          transformAnnotations(pre)};
    };
};

}  // namespace BFRT

}  // namespace P4 

#endif  // DPDK_CONTROL_PLANE_BFRUNTIME_EXT_H_
