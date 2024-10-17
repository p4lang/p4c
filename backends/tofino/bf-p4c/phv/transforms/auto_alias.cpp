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

#include "bf-p4c/phv/transforms/auto_alias.h"

Visitor::profile_t DetermineCandidateHeaders::init_apply(const IR::Node* root) {
    allHeaders.clear();
    headers.clear();
    headersValidatedInParser.clear();
    headersValidatedInMAU.clear();
    for (const auto& f : phv)
        allHeaders.insert(f.header());;
    return Inspector::init_apply(root);
}

bool DetermineCandidateHeaders::preorder(const IR::BFN::Extract* extract) {
    BUG_CHECK(extract->dest, "Extract %1% does not have a destination",
            cstring::to_cstring(extract));
    BUG_CHECK(extract->dest->field, "Extract %1% does not have a destination field",
            cstring::to_cstring(extract));
    const PHV::Field* field = phv.field(extract->dest->field);
    BUG_CHECK(field, "Could not find destination field for extract %1%",
            cstring::to_cstring(extract));
    if (!field->pov) return true;
    headersValidatedInParser.insert(field->header());
    LOG3("\t  Header validated in parser: " << field->header());
    return true;
}

bool DetermineCandidateHeaders::preorder(const IR::MAU::Instruction* inst) {
    const IR::MAU::Action* action = findContext<IR::MAU::Action>();
    if (inst->operands.empty()) return true;
    const PHV::Field* destField = phv.field(inst->operands[0]);
    if (!destField) return true;
    if (!destField->pov) return true;
    if (inst->name != "set") return true;
    if (inst->operands.size() != 2) return true;
    auto* constant = inst->operands[1]->to<IR::Constant>();
    if (constant) {
        uint64_t constVal = constant->asUint64();
        if (constVal == 0) return true;
    }
    BUG_CHECK(action != nullptr,
        "No associated action found for determining candidate header - %1%", inst->name);
    headersValidatedInMAU[destField->header()].insert(action);
    LOG3("\t  Header validated in action " << action->name << ": " << destField->header());
    return true;
}

void DetermineCandidateHeaders::end_apply() {
    for (const auto header : allHeaders) {
        if (headersValidatedInParser.count(header)) continue;
        if (headersValidatedInMAU.count(header))
            headers[header].insert(headersValidatedInMAU.at(header).begin(),
                    headersValidatedInMAU.at(header).end());
    }
    LOG3("\tNumber of candidate headers: " << headers.size());
    for (auto& kv : headers) {
        std::stringstream ss;
        ss << "\t  " << kv.first << " : ";
        for (const auto* action : kv.second)
            ss << action->name << " ";
        LOG3(ss.str());
    }
}

Visitor::profile_t DetermineCandidateFields::init_apply(const IR::Node* root) {
    initialCandidateSet.clear();
    candidateSources.clear();
    LOG3("\tInitial candidate fields:");
    for (const auto& f : phv) {
        if (f.pov) continue;
        if (!headers.isCandidateHeader(f.header())) continue;
        initialCandidateSet.insert(&f);
        LOG3("\t  " << f.name);
    }
    return Inspector::init_apply(root);
}

inline void DetermineCandidateFields::dropFromCandidateSet(const PHV::Field* field) {
    initialCandidateSet.erase(field);
}

bool DetermineCandidateFields::multipleSourcesFound(
        const PHV::Field* dest,
        const PHV::Field* src) const {
    if (!candidateSources.count(dest)) return false;
    bool newSourceFound = false;
    for (auto& kv : candidateSources)
        if (kv.first != src)
            newSourceFound = true;
    return newSourceFound;
}

//
// There are many cases we can't alias two fields, filtering them here ...
//
// (Music suggestion when modifying code here: Fixing a hole - The Beatles)
// :)
bool DetermineCandidateFields::incompatibleConstraints(
        const PHV::Field* dest,
        const PHV::Field* src) const {
    // case1: non-byte aligned arithmetic dests require whole container
    if (src->size % 8 != 0) {
        if (d2i.dest_to_inst.count(src)) {
            for (auto inst : d2i.dest_to_inst.at(src)) {
                if (inst->name == "add" || inst->name == "sub")
                    return true;
            }
        }
    }

    // case2: incompatible alignment and starting bits.
    const auto alignment_ok = [](const PHV::Field* a, const PHV::Field* b) {
        if (a->alignment) {
            const auto align = a->alignment->align;
            if (b->alignment && b->alignment->align != align) {
                return false;
            }
            // must found at least one overlapped start bit.
            for (auto container_size : Device::phvSpec().containerSizes()) {
                const bitvec valid_starts = b->getStartBits(container_size);
                for (int i = align; i <= int(container_size) - 1; i += 8) {
                    if (valid_starts.getbit(i)) {
                        return true;
                    }
                }
            }
            return false;
        }
        return true;
    };
    if (!alignment_ok(dest, src) || !alignment_ok(src, dest)) {
        return true;
    }

    // more to come ...

    return false;
}

bool DetermineCandidateFields::preorder(const IR::MAU::Instruction* inst) {
    const IR::MAU::Action* action = findContext<IR::MAU::Action>();
    if (inst->operands.empty()) return true;
    const PHV::Field* destField = phv.field(inst->operands[0]);
    if (!destField) return true;

    for (auto &candidate : candidateSources) {
        //  see if the destination of the current instruction has been selected
        //  as the source for an alias previously
        if (candidate.second.count(destField)) {
            dropFromCandidateSet(candidate.first);
            LOG3("\tDrop candidate header : " << candidate.first->name
                                    << " because it's source : "
                                    << destField->name
                                    << " is written in further traversal");
            return true;
        }
    }
    if (!initialCandidateSet.count(destField)) return true;
    if (inst->name != "set" || inst->operands.size() != 2) {
        dropFromCandidateSet(destField);
        return true;
    }
    if (inst->operands[1]->is<IR::Constant>()) {
        dropFromCandidateSet(destField);
        return true;
    }
    const PHV::Field* srcField = phv.field(inst->operands[1]);
    if (!srcField) {
        dropFromCandidateSet(destField);
        LOG3("\tDrop " << destField->name << " because source is not a field.");
        return true;
    }
    if (multipleSourcesFound(destField, srcField)) {
        dropFromCandidateSet(destField);
        LOG3("\tDrop " << destField->name << " because a second source " << srcField->name <<
             " found");
        return true;
    }
    if (incompatibleConstraints(destField, srcField)) {
        dropFromCandidateSet(destField);
        LOG3("\tDrop " << destField->name << " because of incompatible constraints with "
             << srcField->name);
        return true;
    }
    if (!no_overlay.can_overlay(destField, srcField)) {
        dropFromCandidateSet(destField);
        LOG3("\tDrop " << destField->name << " because of non-overlay pragma for source "
             << srcField->name);
        return true;
    }

    auto& validationActions = headers.getActionsForCandidateHeader(destField->header());
    LOG3("\t  Found " << validationActions.size() << " actions");
    BUG_CHECK(action != nullptr,
        "No associated action found for determining candidate fields - %1%", inst->name);
    if (!validationActions.count(action)) {
        dropFromCandidateSet(destField);
        LOG3("\tDrop " << destField->name << " because the header is not validated in action " <<
             action->name);
        return true;
    }
    LOG3("\tAdding write to field " << destField->name << " from " << srcField->name);
    candidateSources[destField][srcField].insert(action);
    return true;
}

void DetermineCandidateFields::end_apply() {
    LOG3("\tFinal auto-alias candidate set:");
    for (const auto* f : initialCandidateSet) {
        if (!candidateSources.count(f)) continue;
        std::stringstream ss;
        ss << "\t  auto-alias: " << f->name << ", ";
        if (candidateSources.at(f).size() > 1) continue;
        const PHV::Field* srcField = nullptr;
        for (auto& kv : candidateSources.at(f)) {
            srcField = kv.first;
            ss << kv.first->name << ", ";
            for (const auto* action : kv.second)
                ss << action->name << " ";
        }
        if (srcField != nullptr) {
            if (!pragma.addAlias(f, srcField, true, PragmaAlias::COMPILER))
                LOG1("\tCould not add alias for fields " << f->name << " and " << srcField->name);
            else
                LOG1(ss.str());
        }
    }
}
