#include "backends/p4tools/modules/testgen/lib/packet_vars.h"

#include "ir/id.h"

namespace P4Tools::P4Testgen {

// The methods in P4's packet_in uses 32-bit values. Conform with this to make it easier to produce
// well-typed expressions when manipulating the parser cursor.
const IR::Type_Bits PacketVars::PACKET_SIZE_VAR_TYPE = IR::Type_Bits(32, false);

const IR::Member PacketVars::INPUT_PACKET_LABEL =
    IR::Member(new IR::PathExpression("*"), "inputPacket");

const IR::Member PacketVars::PACKET_BUFFER_LABEL =
    IR::Member(new IR::PathExpression("*"), "packetBuffer");

const IR::Member PacketVars::EMIT_BUFFER_LABEL =
    IR::Member(new IR::PathExpression("*"), "emitBuffer");

const IR::SymbolicVariable PacketVars::PAYLOAD_SYMBOL =
    IR::SymbolicVariable(&PacketVars::PACKET_SIZE_VAR_TYPE, "*payload");

}  // namespace P4Tools::P4Testgen
