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

#ifndef BF_P4C_PARDE_INFER_PAYLOAD_OFFSET_H_
#define BF_P4C_PARDE_INFER_PAYLOAD_OFFSET_H_

#include "ir/ir.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parde_utils.h"

/**
 * @ingroup parde
 *
 * In JBay, parser has new feature of stopping the header length counter in any
 * state, by asserting the hdr_len_inc_stop flag from output action RAM (In
 * Tofino, the header length counter is stopped when parsing terminates). The
 * application of this feature is deep packet parsing (peeking deep into packet
 * without having to extract).
 *
 * This also means the extraction of the last mutable (modifiable) field in the
 * packet is effectively the header/payload boundary. The compiler can use this
 * feature and infer where to assert hdr_len_inc_stop in the parse graph (what
 * this pass does). After hdr_len_inc_stop is asserted, fields can still be
 * extracted, for read-only operation in the MAU, but cannot not be deparsed
 * anymore. The unused fields after hdr_len_inc_stop is asserted can be then
 * dead-code eliminated, alleviating pressure on CLOT allocation, and potentially
 * PHV allocation (if CLOT allocation spills fields onto PHV due to the 3-byte-gap
 * constraint).
 *
 * Concisely, we formalize the problem as below:
 *
 * Given a parse graph G, and a set of states, M, with mutable fields
 * to extract, we want to compute a set of states E where the hdr_len_inc_stop
 * need to be asserted.
 *
 * Consider the parse graph below where state `b` and `c` have mutable fields to
 * extract.
 *
 *                   (a)
 *                   /\
 *                  /  \
 *                 /   (b)
 *                |    /|\
 *                |   / | \
 *                |  /  |  \
 *                | /   |   |
 *               (c)    |   |
 *               / \    |   |
 *              /   \  /    |
 *             |    (d)     |
 *             |     |     /
 *              \    |    /
 *               \   |   /
 *                \  |  /
 *                 [pipe]
 *
 * 1. Compute the bi-partition of set of states, D, that are descendants of M, and
 * its exclusion set D'. In this example,
 *
 *     D = { d, ingress_pipe }
 *     D' = { start, a, b, c }
 *
 * 2. Compute the cut-set, C, which consists of the transitions that cross the
 * bi-parition, D and D'. In this example,
 *
 *     C = { c->ingress_pipe,
 *           c->d,
 *           b->d,
 *           b->ingress_pipe }.
 *
 * 3. Apply the following rules to absorb the transitions into the states in C.
 *
 *   a) If all outgoing transitions of s in G is equal to all outgoing transitions of s in C
 *   b) If all incoming transitions of s in G is equal to all incoming transitions of s in C
 *
 *   In this example, we end up with
 *
 *   C = { c->ingress_pipe,
 *         b,
 *         d }
 *
 * 4. Insert state on remaining transitions in C. In this example, we insert
 *    state $hdr_len_inc_stop_0 on c->ingress_pipe
 *
 *    Finally, We arrive at solution of E = { c, d, $hdr_len_inc_stop_0 }
 *
 *
 * *TODO* someone forgets stop the residual checksum calculation when hdr_len_inc_stop is asserted,
 *        compiler needs to apply SW workaround described in JBAY-2873.
 */

class InferPayloadOffset : public PassManager {
 public:
    InferPayloadOffset(const PhvInfo& phv, const FieldDefUse& defuse);
};

#endif  /* BF_P4C_PARDE_INFER_PAYLOAD_OFFSET_H_ */
