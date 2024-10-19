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
