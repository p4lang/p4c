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

#include "bf-p4c/phv/pragma/pa_no_overlay.h"

#include <numeric>
#include <string>

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/log.h"

/// BFN::Pragma interface
const char *PragmaNoOverlay::name = "pa_no_overlay";
const char *PragmaNoOverlay::description =
    "Specifies that the field can not be overlaid with other field. When only one field is"
    "provided, the field can not be overlaid with any other field. When there are multiple"
    "fields, fields in this group cannot be overlaid with each other.";
const char *PragmaNoOverlay::help =
    "@pragma pa_no_overlay [pipe] gress inst_1.field_1 "
    "[inst2.field_name_2] <inst_3.field_name_3 ...>\n"
    "+ attached to P4 header instances\n"
    "\n"
    "(1) When there is only one field, the field cannot be overlaid with any other field."
    "(2) When there is a group of fields, fields in the group cannot be overlaid with each"
    " other field in the group."
    "The gress value can be either ingress or egress. ";

bool PragmaNoOverlay::preorder(const IR::MAU::Instruction *inst) {
    // TODO: Until we handle concat operations in the backend properly (by adding metadata
    // fields with concat operations to a slice list, also containing padding fields that must be
    // initialized to 0), we have to disable overlay for any field used in concat operations (concat
    // operations that survive in the backend).
    if (inst->operands.empty()) return true;
    for (auto *operand : inst->operands) {
        if (!operand->is<IR::Concat>()) continue;
        const PHV::Field *f = phv_i.field(operand);
        if (f) {
            no_overlay.insert(f);
            LOG1("\tAdding pa_no_overlay on " << f->name
                                              << " because of a concat "
                                                 "operation.");
        }
    }
    return true;
}

bool PragmaNoOverlay::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name != PragmaNoOverlay::name) continue;

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
                                           false, cstring(PragmaNoOverlay::name),
                                           "`gress', `field'"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        // Extract the rest of the arguments
        std::vector<const IR::StringLiteral *> field_irs;
        for (; expr_index < exprs.size(); ++expr_index) {
            const IR::StringLiteral *name = exprs[expr_index]->to<IR::StringLiteral>();
            field_irs.push_back(name);
        }

        bool processPragma = true;
        std::vector<const PHV::Field *> fields;
        for (const auto *field_ir : field_irs) {
            cstring field_name = gress_arg->value + "::"_cs + field_ir->value;
            const auto *field = phv_i.field(field_name);
            if (!field) {
                PHV::Pragmas::reportNoMatchingPHV(pipe, field_ir, field_name);
                processPragma = false;
                break;
            }
            fields.push_back(field);
        }
        if (!processPragma) continue;

        if (fields.size() == 1) {
            no_overlay.insert(*fields.begin());
        } else {
            for (size_t i = 0; i < fields.size(); i++) {
                for (size_t j = i + 1; j < fields.size(); j++) {
                    const auto *f1 = fields[i];
                    const auto *f2 = fields[j];
                    mutually_inclusive(f1->id, f2->id) = true;
                }
            }
        }
    }

    // TODO tmp workaround to disable bridged residual checksum fields from being
    // overlaid. Proper bridged metadata overlay requires mutex analysis on ingress and
    // coordination of ingress deparser and egress parser (TODO).
    for (auto &nf : phv_i.get_all_fields()) {
        std::string f_name(nf.first.c_str());
        if (f_name.find(BFN::COMPILER_META) != std::string::npos &&
            f_name.find("residual_checksum_") != std::string::npos) {
            no_overlay.insert(&nf.second);
        }
    }
    LOG3(*this);
    return true;
}

std::ostream &operator<<(std::ostream &out, const PragmaNoOverlay &pa_no) {
    std::stringstream logs;
    for (auto *f : pa_no.get_no_overlay_fields())
        logs << "@pa_no_overlay specifies that " << f->name << " should not be overlaid"
             << std::endl;
    for (const auto &f1 : pa_no.phv_i) {
        for (const auto &f2 : pa_no.phv_i) {
            if (f1 != f2 && pa_no.mutually_inclusive(f1.id, f2.id)) {
                logs << "@pa_no_overlay specifies that " << f1.name << " and " << f2.name
                     << " should not be overlaid\n";
            }
        }
    }
    out << logs.str();
    return out;
}
