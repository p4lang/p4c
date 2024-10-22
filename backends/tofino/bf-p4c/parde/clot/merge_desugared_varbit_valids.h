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
