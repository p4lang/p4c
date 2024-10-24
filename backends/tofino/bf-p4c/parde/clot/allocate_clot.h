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
