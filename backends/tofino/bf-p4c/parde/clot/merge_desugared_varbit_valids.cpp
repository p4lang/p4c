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

#include "merge_desugared_varbit_valids.h"

#include <cctype>

#include "ir/ir.h"

#include "bf-p4c/ir/gress.h"
#include "bf-p4c/common/alias.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_alias.h"

using namespace P4;

/**
 * Identify headers corresponding to varbit fields that were injected during varbit header
 * desugaring. Create aliases for the validity bits for slices of the same header placed in CLOTs.
 */
class CreateAliasesForVarbitHeaders : public ParserInspector {
 protected:
    const PhvInfo& phv;
    const ClotInfo& clot_info;
    PragmaAlias& pragma;

    /* Desugared varbit headers in each gress */
    std::map<gress_t, std::set<cstring>> desugared_headers;

    Visitor::profile_t init_apply(const IR::Node* root) override {
        auto rv = Inspector::init_apply(root);
        desugared_headers.clear();
        return rv;
    }

    bool preorder(const IR::HeaderOrMetadata* h) override {
        auto* state = findContext<IR::BFN::ParserState>();
        if (!state) return true;

        auto state_name = stripThreadPrefix(state->name);

        if (state_name.startsWith("parse_")) {
            auto field_slice = state_name.substr(6).string();

            if (isVarbitHeaderPattern(h->name) && h->name.name.endsWith(field_slice)) {
                std::stringstream ss;
                ss << field_slice << "_t";
                if (h->type->name.name.endsWith(ss.str())) {
                    desugared_headers[state->gress].emplace(h->name);
                    LOG5("Desugared header " << h->name << " found in state " << state->name);
                }
            }
        }

        return true;
    }

    /**
     * Create aliases for the validity bits for sliced varbit headers that are placed in CLOTS.
     */
    void end_apply() override {
        for (auto& [gress, headers] : desugared_headers) {
            std::map<cstring, std::map<int, cstring>> varbit_headers_to_merge;

            for (auto hdr : headers) {
                auto [base, size] = getHeaderBaseAndSize(hdr);
                if (size > 0) {
                    if (const auto *field = phv.field(hdr + ".field")) {
                        if (clot_info.fully_allocated(field)) {
                            varbit_headers_to_merge[base].emplace(size, hdr);
                        }
                    }
                }
            }

            for (auto& [base, headers] : varbit_headers_to_merge) {
                auto first_hdr = headers.rbegin()->second;
                const auto *first_field = phv.field(first_hdr + ".$valid");
                for (auto itr = std::next(headers.rbegin()); itr != headers.rend(); ++itr) {
                    auto hdr = itr->second;
                    const auto *field = phv.field(hdr + ".$valid");
                    LOG1("Attempting to alias: " << field << " --> " << first_field);
                    if (!pragma.addAlias(field, first_field, true, PragmaAlias::COMPILER))
                        LOG1("Could not add alias for fields " << field->name << " and "
                                                               << first_field->name);
                    else
                        LOG1("Aliased varbit field valids: " << field->name << " --> "
                                                             << first_field->name);
                }
            }
        }
    }

    /**
     * Check whether the header name matches the varbit header pattern
     * used by DesugarVarbitExtract
     */
    static bool isVarbitHeaderPattern(cstring name) {
        auto len = name.size();

        // End with b
        if (name.get(len-1) != 'b') return false;

        // Expect a sequence of digits, preceded by an underscore
        bool seen_digit = false;
        for (int idx = len - 2; idx >= 0; --idx) {
            if (name.get(idx) == '_') return seen_digit;
            if (isdigit(name.get(idx))) {
                seen_digit = true;
            } else {
                return false;
            }
        }
        return false;
    }

    /**
     * Split the header name into a base and a size
     */
    static std::pair<cstring, int>  getHeaderBaseAndSize(cstring name) {
        auto len = name.size();

        // End with b
        if (name.get(len-1) != 'b') return std::make_pair(name, 0);

        // Expect a sequence of digits, preceded by an underscore
        for (int idx = len - 2; idx >= 0; --idx) {
            if (name.get(idx) == '_') {
                if (idx < static_cast<int>(len) - 2) {
                    return std::make_pair(
                        name.substr(0, idx),
                        std::stoi(name.substr(idx + 1, (len - 1) - (idx + 1)).c_str()));
                }
            }
            if (!isdigit(name.get(idx))) return std::make_pair(name, 0);
        }
        return std::make_pair(name, 0);
    }

 public:
    explicit CreateAliasesForVarbitHeaders(const PhvInfo &phv, const ClotInfo &clot_info,
                                           PragmaAlias &pragma)
        : phv(phv), clot_info(clot_info), pragma(pragma) {}
};

MergeDesugaredVarbitValids::MergeDesugaredVarbitValids(const PhvInfo &phv,
                                                       const ClotInfo &clot_info,
                                                       PragmaAlias &pragma) {
    addPasses({
        new CreateAliasesForVarbitHeaders(phv, clot_info, pragma),
        // After creating the aliases, need to replace the aliased field in
        // the IR. Simplified version of the Alias pass.
        new FindExpressionsForFields(phv, field_expressions),
        new ReplaceAllAliases(phv, pragma, field_expressions),
    });
}
