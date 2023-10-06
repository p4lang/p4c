/* Copyright 2021 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef DPDK_CONTROL_PLANE_BFRUNTIME_EXT_H_
#define DPDK_CONTROL_PLANE_BFRUNTIME_EXT_H_

#include "backends/dpdk/constants.h"
#include "backends/dpdk/options.h"
#include "backends/dpdk/p4/config/p4info.pb.h"
#include "control-plane/bfruntime.h"

namespace P4 {

namespace BFRT {

/// This class is in charge of translating the P4Info Protobuf message used in
/// the context of P4Runtime to the BF-RT info JSON used by the BF-RT API.
class BFRuntimeSchemaGenerator : public BFRuntimeGenerator {
 public:
    BFRuntimeSchemaGenerator(const p4configv1::P4Info &p4info, bool isTDI,
                             DPDK::DpdkOptions &options)
        : BFRuntimeGenerator(p4info), isTDI(isTDI), options(options) {}

    /// Generates the schema as a Json object for the provided P4Info instance.
    const Util::JsonObject *genSchema() const override;

 private:
    bool isTDI;
    DPDK::DpdkOptions &options;
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

    void addDPDKExterns(Util::JsonArray *tablesJson, Util::JsonArray *learnFiltersJson) const;
    void addActionSelectorCommon(Util::JsonArray *tablesJson,
                                 const ActionSelector &actionProf) const;
    void addActionSelectorGetMemberCommon(Util::JsonArray *tablesJson,
                                          const ActionSelector &actionProf) const;
    void addConstTableAttr(Util::JsonArray *attrJson) const override;
    bool addMatchTypePriority(std::optional<cstring> &matchType) const override;
    void addActionProfs(Util::JsonArray *tablesJson) const override;
    bool addActionProfIds(const p4configv1::Table &table,
                          Util::JsonObject *tableJson) const override;
    void addMatchActionData(const p4configv1::Table &table, Util::JsonObject *tableJson,
                            Util::JsonArray *dataJson, P4Id maxActionParamId) const;

    std::optional<bool> actProfHasSelector(P4Id actProfId) const override;

    static std::optional<ActionProf> fromDPDKActionProfile(
        const p4configv1::P4Info &p4info, const p4configv1::ExternInstance &externInstance) {
        const auto &pre = externInstance.preamble();
        p4configv1::ActionProfile actionProfile;
        if (!externInstance.info().UnpackTo(&actionProfile)) {
            ::error(ErrorType::ERR_NOT_FOUND,
                    "Extern instance %1% does not pack an ActionProfile object", pre.name());
            return std::nullopt;
        }
        auto tableIds = collectTableIds(p4info, actionProfile.table_ids().begin(),
                                        actionProfile.table_ids().end());
        return ActionProf{pre.name(), pre.id(), actionProfile.size(), tableIds,
                          transformAnnotations(pre)};
    };
};

}  // namespace BFRT

}  // namespace P4

#endif  // DPDK_CONTROL_PLANE_BFRUNTIME_EXT_H_
