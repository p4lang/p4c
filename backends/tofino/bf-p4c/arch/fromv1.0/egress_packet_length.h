/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_ARCH_FROMV1_0_EGRESS_PACKET_LENGTH_H_
#define BF_P4C_ARCH_FROMV1_0_EGRESS_PACKET_LENGTH_H_

#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"

// Issue:
//
// When packet is sent from ingress to egress, packet length presented to egress pipeline is
// the original packet length as the packet entered the ingress pipeline (including FCS).
// There is no addition/subtraction based on headers added/removed or if bridged metadata is added.
//
// For mirrored packets, the packet length presented to the egress pipeline is different:
//     = the original packet length as it entered ingress (including FCS) + the mirror header size
// We have to perform adjustment for mirrored packets to exclude the mirror header size from
// the packet length.
//
//
// Implementation:
//
// 1) In each mirror field list parser state, we insert an assignment which writes the adjust
//    amount to a field, "egress_pkt_len_adjust". The value to assign, a compile time constant,
//    returned by "sizeInBytes(<header>)", is known after PHV allocation, when we known which
//    containers mirror metadata is allocated in, and therefore can calculate total number of
//    bytes to adjust (in terms of container size). To carry this late binding constant in the IR,
//    we introduce the IR::BFN::TotalContainerSize type. We resolve these in the LowerParser pass.
//
// 2) Insert a no-match table that runs the action below, in the front of the egress table sequence
//    before the first use of "egress_pkt_length":
//        egress_pkt_length = egress_pkt_length - egress_pkt_len_adjust
//

class AdjustEgressPacketLength : public PassManager {
    bool egressParsesMirror = false;
    bool egressUsesPacketLength = false;
 public:
    AdjustEgressPacketLength(P4::ReferenceMap* refMap, P4::TypeMap* typeMap);
};

#endif  /* BF_P4C_ARCH_FROMV1_0_EGRESS_PACKET_LENGTH_H_ */
