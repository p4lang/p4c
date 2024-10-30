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

#include "bf-p4c/phv/pragma/pa_solitary.h"

#include <numeric>
#include <string>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/log.h"

/// BFN::Pragma interface
const char *PragmaSolitary::name = "pa_solitary";
const char *PragmaSolitary::description =
    "Specifies that the field can not share a PHV container with any other field.";
const char *PragmaSolitary::help =
    "@pragma pa_solitary [pipe] gress instance_name.field_name\n"
    "+ attached to P4 header instances\n"
    "\n"
    "Specifies that the indicated packet or metadata field cannot share a "
    "container with any other field.  It can be overlaid with other "
    "field(s) if they are mutually exclusive.  The gress value can be "
    "either ingress or egress. "
    "If the optional pipe value is provided, the pragma is applied only "
    "to the corresponding pipeline. If not provided, it is applied to "
    "all pipelines. "
    "A constraint error will be generated if the "
    "indicated field is a packet field and the packet field bit width is "
    "not a multiple of eight or the field is not byte aligned.";

bool PragmaSolitary::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name != PragmaSolitary::name) continue;

        auto &exprs = annotation->expr;

        if (!PHV::Pragmas::checkStringLiteralArgs(exprs)) {
            continue;
        }

        const unsigned min_required_arguments = 2;  // gress, field
        unsigned required_arguments = min_required_arguments;
        unsigned expr_index = 0;
        const IR::StringLiteral *pipe_arg = nullptr;
        const IR::StringLiteral *gress_arg = nullptr;

        if (!PHV::Pragmas::determinePipeGressArgs(exprs, expr_index, required_arguments, pipe_arg,
                                                  gress_arg)) {
            continue;
        }

        if (!PHV::Pragmas::checkNumberArgs(annotation, required_arguments, min_required_arguments,
                                           true, cstring(PragmaSolitary::name),
                                           "`gress', `field'"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        auto field_ir = exprs[expr_index++]->to<IR::StringLiteral>();

        // check field name
        auto field_name = gress_arg->value + "::" + field_ir->value;
        auto field = phv_i.field(field_name);
        if (!field) {
            PHV::Pragmas::reportNoMatchingPHV(pipe, field_ir, field_name);
            continue;
        }

        // check if it is non-container-sized header field
        if (field->exact_containers() &&
            (field->size != int(PHV::Size::b8) && field->size != int(PHV::Size::b16) &&
             field->size != int(PHV::Size::b32))) {
            warning(
                "@pragma pa_solitary's argument %1% can not be solitary, "
                "because it is a deparsed header field"
                " and it is not container-sized ",
                field_name);
            continue;
        }

        // set solitary
        field->set_solitary(PHV::SolitaryReason::PRAGMA_SOLITARY);
        LOG1("@pragma pa_solitary set " << field->name << " to be solitary");
    }
    return true;
}
