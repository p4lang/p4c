#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_PACKET_VARS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_PACKET_VARS_H_

#include "ir/ir.h"

namespace P4Tools::P4Testgen {

/// Contains utility functions and variables related to the creation and manipulation of input
/// packets.
class PacketVars {
 public:
    /// Specifies the type of the packet size variable.
    /// This is mostly used to generate bit vector constants.
    static const IR::Type_Bits PACKET_SIZE_VAR_TYPE;

    /// The name of the input packet. The input packet defines the minimum size of the packet
    /// requires to pass this particular path. Typically, calls such as extract, advance, or
    /// lookahead cause the input packet to grow.
    static const IR::Member INPUT_PACKET_LABEL;

    /// The name of packet buffer. The packet buffer defines the data that can be consumed by the
    /// parser. If the packet buffer is empty, extract/advance/lookahead calls will cause the
    /// minimum packet size to grow. The packet buffer also forms the final output packet.
    static const IR::Member PACKET_BUFFER_LABEL;

    /// The name of the emit buffer. Each time, emit is called, the emitted content is appended to
    /// this buffer. Typically, after exiting the control, the emit buffer is appended to the packet
    /// buffer.
    static const IR::Member EMIT_BUFFER_LABEL;

    /// Canonical name for the payload. This is used for consistent naming when attaching a payload
    /// to the packet.
    static const IR::SymbolicVariable PAYLOAD_SYMBOL;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_PACKET_VARS_H_ */
