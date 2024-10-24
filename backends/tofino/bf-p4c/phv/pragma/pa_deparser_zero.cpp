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

#include "bf-p4c/phv/pragma/pa_deparser_zero.h"

#include "bf-p4c/phv/pragma/phv_pragmas.h"

const std::vector<std::string> *PragmaDeparserZero::supported_pragmas =
    new std::vector<std::string>{PHV::pragma::NOT_PARSED, PHV::pragma::NOT_DEPARSED,
                                 PHV::pragma::DISABLE_DEPARSE_ZERO};

Visitor::profile_t PragmaDeparserZero::init_apply(const IR::Node *root) {
    notParsedFields.clear();
    notDeparsedFields.clear();
    disableDeparseZeroFields.clear();
    headerFields.clear();
    for (auto &f : phv_i) headerFields[f.header()].insert(&f);
    return Inspector::init_apply(root);
}

bool PragmaDeparserZero::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;
    for (const auto *annotation : global_pragmas) {
        std::string pragma_name = annotation->name.name.string();

        if (std::find(supported_pragmas->begin(), supported_pragmas->end(), pragma_name) ==
            supported_pragmas->end())
            continue;

        auto &exprs = annotation->expr;

        const IR::StringLiteral *pipe_arg = nullptr;
        if (exprs.at(0)) {
            // If pipe is not present, the first argument is nullptr
            pipe_arg = exprs.at(0)->to<IR::StringLiteral>();
            if (!pipe_arg) {
                warning(ErrorType::WARN_INVALID,
                        "%1%: Found a non-string literal argument `pipe'. Ignoring pragma.",
                        exprs.at(0));
                continue;
            }
        }
        auto gress_arg = exprs.at(1)->to<IR::StringLiteral>();
        if (!gress_arg) {
            warning(ErrorType::WARN_INVALID,
                    "%1%: Found a non-string literal argument `gress'. Ignoring pragma.",
                    exprs.at(1));
            continue;
        }
        auto field_ir = exprs.at(2)->to<IR::StringLiteral>();
        if (!field_ir) {
            warning(ErrorType::WARN_INVALID,
                    "%1%: Found a non-string literal argument `field'. Ignoring pragma.",
                    exprs.at(2));
            continue;
        }

        // Check whether the pragma should be applied for this pipe
        if (pipe_arg && pipe->canon_name() && pipe->canon_name() != pipe_arg->value) {
            LOG4("Skipping pragma" << pragma_name << " at the pipe " << pipe->canon_name() << " (`"
                                   << pipe_arg->value << "' passed as pipe name).");
            continue;
        }

        cstring header_name = gress_arg->value + "::"_cs + field_ir->value;
        LOG1("    Marking pragma " << pragma_name << " for header " << header_name);
        if (!headerFields.count(header_name)) continue;
        // TODO: BUG HERE.
        // This pass is simply reading from pragma annotations attached before. However,
        // when alias is involved, the previous annotations could be wrong.
        // For example, when a header field is marked not parsed before but its alias source is,
        // then after IR replacing in the Alias pass, the header field will be extracted in parser.
        // So the header field should NOT be marked as notParsedField, as it will generate wrong
        // metadata overlay info.
        for (const auto *f : headerFields.at(header_name))
            if (pragma_name == PHV::pragma::NOT_PARSED)
                notParsedFields.insert(f);
            else if (pragma_name == PHV::pragma::NOT_DEPARSED)
                notDeparsedFields.insert(f);
            else if (pragma_name == PHV::pragma::DISABLE_DEPARSE_ZERO)
                disableDeparseZeroFields.insert(f);
    }

    // Remove the fields specified with pa_disable_deparse_0_optimization from the notParsedFields
    // and notDeparsedFields.
    if (disableDeparseZeroFields.size() == 0) return true;
    for (const auto *f : disableDeparseZeroFields) {
        if (notParsedFields.count(f)) notParsedFields.erase(f);
        if (notDeparsedFields.count(f)) notDeparsedFields.erase(f);
    }
    return false;
}

void PragmaDeparserZero::end_apply() {
    if (!LOGGING(1)) return;
    if (notParsedFields.size() > 0) {
        LOG1("\tFields marked notParsed: ");
        for (const auto *f : notParsedFields) LOG1("\t  " << f);
    }
    if (notDeparsedFields.size() > 0) {
        LOG1("\n\tFields marked notDeparsed: ");
        for (const auto *f : notDeparsedFields) LOG1("\t  " << f);
    }
    if (disableDeparseZeroFields.size() > 0) {
        LOG1("\n\tFields marked disable deparse zero:");
        for (const auto *f : disableDeparseZeroFields) LOG1("\t  " << f);
    }
    return;
}

std::ostream &operator<<(std::ostream &out, const PragmaDeparserZero &pa_np) {
    std::stringstream logs;
    logs << "@pragma not_parsed specified for: " << std::endl;
    for (const auto *f : pa_np.getNotParsedFields()) logs << "    " << f->name << std::endl;
    logs << std::endl << "@pragma not_deparsed specified for: " << std::endl;
    for (const auto *f : pa_np.getNotDeparsedFields()) logs << "    " << f->name << std::endl;
    out << logs.str();
    return out;
}
