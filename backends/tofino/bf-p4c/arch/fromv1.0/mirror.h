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

#ifndef BF_P4C_ARCH_FROMV1_0_MIRROR_H_
#define BF_P4C_ARCH_FROMV1_0_MIRROR_H_

#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace BFN {

using FieldListId = std::tuple<gress_t, unsigned, cstring>;
using MirroredFieldList = IR::Vector<IR::Expression>;
using MirroredFieldLists = std::map<FieldListId, const MirroredFieldList *>;

/**
 * Searches for invocations of the `mirror_packet.add_metadata()` extern in
 * deparser controls and generates a parser program that will extract the
 * mirrored fields.
 *
 * `mirror_packet.emit()` (which is used by `tofino.p4`) is accepted as a
 * synonym for `mirror_packet.add_metadata()`.
 *
 * @param pipe  The Tofino pipe for this P4 program. The generated parser IR
 *              will be attached to this pipe's egress parser.
 * @param ingressDeparser  The ingress deparser to check for mirror field lists.
 * @param egressDeparser  The egress deparser to check for mirror field lists.
 * @param refMap  An up-to-date reference map for this P4 program.
 * @param typeMap  An up-to-date type map for this P4 program.
 */

class FixupMirrorMetadata : public PassManager {
    MirroredFieldLists fieldLists;

 public:
    FixupMirrorMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, bool use_bridge_metadata);
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_FROMV1_0_MIRROR_H_ */
