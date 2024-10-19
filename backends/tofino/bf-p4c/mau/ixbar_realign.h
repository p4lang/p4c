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

#ifndef BF_P4C_MAU_IXBAR_REALIGN_H_
#define BF_P4C_MAU_IXBAR_REALIGN_H_

#include <array>

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource.h"
#include "lib/safe_vector.h"

using namespace P4;

class IXBarVerify : public MauModifier {
    const PhvInfo &phv;
    const IR::MAU::Table *currentTable = nullptr;
    profile_t init_apply(const IR::Node *) override;
    bool preorder(IR::Expression *) override { return false; }
    void postorder(IR::MAU::Table *) override;
    void verify_format(const IXBar::Use *);
    class GetCurrentUse;
    // Array of Map of Stage -> Input Xbar
    // Tofino 1/2 only uses ixbar[0] for both ingress and egress.
    std::map<int, std::unique_ptr<IXBar>> ixbar[2];

 public:
    explicit IXBarVerify(const PhvInfo &phv) : phv(phv) {}
};

#endif /* BF_P4C_MAU_IXBAR_REALIGN_H_ */
