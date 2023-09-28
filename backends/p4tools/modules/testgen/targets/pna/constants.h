#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_CONSTANTS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_CONSTANTS_H_

#include "ir/ir.h"

namespace P4Tools::P4Testgen::Pna {

enum PacketDirection { NET_TO_HOST, HOST_TO_NET };

class PnaConstants {
 public:
    /// Match bits exactly or not at all.
    static constexpr const char *MATCH_KIND_OPT = "optional";
    /// A match that is used as an argument for the selector.
    static constexpr const char *MATCH_KIND_SELECTOR = "selector";
    /// Entries that can match a range.
    static constexpr const char *MATCH_KIND_RANGE = "range";

    /// PNA-internal drop variable.
    static const IR::Member DROP_VAR;
    /// PNA-internal egress port variable.
    static const IR::Member OUTPUT_PORT_VAR;
    /// PNA-internal parser error label.
    static const IR::Member PARSER_ERROR;
    /// Port width in bits.
    static constexpr int PORT_BIT_WIDTH = 32;
};

/// variabless are variables that can be controlled and set by P4Testgen.
class PnaSymbolicVars {
 public:
    /// The input direction.
    static const IR::SymbolicVariable DIRECTION;
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_CONSTANTS_H_ */
