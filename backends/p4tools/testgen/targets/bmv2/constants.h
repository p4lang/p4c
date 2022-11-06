#ifndef BACKENDS_P4TOOLS_TESTGEN_TARGETS_BMV2_CONSTANTS_H_
#define BACKENDS_P4TOOLS_TESTGEN_TARGETS_BMV2_CONSTANTS_H_

#include <cstdint>

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

enum bmv2_gress_t { BMV2_INGRESS, BMV2_EGRESS };

class BMv2Constants {
 public:
    /// Match bits exactly or not at all.
    static constexpr const char* MATCH_KIND_OPT = "optional";
    /// A match that is used as an argument for the selector.
    static constexpr const char* MATCH_KIND_SELECTOR = "selector";
    /// Entries that can match a range.
    static constexpr const char* MATCH_KIND_RANGE = "range";

    // These definitions are derived from the numerical values of the enum
    // named "PktInstanceType" in the p4lang/behavioral-model source file
    // targets/simple_switch/simple_switch.h
    // https://github.com/p4lang/behavioral-model/blob/main/targets/simple_switch/simple_switch.h#L146
    static constexpr uint64_t PKT_INSTANCE_TYPE_NORMAL = 0x0000;
    static constexpr uint64_t PKT_INSTANCE_TYPE_INGRESS_CLONE = 0x0001;
    static constexpr uint64_t PKT_INSTANCE_TYPE_EGRESS_CLONE = 0x0002;
    static constexpr uint64_t PKT_INSTANCE_TYPE_COALESCED = 0x0003;
    static constexpr uint64_t PKT_INSTANCE_TYPE_RECIRC = 0x0004;
    static constexpr uint64_t PKT_INSTANCE_TYPE_REPLICATION = 0x005;
    static constexpr uint64_t PKT_INSTANCE_TYPE_RESUBMIT = 0x006;
    // Clone type is derived from v1model.p4
    static constexpr int CLONE_TYPE_I2E = 0x0000;
    static constexpr int CLONE_TYPE_E2E = 0x0001;
};

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_TESTGEN_TARGETS_BMV2_CONSTANTS_H_ */
