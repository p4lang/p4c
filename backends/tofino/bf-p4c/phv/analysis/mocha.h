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

#ifndef EXTENSIONS_BF_P4C_PHV_ANALYSIS_MOCHA_H_
#define EXTENSIONS_BF_P4C_PHV_ANALYSIS_MOCHA_H_

#include "ir/ir.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/analysis/non_mocha_dark_fields.h"

/** This pass analyzes all fields used in the program and marks the fields suitable for allocation
  * in mocha containers by setting the `is_mocha_candidate()` property of the corresponding
  * PHV::Field object to true.
  * Here are the generic rules for mocha containers:
  * 1. Mocha containers can be used as sources for any ALU operation writing normal PHVs.
  * 2. Mocha containers can be used on the input crossbar.
  * 3. Mocha containers cannot be written using nonset operations.
  * 4. Set operations on mocha containers can only operate on entire containers (so restrictions on
  *    packing).
  */
class CollectMochaCandidates : public Inspector {
 private:
    PhvInfo                  &phv;
    const PhvUse             &uses;
    const ReductionOrInfo    &red_info;
    const NonMochaDarkFields &nonMochaDark;
    bool                      pov_on_mocha = false;

    /// Number of mocha candidates detected.
    size_t          mochaCount;
    /// Size in bits of mocha candidates.
    size_t          mochaSize;

    profile_t init_apply(const IR::Node* root) override;
    bool preorder(const IR::MAU::Action* act) override;
    void end_apply() override;

 public:
    CollectMochaCandidates(PhvInfo& p, const PhvUse& u, const ReductionOrInfo &ri,
                           const NonMochaDarkFields &nmd) :
        phv(p), uses(u), red_info(ri), nonMochaDark(nmd), mochaCount(0), mochaSize(0) { }

    /// @returns true when @p f is a field from a packet (not metadata, pov, or bridged field).
    static bool isPacketField(const PHV::Field* f) {
        return (f && !f->metadata && !f->pov && !f->bridged && !f->overlayable &&
                !f->is_intrinsic());
    }
};

#endif  /* EXTENSIONS_BF_P4C_PHV_ANALYSIS_MOCHA_H_ */
