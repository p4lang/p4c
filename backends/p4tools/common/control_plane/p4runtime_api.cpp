#include "backends/p4tools/common/control_plane/p4runtime_api.h"

#include "control-plane/p4RuntimeArchStandard.h"
#include "p4/config/v1/p4info.pb.h"

namespace P4::ControlPlaneAPI {

P4RuntimeMaps::P4RuntimeMaps(const p4::config::v1::P4Info &p4Info) { buildP4RuntimeMaps(p4Info); }

p4rt_id_t szudzikPairing(p4rt_id_t x, p4rt_id_t y) { return x >= y ? x * x + x + y : x + y * y; }

void P4RuntimeMaps::buildP4RuntimeMaps(const p4::config::v1::P4Info &p4Info) {
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

std::optional<p4rt_id_t> P4RuntimeMaps::lookupP4RuntimeId(cstring controlPlaneName) const {
    auto it = nameToIdMap.find(controlPlaneName);
    if (it == nameToIdMap.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<cstring> P4RuntimeMaps::lookupControlPlaneName(p4rt_id_t id) const {
    auto it = idToNameMap.find(id);
    if (it == idToNameMap.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<p4::config::v1::Table> findP4RuntimeTable(const p4::config::v1::P4Info &p4Info,
                                                        cstring controlPlaneName) {
    for (const auto &table : p4Info.tables()) {
        if (table.preamble().name() == controlPlaneName) {
            return table;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Table> findP4RuntimeTable(const p4::config::v1::P4Info &p4Info,
                                                        p4rt_id_t id) {
    for (const auto &table : p4Info.tables()) {
        if (table.preamble().id() == id) {
            return table;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::MatchField> findP4RuntimeMatchField(
    const p4::config::v1::Table &p4Table, cstring controlPlaneName) {
    for (const auto &p4InfoMatchField : p4Table.match_fields()) {
        if (controlPlaneName == p4InfoMatchField.name()) {
            return p4InfoMatchField;
        }
    }
    ::error("Match field %1% not found in the P4Info table", controlPlaneName);
    return std::nullopt;
}

std::optional<p4::config::v1::MatchField> findP4RuntimeMatchField(
    const p4::config::v1::Table &p4Table, p4rt_id_t id) {
    for (const auto &p4InfoMatchField : p4Table.match_fields()) {
        if (id == p4InfoMatchField.id()) {
            return p4InfoMatchField;
        }
    }
    ::error("Match field %1% not found in the P4Info table", id);
    return std::nullopt;
}

std::optional<p4::config::v1::Action> findP4RuntimeAction(const p4::config::v1::P4Info &p4Info,
                                                          cstring controlPlaneName) {
    for (const auto &action : p4Info.actions()) {
        if (controlPlaneName == action.preamble().name()) {
            return action;
        }
    }
    ::error("Failed to find default action %1% in the P4Info object", controlPlaneName);
    return std::nullopt;
}

std::optional<p4::config::v1::Action> findP4RuntimeAction(const p4::config::v1::P4Info &p4Info,
                                                          p4rt_id_t id) {
    for (const auto &action : p4Info.actions()) {
        if (action.preamble().id() == id) {
            return action;
        }
    }
    ::error("Failed to find default action id %1% in the P4Info object", id);
    return std::nullopt;
}

std::optional<p4::config::v1::ActionProfile> findP4RuntimeActionProfile(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName) {
    for (const auto &action : p4Info.action_profiles()) {
        if (action.preamble().name() == controlPlaneName) {
            return action;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::ActionProfile> findP4RuntimeActionProfile(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id) {
    for (const auto &action : p4Info.action_profiles()) {
        if (action.preamble().id() == id) {
            return action;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Counter> findP4RuntimeCounter(const p4::config::v1::P4Info &p4Info,
                                                            cstring controlPlaneName) {
    for (const auto &counter : p4Info.counters()) {
        if (counter.preamble().name() == controlPlaneName) {
            return counter;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Counter> findP4RuntimeCounter(const p4::config::v1::P4Info &p4Info,
                                                            p4rt_id_t id) {
    for (const auto &counter : p4Info.counters()) {
        if (counter.preamble().id() == id) {
            return counter;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::DirectCounter> findP4RuntimeDirectCounter(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName) {
    for (const auto &counter : p4Info.direct_counters()) {
        if (counter.preamble().name() == controlPlaneName) {
            return counter;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::DirectCounter> findP4RuntimeDirectCounter(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id) {
    for (const auto &counter : p4Info.direct_counters()) {
        if (counter.preamble().id() == id) {
            return counter;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Meter> findP4RuntimeMeter(const p4::config::v1::P4Info &p4Info,
                                                        cstring controlPlaneName) {
    for (const auto &meter : p4Info.meters()) {
        if (meter.preamble().name() == controlPlaneName) {
            return meter;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Meter> findP4RuntimeMeter(const p4::config::v1::P4Info &p4Info,
                                                        p4rt_id_t id) {
    for (const auto &meter : p4Info.meters()) {
        if (meter.preamble().id() == id) {
            return meter;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::DirectMeter> findP4RuntimeDirectMeter(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName) {
    for (const auto &meter : p4Info.direct_meters()) {
        if (meter.preamble().name() == controlPlaneName) {
            return meter;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::DirectMeter> findP4RuntimeDirectMeter(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id) {
    for (const auto &meter : p4Info.direct_meters()) {
        if (meter.preamble().id() == id) {
            return meter;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::ControllerPacketMetadata> findP4RuntimeControllerMetadata(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName) {
    for (const auto &controllerMetadata : p4Info.controller_packet_metadata()) {
        if (controllerMetadata.preamble().name() == controlPlaneName) {
            return controllerMetadata;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::ControllerPacketMetadata> findP4RuntimeControllerMetadata(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id) {
    for (const auto &controllerMetadata : p4Info.controller_packet_metadata()) {
        if (controllerMetadata.preamble().id() == id) {
            return controllerMetadata;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::ValueSet> findP4RuntimeValueSet(const p4::config::v1::P4Info &p4Info,
                                                              cstring controlPlaneName) {
    for (const auto &valueSet : p4Info.value_sets()) {
        if (valueSet.preamble().name() == controlPlaneName) {
            return valueSet;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::ValueSet> findP4RuntimeValueSet(const p4::config::v1::P4Info &p4Info,
                                                              p4rt_id_t id) {
    for (const auto &valueSet : p4Info.value_sets()) {
        if (valueSet.preamble().id() == id) {
            return valueSet;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Register> findP4RuntimeRegister(const p4::config::v1::P4Info &p4Info,
                                                              cstring controlPlaneName) {
    for (const auto &p4Register : p4Info.registers()) {
        if (p4Register.preamble().name() == controlPlaneName) {
            return p4Register;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Register> findP4RuntimeRegister(const p4::config::v1::P4Info &p4Info,
                                                              p4rt_id_t id) {
    for (const auto &p4Register : p4Info.registers()) {
        if (p4Register.preamble().id() == id) {
            return p4Register;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Digest> findP4RuntimeDigest(const p4::config::v1::P4Info &p4Info,
                                                          cstring controlPlaneName) {
    for (const auto &digest : p4Info.digests()) {
        if (digest.preamble().name() == controlPlaneName) {
            return digest;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Digest> findP4RuntimeDigest(const p4::config::v1::P4Info &p4Info,
                                                          p4rt_id_t id) {
    for (const auto &digest : p4Info.digests()) {
        if (digest.preamble().id() == id) {
            return digest;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Extern> findP4RuntimeExtern(const p4::config::v1::P4Info &p4Info,
                                                          cstring controlPlaneName) {
    for (const auto &p4Extern : p4Info.externs()) {
        if (p4Extern.extern_type_name() == controlPlaneName) {
            return p4Extern;
        }
    }
    return std::nullopt;
}

std::optional<p4::config::v1::Extern> findP4RuntimeExtern(const p4::config::v1::P4Info &p4Info,
                                                          p4rt_id_t id) {
    for (const auto &p4Extern : p4Info.externs()) {
        if (p4Extern.extern_type_id() == id) {
            return p4Extern;
        }
    }
    return std::nullopt;
}

std::optional<p4rt_id_t> getP4RuntimeId(const p4::config::v1::P4Info &p4Info,
                                        const P4RuntimeSymbolType &type, cstring controlPlaneName) {
    if (type == Standard::SymbolType::P4RT_TABLE()) {
        auto findP4RuntimeTableOpt = findP4RuntimeTable(p4Info, controlPlaneName);
        if (!findP4RuntimeTableOpt.has_value()) {
            error("Unable to find P4Runtime Object for table %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeTable = findP4RuntimeTableOpt.value();
        return p4RuntimeTable.preamble().id();
    }
    if (type == Standard::SymbolType::P4RT_ACTION()) {
        auto findP4RuntimeActionOpt = findP4RuntimeAction(p4Info, controlPlaneName);
        if (!findP4RuntimeActionOpt.has_value()) {
            error("Unable to find P4Runtime Object for action %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeAction = findP4RuntimeActionOpt.value();
        return p4RuntimeAction.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_ACTION_PROFILE()) {
        auto findP4RuntimeActionProfileOpt = findP4RuntimeActionProfile(p4Info, controlPlaneName);
        if (!findP4RuntimeActionProfileOpt.has_value()) {
            error("Unable to find P4Runtime Object for action profile %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeActionProfile = findP4RuntimeActionProfileOpt.value();
        return p4RuntimeActionProfile.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_COUNTER()) {
        auto findP4RuntimeCounterOpt = findP4RuntimeCounter(p4Info, controlPlaneName);
        if (!findP4RuntimeCounterOpt.has_value()) {
            error("Unable to find P4Runtime Object for counter %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeCounter = findP4RuntimeCounterOpt.value();
        return p4RuntimeCounter.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_DIRECT_COUNTER()) {
        auto findP4RuntimeDirectCounterOpt = findP4RuntimeDirectCounter(p4Info, controlPlaneName);
        if (!findP4RuntimeDirectCounterOpt.has_value()) {
            error("Unable to find P4Runtime Object for direct counter %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeDirectCounter = findP4RuntimeDirectCounterOpt.value();
        return p4RuntimeDirectCounter.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_METER()) {
        auto findP4RuntimeMeterOpt = findP4RuntimeMeter(p4Info, controlPlaneName);
        if (!findP4RuntimeMeterOpt.has_value()) {
            error("Unable to find P4Runtime Object for meter %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeMeter = findP4RuntimeMeterOpt.value();
        return p4RuntimeMeter.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_DIRECT_METER()) {
        auto findP4RuntimeDirectMeterOpt = findP4RuntimeDirectMeter(p4Info, controlPlaneName);
        if (!findP4RuntimeDirectMeterOpt.has_value()) {
            error("Unable to find P4Runtime Object for direct meter %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeDirectMeter = findP4RuntimeDirectMeterOpt.value();
        return p4RuntimeDirectMeter.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_CONTROLLER_HEADER()) {
        auto findP4RuntimeControllerMetadataOpt =
            findP4RuntimeControllerMetadata(p4Info, controlPlaneName);
        if (!findP4RuntimeControllerMetadataOpt.has_value()) {
            error("Unable to find P4Runtime Object for controller metadata %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeControllerMetadata = findP4RuntimeControllerMetadataOpt.value();
        return p4RuntimeControllerMetadata.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_VALUE_SET()) {
        auto findP4RuntimeValueSetOpt = findP4RuntimeValueSet(p4Info, controlPlaneName);
        if (!findP4RuntimeValueSetOpt.has_value()) {
            error("Unable to find P4Runtime Object for value set %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeValueSet = findP4RuntimeValueSetOpt.value();
        return p4RuntimeValueSet.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_REGISTER()) {
        auto findP4RuntimeRegisterOpt = findP4RuntimeRegister(p4Info, controlPlaneName);
        if (!findP4RuntimeRegisterOpt.has_value()) {
            error("Unable to find P4Runtime Object for register %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeRegister = findP4RuntimeRegisterOpt.value();
        return p4RuntimeRegister.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_DIGEST()) {
        auto findP4RuntimeDigestOpt = findP4RuntimeDigest(p4Info, controlPlaneName);
        if (!findP4RuntimeDigestOpt.has_value()) {
            error("Unable to find P4Runtime Object for digest %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeDigest = findP4RuntimeDigestOpt.value();
        return p4RuntimeDigest.preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_OTHER_EXTERNS_START()) {
        auto findP4RuntimeExternOpt = findP4RuntimeExtern(p4Info, controlPlaneName);
        if (!findP4RuntimeExternOpt.has_value()) {
            error("Unable to find P4Runtime Object for extern %1%", controlPlaneName);
            return std::nullopt;
        }
        auto p4RuntimeExtern = findP4RuntimeExternOpt.value();
        return p4RuntimeExtern.extern_type_id();
    }

    ::error("Unsupported P4Runtime ID type");

    return std::nullopt;
}

}  // namespace P4::ControlPlaneAPI
