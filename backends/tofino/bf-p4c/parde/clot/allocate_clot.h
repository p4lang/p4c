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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_ALLOCATE_CLOT_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_ALLOCATE_CLOT_H_

#include "clot_info.h"

class PragmaAlias;

class AllocateClot : public PassManager {
    ClotInfo clotInfo;

 public:
    explicit AllocateClot(ClotInfo &clotInfo, const PhvInfo &phv, PhvUse &uses,
                          PragmaDoNotUseClot &pragmaDoNotUseClot, PragmaAlias &pragmaAlias,
                          bool log = true);
};

/**
 * Adjusts allocated CLOTs to avoid PHV containers that straddle the CLOT boundary. See
 * \ref clot_alloc_adjust "CLOT allocation adjustment" (README.md).
 */
class ClotAdjuster : public Visitor {
    ClotInfo &clotInfo;
    const PhvInfo &phv;
    Logging::FileLog *log = nullptr;

 public:
    ClotAdjuster(ClotInfo &clotInfo, const PhvInfo &phv) : clotInfo(clotInfo), phv(phv) {}

    Visitor::profile_t init_apply(const IR::Node *root) override;
    const IR::Node *apply_visitor(const IR::Node *root, const char *) override;
    void end_apply(const IR::Node *root) override;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_ALLOCATE_CLOT_H_ */
