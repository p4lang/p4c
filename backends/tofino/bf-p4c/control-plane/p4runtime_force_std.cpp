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

#ifndef EXTENSIONS_BF_P4C_CONTROL_PLANE_P4RUNTIME_FORCE_STD_H_
#define EXTENSIONS_BF_P4C_CONTROL_PLANE_P4RUNTIME_FORCE_STD_H_

#include "p4runtime_force_std.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include <p4/config/v1/p4info.pb.h>

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "barefoot/p4info.pb.h"
#pragma GCC diagnostic pop
#include "control-plane/p4RuntimeSerializer.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"

namespace p4configv1 = ::p4::config::v1;

namespace BFN {

using namespace P4;

class P4RuntimeStdConverter {
 public:
    explicit P4RuntimeStdConverter(const p4configv1::P4Info *p4infoIn) : p4infoIn(p4infoIn) {}

    const p4configv1::P4Info *convert() {
        p4configv1::P4Info *p4info = new p4configv1::P4Info;
        p4info->CopyFrom(*p4infoIn);

        using ConverterFn =
            std::function<void(p4configv1::P4Info *, const p4configv1::ExternInstance &)>;
        using UpdatorFn = std::function<void(p4configv1::ExternInstance &)>;
        using C = P4RuntimeStdConverter;
        using std::placeholders::_1;
        using std::placeholders::_2;

        const std::unordered_map<P4Id, ConverterFn> converters = {
            {::barefoot::P4Ids::ACTION_PROFILE, std::bind(&C::convertActionProfile, this, _1, _2)},
            {::barefoot::P4Ids::ACTION_SELECTOR,
             std::bind(&C::convertActionSelector, this, _1, _2)},
            {::barefoot::P4Ids::COUNTER, std::bind(&C::convertCounter, this, _1, _2)},
            {::barefoot::P4Ids::DIRECT_COUNTER, std::bind(&C::convertDirectCounter, this, _1, _2)},
            {::barefoot::P4Ids::METER, std::bind(&C::convertMeter, this, _1, _2)},
            {::barefoot::P4Ids::DIRECT_METER, std::bind(&C::convertDirectMeter, this, _1, _2)},
            {::barefoot::P4Ids::DIGEST, std::bind(&C::convertDigest, this, _1, _2)},
            {::barefoot::P4Ids::REGISTER, std::bind(&C::convertRegister, this, _1, _2)},
            {::barefoot::P4Ids::VALUE_SET, std::bind(&C::convertValueSet, this, _1, _2)},
        };

        std::unordered_map<P4Id, UpdatorFn> updators = {
            {::barefoot::P4Ids::PORT_METADATA, std::bind(&C::updatePortMetadata, this, _1)},
        };

        static const std::unordered_set<P4Id> suppressWarnings = {
            ::barefoot::P4Ids::REGISTER_PARAM, ::barefoot::P4Ids::PORT_METADATA,
            ::barefoot::P4Ids::SNAPSHOT,       ::barefoot::P4Ids::SNAPSHOT_TRIGGER,
            ::barefoot::P4Ids::SNAPSHOT_DATA,  ::barefoot::P4Ids::SNAPSHOT_LIVENESS,
        };

        // Some TNA specific externs may be required to be generated
        // within the 'externs' block in p4info output. Add them to the list
        // below
        static const std::unordered_set<P4Id> allowedExterns = {
            ::barefoot::P4Ids::PORT_METADATA,
        };

        // When converting action profile instances, we need to know if the
        // action profile is associated with a selector, and if yes we need the
        // information (e.g. max_group_size) about the selector. So we iterate
        // once over all extern instances first and build a map from action
        // profile id to corresponding selector (if any).
        buildActionSelectorMap(*p4info);

        // Convert TNA Externs to P4 Externs
        for (const auto &externType : p4info->externs()) {
            auto externTypeId = static_cast<::barefoot::P4Ids::Prefix>(externType.extern_type_id());
            auto converterIt = converters.find(externTypeId);
            if (converterIt == converters.end()) {
                if (suppressWarnings.count(externTypeId) == 0) {
                    warning("No known conversion to standard P4Info for '%1%' extern type",
                            externType.extern_type_name());
                }
                continue;
            }
            for (const auto &externInstance : externType.instances())
                converterIt->second(p4info, externInstance);
        }

        // Update allowed TNA externs
        for (auto &externType : *p4info->mutable_externs()) {
            auto externTypeId = static_cast<::barefoot::P4Ids::Prefix>(externType.extern_type_id());
            auto updatorIt = updators.find(externTypeId);
            if (updatorIt == updators.end()) continue;
            for (auto &externInstance : *externType.mutable_instances())
                updatorIt->second(externInstance);
        }

        // remove all (except allowed) TNA-specific externs now that we have
        // converted / updated them to standard P4Info objects (when it was possible)
        typedef google::protobuf::RepeatedPtrField<p4configv1::Extern> protoExterns;
        auto externs = new protoExterns(p4info->externs());
        p4info->clear_externs();
        for (auto externIt = externs->begin(); externIt != externs->end(); externIt++) {
            auto externTypeId = static_cast<::barefoot::P4Ids::Prefix>(externIt->extern_type_id());
            if (allowedExterns.count(externTypeId)) {
                auto *externType = p4info->add_externs();
                externType->CopyFrom(*externIt);
            }
        }

        // update implementation_id and direct_resource_ids fields in Table
        // messages because the ids have changed
        updateTables(p4info);

        return p4info;
    }

 private:
    using P4Id = uint32_t;

    template <typename T>
    static constexpr P4Id makeStdId(T base, p4configv1::P4Ids::Prefix prefix) {
        return static_cast<P4Id>((base & 0xffffff) | (prefix << 24));
    }

    template <typename U>
    static void unpackExternInstance(const p4configv1::ExternInstance &externInstance,
                                     U *unpackedMessage) {
        auto success = externInstance.info().UnpackTo(unpackedMessage);
        // this is a BUG_CHECK because the p4Info message being converted was
        // itself produced by this compiler just a moment ago...
        BUG_CHECK(success, "Could not unpack extern instance '%1%'",
                  externInstance.preamble().name());
    }

    // Build the oldActionSelectors map, which maps action profile id (from the
    // original TNA-specific P4Info message) to the corresponding action
    // selector ExternInstance message (if any).
    void buildActionSelectorMap(const p4configv1::P4Info &p4info) {
        for (const auto &externType : p4info.externs()) {
            auto externTypeId = static_cast<::barefoot::P4Ids::Prefix>(externType.extern_type_id());
            if (externTypeId != ::barefoot::P4Ids::ACTION_SELECTOR) continue;
            for (const auto &externInstance : externType.instances()) {
                ::barefoot::ActionSelector actionSelector;
                unpackExternInstance(externInstance, &actionSelector);
                BUG_CHECK(actionSelector.action_profile_id() != 0,
                          "Action selector '%1%' does not specify an action profile",
                          externInstance.preamble().name());
                auto r =
                    oldActionSelectors.emplace(actionSelector.action_profile_id(), externInstance);
                BUG_CHECK(r.second, "Action profile id %1% is referenced by multiple selectors",
                          actionSelector.action_profile_id());
            }
            break;
        }
    }

    void insertId(P4Id oldId, P4Id newId) {
        auto r = oldToNewIds.emplace(oldId, newId);
        BUG_CHECK(r.second, "Id %1% was already in id map when converting P4Info to standard",
                  oldId);
    }

    // Strip the pipe prefix from the P4Info name
    // FIXME: This is currently required so that the drivers code can reconcile
    // the P4Info names with the context JSON names (for TNA progams). This
    // stripping should not be required and the drivers should be able to do the
    // reconciliation even when the pipe name is present based on which P4
    // architecture is used. However, the architecture information is currently
    // not available in _pi_update_device_start because it is stripped by the
    // P4Runtime server from P4Info. When this is fixed, we will no longer need
    // to strip the pipe prefix here.
    void stripPipePrefix(std::string *name) {
        auto pos = name->find_first_of('.');
        if (pos == std::string::npos) return;
        *name = name->substr(pos + 1);
    }

    template <typename T>
    void setPreamble(const p4configv1::ExternInstance &externInstance,
                     p4configv1::P4Ids::Prefix stdPrefix, T *stdMessage) {
        const auto &preIn = externInstance.preamble();
        auto *preOut = stdMessage->mutable_preamble();
        preOut->CopyFrom(preIn);
        stripPipePrefix(preOut->mutable_name());
        P4Id idBase = preIn.id() & 0xffffff;
        P4Id id;
        // Convert the TNA P4Info id to the Standard P4Info id. Most of the
        // time, this just means doing the correct prefix substitution. But
        // there can be some subtle differences between the 2, which may cause
        // id collisions. We therefore add some logic to handle these
        // collisions, to be safe and futureproof.
        do {
            id = makeStdId(idBase++, stdPrefix);
        } while (allocatedIds.count(id) > 0);
        allocatedIds.insert(id);
        preOut->set_id(id);
        insertId(preIn.id(), preOut->id());
    }

    void convertActionProfile(p4configv1::P4Info *p4info,
                              const p4configv1::ExternInstance &externInstance) {
        ::barefoot::ActionProfile actionProfile;
        unpackExternInstance(externInstance, &actionProfile);
        auto *actionProfileStd = p4info->add_action_profiles();
        setPreamble(externInstance, p4configv1::P4Ids::ACTION_PROFILE, actionProfileStd);
        actionProfileStd->mutable_table_ids()->CopyFrom(actionProfile.table_ids());
        actionProfileStd->set_size(actionProfile.size());

        auto oldId = externInstance.preamble().id();
        auto actionSelectorIt = oldActionSelectors.find(oldId);
        if (actionSelectorIt != oldActionSelectors.end()) {
            ::barefoot::ActionSelector actionSelector;
            unpackExternInstance(actionSelectorIt->second, &actionSelector);
            actionProfileStd->set_with_selector(true);
            actionProfileStd->set_max_group_size(actionSelector.max_group_size());
            // map (old) action selector id to (new / std) action profile id, so
            // that we can translate the implementation_id field in the Table
            // message.
            insertId(actionSelectorIt->second.preamble().id(), actionProfileStd->preamble().id());
        }
    }

    void convertActionSelector(p4configv1::P4Info *, const p4configv1::ExternInstance &) {
        // The standard P4Info only has one message (ActionProfile) whereas the
        // TNA-specific P4Info defines one message for action profiles
        // (ActionProfile) and one for action selectors (ActionSelector). When
        // visiting the TNA-specific ActionProfile messages (in
        // convertActionProfile), we generate the appropriate standard
        // ActionProfile message (in particular we set the with_selector field)
        // to the appropriate value, which means we have nothing to do for
        // TNA-specific ActionSelector messages (which is why this function
        // is empty). Note that by definition, each ActionSelector message in
        // the TNA-specific P4Info must have a corresponding ActionProfile
        // message (no selector table without action data table).
    }

    static void setCounterSpec(const ::barefoot::CounterSpec &specIn,
                               p4configv1::CounterSpec *specOut) {
        // works because the numerical values of the enum members are the same
        specOut->set_unit(static_cast<p4configv1::CounterSpec::Unit>(specIn.unit()));
    }

    void convertCounter(p4configv1::P4Info *p4info,
                        const p4configv1::ExternInstance &externInstance) {
        ::barefoot::Counter counter;
        unpackExternInstance(externInstance, &counter);
        auto *counterStd = p4info->add_counters();
        setPreamble(externInstance, p4configv1::P4Ids::COUNTER, counterStd);
        counterStd->set_size(counter.size());
        setCounterSpec(counter.spec(), counterStd->mutable_spec());
    }

    void convertDirectCounter(p4configv1::P4Info *p4info,
                              const p4configv1::ExternInstance &externInstance) {
        ::barefoot::DirectCounter directCounter;
        unpackExternInstance(externInstance, &directCounter);
        auto *directCounterStd = p4info->add_direct_counters();
        setPreamble(externInstance, p4configv1::P4Ids::DIRECT_COUNTER, directCounterStd);
        directCounterStd->set_direct_table_id(directCounter.direct_table_id());
        setCounterSpec(directCounter.spec(), directCounterStd->mutable_spec());
    }

    static void setMeterSpec(const ::barefoot::MeterSpec &specIn, p4configv1::MeterSpec *specOut) {
        // works because the numerical values of the enum members are the same
        specOut->set_unit(static_cast<p4configv1::MeterSpec::Unit>(specIn.unit()));
    }

    void convertMeter(p4configv1::P4Info *p4info,
                      const p4configv1::ExternInstance &externInstance) {
        ::barefoot::Meter meter;
        unpackExternInstance(externInstance, &meter);
        auto *meterStd = p4info->add_meters();
        setPreamble(externInstance, p4configv1::P4Ids::METER, meterStd);
        meterStd->set_size(meter.size());
        setMeterSpec(meter.spec(), meterStd->mutable_spec());
    }

    void convertDirectMeter(p4configv1::P4Info *p4info,
                            const p4configv1::ExternInstance &externInstance) {
        ::barefoot::DirectMeter directMeter;
        unpackExternInstance(externInstance, &directMeter);
        auto *directMeterStd = p4info->add_direct_meters();
        setPreamble(externInstance, p4configv1::P4Ids::DIRECT_METER, directMeterStd);
        directMeterStd->set_direct_table_id(directMeter.direct_table_id());
        setMeterSpec(directMeter.spec(), directMeterStd->mutable_spec());
    }

    void convertDigest(p4configv1::P4Info *p4info,
                       const p4configv1::ExternInstance &externInstance) {
        ::barefoot::Digest digest;
        unpackExternInstance(externInstance, &digest);
        auto *digestStd = p4info->add_digests();
        setPreamble(externInstance, p4configv1::P4Ids::DIGEST, digestStd);
        digestStd->mutable_type_spec()->CopyFrom(digest.type_spec());
    }

    void convertRegister(p4configv1::P4Info *p4info,
                         const p4configv1::ExternInstance &externInstance) {
        ::barefoot::Register reg;
        unpackExternInstance(externInstance, &reg);
        auto *regStd = p4info->add_registers();
        setPreamble(externInstance, p4configv1::P4Ids::REGISTER, regStd);
        regStd->set_size(reg.size());
        regStd->mutable_type_spec()->CopyFrom(reg.type_spec());
    }

    void convertValueSet(p4configv1::P4Info *p4info,
                         const p4configv1::ExternInstance &externInstance) {
        ::barefoot::ValueSet valueSet;
        unpackExternInstance(externInstance, &valueSet);
        auto *valueSetStd = p4info->add_value_sets();
        setPreamble(externInstance, p4configv1::P4Ids::VALUE_SET, valueSetStd);
        valueSetStd->set_size(valueSet.size());
    }

    void updatePortMetadata(p4configv1::ExternInstance &externInstance) {
        auto *preamble = externInstance.mutable_preamble();
        stripPipePrefix(preamble->mutable_name());
        auto *info = externInstance.mutable_info();
        auto *value = info->mutable_value();
        *value = "ig_intr_md.ingress_port";
    }

    void updateTables(p4configv1::P4Info *p4info) {
        for (auto &table : *p4info->mutable_tables()) {
            stripPipePrefix(table.mutable_preamble()->mutable_name());
            auto &tableName = table.preamble().name();
            auto implementationId = table.implementation_id();
            if (implementationId) {
                auto idIt = oldToNewIds.find(implementationId);
                if (idIt != oldToNewIds.end()) {
                    table.set_implementation_id(idIt->second);
                } else {
                    error(
                        "Unknown implementation id %1% for table '%2%' "
                        "(maybe the implementation extern type is not supported)",
                        implementationId, tableName);
                    table.clear_implementation_id();
                }
            }
            int idx = 0;
            for (auto directResourceId : table.direct_resource_ids()) {
                auto idIt = oldToNewIds.find(directResourceId);
                if (idIt != oldToNewIds.end()) {
                    table.set_direct_resource_ids(idx++, idIt->second);
                } else {
                    warning(
                        "Unknown direct resource id %1% for table '%2%' "
                        "(maybe the extern type is not supported), "
                        "so dropping the direct resource for the table",
                        directResourceId, tableName);
                }
            }
            table.mutable_direct_resource_ids()->Truncate(idx);
        }
    }

    const p4configv1::P4Info *p4infoIn;
    std::unordered_set<P4Id> allocatedIds{};
    std::unordered_map<P4Id, P4Id> oldToNewIds{};
    // Maps an action profile id to the corresponding action selector. If an
    // action profile does not have an action selector, there is no entry in the
    // map. Note that it is not possible to have one action selector map to
    // multiple action profiles.
    std::unordered_map<P4Id, p4configv1::ExternInstance> oldActionSelectors{};
};

P4::P4RuntimeAPI convertToStdP4Runtime(const P4::P4RuntimeAPI &p4RuntimeIn) {
    CHECK_NULL(p4RuntimeIn.p4Info);
    P4RuntimeStdConverter converter(p4RuntimeIn.p4Info);

    P4::P4RuntimeAPI p4RuntimeOut(converter.convert(), p4RuntimeIn.entries);

    // Note that the output P4Info may include some additional data in
    // P4TypeInfo which is not required any more, however, this is unlikely to
    // be an issue.
    return p4RuntimeOut;
}

}  // namespace BFN

#endif /* EXTENSIONS_BF_P4C_CONTROL_PLANE_P4RUNTIME_FORCE_STD_H_ */
