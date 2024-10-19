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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_IXBAR_INFO_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_IXBAR_INFO_H_

#include <array>

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource.h"
#include "lib/safe_vector.h"

namespace BFN {

using namespace P4;

/**
 * Log IXBar allocation
 */
class CollectIXBarInfo : public MauInspector {
    const PhvInfo &phv;
    std::map<int, safe_vector<IXBar::Use::Byte>> _stage;
    std::map<IXBar::Use::Byte, const IR::MAU::Table *> _byteToTables;

    profile_t init_apply(const IR::Node *) override;

    void end_apply(const IR::Node *) override;

    void postorder(const IR::MAU::Table *) override;

    void sort_ixbar_byte();
    std::string print_ixbar_byte() const;

 public:
    explicit CollectIXBarInfo(const PhvInfo &phv) : phv(phv) {}
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_IXBAR_INFO_H_ */
