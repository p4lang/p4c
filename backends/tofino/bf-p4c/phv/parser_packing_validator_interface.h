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
