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

#ifndef BF_P4C_PHV_CHECK_UNALLOCATED_H_
#define BF_P4C_PHV_CHECK_UNALLOCATED_H_

#include <sstream>

#include "bf-p4c/mau/table_placement.h"
#include "bf-p4c/phv/phv_analysis.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "ir/ir.h"

/** If any fields (tempvars) were created after PHV allocation, we need to rerun
 * allocation from scratch (currently) to get those field allocated.  It would be
 * be better to do incremental allocation.
 */
class CheckForUnallocatedTemps : public PassManager {
    const PhvInfo &phv;
    const PhvUse &uses;
    const ClotInfo &clot;
    ordered_set<PHV::Field *> unallocated_fields;

    /** Produce the set of fields that are not fully allocated to PHV containers.
     */
    void collect_unallocated_fields();

 public:
    CheckForUnallocatedTemps(const PhvInfo &phv, PhvUse &uses, const ClotInfo &clot,
                             PHV_AnalysisPass *phv_alloc);
};

#endif /* BF_P4C_PHV_CHECK_UNALLOCATED_H_ */
