#include "control-plane/p4infoApi.h"

#include <functional>

#include "control-plane/p4RuntimeArchStandard.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/config/v1/p4info.pb.h"
#pragma GCC diagnostic pop

namespace P4::ControlPlaneAPI {

const p4::config::v1::Table *findP4RuntimeTable(const p4::config::v1::P4Info &p4Info,
                                                cstring controlPlaneName) {
    return findP4InfoObject(p4Info.tables().begin(), p4Info.tables().end(), controlPlaneName);
}

const p4::config::v1::Table *findP4RuntimeTable(const p4::config::v1::P4Info &p4Info,
                                                p4rt_id_t id) {
    return findP4InfoObject(p4Info.tables().begin(), p4Info.tables().end(), id);
}

const p4::config::v1::MatchField *findP4RuntimeMatchField(const p4::config::v1::Table &p4Table,
                                                          cstring controlPlaneName) {
    for (const auto &p4InfoMatchField : p4Table.match_fields()) {
        if (controlPlaneName == p4InfoMatchField.name()) {
            return &p4InfoMatchField;
        }
    }
    return nullptr;
}

const p4::config::v1::MatchField *findP4RuntimeMatchField(const p4::config::v1::Table &p4Table,
                                                          p4rt_id_t id) {
    for (const auto &p4InfoMatchField : p4Table.match_fields()) {
        if (id == p4InfoMatchField.id()) {
            return &p4InfoMatchField;
        }
    }
    return nullptr;
}

const p4::config::v1::Action *findP4RuntimeAction(const p4::config::v1::P4Info &p4Info,
                                                  cstring controlPlaneName) {
    return findP4InfoObject(p4Info.actions().begin(), p4Info.actions().end(), controlPlaneName);
}

const p4::config::v1::Action *findP4RuntimeAction(const p4::config::v1::P4Info &p4Info,
                                                  p4rt_id_t id) {
    return findP4InfoObject(p4Info.actions().begin(), p4Info.actions().end(), id);
}

const p4::config::v1::ActionProfile *findP4RuntimeActionProfile(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName) {
    return findP4InfoObject(p4Info.action_profiles().begin(), p4Info.action_profiles().end(),
                            controlPlaneName);
}

const p4::config::v1::ActionProfile *findP4RuntimeActionProfile(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id) {
    return findP4InfoObject(p4Info.action_profiles().begin(), p4Info.action_profiles().end(), id);
}

const p4::config::v1::Counter *findP4RuntimeCounter(const p4::config::v1::P4Info &p4Info,
                                                    cstring controlPlaneName) {
    return findP4InfoObject(p4Info.counters().begin(), p4Info.counters().end(), controlPlaneName);
}

const p4::config::v1::Counter *findP4RuntimeCounter(const p4::config::v1::P4Info &p4Info,
                                                    p4rt_id_t id) {
    return findP4InfoObject(p4Info.counters().begin(), p4Info.counters().end(), id);
}

const p4::config::v1::DirectCounter *findP4RuntimeDirectCounter(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName) {
    return findP4InfoObject(p4Info.direct_counters().begin(), p4Info.direct_counters().end(),
                            controlPlaneName);
}

const p4::config::v1::DirectCounter *findP4RuntimeDirectCounter(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id) {
    return findP4InfoObject(p4Info.direct_counters().begin(), p4Info.direct_counters().end(), id);
}

const p4::config::v1::Meter *findP4RuntimeMeter(const p4::config::v1::P4Info &p4Info,
                                                cstring controlPlaneName) {
    return findP4InfoObject(p4Info.meters().begin(), p4Info.meters().end(), controlPlaneName);
}

const p4::config::v1::Meter *findP4RuntimeMeter(const p4::config::v1::P4Info &p4Info,
                                                p4rt_id_t id) {
    return findP4InfoObject(p4Info.meters().begin(), p4Info.meters().end(), id);
}

const p4::config::v1::DirectMeter *findP4RuntimeDirectMeter(const p4::config::v1::P4Info &p4Info,
                                                            cstring controlPlaneName) {
    return findP4InfoObject(p4Info.direct_meters().begin(), p4Info.direct_meters().end(),
                            controlPlaneName);
}

const p4::config::v1::DirectMeter *findP4RuntimeDirectMeter(const p4::config::v1::P4Info &p4Info,
                                                            p4rt_id_t id) {
    return findP4InfoObject(p4Info.direct_meters().begin(), p4Info.direct_meters().end(), id);
}

const p4::config::v1::ControllerPacketMetadata *findP4RuntimeControllerPacketMetadata(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName) {
    return findP4InfoObject(p4Info.controller_packet_metadata().begin(),
                            p4Info.controller_packet_metadata().end(), controlPlaneName);
}

const p4::config::v1::ControllerPacketMetadata *findP4RuntimeControllerPacketMetadata(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id) {
    return findP4InfoObject(p4Info.controller_packet_metadata().begin(),
                            p4Info.controller_packet_metadata().end(), id);
}

const p4::config::v1::ValueSet *findP4RuntimeValueSet(const p4::config::v1::P4Info &p4Info,
                                                      cstring controlPlaneName) {
    return findP4InfoObject(p4Info.value_sets().begin(), p4Info.value_sets().end(),
                            controlPlaneName);
}

const p4::config::v1::ValueSet *findP4RuntimeValueSet(const p4::config::v1::P4Info &p4Info,
                                                      p4rt_id_t id) {
    return findP4InfoObject(p4Info.value_sets().begin(), p4Info.value_sets().end(), id);
}

const p4::config::v1::Register *findP4RuntimeRegister(const p4::config::v1::P4Info &p4Info,
                                                      cstring controlPlaneName) {
    return findP4InfoObject(p4Info.registers().begin(), p4Info.registers().end(), controlPlaneName);
}

const p4::config::v1::Register *findP4RuntimeRegister(const p4::config::v1::P4Info &p4Info,
                                                      p4rt_id_t id) {
    return findP4InfoObject(p4Info.registers().begin(), p4Info.registers().end(), id);
}

const p4::config::v1::Digest *findP4RuntimeDigest(const p4::config::v1::P4Info &p4Info,
                                                  cstring controlPlaneName) {
    return findP4InfoObject(p4Info.digests().begin(), p4Info.digests().end(), controlPlaneName);
}

const p4::config::v1::Digest *findP4RuntimeDigest(const p4::config::v1::P4Info &p4Info,
                                                  p4rt_id_t id) {
    return findP4InfoObject(p4Info.digests().begin(), p4Info.digests().end(), id);
}

const p4::config::v1::Extern *findP4RuntimeExtern(const p4::config::v1::P4Info &p4Info,
                                                  cstring controlPlaneName) {
    for (const auto &p4Extern : p4Info.externs()) {
        if (p4Extern.extern_type_name() == controlPlaneName) {
            return &p4Extern;
        }
    }
    return nullptr;
}

const p4::config::v1::Extern *findP4RuntimeExtern(const p4::config::v1::P4Info &p4Info,
                                                  p4rt_id_t id) {
    for (const auto &p4Extern : p4Info.externs()) {
        if (p4Extern.extern_type_id() == id) {
            return &p4Extern;
        }
    }
    return nullptr;
}

std::optional<p4rt_id_t> getP4RuntimeId(const p4::config::v1::P4Info &p4Info,
                                        const P4RuntimeSymbolType &type, cstring controlPlaneName) {
    if (type == Standard::SymbolType::P4RT_TABLE()) {
        auto p4RuntimeTableOpt = findP4RuntimeTable(p4Info, controlPlaneName);
        if (p4RuntimeTableOpt == nullptr) {
            error("Unable to find P4Runtime Object for table %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeTableOpt->preamble().id();
    }
    if (type == Standard::SymbolType::P4RT_ACTION()) {
        auto p4RuntimeActionOpt = findP4RuntimeAction(p4Info, controlPlaneName);
        if (p4RuntimeActionOpt == nullptr) {
            error("Unable to find P4Runtime Object for action %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeActionOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_ACTION_PROFILE()) {
        auto p4RuntimeActionProfileOpt = findP4RuntimeActionProfile(p4Info, controlPlaneName);
        if (p4RuntimeActionProfileOpt == nullptr) {
            error("Unable to find P4Runtime Object for action profile %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeActionProfileOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_COUNTER()) {
        auto p4RuntimeCounterOpt = findP4RuntimeCounter(p4Info, controlPlaneName);
        if (p4RuntimeCounterOpt == nullptr) {
            error("Unable to find P4Runtime Object for counter %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeCounterOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_DIRECT_COUNTER()) {
        auto p4RuntimeDirectCounterOpt = findP4RuntimeDirectCounter(p4Info, controlPlaneName);
        if (p4RuntimeDirectCounterOpt == nullptr) {
            error("Unable to find P4Runtime Object for direct counter %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeDirectCounterOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_METER()) {
        auto p4RuntimeMeterOpt = findP4RuntimeMeter(p4Info, controlPlaneName);
        if (p4RuntimeMeterOpt == nullptr) {
            error("Unable to find P4Runtime Object for meter %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeMeterOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_DIRECT_METER()) {
        auto p4RuntimeDirectMeterOpt = findP4RuntimeDirectMeter(p4Info, controlPlaneName);
        if (p4RuntimeDirectMeterOpt == nullptr) {
            error("Unable to find P4Runtime Object for direct meter %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeDirectMeterOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_CONTROLLER_HEADER()) {
        auto p4RuntimeControllerMetadataOpt =
            findP4RuntimeControllerPacketMetadata(p4Info, controlPlaneName);
        if (p4RuntimeControllerMetadataOpt == nullptr) {
            error("Unable to find P4Runtime Object for controller metadata %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeControllerMetadataOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_VALUE_SET()) {
        auto p4RuntimeValueSetOpt = findP4RuntimeValueSet(p4Info, controlPlaneName);
        if (p4RuntimeValueSetOpt == nullptr) {
            error("Unable to find P4Runtime Object for value set %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeValueSetOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_REGISTER()) {
        auto p4RuntimeRegisterOpt = findP4RuntimeRegister(p4Info, controlPlaneName);
        if (p4RuntimeRegisterOpt == nullptr) {
            error("Unable to find P4Runtime Object for register %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeRegisterOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_DIGEST()) {
        auto p4RuntimeDigestOpt = findP4RuntimeDigest(p4Info, controlPlaneName);
        if (p4RuntimeDigestOpt == nullptr) {
            error("Unable to find P4Runtime Object for digest %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeDigestOpt->preamble().id();
    }

    if (type == Standard::SymbolType::P4RT_OTHER_EXTERNS_START()) {
        auto p4RuntimeExternOpt = findP4RuntimeExtern(p4Info, controlPlaneName);
        if (p4RuntimeExternOpt == nullptr) {
            error("Unable to find P4Runtime Object for extern %1%", controlPlaneName);
            return std::nullopt;
        }
        return p4RuntimeExternOpt->extern_type_id();
    }

    ::error("Unsupported P4Runtime ID type");

    return std::nullopt;
}

}  // namespace P4::ControlPlaneAPI
