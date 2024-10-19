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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_MERGE_DESUGARED_VARBIT_VALIDS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_MERGE_DESUGARED_VARBIT_VALIDS_H_

#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "lib/ordered_map.h"

class PhvInfo;
class ClotInfo;
class PragmaAlias;

using namespace P4;

class MergeDesugaredVarbitValids : public PassManager {
    ordered_map<cstring, const IR::Member *> field_expressions;

 public:
    explicit MergeDesugaredVarbitValids(const PhvInfo &phv, const ClotInfo &clot_info,
                                        PragmaAlias &pragma);
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_MERGE_DESUGARED_VARBIT_VALIDS_H_ */
