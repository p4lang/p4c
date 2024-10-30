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
