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

#ifndef BF_P4C_PARDE_CREATE_POV_ENCODER_H_
#define BF_P4C_PARDE_CREATE_POV_ENCODER_H_

#include <ir/ir.h>

struct MatchAction {
    MatchAction(std::vector<const IR::Expression *> k, ordered_set<const IR::TempVar *> o,
                ordered_map<unsigned, unsigned> ma)
        : keys(k), outputs(o), match_to_action_param(ma) {}
    std::vector<const IR::Expression *> keys;
    ordered_set<const IR::TempVar *> outputs;
    ordered_map<unsigned, unsigned> match_to_action_param;

    std::string print() const;
};

IR::MAU::Table *create_pov_encoder(gress_t gress, cstring tableName, cstring action_name,
                                   const MatchAction &match_action);

#endif /*BF_P4C_PARDE_CREATE_POV_ENCODER_H_*/
