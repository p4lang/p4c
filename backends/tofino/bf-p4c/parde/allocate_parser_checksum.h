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

#ifndef EXTENSIONS_BF_P4C_PARDE_ALLOCATE_PARSER_CHECKSUM_H_
#define EXTENSIONS_BF_P4C_PARDE_ALLOCATE_PARSER_CHECKSUM_H_

#include "ir/ir.h"
#include "lib/cstring.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/logging/pass_manager.h"
#include "parde_visitor.h"

/**
 * @ingroup AllocateParserChecksums
 */
struct CollectParserChecksums : public ParserInspector {
    explicit CollectParserChecksums(const CollectParserInfo& parser_info) :
        parser_info(parser_info) { }

    const CollectParserInfo& parser_info;

    ordered_map<const IR::BFN::Parser*,
        ordered_set<cstring>> parser_to_decl_names;

    ordered_map<const IR::BFN::Parser*,
        ordered_set<cstring>> parser_to_verifies;

    ordered_map<const IR::BFN::Parser*,
        ordered_set<cstring>> parser_to_residuals;

    ordered_map<const IR::BFN::Parser*,
        ordered_set<cstring>> parser_to_clots;

    ordered_map<const IR::BFN::Parser*,
        ordered_map<cstring,
            std::vector<const IR::BFN::ParserChecksumPrimitive*>>> decl_name_to_prims;

    ordered_map<const IR::BFN::Parser*,
        ordered_map<cstring,
            ordered_set<const IR::BFN::ParserState*>>> decl_name_to_states;

    ordered_map<const IR::BFN::ParserState*,
        std::vector<const IR::BFN::ParserChecksumPrimitive*>> state_to_prims;

    ordered_map<const IR::BFN::ParserChecksumPrimitive*,
             const IR::BFN::ParserState*> prim_to_state;

    profile_t init_apply(const IR::Node* root) override {
        parser_to_decl_names.clear();
        parser_to_verifies.clear();
        parser_to_residuals.clear();
        parser_to_clots.clear();
        decl_name_to_prims.clear();
        decl_name_to_states.clear();
        state_to_prims.clear();
        prim_to_state.clear();
        return Inspector::init_apply(root);
    }
    bool preorder(const IR::BFN::Parser* parser) override {
        auto sorted_states = parser_info.graph(parser).topological_sort();

        for (auto state : sorted_states)
            for (auto stmt : state->statements)
                if (auto csum = stmt->to<IR::BFN::ParserChecksumPrimitive>())
                    visit(parser, state, csum);

        return false;
    }

    void visit(const IR::BFN::Parser* parser,
               const IR::BFN::ParserState* state,
               const IR::BFN::ParserChecksumPrimitive* csum) {
        parser_to_decl_names[parser].insert(csum->declName);
        prim_to_state[csum] = state;
        state_to_prims[state].push_back(csum);
        decl_name_to_states[parser][csum->declName].insert(state);
        decl_name_to_prims[parser][csum->declName].push_back(csum);
    }

    bool is_verification(const IR::BFN::Parser* parser, cstring decl) const {
        for (auto p : decl_name_to_prims.at(parser).at(decl)) {
            if (!p->is<IR::BFN::ChecksumAdd>() && !p->is<IR::BFN::ChecksumVerify>())
                return false;
        }
        return true;
    }

    bool is_residual(const IR::BFN::Parser* parser, cstring decl) const {
        for (auto p : decl_name_to_prims.at(parser).at(decl)) {
            if (!p->is<IR::BFN::ChecksumSubtract>() && !p->is<IR::BFN::ChecksumResidualDeposit>())
                return false;
        }
        return true;
    }

    bool is_clot(const IR::BFN::Parser* parser, cstring decl) const {
        for (auto p : decl_name_to_prims.at(parser).at(decl)) {
            if (!p->is<IR::BFN::ChecksumAdd>() && !p->is<IR::BFN::ChecksumDepositToClot>())
                return false;
        }
        return true;
    }

    IR::BFN::ChecksumMode get_type(const IR::BFN::Parser* parser, cstring name) const {
        if (is_verification(parser, name))
            return IR::BFN::ChecksumMode::VERIFY;
        else if (is_residual(parser, name))
            return IR::BFN::ChecksumMode::RESIDUAL;
        else if (is_clot(parser, name))
            return IR::BFN::ChecksumMode::CLOT;
        else
            BUG("Unknown checksum type for %1%", name);
    }

    void end_apply() override {
        for (auto& pd : parser_to_decl_names) {
            for (auto decl : pd.second) {
                if (is_verification(pd.first, decl))
                    parser_to_verifies[pd.first].insert(decl);
                else if (is_residual(pd.first, decl))
                    parser_to_residuals[pd.first].insert(decl);
                else if (is_clot(pd.first, decl))
                    parser_to_clots[pd.first].insert(decl);
                else
                    error("Inconsistent use of checksum declaration %1% (it can be used either "
                            "to verify checksum or to calculate residual, but not for both)",
                            decl);  // or for CLOTs but that should not be visible to users
            }
        }
    }
};

/**
 * @defgroup AllocateParserChecksums AllocateParserChecksums
 * @ingroup LowerParserIR
 * @brief Allocates parser checksums.
 */
class AllocateParserChecksums : public Logging::PassManager {
    CollectParserInfo      parser_info;
    CollectParserChecksums checksum_info;

 public:
    ordered_map<const IR::BFN::Parser*,
        ordered_map<cstring, unsigned>> decl_to_checksum_id;

    ordered_map<const IR::BFN::Parser*,
        ordered_map<unsigned, ordered_set<cstring>>> checksum_id_to_decl;

    ordered_map<const IR::BFN::Parser*,
        ordered_map<cstring, ordered_set<const IR::BFN::ParserState*>>> decl_to_start_states;

    ordered_map<const IR::BFN::Parser*,
        ordered_map<cstring, ordered_set<const IR::BFN::ParserState*>>> decl_to_end_states;

    AllocateParserChecksums(const PhvInfo& phv, const ClotInfo& clot);

    bool is_start_state(const IR::BFN::Parser* parser, cstring name,
                        const IR::BFN::ParserState* state) const {
        return decl_to_start_states.at(parser).at(name).count(state);
    }

    bool is_end_state(const IR::BFN::Parser* parser, cstring name,
                      const IR::BFN::ParserState* state) const {
        return decl_to_end_states.at(parser).at(name).count(state);
    }

    IR::BFN::ChecksumMode get_type(const IR::BFN::Parser* parser, cstring name) const {
        return checksum_info.get_type(parser, name);
    }

    unsigned get_id(const IR::BFN::Parser* parser, cstring name) const {
        return decl_to_checksum_id.at(parser).at(name);
    }
};

#endif  /* EXTENSIONS_BF_P4C_PARDE_ALLOCATE_PARSER_CHECKSUM_H_ */
