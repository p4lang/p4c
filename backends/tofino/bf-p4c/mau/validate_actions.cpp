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

#include "validate_actions.h"

#include "bf-p4c/mau/action_analysis.h"
#include "ir/ir.h"
#include "lib/log.h"

bool ValidateActions::preorder(const IR::MAU::Action *act) {
    auto tbl = findContext<IR::MAU::Table>();
    CHECK_NULL(tbl);
    Log::TempIndent indent;
    LOG3("ValidateActions for table: " << tbl->externalName() << ", action: " << act->name
                                       << indent);
    ActionAnalysis::FieldActionsMap field_actions_map;
    ActionAnalysis::ContainerActionsMap container_actions_map;
    ActionAnalysis aa(phv, phv_alloc, ad_alloc, tbl, red_info, false,
                      false);  // action block is parallel.
    if (phv_alloc)
        aa.set_container_actions_map(&container_actions_map);
    else
        aa.set_field_actions_map(&field_actions_map);
    aa.set_error_verbose();
    aa.set_verbose();
    act->apply(aa);
    warning_found |= aa.warning_found();
    if (warning_found) LOG3("Warning found");
    error_found |= aa.error_found();
    if (error_found) LOG3("Error found");
    return false;
}

Visitor::profile_t ValidateActions::init_apply(const IR::Node *root) {
    profile_t rv = Inspector::init_apply(root);
    warning_found = false;
    return rv;
}

void ValidateActions::end_apply() {
    cstring error_message;
    if (phv_alloc)
        error_message =
            cstring("PHV allocation creates an invalid container action within a Tofino ALU");
    else
        error_message = cstring(
            "Instruction selection creates an instruction that the rest of the compiler cannot "
            "correctly interpret");
    if (error_found) {
        error(error_message);
    } else if (warning_found) {
        if (stop_compiler)
            error(error_message);
        else
            warning(error_message);
    }
}
