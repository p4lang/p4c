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

#include "bf-p4c/phv/slicing/phv_slicing_iterator.h"

#include "bf-p4c/phv/action_packing_validator_interface.h"
#include "bf-p4c/phv/slicing/phv_slicing_dfs_iterator.h"

namespace PHV {
namespace Slicing {

ItrContext::ItrContext(const PhvInfo& phv,
                       const MapFieldToParserStates& fs,
                       const CollectParserInfo& pi,
                       const SuperCluster* sc,
                       const PHVContainerSizeLayout& pa,
                       const ActionPackingValidatorInterface& action_packing_validator,
                       const ParserPackingValidatorInterface& parser_packing_validator,
                       const PackConflictChecker pack_conflict,
                       const IsReferencedChecker is_referenced)
    : pImpl(new DfsItrContext(phv,
                              fs,
                              pi,
                              sc,
                              pa,
                              action_packing_validator,
                              parser_packing_validator,
                              pack_conflict,
                              is_referenced)) {}

}  // namespace Slicing
}  // namespace PHV
