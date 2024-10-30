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

#ifndef BF_P4C_PHV_V2_METADATA_INITIALIZATION_H_
#define BF_P4C_PHV_V2_METADATA_INITIALIZATION_H_

#include <algorithm>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/pragma/pa_no_init.h"

namespace PHV {

namespace v2 {
class MetadataInitialization : public PassManager {
    MauBacktracker &backtracker;

 public:
    MetadataInitialization(MauBacktracker &backtracker, const PhvInfo &phv, FieldDefUse &defuse);
};

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_METADATA_INITIALIZATION_H_ */
