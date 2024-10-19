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

#include "bf-p4c/phv/pragma/pa_no_init.h"

#include <numeric>
#include <string>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/log.h"

/// BFN::Pragma interface
const char *PragmaNoInit::name = "pa_no_init";
const char *PragmaNoInit::description = "Specifies that the field does not require initialization.";
const char *PragmaNoInit::help =
    "@pragma pa_no_init [pipe] gress inst_name.field_name\n"
    "+ attached to P4 header instances\n"
    "\n"
    "Specifies that the indicated metadata field does not require "
    "initialization to 0 before its first use because the control plane "
    "guarantees that, in cases where this field's value is needed, it will "
    "always be written before it is read.\n"
    "\n"
    "Use this pragma if your program contains logic to ensure that it only "
    "evaluates this field's value when executing control paths in which a "
    "meaningful value has been written to the field, whereas for other "
    "control paths your program may read from the field but does not care "
    "what value appears there.  For such programs, the compiler cannot "
    "determine that the control plane program logic guarantees a write "
    "before a desired read.  This indeterminacy occurs when some control "
    "paths exist that require reading from the field before writing to it, "
    "while other control paths result in the field being read even though "
    "it was not written with a meaningful value, but in these latter "
    "control paths the program does not care what value appears in the "
    "field.\n"
    "\n"
    "The pragma is ignored if the field's allocation does not require "
    "initialization.  It is also ignored if it is attached to any "
    "field other than a metadata field.  The gress value can be either "
    "ingress or egress. "
    "If the optional pipe value is provided, the pragma is applied only "
    "to the corresponding pipeline. If not provided, it is applied to "
    "all pipelines. "
    "Use this pragma with care.  If that unexpected "
    "control flow path is exercised, the field will have an unknown value.";

bool PragmaNoInit::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name != PragmaNoInit::name) continue;

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
                                           true, cstring(PragmaNoInit::name),
                                           "`gress', `field'"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        auto field_ir = exprs[expr_index++]->to<IR::StringLiteral>();

        auto field_name = gress_arg->value + "::" + field_ir->value;
        const PHV::Field *field = phv_i.field(field_name);
        if (!field) {
            PHV::Pragmas::reportNoMatchingPHV(pipe, field_ir, field_name);
            continue;
        }

        if (!field->metadata) {
            warning("@pragma pa_no_init ignored for non-metadata field %1%", field_name);
            continue;
        }

        fields.insert(field);
        LOG1("@pragma pa_no_init set for " << field_name);
    }
    return true;
}

std::ostream &operator<<(std::ostream &out, const PragmaNoInit &pa_no) {
    std::stringstream logs;
    for (auto *f : pa_no.getFields())
        logs << "@pa_no_init specifies that " << f->name << " should be marked no_init"
             << std::endl;
    out << logs.str();
    return out;
}
