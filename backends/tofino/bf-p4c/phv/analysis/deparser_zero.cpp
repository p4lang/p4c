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

#include "bf-p4c/phv/analysis/deparser_zero.h"

Visitor::profile_t IdentifyDeparserZeroCandidates::init_apply(const IR::Node *root) {
    profile_t rv = Inspector::init_apply(root);
    candidateFields.clear();
    mauReadFields.clear();
    mauWrittenFields.clear();
    mauWrittenToZeroFields.clear();
    mauWrittenToNonZeroFields.clear();
    mauWrittenByNonSets.clear();
    return rv;
}

bool IdentifyDeparserZeroCandidates::preorder(const IR::MAU::Action *act) {
    auto *tbl = findContext<IR::MAU::Table>();
    ActionAnalysis aa(phv, false, false, tbl, red_info);
    ActionAnalysis::FieldActionsMap fieldActionsMap;
    aa.set_field_actions_map(&fieldActionsMap);
    act->apply(aa);
    for (auto &fieldAction : Values(fieldActionsMap)) {
        PHV::Field *write = phv.field(fieldAction.write.expr);
        BUG_CHECK(write, "Action %1% does not have a write?", fieldAction.write.expr);
        mauWrittenFields[write->id] = true;
        if (fieldAction.name != "set") mauWrittenByNonSets[write->id] = true;
        LOG4("\t\tField " << write->name << " written in action " << act->name);
        for (auto &readSrc : fieldAction.reads) {
            if (readSrc.type == ActionAnalysis::ActionParam::CONSTANT) {
                const IR::Expression *sourceExpr = readSrc.expr;
                if (!sourceExpr->is<IR::Constant>()) {
                    mauWrittenToNonZeroFields[write->id] = true;
                    LOG4("\t\tField " << write->name << " written by non-constant in "
                                      << act->name);
                    continue;
                }
                const IR::Constant *const_mem = sourceExpr->to<IR::Constant>();
                if (!const_mem->fitsLong()) {
                    mauWrittenToNonZeroFields[write->id] = true;
                    LOG4("\t\tField " << write->name << " set using a constant " << const_mem
                                      << " in " << act->name);
                } else if (const_mem->asLong() == 0) {
                    mauWrittenToZeroFields[write->id] = true;
                    LOG4("\t\tField " << write->name << " set to 0 in " << act->name);
                } else {
                    mauWrittenToNonZeroFields[write->id] = true;
                    LOG4("\t\tField " << write->name << " set to non-zero constant "
                                      << const_mem->asLong() << " in " << act->name);
                }
            } else if (readSrc.type == ActionAnalysis::ActionParam::PHV) {
                PHV::Field *read = phv.field(readSrc.expr);
                if (!read) continue;
                mauReadFields[read->id] = true;
                mauWrittenToNonZeroFields[write->id] = true;
                LOG4("\t\tField " << read->name << " read in " << act->name);
            } else if (readSrc.type == ActionAnalysis::ActionParam::ACTIONDATA) {
                // Assume that any write from action data could be a nonzero write.
                mauWrittenToNonZeroFields[write->id] = true;
                LOG4("\t\tField " << write->name << " written by action data in " << act->name);
            }
        }
    }
    return true;
}

bool IdentifyDeparserZeroCandidates::preorder(const IR::BFN::DigestFieldList *list) {
    for (auto source : list->sources) {
        if (auto temp = source->field->to<IR::TempVar>()) {
            if (temp->deparsed_zero) {
                auto field = phv.field(temp);
                candidateFields.insert(field);
            }
        }
    }
    return false;
}

void IdentifyDeparserZeroCandidates::eliminateNonByteAlignedFields() {
    LOG1("\tEliminating non byte aligned fields");
    ordered_set<const PHV::Field *> fieldsToBeRemoved;
    for (const auto *f : candidateFields) {
        if (f->size % 8 == 0 && f->offset % 8 == 0) continue;
        auto lo = f->offset % 8;
        auto minOffset = (lo == 0) ? f->offset : f->offset - lo;
        auto maxOffset = (8 * (((f->offset + f->size - 1) + 7) / 8)) - 1;
        bitvec totalOccupancy(f->offset - minOffset, f->size);
        LOG4("\t  Field f " << f << ", min offset: " << minOffset << ", max offset: " << maxOffset
                            << ", occupancy: " << totalOccupancy);
        // At this point, field f shares its byte with at least one other field.
        // [minOffset, maxOffset] is the range of offsets within the header that we are interested
        // in checking now. If the field f shares a byte with only deparsed zero fields, then allow
        // the deparsed zero optimization for f, otherwise remove f from the list of deparsed zero
        // fields.
        for (const auto *f1 : candidateFields) {
            if (f == f1) continue;
            if (f1->size % 8 == 0) continue;
            if (f->header() != f1->header()) continue;
            bool fAfterf1 = ((f->offset / 8) == ((f1->offset + f1->size - 1) / 8));
            bool f1Afterf = (((f->offset + f->size - 1) / 8) == (f1->offset / 8));
            if (!fAfterf1 && !f1Afterf) continue;
            // At this point, we know that fields f and f1 share the same byte.
            auto f1_lo = f1->offset % 8;
            auto min_f1_offset = (f1_lo == 0) ? f1->offset : f1->offset - f1_lo;
            auto max_f1_offset = (8 * (((f1->offset + f1->size - 1) + 7) / 8)) - 1;
            bitvec f1_occupancy(f1_lo, f1->size);
            if (min_f1_offset < minOffset) {
                totalOccupancy |= totalOccupancy << (minOffset - min_f1_offset);
                minOffset = min_f1_offset;
            } else if (minOffset < min_f1_offset) {
                f1_occupancy |= f1_occupancy << (min_f1_offset - minOffset);
                min_f1_offset = minOffset;
            }
            LOG4("\t    Field f1 " << f1 << ", occupancy: " << f1_occupancy << ", min offset: "
                                   << min_f1_offset << ", max_f1_offset: " << max_f1_offset);
            if (fAfterf1 || f1Afterf) {
                LOG4("\t\tFields " << f->name << " and " << f1->name << " share same byte.");
                totalOccupancy |= f1_occupancy;
            }
        }
        LOG4("\t\tTotal occupancy of relevant bits: " << totalOccupancy);
        if (totalOccupancy.popcount() % 8 != 0) fieldsToBeRemoved.insert(f);
    }
    for (const auto *f : fieldsToBeRemoved) candidateFields.erase(f);
}

void IdentifyDeparserZeroCandidates::end_apply() {
    LOG4("\tExamining fields for deparser zero optimization:");
    for (auto &f : phv) {
        if (f.isGhostField()) continue;
        if (f.metadata || f.pov) continue;
        if (f.padding) continue;
        if (f.bridged) continue;
        bool pragmaSpecified = pragmaFields.getNotParsedFields().count(&f);
        bool usedInParser = defuse.isUsedInParser(&f);
        if (!pragmaSpecified && usedInParser) continue;
        LOG4("\t  Nonbridged, nonmetadata, nonpov field specified as not parsed: " << f);
        if (mauReadFields[f.id]) {
            LOG4("\t\tField read in MAU");
            continue;
        }
        // Do not assign to zero container if the field is neither read nor written in the MAU.
        if (!mauWrittenFields[f.id]) {
            LOG4("\t\tField not written in MAU");
            continue;
        }
        if (mauWrittenByNonSets[f.id]) {
            LOG4("\t\tField written by non set operations");
            continue;
        }
        if (mauWrittenFields[f.id] &&
            (!mauWrittenToZeroFields[f.id] || mauWrittenToNonZeroFields[f.id])) {
            LOG4("\t\tField written by PHV/action data/non-zero constant in MAU: " << f);
            continue;
        }
        candidateFields.insert(&f);
    }
    eliminateNonByteAlignedFields();
    LOG1("\tCandidates for deparser zero optimization:");
    for (auto &f : phv) {
        if (!candidateFields.count(&f)) continue;
        LOG1("\t\t" << f);
        f.set_deparser_zero_candidate(true);
    }
}

IR::Node *ImplementDeparserZero::preorder(IR::BFN::Extract *extract) {
    auto *fieldLVal = extract->dest->to<IR::BFN::FieldLVal>();
    if (!fieldLVal) return extract;
    auto *f = phv.field(fieldLVal->field);
    if (!f) return extract;
    // For now, ignore CLOT fields in the deparser zero optimization.
    // TODO: Move deparser zero before CLOT allocation so that we save some bandwidth for
    // CLOTs.
    if (clots.fully_allocated(f)) return extract;
    if (candidateFields.count(f)) return nullptr;
    return extract;
}

IR::Node *ImplementDeparserZero::preorder(IR::MAU::Instruction *inst) {
    if (inst->operands.empty()) return inst;
    auto *dst = phv.field(inst->operands[0]);
    if (!dst) return inst;
    if (clots.fully_allocated(dst)) return inst;
    if (candidateFields.count(dst)) return nullptr;
    return inst;
}

DeparserZeroOptimization::DeparserZeroOptimization(PhvInfo &p, const FieldDefUse &d,
                                                   const ReductionOrInfo &ri,
                                                   const PragmaDeparserZero &pf,
                                                   const ClotInfo &c) {
    addPasses({new IdentifyDeparserZeroCandidates(p, d, ri, pf, candidateFields),
               new ImplementDeparserZero(p, candidateFields, c)});
}
