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

#include "header_validity_analysis.h"

HeaderValidityAnalysis::HeaderValidityAnalysis(const PhvInfo &phvInfo,
                                               const std::set<FieldSliceSet> &correlations)
    : phvInfo(phvInfo), resultMap() {
    // Build the index of correlations.
    interestedCorrelations =
        new std::map<const PHV::FieldSlice *, std::set<FieldSliceSet>, PHV::FieldSlice::Less>;
    for (auto &correlation : correlations) {
        for (const auto *povBit : correlation) {
            (*interestedCorrelations)[povBit].insert(correlation);
        }
    }
}

Visitor::profile_t HeaderValidityAnalysis::init_apply(const IR::Node *root) {
    auto result = Inspector::init_apply(root);

    resultMap.clear();
    for (auto &correlations : Values(*interestedCorrelations)) {
        for (auto &correlation : correlations) {
            resultMap[correlation] = {{}};
        }
    }

    povBitsSetInvalidInMau.clear();
    povBitsSetValidInMau.clear();

    povBitsUpdateActions.clear();
    povBitsUpdateOrInvalidateActions.clear();
    povBitsUpdateOrValidateActions.clear();
    povBitsInvalidateActions.clear();
    povBitsValidateActions.clear();

    povBitsAlwaysInvalidateTogether.clear();
    povBitsAlwaysValidateTogether.clear();

    return result;
}

bool HeaderValidityAnalysis::preorder(const IR::MAU::Action *act) {
    act->action.visit_children(*this);
    return false;
}

bool HeaderValidityAnalysis::preorder(const IR::MAU::Instruction *instruction) {
    // Make sure we have a call to "set".
    if (instruction->name != "set") return true;

    auto dst = instruction->operands.at(0);
    auto src = instruction->operands.at(1);

    // Make sure we are setting a header's validity bit.
    le_bitrange bitrange;
    const PHV::Field *dstField = nullptr;
    if (auto alias_slice = dst->to<IR::BFN::AliasSlice>())
        dstField = phvInfo.field(alias_slice->source, &bitrange);
    else
        dstField = phvInfo.field(dst, &bitrange);
    if (!dstField || !dstField->pov) return true;

    // Handle case where we are assigning a non-zero constant to the validity bit. Conservatively,
    // we assume that assigning a non-constant value to the POV bit can clear the bit.
    bool definitely_invalidated = false;
    bool definitely_validated = false;
    if (auto constant = src->to<IR::Constant>()) {
        definitely_invalidated = constant->value == 0;
        definitely_validated = constant->value != 0;
    }
    if (definitely_invalidated || (!definitely_invalidated && !definitely_validated))
        povBitsSetInvalidInMau.insert(dstField);
    if (definitely_validated || (!definitely_invalidated && !definitely_validated))
        povBitsSetValidInMau.insert(dstField);

    // Record the action to allow identifying when multiple headers are invalidated together
    const auto *action = findContext<IR::MAU::Action>();
    povBitsUpdateActions[dstField].insert(action);
    if (definitely_invalidated || (!definitely_invalidated && !definitely_validated))
        povBitsUpdateOrInvalidateActions[dstField].insert(action);
    if (definitely_validated || (!definitely_invalidated && !definitely_validated))
        povBitsUpdateOrValidateActions[dstField].insert(action);
    if (definitely_invalidated) povBitsInvalidateActions[dstField].insert(action);
    if (definitely_validated) povBitsValidateActions[dstField].insert(action);

    // Correlations only currently apply to invalidations -- don't do more
    // processing for validations.
    if (definitely_validated) return true;

    // Ignore if the validity bit is uninteresting to the client.
    const auto *povBit = new PHV::FieldSlice(dstField, bitrange);
    if (!interestedCorrelations->count(povBit)) return true;

    // Mark the POV bit as being cleared.
    for (auto &interestedCorrelation : interestedCorrelations->at(povBit)) {
        std::set<FieldSliceSet> rewritten = {};
        for (auto correlation : resultMap.at(interestedCorrelation)) {
            correlation.insert(povBit);
            rewritten.emplace(correlation);
        }

        std::swap(resultMap.at(interestedCorrelation), rewritten);
    }

    return true;
}

void HeaderValidityAnalysis::end_apply() {
    bitvec fields_encountered;
    for (auto it1 = povBitsUpdateOrInvalidateActions.begin();
         it1 != povBitsUpdateOrInvalidateActions.end(); ++it1) {
        auto *f1 = it1->first;
        auto &update_actions1 = it1->second;
        povBitsAlwaysInvalidateTogether(f1->id, f1->id) = true;
        fields_encountered[f1->id] = true;
        for (auto it2 = std::next(it1); it2 != povBitsUpdateOrInvalidateActions.end(); ++it2) {
            auto *f2 = it2->first;
            auto &update_actions2 = it2->second;
            if (update_actions1 != update_actions2) continue;
            if (povBitsInvalidateActions.count(f1) && povBitsInvalidateActions.count(f2)) {
                auto &invalidate_actions1 = povBitsInvalidateActions.at(f1);
                auto &invalidate_actions2 = povBitsInvalidateActions.at(f2);

                if (update_actions1 != invalidate_actions1 ||
                    update_actions1 != invalidate_actions2)
                    continue;

                povBitsAlwaysInvalidateTogether(f1->id, f2->id) = true;
            }
        }
    }
    for (auto it1 = povBitsUpdateOrValidateActions.begin();
         it1 != povBitsUpdateOrValidateActions.end(); ++it1) {
        auto *f1 = it1->first;
        auto &update_actions1 = it1->second;
        povBitsAlwaysValidateTogether(f1->id, f1->id) = true;
        fields_encountered[f1->id] = true;
        for (auto it2 = std::next(it1); it2 != povBitsUpdateOrValidateActions.end(); ++it2) {
            auto *f2 = it2->first;
            auto &update_actions2 = it2->second;
            if (update_actions1 != update_actions2) continue;
            if (povBitsValidateActions.count(f1) && povBitsValidateActions.count(f2)) {
                auto &validate_actions1 = povBitsValidateActions.at(f1);
                auto &validate_actions2 = povBitsValidateActions.at(f2);

                if (update_actions1 != validate_actions1 || update_actions1 != validate_actions2)
                    continue;

                povBitsAlwaysValidateTogether(f1->id, f2->id) = true;
            }
        }
    }

    if (LOGGING(4)) {
        LOG4("pov bits always invalidated together:");
        for (auto it1 = fields_encountered.begin(); it1 != fields_encountered.end(); ++it1) {
            for (auto it2 = std::next(it1); it2 != fields_encountered.end(); ++it2) {
                if (povBitsAlwaysInvalidateTogether(*it1, *it2)) {
                    const PHV::Field *f1 = phvInfo.field(*it1);
                    CHECK_NULL(f1);
                    const PHV::Field *f2 = phvInfo.field(*it2);
                    CHECK_NULL(f2);
                    LOG4("(" << f1->name << ", " << f2->name << ")");
                }
            }
        }
        LOG4("pov bits always validated together:");
        for (auto it1 = fields_encountered.begin(); it1 != fields_encountered.end(); ++it1) {
            for (auto it2 = std::next(it1); it2 != fields_encountered.end(); ++it2) {
                if (povBitsAlwaysValidateTogether(*it1, *it2)) {
                    const PHV::Field *f1 = phvInfo.field(*it1);
                    CHECK_NULL(f1);
                    const PHV::Field *f2 = phvInfo.field(*it2);
                    CHECK_NULL(f2);
                    LOG4("(" << f1->name << ", " << f2->name << ")");
                }
            }
        }
    }
}

HeaderValidityAnalysis *HeaderValidityAnalysis::clone() const {
    return new HeaderValidityAnalysis(*this);
}

HeaderValidityAnalysis &HeaderValidityAnalysis::flow_clone() { return *clone(); }

void HeaderValidityAnalysis::flow_merge(Visitor &v) {
    HeaderValidityAnalysis &other = dynamic_cast<HeaderValidityAnalysis &>(v);

    povBitsSetInvalidInMau.insert(other.povBitsSetInvalidInMau.begin(),
                                  other.povBitsSetInvalidInMau.end());
    povBitsSetValidInMau.insert(other.povBitsSetValidInMau.begin(),
                                other.povBitsSetValidInMau.end());
    for (const auto *f : Keys(other.povBitsUpdateActions)) {
        povBitsUpdateActions[f].insert(other.povBitsUpdateActions[f].begin(),
                                       other.povBitsUpdateActions[f].end());
    }
    for (const auto *f : Keys(other.povBitsUpdateOrInvalidateActions)) {
        povBitsUpdateOrInvalidateActions[f].insert(
            other.povBitsUpdateOrInvalidateActions[f].begin(),
            other.povBitsUpdateOrInvalidateActions[f].end());
    }
    for (const auto *f : Keys(other.povBitsUpdateOrValidateActions)) {
        povBitsUpdateOrValidateActions[f].insert(other.povBitsUpdateOrValidateActions[f].begin(),
                                                 other.povBitsUpdateOrValidateActions[f].end());
    }
    for (const auto *f : Keys(other.povBitsInvalidateActions)) {
        povBitsInvalidateActions[f].insert(other.povBitsInvalidateActions[f].begin(),
                                           other.povBitsInvalidateActions[f].end());
    }
    for (const auto *f : Keys(other.povBitsValidateActions)) {
        povBitsValidateActions[f].insert(other.povBitsValidateActions[f].begin(),
                                         other.povBitsValidateActions[f].end());
    }
    for (auto &kv : other.resultMap) {
        resultMap.at(kv.first).insert(kv.second.begin(), kv.second.end());
    }
}

#if 0
void HeaderValidityAnalysis::flow_copy(::ControlFlowVisitor& v) {
    HeaderValidityAnalysis& other = dynamic_cast<HeaderValidityAnalysis&>(v);
    resultMap = other.resultMap;
}
#endif
