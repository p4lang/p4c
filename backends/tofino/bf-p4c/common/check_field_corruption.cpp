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

#include "check_field_corruption.h"

bool CheckFieldCorruption::preorder(const IR::BFN::Pipe *pipe) {
    pipe->apply(uses);
    return true;
}

bool CheckFieldCorruption::preorder(const IR::BFN::DeparserParameter *param) {
    if (param->source) pov_protected_fields.insert(phv.field(param->source->field));
    return false;
}

bool CheckFieldCorruption::preorder(const IR::BFN::Digest *digest) {
    pov_protected_fields.insert(phv.field(digest->selector->field));
    return false;
}

bool CheckFieldCorruption::preorder(const IR::Expression *e) {
    if (isWrite()) {
        if (auto state = findContext<IR::BFN::ParserState>()) {
            state_extracts[state].insert(e);
            parser_inits[phv.field(e)].insert(e);
        }
    }
    return false;
}

bool CheckFieldCorruption::preorder(const IR::BFN::ParserZeroInit *pzi) {
    parser_inits[phv.field(pzi->field->field)].insert(pzi->field->field);
    return true;
}

bool CheckFieldCorruption::copackedFieldExtractedSeparately(const FieldDefUse::locpair &use) {
    // Walk through the slices corresponding to the use
    auto read = PHV::FieldUse(PHV::FieldUse::READ);
    const auto &allocs = phv.get_alloc(use.second, PHV::AllocContext::of_unit(use.first), &read);
    for (const auto &sl : allocs) {
        // Skip this field if it's not a parser init
        if (!std::any_of(parser_inits[sl.field()].begin(), parser_inits[sl.field()].end(),
                         [this, sl](const IR::Expression *e) {
                             le_bitrange bits;
                             this->phv.field(e, &bits);
                             return bits.overlaps(sl.field_slice());
                         }))
            continue;

        auto c = sl.container();

        // Dark containers are not extracted
        // Tagalong containers are only populated via extraction
        if (c.is(PHV::Kind::dark) || c.is(PHV::Kind::tagalong)) continue;

        // Check for any other slice in the container that is extracted in the parser, where the
        // source is packet data and is a _not_ mutually exclusive.
        auto write = PHV::FieldUse(PHV::FieldUse::WRITE);
        for (auto &other : phv.get_slices_in_container(c, PHV::AllocContext::PARSER, &write)) {
            if (other.field() == sl.field()) continue;
            if (sl.isLiveRangeDisjoint(other)) continue;
            if (phv.isFieldMutex(other.field(), sl.field())) continue;

            if (Device::currentDevice() == Device::JBAY) {
                if (sl.container_slice().lo / 8 > other.container_slice().hi / 8 ||
                    sl.container_slice().hi / 8 < other.container_slice().lo / 8)
                    continue;
            }

            if (uses.is_extracted_from_pkt(other.field())) {
                std::set<const IR::BFN::ParserState *> seenStates;
                for (const auto &otherDef : defuse.getAllDefs(other.field()->id)) {
                    if (const auto *state = otherDef.first->to<IR::BFN::ParserState>()) {
                        if (seenStates.count(state)) continue;

                        const auto *oExp = otherDef.second;
                        if (oExp->is<ImplicitParserInit>()) continue;

                        le_bitrange oBits;
                        phv.field(oExp, &oBits);
                        if (oBits.overlaps(other.field_slice())) {
                            seenStates.insert(state);

                            bitvec slBits;
                            slBits.setrange(sl.field_slice().lo,
                                            sl.field_slice().hi - sl.field_slice().lo + 1);

                            bitvec extBits;
                            for (const auto *exp : state_extracts[state]) {
                                le_bitrange bits;
                                const auto *f = phv.field(exp, &bits);
                                if (f == sl.field()) {
                                    extBits.setrange(bits.lo, bits.hi - bits.lo + 1);
                                }
                            }

                            extBits &= slBits;
                            if (extBits != slBits) return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

void CheckFieldCorruption::end_apply() {
    auto is_ignored_field = [&](const PHV::Field &field) {
        if (field.pov) {
            // pov is always initialized.
            LOG3("Ignore pov bits: " << field);
            return true;
        } else if (field.is_padding()) {
            // padding fields that are generated by the compiler should be ignored.
            LOG3("Ignore padding field: " << field);
            return true;
        } else if (field.is_deparser_zero_candidate()) {
            LOG3("Ignore deparser zero field: " << field);
            return true;
        } else if (field.is_overlayable()) {
            LOG3("Ignore overlayable field: " << field);
            return true;
        } else if (!Device::hasMetadataPOV()) {
            // Only for tofino, if a field is invalidated by the arch, then this field is pov bit
            // protected and will not overlay with other fields. So no need to check it.
            if (field.is_invalidate_from_arch())
                return true;
            else
                return false;
        } else {
            // For all fields that are pov bit protected fields, no need to check it, since no write
            // means pov bit remains invalid.
            for (const auto &pov_protected_field : pov_protected_fields) {
                if (pov_protected_field->name == field.name) return true;
            }
            return false;
        }
    };

    for (const auto &kv : phv.get_all_fields()) {
        const auto &field = kv.second;
        LOG3("checking " << field);
        if (is_ignored_field(field)) continue;
        for (const auto &use : defuse.getAllUses(field.id)) {
            // Are the slices initialized in always-run actions?
            // This should not be needed, but is currently required because defuse
            // analysis doesn't correctly handle ARA tables.
            auto read = PHV::FieldUse(PHV::FieldUse::READ);
            const auto &allocs =
                phv.get_alloc(use.second, PHV::AllocContext::of_unit(use.first), &read);
            bool has_ara = allocs.size() > 0;
            for (const auto &alloc : allocs) {
                has_ara &= alloc.hasInitPrimitive();
            }
            if (has_ara) continue;

            // If any other fields are packed in the same container(s) as this one, and if any of
            // those fields are extracted from packet data, then make sure we don't have an
            // ImplicitParerInit, otherwise the packet extract will corrupt this field.
            bool pkt_extract = copackedFieldExtractedSeparately(use);
            const auto &defs_of_use = defuse.getDefs(use);
            bool uninit = defs_of_use.empty();
            if (!uninit) {
                for (const auto &def : defs_of_use) {
                    if (def.second->is<ImplicitParserInit>() && pkt_extract) {
                        LOG3("Use: ("
                             << DBPrint::Brief << use.first << ", " << use.second
                             << ") has an ImplicitParserInit def but "
                             << "a field in the same container is extracted from packet data");
                        uninit = true;
                        break;
                    }
                }
            } else {
                // metadata in INGRESS and non bridged metadata in EGRESS will have at least a
                // ImplicitParserInit def.
                BUG_CHECK(
                    !field.metadata || (field.metadata && field.bridged && field.gress == EGRESS),
                    "metadata cannot reach here");
            }
            if (uninit && pkt_extract) {
                warning(
                    "%s is read in %s, but it is totally or partially uninitialized after "
                    "being corrupted by a parser extraction to the same container(s)",
                    field.name, use.first);
            }
        }
    }
}
