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

#ifndef BF_P4C_PARDE_UPDATE_PARSER_WRITE_MODE_H_
#define BF_P4C_PARDE_UPDATE_PARSER_WRITE_MODE_H_

#include <ir/ir.h>
#include "ir/pass_manager.h"

#include "bf-p4c/parde/check_parser_multi_write.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_fields.h"

struct UpdateParserWriteMode : public PassManager {
    explicit UpdateParserWriteMode(const PhvInfo& phv) {
        auto parser_info = new CollectParserInfo;
        auto* field_to_states = new MapFieldToParserStates(phv);
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
