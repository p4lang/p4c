/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_ARCH_FROMV1_0_EGRESS_PACKET_LENGTH_H_
#define BF_P4C_ARCH_FROMV1_0_EGRESS_PACKET_LENGTH_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

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
    AdjustEgressPacketLength(P4::ReferenceMap *refMap, P4::TypeMap *typeMap);
};

#endif /* BF_P4C_ARCH_FROMV1_0_EGRESS_PACKET_LENGTH_H_ */
