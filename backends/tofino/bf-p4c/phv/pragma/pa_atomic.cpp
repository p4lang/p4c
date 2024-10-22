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

#include "bf-p4c/phv/pragma/pa_atomic.h"

#include <numeric>
#include <string>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/log.h"

/// BFN::Pragma interface
const char *PragmaAtomic::name = "pa_atomic";
const char *PragmaAtomic::description =
    "Specifies that the indicated packet or metadata field cannot be split across containers.";
const char *PragmaAtomic::help =
    "@pragma pa_atomic [pipe] gress instance_name.field_name\n"
    "+ attached to P4 header instances\n"
    "\n"
    "Specifies that the indicated packet or metadata field cannot be split "
    "across containers.  The gress value can be either ingress or egress. "
    "If the optional pipe value is provided, the pragma is applied only "
    "to the corresponding pipeline. If not provided, it is applied to "
    "all pipelines. "
    "For example, an 8-bit field could be placed in one 8-bit container or "
    "one 16-bit container or one 32-bit container.  A 16-bit field could be "
    "placed in one 16-bit container or one 32-bit container.  A 24-bit "
    "field could be placed in one 32-bit container.";

// FIXME(zma) these should really be added to the include file
// but because intrinsic metadata name may be changed during translation
// stashing these here for now
static const std::unordered_set<cstring> pa_atomic_from_arch = {
    "ingress::ig_intr_md_from_prsr.parser_err"_cs, "egress::eg_intr_md_from_prsr.parser_err"_cs};

bool PragmaAtomic::add_constraint(const IR::BFN::Pipe *pipe, const IR::Expression *expr,
                                  cstring field_name) {
    // check field name
    auto field = phv_i.field(field_name);
    if (!field) {
        PHV::Pragmas::reportNoMatchingPHV(pipe, expr, field_name);
        return false;
    }

    // Make sure the field can fit into an available container fully
    if (field->size > int(PHV::Size::b32)) {
        warning(
            "@pragma pa_atomic's argument %1% can not be atomic, "
            "because the size of field is greater than the size of "
            "the largest container",
            field_name);
        return false;
    }

    fields.insert(field);
    field->set_no_split(true);
    LOG1("@pragma pa_atomic set " << field->name << " to be no_split");

    return true;
}

bool PragmaAtomic::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name != PragmaAtomic::name) continue;

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
                                           true, cstring(PragmaAtomic::name),
                                           "`gress', `field'"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        auto field_ir = exprs[expr_index++]->to<IR::StringLiteral>();

        cstring field_name = gress_arg->value + "::"_cs + field_ir->value;
        add_constraint(pipe, field_ir, field_name);
    }

    for (auto field : pa_atomic_from_arch) {
        if (phv_i.field(field)) add_constraint(pipe, nullptr, field);
    }

    return true;
}

std::ostream &operator<<(std::ostream &out, const PragmaAtomic &pa_a) {
    std::stringstream logs;
    for (auto *f : pa_a.getFields())
        logs << "@pa_atomic specifies that " << f->name << " should be marked no_split"
             << std::endl;
    out << logs.str();
    return out;
}
