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
