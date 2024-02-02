#ifndef BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_P4INFO_MAP_H_
#define BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_P4INFO_MAP_H_
#include <cstdint>
#include <map>
#include <optional>

#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "lib/cstring.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/config/v1/p4info.pb.h"
#pragma GCC diagnostic pop

/// TODO: Consider migrating this API to the top-level control-plane folder.
/// The reason we have not already done this is because that folder already provides similar utility
/// functions. However, these functions are tied to the P4RuntimeTableIface, which is fairly
/// inflexible. We just need an API that can perform lookup operations on a P4Info or P4RuntimeAPI
/// object.
namespace P4::ControlPlaneAPI {

/// Computes a unique pairing of two input numbers. We use this to generate unique P4Runtime IDs
/// for combinations of tables and key elements, or actions and parameters.
/// https://en.wikipedia.org/wiki/Pairing_function#Other_pairing_functions
/// The maximum szudzikPairing value is 2^64 - 1, which is why we use uint64_t for the p4rt_id_t,
/// which is uint32_t bit.
uint64_t szudzikPairing(p4rt_id_t x, p4rt_id_t y);

/// This object maps P4 control plane names to their P4Runtime IDs and vice versa.
/// It uses the P4Info object to populate the maps.
/// Since ids for action parameters and table keys are not unique, we use a pairing function to
/// compute a unique identifier. This pairing function uses the id of the parent object (e.g., a
/// table or action) and combines it with the id of the parameter or key element to create a unique
/// identifier.
class P4InfoMaps {
    /// Type definitions for convenience.
    using P4RuntimeIdToControlPlaneNameMap = std::map<uint64_t, cstring>;
    using ControlPlaneNameToP4RuntimeIdMap = std::map<cstring, uint64_t>;

 protected:
    /// Maps P4Runtime IDs to control plane names.
    P4RuntimeIdToControlPlaneNameMap idToNameMap;

    /// Maps control plane names to P4Runtime IDs.
    ControlPlaneNameToP4RuntimeIdMap nameToIdMap;

    /// Iterate over the P4Info object and build a mapping from P4 control plane names to their ids.
    virtual void buildP4InfoMaps(const p4::config::v1::P4Info &p4Info);

 public:
    explicit P4InfoMaps(const p4::config::v1::P4Info &p4Info);

    /// Looks up the P4Runtime id for the given control plane name in the pre-computed P4Runtime-ID
    /// map. @returns std::nullopt if the name is not in the map.
    [[nodiscard]] std::optional<uint64_t> lookUpP4RuntimeId(cstring controlPlaneName) const;

    /// Looks up the control plane name for the given P4Runtime id in the pre-computed P4Runtime-ID
    /// map. @returns std::nullopt if the id is not in the map.
    [[nodiscard]] std::optional<cstring> lookUpControlPlaneName(uint64_t id) const;
};

}  // namespace P4::ControlPlaneAPI

#endif /* BACKENDS_P4TOOLS_COMMON_CONTROL_PLANE_P4INFO_MAP_H_ */
