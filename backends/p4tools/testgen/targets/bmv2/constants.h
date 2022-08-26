#ifndef TESTGEN_TARGETS_BMV2_CONSTANTS_H_
#define TESTGEN_TARGETS_BMV2_CONSTANTS_H_

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
};

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_CONSTANTS_H_ */
