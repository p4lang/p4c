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

#ifndef BF_P4C_PHV_PARSER_PACKING_VALIDATOR_INTERFACE_H_
#define BF_P4C_PHV_PARSER_PACKING_VALIDATOR_INTERFACE_H_

#include "bf-p4c/phv/v2/utils_v2.h"

namespace PHV {

class ParserPackingValidatorInterface {
 public:
    /// @returns an error if we allocated slices in the format of @p alloc.
    /// @p c is optional for 32-bit container half-word extract optimization.
    virtual const v2::AllocError *can_pack(const v2::FieldSliceAllocStartMap &alloc,
                                           bool add_mau_inits) const = 0;
};

}  // namespace PHV

#endif /* BF_P4C_PHV_PARSER_PACKING_VALIDATOR_INTERFACE_H_ */
