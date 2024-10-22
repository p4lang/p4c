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

#ifndef BF_P4C_PARDE_MIRROR_CONST_MIRROR_SESSION_OPT_H_
#define BF_P4C_PARDE_MIRROR_CONST_MIRROR_SESSION_OPT_H_

#include "bf-p4c/parde/parde_visitor.h"

/**
 * This pass optimizes the following code pattern:
 *  if (ig_intr_md.mirror_type == 0) {
 *      mirror.emit<HeaderType>(10w0, { ... });
 *
 * For Tofino1, mirror_id is 10b and generally expected to allocate to PHV16.
 * However, the hardware is capable of accepting PHV8 as well. When a PHV8 is
 * used, the hardware will automatically duplicate the PHV8 to PHV16 by
 * concatenating the PHV8 with itself.
 *
 * Therefore, if the mirror_id is constant 0, we can use the B0 container which
 * is reserved for deparse_zero to generate the 10b0 value for mirror_id.
 *
 * This pass can save PHV resource for the following errata implementation:
 * Due to the errata, the packet cannot be "fully" dropped in the deparser –
 * this will cause serious problems. Therefore, it needs to be dropped just past
 * the deparser, i.e. in that mirror component (mirroring engine). Compiler
 * provides the workaround transparently, without requiring everyone to change
 * the way they drop packets. Compiler synthesizes the code that adds packet
 * mirroring using mirror_type 0, which in turn uses mirror session 0, that is
 * reserved and is programmed by the driver to be invalid. So, if P4 program
 * handles a unicast packet (for example), and set drop_ctl=1, the device will
 * see that it still has to mirror that packet to mirror_type 0 and the packet
 * will proceed to the mirroring component. There, it will be mirrored to
 * session 0 (coming from that variable $tmp3), but since it is invalid, the
 * packet will be dropped and will not go to the TM.  As a result, if you want
 * to cancel mirroring, the proper way to do it would be to call
 * invalidate(ig_dprsr_md.mirror_type) and not to set
 * ig_dprsr_md.drop_ctl[2:2]=1.
 *
 */
class ConstMirrorSessionOpt final : public DeparserTransform {
    const PhvInfo &phv;

 public:
    ConstMirrorSessionOpt(PhvInfo &phv) : phv(phv) {}

    IR::Node *preorder(IR::BFN::Deparser *dp) override {
        if (dp->gress != INGRESS) prune();
        return dp;
    }
    IR::Node *preorder(IR::BFN::Digest *dfl) override;
    IR::Node *preorder(IR::TempVar *tv) override;
};

#endif /* BF_P4C_PARDE_MIRROR_CONST_MIRROR_SESSION_OPT_H_ */
