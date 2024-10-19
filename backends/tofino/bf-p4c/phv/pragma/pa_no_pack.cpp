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

#include "bf-p4c/phv/pragma/pa_no_pack.h"

#include "bf-p4c/phv/pragma/phv_pragmas.h"

/// BFN::Pragma interface
const char *PragmaNoPack::name = "pa_no_pack";
const char *PragmaNoPack::description =
    "Suggest PHV allocation to not allocate two fields into the same container.";
const char *PragmaNoPack::help =
    "@pragma pa_no_pack [pipe] gress inst1.field_name_1 inst_2.field_name_2 "
    "<inst_3.field_name_3 ...>\n"
    "+ attached to P4 header instances\n"
    "\n"
    "Suggest that the indicated pair of fields should not be packed into one container."
    "\n"
    "Compiler will ignore invalid pa_no_pack pragma(s) if two fields must be allocated to"
    "one container because of constraints, e.g. header fields in a same byte.";

bool PragmaNoPack::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;

    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name != PragmaNoPack::name) continue;

        auto &exprs = annotation->expr;

        const unsigned min_required_arguments = 3;  // gress, field1, field2....
        unsigned required_arguments = min_required_arguments;
        unsigned expr_index = 0;
        const IR::StringLiteral *pipe_arg = nullptr;
        const IR::StringLiteral *gress_arg = nullptr;

        if (!PHV::Pragmas::determinePipeGressArgs(exprs, expr_index, required_arguments, pipe_arg,
                                                  gress_arg)) {
            continue;
        }

        if (!PHV::Pragmas::checkNumberArgs(annotation, required_arguments, min_required_arguments,
                                           false, cstring(PragmaNoPack::name),
                                           "`gress', `field1', `field2'"_cs)) {
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

        for (size_t i = 0; i < fields.size(); i++) {
            for (size_t j = i + 1; j < fields.size(); j++) {
                const auto *f1 = fields[i];
                const auto *f2 = fields[j];
                LOG1("Record pa_no_pack of " << f1->name << " and " << f2->name);
                no_packs_i.push_back({f1, f2});
            }
        }
        // add extra pa_no_pack provided by alt-phv-alloc
        for (auto [field1_name, fields] : no_pack_constr) {
            const auto *field1 = phv_i.field(field1_name);
            BUG_CHECK(field1, "%1% not fonud", field1_name);
            for (auto field2_name : fields) {
                const auto *field2 = phv_i.field(field2_name);
                BUG_CHECK(field2, "%1% not fonud", field2_name);
                no_packs_i.push_back({field1, field2});
            }
        }
    }
    return true;
}
