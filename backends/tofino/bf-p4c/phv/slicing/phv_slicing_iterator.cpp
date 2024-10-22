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

#include "bf-p4c/phv/slicing/phv_slicing_iterator.h"

#include "bf-p4c/phv/action_packing_validator_interface.h"
#include "bf-p4c/phv/slicing/phv_slicing_dfs_iterator.h"

namespace PHV {
namespace Slicing {

ItrContext::ItrContext(const PhvInfo &phv, const MapFieldToParserStates &fs,
                       const CollectParserInfo &pi, const SuperCluster *sc,
                       const PHVContainerSizeLayout &pa,
                       const ActionPackingValidatorInterface &action_packing_validator,
                       const ParserPackingValidatorInterface &parser_packing_validator,
                       const PackConflictChecker pack_conflict,
                       const IsReferencedChecker is_referenced)
    : pImpl(new DfsItrContext(phv, fs, pi, sc, pa, action_packing_validator,
                              parser_packing_validator, pack_conflict, is_referenced)) {}

}  // namespace Slicing
}  // namespace PHV
