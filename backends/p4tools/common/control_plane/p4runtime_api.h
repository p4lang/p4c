#ifndef BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_P4RUNTIME_API_H_
#define BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_P4RUNTIME_API_H_
#include <map>
#include <optional>

#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "lib/cstring.h"
#include "p4/config/v1/p4info.pb.h"

/// TODO: Consider migrating this API to the top-level control-plane folder.
/// The reason we have not already done this is because that folder already provides similar utility
/// functions. However, these functions are tied to the P4RuntimeTableIface, which is fairly
/// inflexible. We just need an API that can perform lookup operations on a P4Info or P4RuntimeAPI
/// object.
/// TODO: Also consider how to make this API target-specific. Not all these functions are available
/// in all targets.
namespace P4::ControlPlaneAPI {

/// Computes a unique pairing of two input numbers. We use this to generate unique P4Runtime IDs
/// for combinations of tables and key elements, or actions and parameters.
/// https://en.wikipedia.org/wiki/Pairing_function#Other_pairing_functions
p4rt_id_t szudzikPairing(p4rt_id_t x, p4rt_id_t y);

class P4RuntimeMaps {
    /// Type definitions for convenience.
    using P4RuntimeIdToControlPlaneNameMap = std::map<p4rt_id_t, cstring>;
    using ControlPlaneNameToP4RuntimeIdMap = std::map<cstring, p4rt_id_t>;

 private:
    /// Maps P4Runtime IDs to control plane names.
    P4RuntimeIdToControlPlaneNameMap idToNameMap;

    /// Maps control plane names to P4Runtime IDs.
    ControlPlaneNameToP4RuntimeIdMap nameToIdMap;

    /// Iterate over the P4Info object and build a mapping from P4 control plane names to their ids.
    virtual void buildP4RuntimeMaps(const p4::config::v1::P4Info &p4Info);

 public:
    explicit P4RuntimeMaps(const p4::config::v1::P4Info &p4Info);
    P4RuntimeMaps(const P4RuntimeMaps &) = default;
    P4RuntimeMaps(P4RuntimeMaps &&) = default;
    P4RuntimeMaps &operator=(const P4RuntimeMaps &) = default;
    P4RuntimeMaps &operator=(P4RuntimeMaps &&) = default;
    virtual ~P4RuntimeMaps() = default;

    /// Looks up the P4Runtime id for the given control plane name in the pre-computed P4Runtime-ID
    /// map. @returns std::nullopt if the name is not in the map.
    [[nodiscard]] std::optional<p4rt_id_t> lookupP4RuntimeId(cstring controlPlaneName) const;

    /// Looks up the control plane name for the given P4Runtime id in the pre-computed P4Runtime-ID
    /// map. @returns std::nullopt if the id is not in the map.
    [[nodiscard]] std::optional<cstring> lookupControlPlaneName(p4rt_id_t id) const;
};

/// Try to find the P4Info description for the given table. @return std::nullopt if the table is not
/// is not present in the P4Info object.
std::optional<p4::config::v1::Table> findP4RuntimeTable(const p4::config::v1::P4Info &p4Info,
                                                        cstring controlPlaneName);

/// Try to find the P4Info description for the given table. @return std::nullopt if the table is not
/// is not present in the P4Info object.
std::optional<p4::config::v1::Table> findP4RuntimeTable(const p4::config::v1::P4Info &p4Info,
                                                        p4rt_id_t id);

/// Try to find the P4Info description for the given match field in a particular P4Info table.
/// @return std::nullopt if the match field is not present in the table.
std::optional<p4::config::v1::MatchField> findP4RuntimeMatchField(
    const p4::config::v1::Table &p4Table, cstring controlPlaneName);

/// Try to find the P4Info description for the given match field in a particular P4Info table.
/// @return std::nullopt if the match field is not present in the table.
std::optional<p4::config::v1::MatchField> findP4RuntimeMatchField(
    const p4::config::v1::Table &p4Table, p4rt_id_t id);

/// Try to find the P4Info description for the given action. @return std::nullopt if the action
/// is not present in the P4Info object.
std::optional<p4::config::v1::Action> findP4RuntimeAction(const p4::config::v1::P4Info &p4Info,
                                                          cstring controlPlaneName);

/// Try to find the P4Info description for the given action. @return std::nullopt if the action
/// is not present in the P4Info object.
std::optional<p4::config::v1::Action> findP4RuntimeAction(const p4::config::v1::P4Info &p4Info,
                                                          p4rt_id_t id);

/// Try to find the P4Info description for the given action profile. @return std::nullopt if the
/// action is not present in the P4Info object.
std::optional<p4::config::v1::ActionProfile> findP4RuntimeActionProfile(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName);

/// Try to find the P4Info description for the given action profile. @return std::nullopt if the
/// action is not present in the P4Info object.
std::optional<p4::config::v1::ActionProfile> findP4RuntimeActionProfile(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given counter. @return std::nullopt if the counter is
/// not present in the P4Info object.
std::optional<p4::config::v1::Counter> findP4RuntimeCounter(const p4::config::v1::P4Info &p4Info,
                                                            cstring controlPlaneName);

/// Try to find the P4Info description for the given counter. @return std::nullopt if the counter is
/// not present in the P4Info object.
std::optional<p4::config::v1::Counter> findP4RuntimeCounter(const p4::config::v1::P4Info &p4Info,
                                                            p4rt_id_t id);

/// Try to find the P4Info description for the given direct counter. @return std::nullopt if the
/// direct counter is  not present in the P4Info object.
std::optional<p4::config::v1::DirectCounter> findP4RuntimeDirectCounter(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName);

/// Try to find the P4Info description for the given direct counter. @return std::nullopt if the
/// direct counter is not present in the P4Info object.
std::optional<p4::config::v1::DirectCounter> findP4RuntimeDirectCounter(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given meter. @return std::nullopt if the meter is not
/// present in the P4Info object.
std::optional<p4::config::v1::Meter> findP4RuntimeMeter(const p4::config::v1::P4Info &p4Info,
                                                        cstring controlPlaneName);

/// Try to find the P4Info description for the given meter. @return std::nullopt if the meter is not
/// present in the P4Info object.
std::optional<p4::config::v1::Meter> findP4RuntimeMeter(const p4::config::v1::P4Info &p4Info,
                                                        p4rt_id_t id);

/// Try to find the P4Info description for the given direct meter. @return std::nullopt if the
/// direct meter is not present in the P4Info object.
std::optional<p4::config::v1::DirectMeter> findP4RuntimeDirectMeter(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName);

/// Try to find the P4Info description for the given direct meter. @return std::nullopt if the
/// direct meter is not present in the P4Info object.
std::optional<p4::config::v1::DirectMeter> findP4RuntimeDirectMeter(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given controller packet metadata. @return
/// std::nullopt if the controller packet metadata is not present in the P4Info object.
std::optional<p4::config::v1::ControllerPacketMetadata> findP4RuntimeControllerPacketMetadata(
    const p4::config::v1::P4Info &p4Info, cstring controlPlaneName);

/// Try to find the P4Info description for the given controller packet metadata. @return
/// std::nullopt if the controller packet metadata is not present in the P4Info object.
std::optional<p4::config::v1::ControllerPacketMetadata> findP4RuntimeControllerPacketMetadata(
    const p4::config::v1::P4Info &p4Info, p4rt_id_t id);

/// Try to find the P4Info description for the given value set. @return std::nullopt if the value
/// set is not present in the P4Info object.
std::optional<p4::config::v1::ValueSet> findP4RuntimeValueSet(const p4::config::v1::P4Info &p4Info,
                                                              cstring controlPlaneName);

/// Try to find the P4Info description for the given value set. @return std::nullopt if the value
/// set is not present in the P4Info object.
std::optional<p4::config::v1::ValueSet> findP4RuntimeValueSet(const p4::config::v1::P4Info &p4Info,
                                                              p4rt_id_t id);

/// Try to find the P4Info description for the given register. @return std::nullopt if the register
/// is not present in the P4Info object.
std::optional<p4::config::v1::Register> findP4RuntimeRegister(const p4::config::v1::P4Info &p4Info,
                                                              cstring controlPlaneName);

/// Try to find the P4Info description for the given register. @return std::nullopt if the register
/// is not present in the P4Info object.
std::optional<p4::config::v1::Register> findP4RuntimeRegister(const p4::config::v1::P4Info &p4Info,
                                                              p4rt_id_t id);

/// Try to find the P4Info description for the given digest. @return std::nullopt if the digest is
/// not present in the P4Info object.
std::optional<p4::config::v1::Digest> findP4RuntimeDigest(const p4::config::v1::P4Info &p4Info,
                                                          cstring controlPlaneName);

/// Try to find the P4Info description for the given digest. @return std::nullopt if the digest is
/// not present in the P4Info object.
std::optional<p4::config::v1::Digest> findP4RuntimeDigest(const p4::config::v1::P4Info &p4Info,
                                                          p4rt_id_t id);

/// Try to find the P4Info description for the given extern. @return std::nullopt if the extern is
/// not present in the P4Info object.
std::optional<p4::config::v1::Extern> findP4RuntimeExtern(const p4::config::v1::P4Info &p4Info,
                                                          cstring controlPlaneName);

/// Try to find the P4Info description for the given extern. @return std::nullopt if the extern is
/// not present in the P4Info object.
std::optional<p4::config::v1::Extern> findP4RuntimeExtern(const p4::config::v1::P4Info &p4Info,
                                                          p4rt_id_t id);

/// Looks up the P4Runtime id for a P4Runtime symbol. Returns std::nullopt if the object or the
/// type does not exist.
std::optional<p4rt_id_t> getP4RuntimeId(const p4::config::v1::P4Info &p4Info,
                                        const P4RuntimeSymbolType &type, cstring controlPlaneName);

}  // namespace P4::ControlPlaneAPI

#endif /* BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_P4RUNTIME_API_H_ */
