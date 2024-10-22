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

#ifndef BF_P4C_PARDE_UPDATE_PARSER_WRITE_MODE_H_
#define BF_P4C_PARDE_UPDATE_PARSER_WRITE_MODE_H_

#include <ir/ir.h>

#include "bf-p4c/parde/check_parser_multi_write.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/pass_manager.h"

struct UpdateParserWriteMode : public PassManager {
    explicit UpdateParserWriteMode(const PhvInfo &phv) {
        auto parser_info = new CollectParserInfo;
        auto *field_to_states = new MapFieldToParserStates(phv);
        auto *check_write_mode_consistency =
            new CheckWriteModeConsistency(phv, *field_to_states, *parser_info);

        addPasses({
            parser_info,
            field_to_states,
            check_write_mode_consistency,
        });
    }
};

#endif /*BF_P4C_PARDE_UPDATE_PARSER_WRITE_MODE_H_*/
