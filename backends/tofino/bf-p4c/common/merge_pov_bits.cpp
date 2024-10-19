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

#include "bf-p4c/common/merge_pov_bits.h"

#include "bf-p4c/parde/clot/header_validity_analysis.h"
#include "bf-p4c/parde/parde_visitor.h"

namespace BFN {

/**
 * @ingroup common
 * @brief Identify POV bits that can be merged.
 *
 * Two POV bits can be merged if the headers are always extracted together,
 * and if their validity remains in sync in MAU (always invalidated/validated
 * together). The extractions can occur in the same state or in consecutive
 * states.
 *
 * FIXME: Handle headers that are not extracted but which are made valid in
 * MAU.
 */
class IdentifyPovMergeTargets : public ParserInspector {
 protected:
    const PhvInfo &phv;
    const CollectParserInfo &parser_info;
    const HeaderValidityAnalysis &hva;
    ordered_map<const PHV::Field *, const PHV::Field *> &merge_pov;

    ordered_map<const PHV::Field *, std::set<const IR::BFN::ParserState *>> pov_states;
    ordered_map<const IR::BFN::ParserState *, ordered_set<const PHV::Field *>> state_povs;
    std::set<const PHV::Field *> pov_zero_writes;

    Visitor::profile_t init_apply(const IR::Node *root) override {
        auto rv = Inspector::init_apply(root);
        merge_pov.clear();
        pov_states.clear();
        state_povs.clear();
        pov_zero_writes.clear();
        return rv;
    }

    bool preorder(const IR::BFN::Extract *e) override {
        const auto *f = phv.field(e->dest->field);
        CHECK_NULL(f);

        // Skip temp fields: these like correspond to special valid bits that were added
        // FIXME: Is this necessary? Curretnly the fieldNameToExpressionMap does not contain
        // the valid bit because it's only adding HeaderOrMetadata objects.
        if (e->dest->field->is<IR::TempVar>()) return true;

        // Don't attempt to merge $stkvalid
        if (f->name.endsWith("$stkvalid")) return true;

        const auto *src = e->source->to<IR::BFN::ConstantRVal>();
        if (f->pov && src) {
            if (src->constant->value == 1) {
                const auto *state = findContext<IR::BFN::ParserState>();
                LOG3("Found POV write: " << f << " in " << state->name);
                pov_states[f].emplace(state);
                state_povs[state].emplace(f);
            } else if (src->constant->value == 0) {
                const auto *state = findContext<IR::BFN::ParserState>();
                LOG3("Found POV zero-write: " << f << " in " << state->name);
                pov_zero_writes.emplace(f);
            }
        }

        return true;
    }

    void end_apply() override {
        // Two POVs can only be merged if the header validity is not updated in
        // MAU or are updated in sync (both set valid or invalid together)
        auto validity_update_compatible = [this](const PHV::Field *f1, const PHV::Field *f2) {
            return ((hva.povBitsUpdateOrInvalidateActions.count(f1) == 0 &&
                     hva.povBitsUpdateOrInvalidateActions.count(f2) == 0) ||
                    hva.povBitsAlwaysInvalidateTogether(f1->id, f2->id)) &&
                   ((hva.povBitsUpdateOrValidateActions.count(f1) == 0 &&
                     hva.povBitsUpdateOrValidateActions.count(f2) == 0) ||
                    hva.povBitsAlwaysValidateTogether(f1->id, f2->id));
        };

        // Identify headers in the same state that can share a POV bit
        std::set<std::pair<const PHV::Field *, const PHV::Field *>> seen_pairs;
        std::set<const PHV::Field *> merged;
        for (auto &[state, fields] : state_povs) {
            for (auto i1 = fields.begin(); i1 != fields.end(); ++i1) {
                const auto *f1 = *i1;
                if (pov_zero_writes.count(f1)) continue;
                for (auto i2 = std::next(i1); i2 != fields.end(); ++i2) {
                    const auto *f2 = *i2;
                    if (pov_zero_writes.count(f2)) continue;

                    // Check whether we've processed this pair in any order in
                    // case two states extract the same pair in the reverse order.
                    auto f1_f2 = std::make_pair(f1, f2);
                    auto f2_f1 = std::make_pair(f2, f1);
                    if (seen_pairs.count(f1_f2) || seen_pairs.count(f2_f1)) continue;

                    // Only merge a field once. (Iteratively propagate later.)
                    if (merged.count(f2)) continue;

                    bool same = pov_states[f1] == pov_states[f2];
                    if (same && validity_update_compatible(f1, f2)) {
                        merge_pov.emplace(f2, f1);
                        merged.emplace(f2);
                    }
                    seen_pairs.emplace(f1_f2);
                }
            }
        }

        if (LOGGING(3)) {
            LOG3("POVs in the same state that can be merged:");
            for (const auto &[f1, f2] : merge_pov) {
                LOG3("  " << f1->name << " -> " << f2->name);
            }
        }

        // Identify POVs in adjacent states that can be merged
        for (auto i1 = pov_states.begin(); i1 != pov_states.end(); ++i1) {
            for (auto i2 = std::next(i1); i2 != pov_states.end(); ++i2) {
                const auto *f1 = i1->first;
                const auto *f2 = i2->first;
                if (f1->gress != f2->gress) continue;

                const auto &f1_states = i1->second;
                const auto &f2_states = i2->second;

                auto &graph = parser_info.graph(*f1_states.begin());
                auto &preds = graph.predecessors();
                auto &succs = graph.successors();

                std::set<const IR::BFN::ParserState *> f1_state_preds;
                std::set<const IR::BFN::ParserState *> f1_state_succs;

                std::set<const IR::BFN::ParserState *> f2_state_preds;
                std::set<const IR::BFN::ParserState *> f2_state_succs;

                // Don't know whether f1 or f2 is extracted first, so consider
                // both orderings of f1 states and f2 states.
                bool preds_ok = true;
                bool succs_ok = true;
                for (const auto *state : f1_states) {
                    // If there's a branch to the pipe or a loopback transition
                    // then we can't merge the POV bits from different states.
                    succs_ok &= graph.to_pipe(state).size() == 0;
                    succs_ok &= !graph.is_loopback_state(state->name);

                    if (preds.count(state))
                        f1_state_preds.insert(preds.at(state).begin(), preds.at(state).end());
                    if (succs.count(state))
                        f1_state_succs.insert(succs.at(state).begin(), succs.at(state).end());
                }

                for (const auto *state : f2_states) {
                    // If there's a branch to the pipe or a loopback transition
                    // then we can't merge the POV bits from different states.
                    preds_ok &= graph.to_pipe(state).size() == 0;
                    preds_ok &= !graph.is_loopback_state(state->name);

                    if (preds.count(state))
                        f2_state_preds.insert(preds.at(state).begin(), preds.at(state).end());
                    if (succs.count(state))
                        f2_state_succs.insert(succs.at(state).begin(), succs.at(state).end());
                }

                if (preds_ok && f1_state_preds == f2_states && f2_state_succs == f1_states &&
                    validity_update_compatible(f1, f2))
                    merge_pov.emplace(f1, f2);
                if (succs_ok && f1_state_succs == f2_states && f2_state_preds == f1_states &&
                    validity_update_compatible(f1, f2))
                    merge_pov.emplace(f2, f1);
            }
        }

        if (LOGGING(3)) {
            LOG3("POVs that can be merged (targets may still point to other sources):");
            for (const auto &[f1, f2] : merge_pov) {
                LOG3("  " << f1->name << " -> " << f2->name);
            }
        }

        // Find the "base" of all merge candidates.
        // For example, if we have f3 -> f2 and f2 -> f1, then we need to update
        // resolve the f3 merge to f3 -> f1.
        bool changed = false;
        do {
            changed = false;
            ordered_map<const PHV::Field *, const PHV::Field *> merge_pov_new;
            for (const auto &[f1, f2] : merge_pov) {
                if (merge_pov.count(f2)) {
                    merge_pov_new.emplace(f1, merge_pov.at(f2));
                    changed = true;
                } else {
                    merge_pov_new.emplace(f1, f2);
                }
            }
            merge_pov = merge_pov_new;
        } while (changed);

        LOG1("POVs to be merged:");
        for (const auto &[f1, f2] : merge_pov) {
            LOG1("  " << f1->name << " -> " << f2->name);
        }
    }

 public:
    explicit IdentifyPovMergeTargets(const PhvInfo &phv, const CollectParserInfo &parser_info,
                                     const HeaderValidityAnalysis &hva,
                                     ordered_map<const PHV::Field *, const PHV::Field *> &merge_pov)
        : phv(phv), parser_info(parser_info), hva(hva), merge_pov(merge_pov){};
};

/**
 * @ingroup common
 * @brief Merge POV bits
 *
 * Merge POV bits to allow FDE entries to be combined. Useful to increase deparser FDE packing
 * density when there are many small headers.
 *
 * An earlier pass identifies the sources and targets for POV merges.
 *
 * The merging performs these operations:
 *  - Parser: eliminates constant-one extracts of the POV sources (only the extract to the target
 *    needs to be executed).
 *  - MAU: set valid/invalid of the sources are eliminated (only set valid/invalid on the
 *    destinations needs to be kept.
 *  - Deparser: source pov bits in emit statements are replaced by aliases to the target pov bits.
 *  - General: member references to source pov bits statements are replaced by aliases to the target
 *    pov bits.
 *
 * Header stacks are handled differently because valid bits are
 * already aliases to stkvalid slices. In this case:
 *  - Push/pop operations (recorded as $stkvalid manipulations in HeaderValidityAnalysis) override
 *    merging any POV bits in the stack.
 *  - Parser extracts and MAU operations are kept in order to maintain the correctness of all bits
 *    in the stkvalid vector. (Pushing/popping requires that all bits be updated.)
 *  - Deparser source pov bit references are replaced with target pov bit references (cannot replace
 *    with an alias because the $valid bits are already aliased to $stkvalid slices and this would
 *    cause conflicting aliasing).
 */
class UpdatePovBits : public Transform {
 protected:
    const PhvInfo &phv;
    const HeaderValidityAnalysis &hva;
    const ordered_map<const PHV::Field *, const PHV::Field *> &merge_pov;

    /// Map of IR::Expression objects corresponding to the field names.
    ordered_map<cstring, const IR::Member *> fieldNameToExpressionsMap;

    /// Names of stack POV bits
    std::set<cstring> stack_pov_bits;
    std::set<cstring> skip_stack_pov_bits;

    Visitor::profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Transform::init_apply(root);
        fieldNameToExpressionsMap.clear();
        return rv;
    }

    const IR::Node *preorder(IR::HeaderOrMetadata *h) override {
        LOG5("Header: " << h->name);
        LOG5("Header type: " << h->type);
        const IR::HeaderStack *hs = h->to<IR::HeaderStack>();
        if (!hs) hs = h->type->to<IR::HeaderStack>();
        if (hs) {
            cstring stkvalid_name = h->name + ".$stkvalid"_cs;
            const auto *stkvalid = phv.field(stkvalid_name);
            bool skip = hva.povBitsUpdateActions.count(stkvalid);
            for (int i = 0; i < hs->size; i++) {
                cstring name = h->name + "[" + cstring::to_cstring(i) + "].$valid";
                stack_pov_bits.emplace(name);
                if (skip) skip_stack_pov_bits.emplace(name);
                IR::Member *mem = new IR::Member(
                    IR::Type_Bits::get(1),
                    new IR::HeaderStackItemRef(new IR::ConcreteHeaderRef(h), new IR::Constant(i)),
                    "$valid");
                if (mem) {
                    fieldNameToExpressionsMap[name] = mem;
                    LOG5("  Added stack field: " << name << ", " << mem);
                }
            }
        } else if (h->type->is<IR::Type_Header>()) {
            cstring name = h->name + ".$valid"_cs;
            IR::Member *mem =
                new IR::Member(IR::Type_Bits::get(1), new IR::ConcreteHeaderRef(h), "$valid");
            if (mem) {
                fieldNameToExpressionsMap[name] = mem;
                LOG5("  Added field: " << name << ", " << mem);
            }
        }
        return h;
    }

    const IR::Node *preorder(IR::BFN::Emit *emit) override {
        const auto *povBit = phv.field(emit->povBit->field);

        if (merge_pov.count(povBit)) {
            const IR::Member *newMember = nullptr;
            if (stack_pov_bits.count(povBit->name) && skip_stack_pov_bits.count(povBit->name)) {
                LOG1("Skipping " << povBit->name);
                return emit;
            }

            newMember = fieldNameToExpressionsMap.at(merge_pov.at(povBit)->name);
            LOG1("Replacing POV reference in deparser emit: " << emit->povBit->field << " -> "
                                                              << newMember);

            auto *newPovBit = emit->povBit->clone();
            newPovBit->field = newMember;
            emit->povBit = newPovBit;
        }

        return emit;
    }

    const IR::Node *preorder(IR::BFN::ChecksumEntry *csum) override {
        const auto *povBit = phv.field(csum->povBit->field);

        if (merge_pov.count(povBit)) {
            const IR::Member *newMember = nullptr;
            if (stack_pov_bits.count(povBit->name) && skip_stack_pov_bits.count(povBit->name)) {
                LOG1("Skipping " << povBit->name);
                return csum;
            }

            newMember = fieldNameToExpressionsMap.at(merge_pov.at(povBit)->name);
            LOG1("Replacing POV reference in deparser checksum entry: " << csum->povBit->field
                                                                        << " -> " << newMember);

            auto *newPovBit = csum->povBit->clone();
            newPovBit->field = newMember;
            csum->povBit = newPovBit;
        }
        return csum;
    }

 public:
    explicit UpdatePovBits(const PhvInfo &phv, const HeaderValidityAnalysis &hva,
                           const ordered_map<const PHV::Field *, const PHV::Field *> &merge_pov)
        : phv(phv), hva(hva), merge_pov(merge_pov){};
};

/**
 * @ingroup common
 * @brief Eliminate unnecessary parser zero-writes to validity bits
 */
class ElimParserValidZeroWrites : public ParserTransform {
 protected:
    const PhvInfo &phv;
    const CollectParserInfo &parser_info;
    const MapFieldToParserStates &fs;

    const IR::Node *preorder(IR::BFN::Extract *e) override {
        const auto *f = phv.field(e->dest->field);
        if (!f || !f->pov) return e;

        const auto *src = e->source->to<IR::BFN::ConstantRVal>();
        if (f->pov && src) {
            if (src->constant->value == 0) {
                const auto &states = fs.field_to_parser_states.at(f);
                const auto *state = findOrigCtxt<IR::BFN::ParserState>();
                const auto &graph = parser_info.graph(state);
                bool first_write = true;
                for (const auto *otherState : states) {
                    if (state == otherState) continue;
                    first_write &= !graph.is_ancestor(otherState, state);
                }
                if (first_write) {
                    LOG1("Eliminating unnecessary zero-write to POV in " << state->name << ": "
                                                                         << e);
                    return nullptr;
                }
            }
        }

        return e;
    }

 public:
    explicit ElimParserValidZeroWrites(const PhvInfo &phv, const CollectParserInfo &parser_info,
                                       const MapFieldToParserStates &fs)
        : phv(phv), parser_info(parser_info), fs(fs) {}
};

MergePovBits::MergePovBits(const PhvInfo &phv) {
    auto *parser_info = new CollectParserInfo;
    auto *header_validity_analysis = new HeaderValidityAnalysis(phv, {});
    auto *field_to_parser_states = new MapFieldToParserStates(phv);

    addPasses({
        new PassRepeated(
            {parser_info, field_to_parser_states,
             new ElimParserValidZeroWrites(phv, *parser_info, *field_to_parser_states)}),
        parser_info,
        header_validity_analysis,
        new IdentifyPovMergeTargets(phv, *parser_info, *header_validity_analysis, merge_pov),
        new UpdatePovBits(phv, *header_validity_analysis, merge_pov),
    });
}

}  // namespace BFN
