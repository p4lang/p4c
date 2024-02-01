#ifndef CONTROL_PLANE_P4INFOAPI_H_
#define CONTROL_PLANE_P4INFOAPI_H_

#include <optional>

#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "lib/cstring.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/config/v1/p4info.pb.h"
#pragma GCC diagnostic pop

namespace P4::ControlPlaneAPI {

/// Generic meta function which searches an object by @name in the given range
/// and @returns the P4Runtime representation, or std::nullopt if none is found.
/// TODO: Should this return an optional const reference? The advantage is that the value needs to
/// be explicitly unpacked.
template <typename It>
auto findP4InfoObject(const It &first, const It &last, cstring controlPlaneName) -> const
    typename std::iterator_traits<It>::value_type * {
    using T = typename std::iterator_traits<It>::value_type;
    auto desiredObject = std::find_if(
        first, last, [&](const T &object) { return object.preamble().name() == controlPlaneName; });
    if (desiredObject == last) {
        return nullptr;
    }
    return &*desiredObject;
}

/// Generic meta function which searches an object by @id in the given range
/// and @returns the P4Runtime representation, or std::nullopt if none is found.
/// TODO: Should this return an optional const reference? The advantage is that the value needs to
/// be explicitly unpacked.
template <typename It>
auto findP4InfoObject(const It &first, const It &last, p4rt_id_t id) -> const
    typename std::iterator_traits<It>::value_type * {
    using T = typename std::iterator_traits<It>::value_type;
    auto desiredObject =
        std::find_if(first, last, [&](const T &object) { return object.preamble().id() == id; });
    if (desiredObject == last) {
        return nullptr;
    }
    return &*desiredObject;
}

/// Try to find the P4Info description for the given table. @return std::nullopt if the table is not
/// is not present in the P4Info object.
const p4::config::v1::Table *findP4RuntimeTable(const p4::config::v1::P4Info &p4Info,
                                                cstring controlPlaneName);

/// Try to find the P4Info description for the given table. @return std::nullopt if the table is not
/// is not present in the P4Info object.
const p4::config::v1::Table *findP4RuntimeTable(const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given match field in a particular P4Info table.
/// @return std::nullopt if the match field is not present in the table.
const p4::config::v1::MatchField *findP4RuntimeMatchField(const p4::config::v1::Table &p4Table,
                                                          cstring controlPlaneName);

/// Try to find the P4Info description for the given match field in a particular P4Info table.
/// @return std::nullopt if the match field is not present in the table.
const p4::config::v1::MatchField *findP4RuntimeMatchField(const p4::config::v1::Table &p4Table,
                                                          p4rt_id_t id);

/// Try to find the P4Info description for the given action. @return std::nullopt if the action
/// is not present in the P4Info object.
const p4::config::v1::Action *findP4RuntimeAction(const p4::config::v1::P4Info &p4Info,
                                                  cstring controlPlaneName);

/// Try to find the P4Info description for the given action. @return std::nullopt if the action
/// is not present in the P4Info object.
const p4::config::v1::Action *findP4RuntimeAction(const p4::config::v1::P4Info &p4Info,
                                                  p4rt_id_t id);

/// Try to find the P4Info description for the given action profile. @return std::nullopt if the
/// action is not present in the P4Info object.
const p4::config::v1::ActionProfile *findP4RuntimeActionProfile(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName);

/// Try to find the P4Info description for the given action profile. @return std::nullopt if the
/// action is not present in the P4Info object.
const p4::config::v1::ActionProfile *findP4RuntimeActionProfile(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given counter. @return std::nullopt if the counter is
/// not present in the P4Info object.
const p4::config::v1::Counter *findP4RuntimeCounter(const p4::config::v1::P4Info &p4Info,
                                                    cstring controlPlaneName);

/// Try to find the P4Info description for the given counter. @return std::nullopt if the counter is
/// not present in the P4Info object.
const p4::config::v1::Counter *findP4RuntimeCounter(const p4::config::v1::P4Info &p4Info,
                                                    p4rt_id_t id);

/// Try to find the P4Info description for the given direct counter. @return std::nullopt if the
/// direct counter is  not present in the P4Info object.
const p4::config::v1::DirectCounter *findP4RuntimeDirectCounter(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName);

/// Try to find the P4Info description for the given direct counter. @return std::nullopt if the
/// direct counter is not present in the P4Info object.
const p4::config::v1::DirectCounter *findP4RuntimeDirectCounter(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given meter. @return std::nullopt if the meter is not
/// present in the P4Info object.
const p4::config::v1::Meter *findP4RuntimeMeter(const p4::config::v1::P4Info &p4Info,
                                                cstring controlPlaneName);

/// Try to find the P4Info description for the given meter. @return std::nullopt if the meter is not
/// present in the P4Info object.
const p4::config::v1::Meter *findP4RuntimeMeter(const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given direct meter. @return std::nullopt if the
/// direct meter is not present in the P4Info object.
const p4::config::v1::DirectMeter *findP4RuntimeDirectMeter(const p4::config::v1::P4Info &p4Info,
                                                            cstring controlPlaneName);

/// Try to find the P4Info description for the given direct meter. @return std::nullopt if the
/// direct meter is not present in the P4Info object.
const p4::config::v1::DirectMeter *findP4RuntimeDirectMeter(const p4::config::v1::P4Info &p4Info,
                                                            p4rt_id_t id);

/// Try to find the P4Info description for the given controller packet metadata. @return
/// std::nullopt if the controller packet metadata is not present in the P4Info object.
const p4::config::v1::ControllerPacketMetadata *findP4RuntimeControllerPacketMetadata(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName);

/// Try to find the P4Info description for the given controller packet metadata. @return
/// std::nullopt if the controller packet metadata is not present in the P4Info object.
const p4::config::v1::ControllerPacketMetadata *findP4RuntimeControllerPacketMetadata(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given value set. @return std::nullopt if the value
/// set is not present in the P4Info object.
const p4::config::v1::ValueSet *findP4RuntimeValueSet(const p4::config::v1::P4Info &p4Info,
                                                      cstring controlPlaneName);

/// Try to find the P4Info description for the given value set. @return std::nullopt if the value
/// set is not present in the P4Info object.
const p4::config::v1::ValueSet *findP4RuntimeValueSet(const p4::config::v1::P4Info &p4Info,
                                                      p4rt_id_t id);

/// Try to find the P4Info description for the given register. @return std::nullopt if the register
/// is not present in the P4Info object.
const p4::config::v1::Register *findP4RuntimeRegister(const p4::config::v1::P4Info &p4Info,
                                                      cstring controlPlaneName);

/// Try to find the P4Info description for the given register. @return std::nullopt if the register
/// is not present in the P4Info object.
const p4::config::v1::Register *findP4RuntimeRegister(const p4::config::v1::P4Info &p4Info,
                                                      p4rt_id_t id);

/// Try to find the P4Info description for the given digest. @return std::nullopt if the digest is
/// not present in the P4Info object.
const p4::config::v1::Digest *findP4RuntimeDigest(const p4::config::v1::P4Info &p4Info,
                                                  cstring controlPlaneName);

/// Try to find the P4Info description for the given digest. @return std::nullopt if the digest is
/// not present in the P4Info object.
const p4::config::v1::Digest *findP4RuntimeDigest(const p4::config::v1::P4Info &p4Info,
                                                  p4rt_id_t id);

/// Try to find the P4Info description for the given extern. @return std::nullopt if the extern is
/// not present in the P4Info object.
const p4::config::v1::Extern *findP4RuntimeExtern(const p4::config::v1::P4Info &p4Info,
                                                  cstring controlPlaneName);

/// Try to find the P4Info description for the given extern. @return std::nullopt if the extern is
/// not present in the P4Info object.
const p4::config::v1::Extern *findP4RuntimeExtern(const p4::config::v1::P4Info &p4Info,
                                                  p4rt_id_t id);

/// Looks up the P4Runtime id for a P4Runtime symbol. Returns std::nullopt if the object or the
/// type does not exist.
std::optional<p4rt_id_t> getP4RuntimeId(const p4::config::v1::P4Info &p4Info,
                                        const P4RuntimeSymbolType &type, cstring controlPlaneName);

}  // namespace P4::ControlPlaneAPI

#endif /* CONTROL_PLANE_P4INFOAPI_H_ */
