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

#include "bf-p4c/phv/analysis/mutex_overlay.h"

#include <sstream>
#include <typeinfo>

#include "ir/ir.h"
#include "lib/log.h"

Visitor::profile_t BuildParserOverlay::init_apply(const IR::Node *root) {
    auto rv = BuildMutex::init_apply(root);
    LOG4("Beginning BuildParserOverlay");
    return rv;
}

// Given the sets of parser states, k, v, in which two fields can be extracted
// respectively, return true if any state in k is in a loop with any state in v.
bool ExcludeParserLoopReachableFields::is_loop_reachable(
    const ordered_set<const IR::BFN::ParserState *> &k,
    const ordered_set<const IR::BFN::ParserState *> &v) {
    for (auto i : k) {
        for (auto j : v) {
            if (i != j) {
                auto p = fieldToStates.state_to_parser.at(i);
                auto q = fieldToStates.state_to_parser.at(j);
                if (p == q) {
                    if (parserInfo.graph(p).is_loop_reachable(i, j) ||
                        parserInfo.graph(p).is_loop_reachable(j, i)) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

const IR::Node *ExcludeParserLoopReachableFields::apply_visitor(const IR::Node *root,
                                                                const char *) {
    for (auto &kv : fieldToStates.field_to_parser_states) {
        for (auto &xk : fieldToStates.field_to_parser_states) {
            if (kv.first == xk.first || kv.first->gress != xk.first->gress) continue;

            if (is_loop_reachable(kv.second, xk.second)) {
                phv.removeFieldMutex(kv.first, xk.first);
                LOG3("Mark " << kv.first->name << " and " << xk.first->name
                             << " as non mutually exclusive (parser loop reachable)");
            }
        }
    }

    return root;
}

Visitor::profile_t BuildMetadataOverlay::init_apply(const IR::Node *root) {
    auto rv = BuildMutex::init_apply(root);
    LOG4("Beginning BuildMetadataOverlay");
    return rv;
}

void ExcludeAliasedHeaderFields::excludeAliasedField(const IR::Expression *alias) {
    // According to PragmaAlias::postorder, header fields can only be aliased with metadata fields,
    // and for these aliases, the header fields are chosen as the alias destination.
    const PHV::Field *aliasDestination = nullptr;
    if (auto aliasMem = alias->to<IR::BFN::AliasMember>()) {
        aliasDestination = phv.field(aliasMem);
    } else if (auto aliasSlice = alias->to<IR::BFN::AliasSlice>()) {
        aliasDestination = phv.field(aliasSlice);
    }
    BUG_CHECK(aliasDestination, "Reference to alias field %1% not found", alias);

    if (aliasDestination->isPacketField()) {
        LOG1("Marking field as never overlaid due to aliasing: " << aliasDestination);
        neverOverlay.setbit(aliasDestination->id);
    }
}

void ExcludeDeparsedIntrinsicMetadata::end_apply() {
    for (auto &f : phv) {
        if (f.pov || f.deparsed_to_tm() || f.is_invalidate_from_arch()) {
            LOG1("Marking field as never overlaid: " << f);
            neverOverlay.setbit(f.id);
        }
    }
}

void ExcludePragmaNoOverlayFields::end_apply() {
    for (auto *f : pragma.get_no_overlay_fields()) {
        LOG1("Marking field as never overlaid because of pa_no_overlay: " << f);
        neverOverlay.setbit(f->id);
    }
}

bool ExcludeMAUOverlays::preorder(const IR::MAU::Table *tbl) {
    LOG5("\tTable: " << tbl->name);
    ordered_set<PHV::Field *> keyFields;
    for (auto *key : tbl->match_key) {
        PHV::Field *field = phv.field(key->expr);
        if (!field) continue;
        keyFields.insert(field);
    }
    for (auto *f1 : keyFields) {
        for (auto *f2 : keyFields) {
            if (f1 == f2) continue;
            if (!addedFields.isAddedInMAU(f1) && !addedFields.isAddedInMAU(f2)) continue;
            phv.removeFieldMutex(f1, f2);
            LOG5("\t  Mark key fields for table "
                 << tbl->name << " as non mutually exclusive: " << f1->name << ", " << f2->name);
        }
    }
    return true;
}

bool ExcludeMAUOverlays::preorder(const IR::MAU::Instruction *inst) {
    LOG5("\t\tInstruction: " << inst);
    const IR::MAU::Action *act = findContext<IR::MAU::Action>();
    if (!act) return true;
    if (inst->operands.empty()) return true;
    PHV::Field *field = phv.field(inst->operands[0]);
    if (!field) return true;
    LOG5("\t\tWrite: " << field);
    actionToWrites[act].insert(field);
    for (int idx = 1; idx < int(inst->operands.size()); ++idx) {
        PHV::Field *readField = phv.field(inst->operands[idx]);
        if (!readField) continue;
        LOG5("\t\tRead: " << readField);
        actionToReads[act].insert(readField);
    }
    return true;
}

void ExcludeMAUOverlays::markNonMutex(const ActionToFieldsMap &arg) {
    for (auto &kv : arg) {
        LOG3("\tAction: " << kv.first->name);
        for (auto *f1 : kv.second) {
            for (auto *f2 : kv.second) {
                if (f1 == f2) continue;
                if (!addedFields.isAddedInMAU(f1) && !addedFields.isAddedInMAU(f2)) continue;
                LOG3("\t  Mark as non mutually exclusive: " << f1->name << ", " << f2->name);
                phv.removeFieldMutex(f1, f2);
            }
        }
    }
}

void ExcludeMAUOverlays::end_apply() {
    LOG3("\tMarking all writes in the same action non mutually exclusive");
    markNonMutex(actionToWrites);
    LOG3("\tMarking all reads in the same action non mutually exclusive");
    markNonMutex(actionToReads);
}

bool ExcludeCsumOverlays::preorder(const IR::BFN::EmitChecksum *emitChecksum) {
    bool addedInMau = false;
    for (auto sourceField : emitChecksum->sources) {
        auto csumPhvField = phv.field(sourceField->field->field);
        if (addedFields.isAddedInMAU(csumPhvField)) {
            addedInMau = true;
            break;
        }
    }

    auto checksumDest = phv.field(emitChecksum->dest->field);
    auto allFields = phv.get_all_fields();
    for (auto &field : allFields) {
        const PHV::Field *fieldRef = phv.field(field.first);
        if (!phv.isFieldMutex(checksumDest, fieldRef) ||
            (addedInMau && use.is_extracted(fieldRef, fieldRef->gress) &&
             !use.is_used_mau(fieldRef))) {
            for (auto sourceField : emitChecksum->sources) {
                auto csumPhvField = phv.field(sourceField->field->field);
                LOG3("\t  Mark as non mutually exclusive: "
                     << csumPhvField->name << ", " << field.first << " due to "
                     << checksumDest->name << " PHV validitity-based checksum update");
                phv.removeFieldMutex(csumPhvField, fieldRef);
            }
        }
    }
    return false;
}

bool ExcludeCsumOverlaysPOV::preorder(const IR::BFN::EmitChecksum *emitChecksum) {
    for (const auto &source1 : emitChecksum->sources) {
        const PHV::Field *sourceField1 = phv.field(source1->field->field);
        for (const auto &source2 : emitChecksum->sources) {
            const PHV::Field *sourceField2 = phv.field(source2->field->field);
            if (phv.isFieldMutex(sourceField1, sourceField2)) {
                LOG3("\t  Mark as non mutually exclusive: "
                     << sourceField1->name << ", " << sourceField2->name << " due to "
                     << phv.field(emitChecksum->dest->field)->name << " POV-based checksum update");
                phv.removeFieldMutex(sourceField1, sourceField2);
            }
        }
    }

    return false;
}

bool ExcludeDeparserOverlays::preorder(const IR::BFN::EmitField *emit) {
    const auto *source = phv.field(emit->source->field);
    const auto *pov = phv.field(emit->povBit->field);
    povToFieldsMap[pov].insert(source);
    return true;
}

void ExcludeDeparserOverlays::end_apply() {
    for (const auto &kv : povToFieldsMap) {
        LOG3("\t  Marking fields emitted based on POV field: " << kv.first->name);
        for (const auto *f1 : kv.second) {
            for (const auto *f2 : kv.second) {
                if (!f1 || !f2) continue;  // FIXME(hanw): padding fields
                if (f1 == f2) continue;
                phv.removeFieldMutex(f1, f2);
                LOG3("\t\t" << f1->name << ", " << f2->name);
            }
        }
    }
}

Visitor::profile_t MarkMutexPragmaFields::init_apply(const IR::Node *root) {
    // Mark fields specified by pa_mutually_exclusive pragmas
    const ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>> &parsedPragma =
        pragma.mutex_fields();
    for (auto fieldSet : parsedPragma) {
        auto *field1 = fieldSet.first;
        for (auto *field2 : fieldSet.second) {
            if (field1->id == field2->id) continue;
            phv.addFieldMutex(field1, field2);
            LOG4("set " << field1->name << " and " << field2->name
                        << " to be mutually_exclusive because of pragma @pa_mutually_exclusive");
        }
    }

    if (LOGGING(1)) {
        LOG1("Final field mutexes:");
        for (const auto &f1 : phv) {
            if (f1.is_padding()) continue;
            for (const auto &f2 : phv) {
                if (f2.is_padding()) continue;
                if (f1 == f2) continue;
                if (phv.isFieldMutex(&f1, &f2)) LOG1("(" << f1.name << ", " << f2.name << ")");
            }
        }
        LOG1("");
    }

    return Inspector::init_apply(root);
}

bool FindAddedHeaderFields::preorder(const IR::MAU::Primitive *prim) {
    LOG5("Prim name: " << prim->name);
    // If this is a well-formed field modification...
    if (prim->name == "set" && prim->operands.size() > 1) {
        // that writes a non-zero value to `hdrRef.$valid`...
        auto *fld = phv.field(prim->operands[0]);
        if (fld && fld->pov) {
            auto *m = prim->operands[0]->to<IR::Member>();
            if (!m) {
                // Prim's destination not a member operation.
                // Now check if it's a slice operation
                auto slc = prim->operands[0]->to<IR::Slice>();
                if (slc) m = slc->e0->to<IR::Member>();
            }

            auto *c = prim->operands[1]->to<IR::Constant>();
            if (c) {
                LOG5("\t\t\tconstant:" << *c);
            }

            if (!m) {
                LOG5("\t\t\tWARNING: member for primitive writing pov field " << *fld
                                                                              << " not found!");
            } else {
                if (auto *href = m->expr->to<IR::HeaderRef>()) {
                    // then add all fields of the header (not including $valid) to
                    // the set of fields that are part of added headers
                    if (c && c->asInt() != 0) {
                        LOG4("Found added header: " << href);
                        markFields(href);
                        return false;
                    }
                    // process copy header primitives
                    auto *srcM = prim->operands[1]->to<IR::Member>();
                    if (!srcM) return false;
                    auto *srcField = phv.field(srcM);
                    if (!srcField) return false;
                    LOG4("Found copy header destination: " << href);
                    markFields(href);
                }
            }
        }
    }

    return false;
}

void FindAddedHeaderFields::markFields(const IR::HeaderRef *hr) {
    PhvInfo::StructInfo info = phv.struct_info(hr);
    for (int id : info.field_ids()) rv[id] = true;
}
