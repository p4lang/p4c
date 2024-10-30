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

#include "bf-p4c/phv/pragma/pa_mutually_exclusive.h"

#include <numeric>
#include <string>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/log.h"

// BFN::Pragma interface
const char *PragmaMutuallyExclusive::name = "pa_mutually_exclusive";
const char *PragmaMutuallyExclusive::description =
    "Specifies that the two fields/headers are mutually exclusive with each other.";
const char *PragmaMutuallyExclusive::help =
    "@pragma pa_mutually_exclusive [pipe] gress inst_1.node_1 inst_2.node_2\n"
    "+ attached to P4 header instances\n"
    "\n"
    "Specifies that the two indicated nodes which may be either single "
    "fields or headers can be considered mutually exclusive of one "
    "another. In the case of header nodes, the exclusivity applies to "
    "the fields of the header. "
    "PHV allocation uses field exclusivity to optimize container usage "
    "by overlaying mutually exclusive fields in the same container. "
    "This pragma does not guarantee that the mutually exclusive fields "
    "will occupy the same container.  It gives the compiler the "
    "option to do so. The gress value can be either ingress or egress. "
    "If the optional pipe value is provided, the pragma is applied only "
    "to the corresponding pipeline. If not provided, it is applied to "
    "all pipelines.";

bool PragmaMutuallyExclusive::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto &annotation : global_pragmas) {
        if (annotation->name.name != PragmaMutuallyExclusive::name) continue;

        const IR::Vector<IR::Expression> &exprs = annotation->expr;

        if (!PHV::Pragmas::checkStringLiteralArgs(exprs)) {
            continue;
        }

        const unsigned min_required_arguments = 3;  // gress, node1, node2
        unsigned required_arguments = min_required_arguments;
        unsigned expr_index = 0;
        const IR::StringLiteral *pipe_arg = nullptr;
        const IR::StringLiteral *gress_arg = nullptr;

        if (!PHV::Pragmas::determinePipeGressArgs(exprs, expr_index, required_arguments, pipe_arg,
                                                  gress_arg)) {
            continue;
        }

        if (!PHV::Pragmas::checkNumberArgs(annotation, required_arguments, min_required_arguments,
                                           true, cstring(PragmaMutuallyExclusive::name),
                                           "`gress', `node1', `node2'"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        const IR::StringLiteral *node1_ir = exprs[expr_index++]->to<IR::StringLiteral>();
        const IR::StringLiteral *node2_ir = exprs[expr_index++]->to<IR::StringLiteral>();

        LOG4("@pragma pa_mutually_exclusive's arguments: "
             << (pipe_arg ? pipe_arg->value + ", " : "") << gress_arg->value << ", "
             << node1_ir->value << ", " << node2_ir->value);

        cstring node1_name = gress_arg->value + "::"_cs + node1_ir->value;
        cstring node2_name = gress_arg->value + "::"_cs + node2_ir->value;
        const PHV::Field *field1 = phv_i.field(node1_name);
        const PHV::Field *field2 = phv_i.field(node2_name);
        const PhvInfo::StructInfo *hdr1 = field1 ? nullptr : phv_i.hdr(node1_name);
        const PhvInfo::StructInfo *hdr2 = field2 ? nullptr : phv_i.hdr(node2_name);
        ordered_set<const PHV::Field *> n1_flds;
        ordered_set<const PHV::Field *> n2_flds;

        if (field1) n1_flds.insert(field1);
        if (hdr1) phv_i.get_hdr_fields(node1_name, n1_flds);
        if (!field1 && !hdr1) {
            PHV::Pragmas::reportNoMatchingPHV(pipe, node1_ir);
            continue;
        }

        if (field2) n2_flds.insert(field2);
        if (hdr2) phv_i.get_hdr_fields(node2_name, n2_flds);
        if (!field2 && !hdr2) {
            PHV::Pragmas::reportNoMatchingPHV(pipe, node2_ir);
            continue;
        }

        if (hdr1 && hdr2) {
            cstring hdr1_name = phv_i.full_hdr_name(node1_name);
            cstring hdr2_name = phv_i.full_hdr_name(node2_name);
            LOG3("Adding into mutually exclusive headers: (" << hdr1_name << ", " << hdr2_name
                                                             << ")");
            mutually_exclusive_headers[hdr1_name].insert(hdr2_name);
            mutually_exclusive_headers[hdr2_name].insert(hdr1_name);
        }

        for (const auto &fld1 : n1_flds) {
            for (const auto &fld2 : n2_flds) {
                LOG4("Adding into mutually exclusive pairs: (" << fld1->name << ", " << fld2->name
                                                               << ")");

                pa_mutually_exclusive_i[fld1].insert(fld2);
                pa_mutually_exclusive_i[fld2].insert(fld1);
            }
        }
    }
    return false;
}

std::ostream &operator<<(std::ostream &out, const PragmaMutuallyExclusive &pa_me) {
    std::stringstream logs;
    logs << "Printing all fields marked mutually exclusive by @pragma pa_mutually_exclusive:"
         << std::endl;
    for (const auto &entry : pa_me.mutex_fields()) {
        for (const auto &field : entry.second) {
            logs << "  (" << entry.first->name << ", " << field->name << ")" << std::endl;
        }
    }
    out << logs.str();
    return out;
}
