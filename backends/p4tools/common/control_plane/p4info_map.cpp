#include "backends/p4tools/common/control_plane/p4info_map.h"

namespace P4::ControlPlaneAPI {

P4InfoMaps::P4InfoMaps(const p4::config::v1::P4Info &p4Info) { buildP4InfoMaps(p4Info); }

uint64_t szudzikPairing(p4rt_id_t x, p4rt_id_t y) {
    // See https://en.wikipedia.org/wiki/Pairing_function#Other_pairing_functions
    // This static assert ensures that p4rt_id_t used here is a uint32_t.
    // In case things change down the road.
    static_assert(std::is_convertible_v<p4rt_id_t, uint32_t> &&
                  std::is_same_v<p4rt_id_t, uint32_t>);
    return x >= y ? static_cast<uint64_t>(x) * x + x + y : x + static_cast<uint64_t>(y) * y;
}

void P4InfoMaps::buildP4InfoMaps(const p4::config::v1::P4Info &p4Info) {
    for (const auto &table : p4Info.tables()) {
        nameToIdMap[table.preamble().name()] = table.preamble().id();
        idToNameMap[table.preamble().id()] = table.preamble().name();
        for (const auto &matchField : table.match_fields()) {
            auto combinedName = table.preamble().name() + "_" + matchField.name();
            nameToIdMap[combinedName] = matchField.id();
            idToNameMap[szudzikPairing(table.preamble().id(), matchField.id())] = matchField.name();
        }
    }
    for (const auto &action : p4Info.actions()) {
        nameToIdMap[action.preamble().name()] = action.preamble().id();
        idToNameMap[action.preamble().id()] = action.preamble().name();
        for (const auto &param : action.params()) {
            auto combinedName = action.preamble().name() + "_" + param.name();
            nameToIdMap[combinedName] = param.id();
            idToNameMap[szudzikPairing(action.preamble().id(), param.id())] = param.name();
        }
    }
    for (const auto &actionProfile : p4Info.action_profiles()) {
        nameToIdMap[actionProfile.preamble().name()] = actionProfile.preamble().id();
        idToNameMap[actionProfile.preamble().id()] = actionProfile.preamble().name();
    }
    for (const auto &counter : p4Info.counters()) {
        nameToIdMap[counter.preamble().name()] = counter.preamble().id();
        idToNameMap[counter.preamble().id()] = counter.preamble().name();
    }
    for (const auto &counter : p4Info.direct_counters()) {
        nameToIdMap[counter.preamble().name()] = counter.preamble().id();
        idToNameMap[counter.preamble().id()] = counter.preamble().name();
    }
    for (const auto &meter : p4Info.meters()) {
        nameToIdMap[meter.preamble().name()] = meter.preamble().id();
        idToNameMap[meter.preamble().id()] = meter.preamble().name();
    }
    for (const auto &directMeter : p4Info.direct_meters()) {
        nameToIdMap[directMeter.preamble().name()] = directMeter.preamble().id();
        idToNameMap[directMeter.preamble().id()] = directMeter.preamble().name();
    }
    for (const auto &controllerMetadata : p4Info.controller_packet_metadata()) {
        nameToIdMap[controllerMetadata.preamble().name()] = controllerMetadata.preamble().id();
        idToNameMap[controllerMetadata.preamble().id()] = controllerMetadata.preamble().name();
    }
    for (const auto &valueSet : p4Info.value_sets()) {
        nameToIdMap[valueSet.preamble().name()] = valueSet.preamble().id();
        idToNameMap[valueSet.preamble().id()] = valueSet.preamble().name();
    }
    for (const auto &p4Register : p4Info.registers()) {
        nameToIdMap[p4Register.preamble().name()] = p4Register.preamble().id();
        idToNameMap[p4Register.preamble().id()] = p4Register.preamble().name();
    }
    for (const auto &digest : p4Info.digests()) {
        nameToIdMap[digest.preamble().name()] = digest.preamble().id();
        idToNameMap[digest.preamble().id()] = digest.preamble().name();
    }
    for (const auto &p4Extern : p4Info.externs()) {
        nameToIdMap[p4Extern.extern_type_name()] = p4Extern.extern_type_id();
        idToNameMap[p4Extern.extern_type_id()] = p4Extern.extern_type_name();
    }
}

std::optional<uint64_t> P4InfoMaps::lookUpP4RuntimeId(cstring controlPlaneName) const {
    auto it = nameToIdMap.find(controlPlaneName);
    if (it == nameToIdMap.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<cstring> P4InfoMaps::lookUpControlPlaneName(uint64_t id) const {
    auto it = idToNameMap.find(id);
    if (it == idToNameMap.end()) {
        return std::nullopt;
    }
    return it->second;
}

}  // namespace P4::ControlPlaneAPI
