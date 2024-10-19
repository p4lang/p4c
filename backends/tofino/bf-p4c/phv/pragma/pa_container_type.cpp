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

#include "bf-p4c/phv/pragma/pa_container_type.h"

#include <numeric>
#include <string>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/log.h"

namespace {

std::optional<PHV::Kind> str_to_kind(cstring str) {
    if (str == "tagalong") {
        return PHV::Kind::tagalong;
    } else if (str == "normal") {
        return PHV::Kind::normal;
    } else if (str == "mocha") {
        return PHV::Kind::mocha;
    } else if (str == "dark") {
        return PHV::Kind::dark;
    }
    return std::nullopt;
}

}  // namespace

/// BFN::Pragma interface
const char *PragmaContainerType::name = "pa_container_type";
const char *PragmaContainerType::description =
    "Forces the allocation of a field in the specified container type.";
const char *PragmaContainerType::help =
    "@pragma pa_container_type gress field field_type\n"
    "+ attached to P4 header instances\n"
    "\n"
    "Valid field_type values are:\n"
    " - normal\n"
    " - mocha\n"
    " - dark\n"
    " - tagalong\n"
    "\n"
    "P4 example:\n"
    "\n"
    "@pa_container_type(\"ingress\", \"hdr.data.f1\", \"normal\")\n"
    "\n"
    "This specifies that the field hdr.data.f1 in the ingress pipe should be placed "
    "in a normal container.";

bool PragmaContainerType::add_constraint(const IR::BFN::Pipe *pipe, const IR::Expression *expr,
                                         cstring field_name, PHV::Kind kind) {
    // check field name
    PHV::Field *field = phv_i.field(field_name);
    if (!field) {
        PHV::Pragmas::reportNoMatchingPHV(pipe, expr, field_name);
        return false;
    }
    if (kind == PHV::Kind::mocha) {
        field->set_mocha_candidate(true);
        field->set_dark_candidate(false);
    } else if (kind == PHV::Kind::dark) {
        field->set_mocha_candidate(false);
        field->set_dark_candidate(true);
    } else if (kind == PHV::Kind::normal) {
        field->set_mocha_candidate(false);
        field->set_dark_candidate(false);
    } else if (kind == PHV::Kind::tagalong) {
        warning(
            "@prama pa_container_type currently does not support tagalong containers, "
            "skipped",
            field_name);
        return false;
    }
    fields[field] = kind;
    return true;
}

bool PragmaContainerType::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name != PragmaContainerType::name) continue;

        auto &exprs = annotation->expr;

        if (!PHV::Pragmas::checkStringLiteralArgs(exprs)) {
            continue;
        }

        const unsigned min_required_arguments = 3;  // gress, field, type
        unsigned required_arguments = min_required_arguments;
        unsigned expr_index = 0;
        const IR::StringLiteral *pipe_arg = nullptr;
        const IR::StringLiteral *gress_arg = nullptr;

        if (!PHV::Pragmas::determinePipeGressArgs(exprs, expr_index, required_arguments, pipe_arg,
                                                  gress_arg)) {
            continue;
        }

        if (!PHV::Pragmas::checkNumberArgs(annotation, required_arguments, min_required_arguments,
                                           true, cstring(PragmaContainerType::name),
                                           "`gress', `field', `type'"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        auto field_ir = exprs[expr_index++]->to<IR::StringLiteral>();
        auto container_type = exprs[expr_index++]->to<IR::StringLiteral>();

        auto field_name = gress_arg->value + "::" + field_ir->value;
        LOG1("Adding container type " << container_type->value << " for " << field_name);
        auto kind = str_to_kind(container_type->value);
        if (!kind) {
            warning(
                "@pragma pa_container_type's argument %1% does not match any valid container "
                "types, skipped",
                field_name);
        } else {
            add_constraint(pipe, field_ir, field_name, *kind);
        }
    }
    return true;
}

std::optional<PHV::Kind> PragmaContainerType::required_kind(const PHV::Field *f) const {
    if (fields.count(f)) {
        return fields.at(f);
    }
    return std::nullopt;
}

std::ostream &operator<<(std::ostream &out, const PragmaContainerType &pa_ct) {
    std::stringstream logs;
    for (auto kv : pa_ct.getFields())
        logs << "@pa_container_type specifies that " << kv.first->name
             << " should be allocated to "
                "a "
             << kv.second << " container." << std::endl;
    out << logs.str();
    return out;
}
