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

#include "bf-p4c/parde/clot/pragma/do_not_use_clot.h"

#include <string>
#include <numeric>

#include "lib/log.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"

// BFN::Pragma interface
const char *PragmaDoNotUseClot::name = "do_not_use_clot";
const char *PragmaDoNotUseClot::description =
    "Specifies that a field or all fields in a header will not be allocated to CLOT "
    "(Tofino 2 only).";
const char *PragmaDoNotUseClot::help =
    "@pragma do_not_use_clot [pipe] gress inst.node\n"
    "+ attached to P4 header instance\n"
    "\n"
    "Specifies that the indicated node, which may be either a single "
    "field or a header, will not be allocated to CLOTs when compiling for Tofino 2. "
    "If a header is specified, this pragma will apply to all of its fields. "
    "If some or all of the fields were previously eligible for CLOT allocation due to being "
    "unused or read-only in the pipeline, the eligible fields will be marked as ineligible and "
    "will be allocated to PHV instead. "
    "Fields that were already ineligible for CLOT allocation passed to this pragma are ignored. "
    "The gress value can be either ingress or egress. "
    "If the optional pipe value is provided, the pragma is applied only "
    "to the corresponding pipeline. If not provided, it is applied to "
    "all pipelines.";

bool PragmaDoNotUseClot::preorder(const IR::BFN::Pipe* pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto& annotation : global_pragmas) {
        if (annotation->name.name != PragmaDoNotUseClot::name)
            continue;

        const IR::Vector<IR::Expression>& exprs = annotation->expr;

        if (!PHV::Pragmas::checkStringLiteralArgs(exprs)) {
            continue;
        }

        const unsigned min_required_arguments = 2;  // gress, node
        unsigned required_arguments = min_required_arguments;
        unsigned expr_index = 0;
        const IR::StringLiteral* pipe_arg = nullptr;
        const IR::StringLiteral* gress_arg = nullptr;

        if (!PHV::Pragmas::determinePipeGressArgs(exprs, expr_index,
                required_arguments, pipe_arg, gress_arg)) {
            continue;
        }

        if (!PHV::Pragmas::checkNumberArgs(annotation, required_arguments,
                min_required_arguments, true, cstring(PragmaDoNotUseClot::name),
                "`gress', `node'"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        const IR::StringLiteral* node_ir = exprs[expr_index++]->to<IR::StringLiteral>();

        LOG4("@pragma do_not_use_clot's arguments: "
             << (pipe_arg ? pipe_arg->value + ", " : "")
             << gress_arg->value << ", "
             << node_ir->value);

        cstring node_name = gress_arg->value + "::"_cs + node_ir->value;
        const PHV::Field* field = phv_info.field(node_name);
        const PhvInfo::StructInfo* header = field ? nullptr : phv_info.hdr(node_name);
        ordered_set<const PHV::Field*> node_fields;

        if (field) {
            node_fields.insert(field);
        } else if (header) {
            phv_info.get_hdr_fields(node_name, node_fields);
        } else {
            PHV::Pragmas::reportNoMatchingPHV(pipe, node_ir);
            continue;
        }

        for (const auto& field : node_fields) {
            LOG4("Adding into fields that will not be allocated to CLOTs: " << field->name);
            do_not_use_clot.insert(field);
        }
    }
    return false;
}

std::ostream& operator<<(std::ostream& out, const PragmaDoNotUseClot& do_not_use_clot) {
    std::stringstream logs;
    logs << "Printing all fields made ineligible for CLOT allocation by @pragma do_not_use_clot:"
         << std::endl;
    for (const auto& field : do_not_use_clot.do_not_use_clot_fields())
        logs << "  " << field->name << std::endl;
    out << logs.str();
    return out;
}
