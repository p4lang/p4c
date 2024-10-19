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

#include "bf-p4c/common/bridged_packing.h"

#include <z3++.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <numeric>
#include <queue>
#include <sstream>
#include <stack>

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/arch/collect_hardware_constrained_fields.h"
#include "bf-p4c/backend.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/size_of.h"
#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/lib/pad_alignment.h"
#include "bf-p4c/logging/event_logger.h"
#include "bf-p4c/midend/check_header_alignment.h"
#include "bf-p4c/midend/simplify_args.h"
#include "bf-p4c/phv/cluster_phv_operations.h"
#include "bf-p4c/phv/constraints/constraints.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_atomic.h"
#include "bf-p4c/phv/pragma/pa_container_size.h"
#include "bf-p4c/phv/pragma/pa_no_pack.h"
#include "bf-p4c/phv/pragma/pa_solitary.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/reassociation.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/uselessCasts.h"
#include "midend/local_copyprop.h"

// included by PackFlexibleHeaders
#include "bf-p4c/common/alias.h"
#include "bf-p4c/common/check_for_unimplemented_features.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/multiple_apply.h"
#include "bf-p4c/mau/empty_controls.h"
#include "bf-p4c/mau/instruction_selection.h"
#include "bf-p4c/mau/push_pop.h"
#include "bf-p4c/mau/selector_update.h"
#include "bf-p4c/mau/stateful_alu.h"
#include "bf-p4c/parde/add_metadata_pov.h"
#include "bf-p4c/parde/stack_push_shims.h"
#include "bf-p4c/phv/create_thread_local_instances.h"
#include "bf-p4c/phv/pragma/pa_no_overlay.h"

Visitor::profile_t CollectIngressBridgedFields::init_apply(const IR::Node *root) {
    profile_t rv = Inspector::init_apply(root);
    bridged_to_orig.clear();
    return rv;
}

void CollectIngressBridgedFields::postorder(const IR::BFN::AliasMember *mem) {
    const auto *dest = phv.field(mem);
    const auto *src = phv.field(mem->source);
    CHECK_NULL(dest);
    CHECK_NULL(src);
    if (!dest->is_flexible()) return;
    if (bridged_to_orig.count(dest->name)) return;

    bridged_to_orig[dest->name] = src->name;
    LOG5("bridged to orig " << dest->name << " " << src->name);
}

void CollectIngressBridgedFields::end_apply() {
    for (const auto &f : phv) {
        if (f.gress == EGRESS) continue;
        // Indicator does not need to be initialzed in mau.
        // TODO: check if still required.
        if (f.name.endsWith(BFN::BRIDGED_MD_INDICATOR.string())) continue;

        if (f.bridged && !bridged_to_orig.count(f.name)) {
            LOG5("Missing initialzation of bridged field: " << f);
        }
    }
}

bool CollectEgressBridgedFields::preorder(const IR::BFN::Extract *extract) {
    const IR::BFN::ParserState *state = findContext<IR::BFN::ParserState>();
    BUG_CHECK(extract->dest, "Extract %1% does not have a destination",
              cstring::to_cstring(extract));
    BUG_CHECK(extract->dest->field, "Extract %1% does not have a destination field",
              cstring::to_cstring(extract));
    const PHV::Field *destField = phv.field(extract->dest->field);
    BUG_CHECK(destField, "Could not find destination field for extract %1%",
              cstring::to_cstring(extract));
    // if destination field is used in arithmetic operation, do not add alias constraint
    if (destField->is_solitary()) return false;
    if (auto rval = extract->source->to<IR::BFN::SavedRVal>()) {
        if (auto mem = rval->source->to<IR::Member>()) {
            if (mem->expr->is<IR::ConcreteHeaderRef>()) {
                auto srcField = phv.field(mem);
                CHECK_NULL(srcField);
                // only aliasing meta = hdr.f where f is a field with flexible annotation
                if (srcField->is_flexible()) {
                    LOG5("candidate field " << destField << " " << srcField);
                    candidateSourcesInParser[destField][srcField].insert(state);
                }
            }
        }
    }
    return true;
}

Visitor::profile_t CollectEgressBridgedFields::init_apply(const IR::Node *root) {
    candidateSourcesInParser.clear();
    return Inspector::init_apply(root);
}

void CollectEgressBridgedFields::end_apply() {
    for (auto &f : candidateSourcesInParser) {
        std::stringstream ss;
        ss << "\t  " << f.first->name << ", ";
        const PHV::Field *srcField = nullptr;
        if (f.second.size() > 1) continue;
        // The only source
        for (const auto &kv : f.second) {
            srcField = kv.first;
            ss << kv.first->name << ", ";
            for (const auto *state : kv.second) ss << state->name << " ";
        }
        if (bridged_to_orig.count(srcField->name)) continue;
        if (srcField != nullptr) {
            bridged_to_orig[srcField->name] = f.first->name;
        }
    }
}

bool GatherParserExtracts::preorder(const IR::BFN::Extract *e) {
    auto *fieldLVal = e->dest->to<IR::BFN::FieldLVal>();
    if (!fieldLVal) return true;
    auto *f = phv.field(fieldLVal->field);
    if (!f) return true;
    auto *source = e->source->to<IR::BFN::SavedRVal>();
    if (!source) return true;
    auto *sourceField = phv.field(source->source);
    if (!sourceField) return true;
    parserAlignedFields[f].insert(sourceField);
    reverseParserAlignMap[sourceField].insert(f);
    LOG5("    Initialization due to SavedRVal in parser: " << f);
    return true;
}

bool GatherDigestFields::preorder(const IR::BFN::DigestFieldList *fl) {
    /// TODO when does a field list not have a type?
    if (!fl->type) return false;

    auto isFlexible = [&](const IR::Type_StructLike *st) -> bool {
        for (auto f : st->fields) {
            auto anno = f->getAnnotation("flexible"_cs);
            if (anno != nullptr) return true;
        }
        return false;
    };
    if (!isFlexible(fl->type)) return false;

    std::vector<const PHV::Field *> digest_fields;
    auto iter = fl->sources.begin();
    forAllMatchingDoPostOrder<IR::StructField>(fl->type, [&](const IR::StructField *f) {
        if (f->getAnnotation("flexible"_cs)) {
            const auto *field = phv.field((*iter)->field);
            digest_fields.push_back(field);
        }
        iter++;
    });

    digests[fl->type->name].insert(digest_fields);
    return false;
}

bool GatherDigestFields::preorder(const IR::HeaderOrMetadata *hdr) {
    auto isFlexible = [&](const IR::Type_StructLike *st) -> bool {
        for (auto f : st->fields) {
            auto anno = f->getAnnotation("flexible"_cs);
            if (anno != nullptr) return true;
        }
        return false;
    };
    if (!isFlexible(hdr->type)) return false;

    std::vector<const PHV::Field *> flexible_fields;
    std::vector<cstring> header_stack;
    forAllMatchingDoPreAndPostOrder<IR::StructField>(
        hdr, [&](const IR::StructField *f) { header_stack.push_back(f->name); },
        [&](const IR::StructField *f) {
            if (f->getAnnotation("flexible"_cs)) {
                cstring fn = hdr->name;
                for (auto hd : header_stack) {
                    fn = fn + "." + hd;
                }
                const auto *field = phv.field(fn);
                BUG_CHECK(field != nullptr, "Could not find field %1%", fn);
                flexible_fields.push_back(field);
            }
            header_stack.pop_back();
        });
    headers[hdr->type->name].insert(flexible_fields);
    return false;
}

// build a map between digest field and its sources.
void GatherDigestFields::end_apply() {
    // for all flexible digest types
    ordered_set<cstring> digest_types;
    for (auto d : digests) {
        digest_types.push_back(d.first);
    }

    // given { A, B, C } = { D, E, F }
    // generate mapping of A->D, B->E, C->F
    auto generate_mapping = [&](std::vector<const PHV::Field *> dst,
                                std::vector<const PHV::Field *> src) {
        std::vector<std::pair<const PHV::Field *, const PHV::Field *>> mapping;
        std::transform(
            dst.begin(), dst.end(), src.begin(), std::back_inserter(mapping),
            [](const PHV::Field *aa, const PHV::Field *bb) { return std::make_pair(aa, bb); });
        // build a mapping and reverse mapping
        for (auto o : mapping) {
            digestFieldMap[o.first].insert(o.second);
            reverseDigestFieldMap[o.second].insert(o.first);
        }
    };

    for (auto d : digests) {
        LOG4("d " << d.first);
    }

    for (auto h : headers) {
        LOG4("h " << h.first);
    }

    for (auto t : digest_types) {
        LOG4("t " << t);
        auto src_lists = digests.at(t);
        if (!headers.count(t)) continue;
        auto dst_lists = headers.at(t);
        for (auto dst : dst_lists) {
            for (auto src : src_lists) {
                generate_mapping(dst, src);
            }
        }
    }
}

bool CollectConstraints::preorder(const IR::HeaderOrMetadata *hdr) {
    ordered_set<const PHV::Field *> flexible_fields;
    std::vector<cstring> header_stack;
    forAllMatchingDoPreAndPostOrder<IR::StructField>(
        hdr, [&](const IR::StructField *f) { header_stack.push_back(f->name); },
        [&](const IR::StructField *f) {
            if (f->getAnnotation("flexible"_cs)) {
                cstring fn = hdr->name;
                for (auto hd : header_stack) {
                    fn = fn + "." + hd;
                }
                const auto *field = phv.field(fn);
                BUG_CHECK(field != nullptr, "Could not find field %1%", fn);
                flexible_fields.insert(field);
            }
            header_stack.pop_back();
        });

    if (!flexible_fields.size()) return false;

    for (auto f : flexible_fields) {
        LOG4("compute constraints for " << f);
    }

    computeAlignmentConstraint(flexible_fields, true);
    computeMustPackConstraints(flexible_fields, false);
    computeNoPackIfIntrinsicMeta(flexible_fields, false);
    computeNoPackIfActionDataWrite(flexible_fields, false);
    computeNoPackIfSpecialityRead(flexible_fields, false);
    computeNoPackIfDigestUse(flexible_fields, false);
    computeNoSplitConstraints(flexible_fields, false);

    return false;
}

bool CollectConstraints::preorder(const IR::BFN::DigestFieldList *fl) {
    /// TODO when does a field list not have a type?
    if (!fl->type) return false;

    ordered_set<const PHV::Field *> flexible_fields;
    auto iter = fl->sources.begin();
    forAllMatchingDoPostOrder<IR::StructField>(fl->type, [&](const IR::StructField *f) {
        if (f->getAnnotation("flexible"_cs)) {
            const auto *field = phv.field((*iter)->field);
            flexible_fields.push_back(field);
        }
        iter++;
    });

    for (auto f : flexible_fields) {
        LOG4("compute constraints for " << f);
    }

    computeAlignmentConstraint(flexible_fields, true);
    computeMustPackConstraints(flexible_fields, false);
    computeNoPackIfIntrinsicMeta(flexible_fields, false);
    computeNoPackIfActionDataWrite(flexible_fields, false);
    computeNoPackIfSpecialityRead(flexible_fields, false);
    computeNoPackIfDigestUse(flexible_fields, false);
    computeNoSplitConstraints(flexible_fields, false);

    return false;
}

cstring CollectConstraints::getOppositeGressFieldName(cstring name) {
    if (name.startsWith("ingress::")) {
        return ("egress::" + name.substr(9));
    } else if (name.startsWith("egress::")) {
        return ("ingress::" + name.substr(8));
    } else if (name.startsWith("ghost::")) {
        return ("ghost::" + name.substr(8));
    } else {
        BUG("Called getOppositeGressFieldName on unknown gress fieldname %1%", name);
        return cstring();
    }
}

cstring CollectConstraints::getEgressFieldName(cstring name) {
    if (name.startsWith("egress::")) {
        return name;
    } else {
        return getOppositeGressFieldName(name);
    }
}

// Given a data flow graph:
//
//     a -> b -> c
//               ^
//     d -> e ---|
//
//     f -> g -> h
//
//  and a starting point, say 'a'. Return a set of fields are connected to 'a'
//  in both forward and backward direction, in this case { a, b, c, d, e }.
//
//  Used to find all related field to a bridged field along all paths.
//
ordered_set<const PHV::Field *> CollectConstraints::findAllRelatedFields(const PHV::Field *field) {
    ordered_set<const PHV::Field *> relatedFields;
    // Skip ghost fields as they are not relevant to bridge packing
    if (field->isGhostField()) return relatedFields;
    std::queue<const PHV::Field *> fieldsNotVisited;
    LOG6("\tDetermining all related fields for " << field->name);

    fieldsNotVisited.push(field);
    // relate ingress field to the bridged field
    if (ingressBridgedFields.bridged_to_orig.count(field->name)) {
        const auto *origField = phv.field(ingressBridgedFields.bridged_to_orig.at(field->name));
        if (origField) {
            fieldsNotVisited.push(origField);
            LOG6("\t\t\tDetected original bridged field: " << origField);
        }
    }

    cstring egressFieldName = getEgressFieldName(field->name);
    const auto *egressField = phv.field(egressFieldName);
    if (egressField) {
        LOG6("\t\t\tDetected egress field: " << egressField);
        fieldsNotVisited.push(egressField);

        // relate bridged field to egress field
        if (egressBridgedFields.bridged_to_orig.count(egressField->name)) {
            const auto origField =
                phv.field(egressBridgedFields.bridged_to_orig.at(egressField->name));
            if (origField) {
                fieldsNotVisited.push(origField);
                LOG6("\t\t\tDetected original bridged field: " << origField);
            }
        }
    }

    while (!fieldsNotVisited.empty()) {
        const PHV::Field *currentField = fieldsNotVisited.front();
        relatedFields.insert(currentField);
        fieldsNotVisited.pop();
        LOG6("\t\tVisiting new field: " << currentField);
        auto operands =
            actionConstraints.slices_sources(currentField, {PHV::FieldSlice(currentField)});
        // Add all unvisited operands to fieldsNotVisited.
        for (const PHV::Field *f : operands) {
            if (!relatedFields.count(f)) fieldsNotVisited.push(f);
        }
        auto destinations =
            actionConstraints.slices_destinations(currentField, {PHV::FieldSlice(currentField)});
        // Add all unvisited destinations to fieldsNotVisited.
        for (const PHV::Field *f : destinations) {
            if (!relatedFields.count(f)) fieldsNotVisited.push(f);
        }
        // Add alignment constraints inferred from extracts
        // e.g.,
        // state A:
        //   extract [ egress::hdr.mirrored_md.meta_port; ] to egress::ig_md.port;
        // state B:
        //   extract [ egress::hdr.bridged_md.meta_port; ] to egress::ig_md.port;
        //
        // will infer egress::hdr.mirrored_md.meta_port and
        // egress::hdr.bridged_md.meta_port; has mutual alignment constraint.
        // Even though both fields may be @flexible and has no known alignment
        // yet. The solver must know of the mutual alignment constraint to
        // generate the same alignment for both of them.
        if (parserAlignedFields.count(currentField))
            for (const auto *f : parserAlignedFields.at(currentField))
                if (!relatedFields.count(f)) fieldsNotVisited.push(f);
        if (parserAlignedFields.revCount(currentField))
            for (const auto *f : parserAlignedFields.revAt(currentField))
                if (!relatedFields.count(f)) fieldsNotVisited.push(f);
        LOG6("\t\t  Now, we have " << fieldsNotVisited.size() << " fields unvisited");
    }

    return relatedFields;
}

/**
 * Initialize a set of fields 'M' to empty.
 * Given a field 'f',
 *  find a set of fields 'S' that are related to 'f' by ALU operations.
 *  for each field 's' in 'S':
 *      find a set of fields 'G' that are packed to the same byte with 's'
 *          for each field 'g' in 'G':
 *              add to 'M' the set of fields that are related to 'g' by ALU operations.
 *  return 'M'.
 */
ordered_set<const PHV::Field *> CollectConstraints::findAllRelatedFieldsOfCopackedFields(
    const PHV::Field *field) {
    ordered_set<const PHV::Field *> packedFields;

    auto strip_gress = [&](cstring name) {
        std::string str = name.c_str();
        size_t pos = str.find("::");
        BUG_CHECK(pos != std::string::npos, "Invalid internal header name %1%", name);
        return str.substr(pos);
    };

    auto relatedFields = findAllRelatedFields(field);
    for (const PHV::Field *f : relatedFields) {
        // skip non header field
        if (!f->isPacketField()) continue;
        // skip field itself
        if (f->id == field->id) continue;
        // skip the same header type
        if (strip_gress(f->header()) == strip_gress(field->header())) continue;
        // look for field that are packed in the same byte as this field
        const PhvInfo::StructInfo &struct_info = phv.struct_info(f->header());
        if (struct_info.metadata || struct_info.size == 0) continue;
        ordered_map<const PHV::Field *, int> offset_map;
        int accumulator_bits = 0;
        for (auto id : struct_info.field_ids()) {
            auto field_of_id = phv.field(id);
            if (!field_of_id) continue;
            offset_map.emplace(field_of_id, accumulator_bits);
            accumulator_bits += field_of_id->size;
        }

        auto f_offset = offset_map.at(f);
        for (auto offset : offset_map) {
            if (offset.first->id == f->id) continue;
            // find fields that are in the same byte.
            if (offset.second / 8 == f_offset / 8) {
                auto related = findAllRelatedFields(offset.first);
                for (auto f : related) {
                    packedFields.insert(f);
                }
            }
        }
    }
    return packedFields;
}

// Given a data flow graph:
//
//     a -> b -> c
//               ^
//     d -> e ---|
//
//     f -> g -> h
//
//  and a starting point, say 'a'. Return a set of fields are connected to 'a'
//  in forward direction, in this case { a, b, c }.
//
//  Used to find related fields to a bridged field along a single path.
ordered_set<const PHV::Field *> CollectConstraints::findAllReachingFields(const PHV::Field *field) {
    ordered_set<const PHV::Field *> relatedFields;
    std::queue<const PHV::Field *> fieldsNotVisited;
    LOG6("\tDetermining all related downstream fields for " << field->name);

    fieldsNotVisited.push(field);
    // relate ingress field to the bridged field
    if (ingressBridgedFields.bridged_to_orig.count(field->name)) {
        const auto *origField = phv.field(ingressBridgedFields.bridged_to_orig.at(field->name));
        if (origField) {
            fieldsNotVisited.push(origField);
            LOG6("\t\t\tDetected original bridged field: " << origField);
        }
    }

    cstring egressFieldName = getEgressFieldName(field->name);
    const auto *egressField = phv.field(egressFieldName);
    if (egressField) {
        LOG6("\t\t\tDetected egress field: " << egressField);
        fieldsNotVisited.push(egressField);

        // relate bridged field to egress field
        if (egressBridgedFields.bridged_to_orig.count(egressField->name)) {
            const auto origField =
                phv.field(egressBridgedFields.bridged_to_orig.at(egressField->name));
            if (origField) {
                fieldsNotVisited.push(origField);
                LOG6("\t\t\tDetected original bridged field: " << origField);
            }
        }
    }

    while (!fieldsNotVisited.empty()) {
        const PHV::Field *currentField = fieldsNotVisited.front();
        relatedFields.insert(currentField);
        fieldsNotVisited.pop();
        LOG6("\t\tVisiting new field: " << currentField);
        auto destinations =
            actionConstraints.slices_destinations(currentField, {PHV::FieldSlice(currentField)});
        // Add all unvisited destinations to fieldsNotVisited.
        for (const PHV::Field *f : destinations) {
            if (!relatedFields.count(f)) fieldsNotVisited.push(f);
        }
        LOG6("\t\t  Now, we have " << fieldsNotVisited.size() << " fields unvisited");
    }

    return relatedFields;
}

ordered_set<const PHV::Field *> CollectConstraints::findAllSourcingFields(const PHV::Field *field) {
    ordered_set<const PHV::Field *> relatedFields;
    std::queue<const PHV::Field *> fieldsNotVisited;
    LOG6("\tDetermining all related upstream fields for " << field->name);

    fieldsNotVisited.push(field);
    // relate ingress field to the bridged field
    if (ingressBridgedFields.bridged_to_orig.count(field->name)) {
        const auto *origField = phv.field(ingressBridgedFields.bridged_to_orig.at(field->name));
        if (origField) {
            fieldsNotVisited.push(origField);
            LOG6("\t\t\tDetected original bridged field: " << origField);
        }
    }

    cstring egressFieldName = getEgressFieldName(field->name);
    const auto *egressField = phv.field(egressFieldName);
    if (egressField) {
        LOG6("\t\t\tDetected egress field: " << egressField);
        fieldsNotVisited.push(egressField);

        // relate bridged field to egress field
        if (egressBridgedFields.bridged_to_orig.count(egressField->name)) {
            const auto origField =
                phv.field(egressBridgedFields.bridged_to_orig.at(egressField->name));
            if (origField) {
                fieldsNotVisited.push(origField);
                LOG6("\t\t\tDetected original bridged field: " << origField);
            }
        }
    }

    while (!fieldsNotVisited.empty()) {
        const PHV::Field *currentField = fieldsNotVisited.front();
        relatedFields.insert(currentField);
        fieldsNotVisited.pop();
        LOG6("\t\tVisiting new field: " << currentField);
        auto destinations =
            actionConstraints.slices_destinations(currentField, {PHV::FieldSlice(currentField)});
        // Add all unvisited destinations to fieldsNotVisited.
        auto operands =
            actionConstraints.slices_sources(currentField, {PHV::FieldSlice(currentField)});
        // Add all unvisited operands to fieldsNotVisited.
        for (const PHV::Field *f : operands) {
            if (!relatedFields.count(f)) fieldsNotVisited.push(f);
        }
        LOG6("\t\t  Now, we have " << fieldsNotVisited.size() << " fields unvisited");
    }

    return relatedFields;
}

void CollectConstraints::computeAlignmentConstraint(const ordered_set<const PHV::Field *> &fields,
                                                    bool debug = false) {
    auto computeAlignment = [&](const PHV::Field *field) {
        auto relatedFields = findAllRelatedFields(field);

        if (debug) LOG5("Checking " << field->name);
        for (const PHV::Field *f : relatedFields) {
            // ignore constraint from itself
            if (f->id == field->id) continue;
            std::ostringstream str;
            // ignore constraint on existing flexible field
            str << std::string("\t Related field: ");
            str << f;
            if (f->is_flexible()) {
                str << " saved as mutually aligned";
                constraints.mutualAlignment[field].push_back(f);
                if (debug) LOG5(str.str());
                continue;
            }
            if (debug) LOG5(str.str());
            // ignore metadata and pov bit
            if (!f->isPacketField()) continue;
            if (f->alignment) {
                if (debug) LOG5("\t\t  New candidate alignment: " << *(f->alignment));
                AlignmentConstraint constraint;
                PHV::AlignmentReason reason;
                // See comment above 'rank' function for why.
                if (f->is_intrinsic())
                    reason = PHV::AlignmentReason::INTRINSIC;
                else
                    reason = PHV::AlignmentReason::BRIDGE;
                constraint.addConstraint(reason, f->alignment->align);

                // IMPL_NOTE(0)
                auto container_size = (f->alignment->align + f->size + 7) / 8 * 8;
                constraint.setContainerSize(container_size);

                constraints.alignment[field].insert(constraint);
            }
        }
        if (!constraints.alignment.count(field)) constraints.alignment.emplace(field);
    };

    for (auto f : fields) {
        computeAlignment(f);
    }
}

bool CollectConstraints::mustPack(const ordered_set<const PHV::Field *> &fs1,
                                  const ordered_set<const PHV::Field *> &fs2,
                                  const ordered_set<const IR::MAU::Action *> &common_reads) {
    for (auto f1 : fs1) {
        for (auto f2 : fs2) {
            if (f1->gress != f2->gress) continue;
            if (f1->name == f2->name) continue;
            if (f1->header() != f2->header()) continue;
            if (f1->offset / 8 != f2->offset / 8) continue;

            ordered_set<const IR::MAU::Action *> common_writes;
            auto f1_write = actionConstraints.actions_writing_fields(f1);
            auto f2_write = actionConstraints.actions_writing_fields(f2);
            std::copy_if(f1_write.begin(), f1_write.end(), std::back_inserter(common_writes),
                         [&](const IR::MAU::Action *act) { return f2_write.count(act) > 0; });

            LOG5("\t\t Write action: " << f1->name << " " << f2->name);
            for (auto w : common_writes) LOG5("\t\t\t " << w->name);

            ordered_set<const IR::MAU::Action *> overlap;
            std::copy_if(common_reads.begin(), common_reads.end(), std::back_inserter(overlap),
                         [&](const IR::MAU::Action *act) { return common_writes.count(act) > 0; });
            if (!overlap.size()) continue;

            LOG5("\t\t Overlap action: ");
            for (auto o : overlap) LOG5("\t\t\t " << o->name);

            return true;
        }
    }
    return false;
}

void CollectConstraints::computeMustPackConstraints(const ordered_set<const PHV::Field *> &fields,
                                                    bool debug = false) {
    ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>> related_fields;
    for (auto f : fields) {
        auto rf = findAllReachingFields(f);
        related_fields.emplace(f, rf);
    }

    // filter bridged field and metadata
    ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>> bridged_to_orig;
    for (auto rf : related_fields) {
        auto bridged_field = rf.first;
        if (debug) LOG6("\tDetermining must_pack constraint " << bridged_field);
        for (auto f : rf.second) {
            if (f->name == bridged_field->name) continue;
            if (ingressBridgedFields.bridged_to_orig.count(f->name)) continue;
            if (egressBridgedFields.bridged_to_orig.count(f->name)) continue;
            if (debug) LOG6("\t  Related field " << f);
            bridged_to_orig[bridged_field].insert(f);
        }
    }

    ordered_map<const PHV::Field *, ordered_set<const IR::MAU::Action *>> reads;
    for (auto f : bridged_to_orig) {
        reads[f.first] = actionConstraints.actions_reading_fields(f.first);

        for (auto g : f.second) {
            reads[g] = actionConstraints.actions_reading_fields(g);
            reads[f.first].insert(reads[g].begin(), reads[g].end());
        }
    }

    using const_iterator =
        ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>>::const_iterator;
    for (const_iterator it = bridged_to_orig.begin(); it != bridged_to_orig.end(); it++) {
        for (const_iterator it2 = it; ++it2 != bridged_to_orig.end();) {
            if (debug) LOG5("\tCheck must_pack for " << it->first->name << " " << it2->first->name);

            auto f1_read = reads.at(it->first);
            auto f2_read = reads.at(it2->first);
            ordered_set<const IR::MAU::Action *> common_reads;

            std::copy_if(f1_read.begin(), f1_read.end(), std::back_inserter(common_reads),
                         [&](const IR::MAU::Action *act) { return f2_read.count(act) > 0; });

            LOG5("\t\t Read action " << it->first->name << " " << it2->first->name);
            if (debug)
                for (auto r : common_reads) LOG5("\t\t\t " << r->name);

            if (mustPack(it->second, it2->second, common_reads)) {
                LOG6("\t\tMust pack fields " << it->first->name << " and " << it2->first->name
                                             << " together.");
                constraints.mustPack.insert(std::make_pair(it->first, it2->first));
            }
        }
    }
}

void CollectConstraints::computeNoSplitConstraints(const ordered_set<const PHV::Field *> &fields,
                                                   bool debug = false) {
    auto markAsNoSplit = [&](const PHV::Field *field) {
        PHV::Field *f = phv.field(field->name);
        CHECK_NULL(f);
        f->set_no_split(true);
    };

    auto setNoSplitContainerSize = [&](const PHV::Field *field, int container_size) {
        PHV::Field *f = phv.field(field->name);
        CHECK_NULL(f);
        f->set_no_split_container_size(container_size);
    };

    auto computeNoSplit = [&](const PHV::Field *field) {
        if (debug) LOG5("Computing NoSplit for " << field);
        auto relatedFields = findAllRelatedFields(field);

        // derive no_split constraint from copacked fields,
        // see comment above 'findAllRelatedFieldsOfCopackedFields' function
        auto packedFields = findAllRelatedFieldsOfCopackedFields(field);

        ordered_set<const PHV::Field *> allFields;
        std::set_union(relatedFields.sorted_begin(), relatedFields.sorted_end(),
                       packedFields.sorted_begin(), packedFields.sorted_end(),
                       std::inserter(allFields, allFields.begin()));

        bool no_split = false;
        int max_split_size = -1;
        for (const PHV::Field *f : allFields) {
            if (f->id == field->id) continue;
            if (f->no_split()) {
                no_split = true;
                if (max_split_size < f->size) max_split_size = f->size;
            }
        }
        if (no_split) {
            if (debug)
                LOG5("\tmark " << field << " as no_split " << " with size " << max_split_size);
            markAsNoSplit(field);

            if (max_split_size != -1) {
                if (max_split_size <= 8)
                    setNoSplitContainerSize(field, 8);
                else if (max_split_size <= 16)
                    setNoSplitContainerSize(field, 16);
                else if (max_split_size <= 32)
                    setNoSplitContainerSize(field, 32);
                else
                    LOG6("ERROR: Cannot apply no_split constraint on field "
                         << field
                         << " that is"
                            "more than 32 bit wide");
            }
        }
    };

    for (auto f : fields) {
        computeNoSplit(f);
    }
}

// intrinsic metadata that is extracted in parser as a header type and also
// bridged should not be packed to other metadata fields. The reason is if an
// intrinsic metadata is treated as a header field, it is often packed with
// other intrinsic metadata field in the same container. An canonical example
// is the ig_intr_md.ingress_port, which is packed with resubmit_flag and
// packet_version.
void CollectConstraints::computeNoPackIfIntrinsicMeta(const ordered_set<const PHV::Field *> &fields,
                                                      bool debug = false) {
    (void)debug;
    auto noPackWithAll = [&](const PHV::Field *field) {
        for (auto f : fields) {
            if (field->id == f->id) continue;
            phv.addFieldNoPack(field, f);
        }
    };

    auto computeNoPackWithIntrinsic = [&](const PHV::Field *field) {
        auto relatedFields = findAllRelatedFields(field);

        bool no_pack = false;
        for (const PHV::Field *f : relatedFields) {
            if (f->id == field->id) continue;
            // mark fields in @intrinsic header as no_pack with all
            if (f->is_intrinsic() && f->isPacketField()) no_pack = true;
        }
        if (no_pack) {
            LOG4("\t\tNoPack due to intrinsic " << field);
            noPackWithAll(field);
        }
    };

    for (auto f : fields) {
        computeNoPackWithIntrinsic(f);
    }
}

void CollectConstraints::computeNoPackIfActionDataWrite(
    const ordered_set<const PHV::Field *> &fields, bool debug = false) {
    (void)debug;
    // bridge field written by action data constant and metadata should not be
    // packed together.
    auto isWrittenByConstant = [&](const PHV::Field *field) {
        auto writeActions = actionConstraints.actions_writing_fields(field);
        for (const auto *a : writeActions) {
            // if any of the bridge metadata is written
            for (auto f : actionConstraints.actionWrites(a)) {
                if (f == field) continue;
                if (!fields.count(f)) continue;
                if (actionConstraints.written_by_ad_constant(field, a) &&
                    !actionConstraints.written_by_ad_constant(f, a)) {
                    // do not pack field if one is written by constant, other is not.
                    LOG4("\t\tNoPack in action " << a->name << " " << field->name << " "
                                                 << f->name);
                    phv.addFieldNoPack(field, f);
                }
            }
        }
    };

    for (auto f : fields) isWrittenByConstant(f);
}

void CollectConstraints::computeNoPackIfSpecialityRead(
    const ordered_set<const PHV::Field *> &fields, bool debug = false) {
    (void)debug;
    auto computeNoPackWithSpecialityRead = [&](const PHV::Field *field) {
        bool isSpecialityRead = actionConstraints.hasSpecialityReads(field);
        auto writeActions = actionConstraints.actions_writing_fields(field);
        if (!isSpecialityRead) return;
        for (auto f : fields) {
            auto writeActions2 = actionConstraints.actions_writing_fields(f);
            ordered_set<const IR::MAU::Action *> intersection;
            std::set_intersection(writeActions.sorted_begin(), writeActions.sorted_end(),
                                  writeActions2.sorted_begin(), writeActions2.sorted_end(),
                                  std::inserter(intersection, intersection.begin()));
            if (intersection.size() != 0) {
                LOG6("\t\tNoPack due to speciality read " << field);
                phv.addFieldNoPack(field, f);
            }
        }
    };

    for (auto f : fields) computeNoPackWithSpecialityRead(f);
}

// mark field used in digest as no_pack with all other bridged fields
// TODO: Overly constrained.
// This was part of the old flexible packing algorithm. I think it was
// introduced to workaround PHV allocation issues with switch profile. In order
// to remove this constraint, we need to improve the slicing algorithm. To
// reproduce the issue, try to disable these two lines and compile switch-14
// INT profile. MakeSuperClusters::visitHeaderRef must be changed in order
// to get rid of this constraint.
void CollectConstraints::computeNoPackIfDigestUse(const ordered_set<const PHV::Field *> &fields,
                                                  bool debug = false) {
    auto noPackWithAll = [&](const PHV::Field *field) {
        for (auto f : fields) {
            if (field->id == f->id) continue;
            phv.addFieldNoPack(field, f);
        }
    };

    auto computeNoPackInDigest = [&](const PHV::Field *field) {
        std::ostringstream str;
        str << std::string("Finding digest no_pack for: ");
        str << field;
        auto relatedFields = findAllRelatedFields(field);

        bool no_pack = field->is_digest();
        for (const PHV::Field *f : relatedFields) {
            str << "\n\t" << f;
            if (f->id == field->id) continue;
            if (f->is_digest()) no_pack = true;
        }
        if (no_pack) {
            str << "\tfound!";
            noPackWithAll(field);
        }
        if (debug) LOG6(str.str());
    };

    for (auto f : fields) {
        computeNoPackInDigest(f);
    }
}

Visitor::profile_t CollectConstraints::init_apply(const IR::Node *root) {
    constraints = Constraints();
    return Inspector::init_apply(root);
}

// Update alignment constraint in PhvInfo
void CollectConstraints::end_apply() {
    if (LOGGING(4)) {
        if (constraints.alignment.size() > 0) {
            LOG6("\tPrinting bridged fields with alignment constraints:");
            for (auto kv : constraints.alignment)
                for (auto c : kv.second) LOG4("\t  " << kv.first << " : " << c.getAlignment());
        }
        if (constraints.mutualAlignment.size() > 0) {
            LOG6("\tPrinting mutually aligned fields:");
            for (auto kv : constraints.mutualAlignment)
                for (auto f : kv.second) LOG4("\t  " << kv.first << " and " << f);
        }
    }

    // TODO: Here a bit of heuristics is need to generate the
    // 'best' alignment constraint for a field. The reason is the
    // special 'bit-masked-set' and 'byte-rotate-merge' instruction
    // can alleviate the pressure on alignment requirement, but we
    // do not have access to the analysis here to determine whether
    // an alignment constraint is valid or not. If we do, we can
    // encode the validity check as a constraint to filter out some
    // of the alignment constraint candidates and eliminate the
    // heuristics.
    auto rank = [&](ordered_set<AlignmentConstraint> &constraints) -> AlignmentConstraint {
        // sort the alignment by reason by inserting them in a map.
        std::map<unsigned, std::vector<AlignmentConstraint>> byReason;
        for (auto c : constraints) byReason[c.getReason()].push_back(c);

        // take alignment with the higer-valued reason
        auto align = *byReason.rbegin();

        // sort alignment withint the same reason.
        std::sort(align.second.begin(), align.second.end());

        auto ret = *align.second.rbegin();
        return ret;
    };

    for (auto align : constraints.alignment) {
        // update PhvInfo& with inferred alignment constraint
        PHV::Field *f = phv.field(align.first->name);
        BUG_CHECK(f, "De-referencing invalid field %s", align.first->name);

        // if related fields impose no alignment constraints on bridged field,
        // then the bridged field can be packed to any offset in bridged
        // header.
        if (!align.second.size()) {
            f->erase_alignment();
            LOG5("\t Clearing alignment constraint of " << align.first->name);
            continue;
        }

        // sort alignment constraint by satisfying difficulties
        auto alignment = rank(align.second);
        f->set_alignment(alignment);
        LOG5("\t Updating alignment constraint of " << align.first->name << " to "
                                                    << alignment.getAlignment());
    }

    for (auto fields : constraints.mutualAlignment) {
        auto &matrix = phv.getMutuallyAligned();
        for (auto f : fields.second) {
            matrix(fields.first->id, f->id) = true;
        }
    }

    for (auto pack : constraints.mustPack) {
        auto &matrix = phv.getBridgedExtractedTogether();
        matrix(std::get<0>(pack)->id, std::get<1>(pack)->id) = true;
    }

    for (auto pack : constraints.noPack) {
        PHV::Field *f = phv.field(pack.first->name);
        BUG_CHECK(f, "De-referencing invalid field %s", pack.first->name);
        f->set_solitary(pack.second);
    }
}

// compute do_not_pack constraints

void ConstraintSolver::add_field_alignment_constraints(cstring hdr, const PHV::Field *f,
                                                       int upper_bound) {
    z3::expr v = context.bv_const(f->name, 16);

    // lower bound for variable
    solver.add(v >= 0);

    // upper bound for variable
    solver.add(v < upper_bound);

    // alignment constraint
    auto align = f->getAlignmentConstraint();
    if (align.hasConstraint()) {
        LOG6("Alignment constraint: " << align.getAlignment() << " for " << f->name);
        solver.add(v % 8 == static_cast<int>(align.getAlignment()));

        std::stringstream str;
        str << "(";
        str << f->name;
        str << " ";
        str << align.getAlignment();
        str << ")";
        debug_info[hdr]["alignment"_cs].insert(str.str());
    }

    // optimization goal, pack everything as close to 0 as possible
    solver.minimize(v);
}

void ConstraintSolver::add_non_overlap_constraints(cstring hdr,
                                                   ordered_set<const PHV::Field *> &fields) {
    if (!fields.size()) return;

    // no overlap constraint
    using const_iterator = ordered_set<const PHV::Field *>::const_iterator;
    for (const_iterator it = fields.begin(); it != fields.end(); it++) {
        for (const_iterator it2 = it; ++it2 != fields.end();) {
            z3::expr v1 = context.bv_const((*it)->name, 16);
            z3::expr v2 = context.bv_const((*it2)->name, 16);
            solver.add((v2 - v1 >= (*it)->size) || (v1 - v2 >= (*it2)->size));

            std::stringstream str;
            str << "(";
            str << (*it)->name;
            str << " ";
            str << (*it2)->name;
            str << ")";
            debug_info[hdr]["no_overlap"_cs].insert(str.str());
        }
    }
}

void ConstraintSolver::add_extract_together_constraints(cstring hdr,
                                                        ordered_set<const PHV::Field *> &fields) {
    if (!fields.size()) return;

    // extract_together constraint
    using const_iterator = ordered_set<const PHV::Field *>::const_iterator;
    for (const_iterator it = fields.begin(); it != fields.end(); it++) {
        for (const_iterator it2 = it; ++it2 != fields.end();) {
            if (phv.are_bridged_extracted_together(*it, *it2)) {
                z3::expr v1 = context.bv_const((*it)->name, 16);
                z3::expr v2 = context.bv_const((*it2)->name, 16);
                LOG6("Copack constraint: " << v1 << " and " << v2);
                z3::expr pack_same_byte = (v1 / 8 == v2 / 8);
                solver.add(pack_same_byte);

                packingConstraints[(*it)->name].push_back(pack_same_byte);

                std::stringstream str;
                str << "(";
                str << (*it)->name;
                str << " ";
                str << (*it2)->name;
                str << ")";
                debug_info[hdr]["copack"_cs].insert(str.str());
            }
        }
    }
}

/**
 * Initializes the Z3 solver with mutually aligned constraints from PhvInfo.
 */
void ConstraintSolver::add_mutually_aligned_constraints(ordered_set<const PHV::Field *> &fields) {
    if (!fields.size()) return;

    // mutually aligned constraint
    using const_iterator = ordered_set<const PHV::Field *>::const_iterator;
    for (const_iterator it = fields.begin(); it != fields.end(); it++) {
        for (const_iterator it2 = it; ++it2 != fields.end();) {
            if (phv.are_mutually_aligned(*it, *it2)) {
                z3::expr v1 = context.bv_const((*it)->name, 16);
                z3::expr v2 = context.bv_const((*it2)->name, 16);
                LOG6("Mutually aligned constraint: " << v1 << " and " << v2);
                solver.add(v1 % 8 == v2 % 8);

                std::stringstream str;
                str << "(";
                str << (*it)->name;
                str << " ";
                str << (*it2)->name;
                str << ")";
                debug_info["_"_cs]["mutual_align"_cs].insert(str.str());
            }
        }
    }
}

void ConstraintSolver::add_solitary_constraints(cstring hdr,
                                                ordered_set<const PHV::Field *> &fields) {
    for (auto f1 : fields) {
        if (!f1->is_solitary()) continue;
        for (auto f2 : fields) {
            if (f1->id == f2->id) continue;
            z3::expr v1 = context.bv_const(f1->name, 16);
            z3::expr v2 = context.bv_const(f2->name, 16);
            //
            //   byte1     byte2    byte3
            // |......22|...111..|22.......|
            //
            // when field 1 has a solitary constraint, it occupies an entire
            // container. The start bit of field 2 must be
            // - either larger than the start_bit(f1) + size_of(f1) when
            //    rounded up to nearest byte boundary.
            // - or smaller than the start_bit(f1) when rounded down to nearest
            //    byte boundary.
            z3::expr upperBound =
                (((v1 + f1->size) % 8 == 0) && (v2 >= (((v1 + f1->size) / 8) * 8))) ||
                (((v1 + f1->size) % 8 == 1) && (v2 >= ((((v1 + f1->size) / 8) + 1) * 8)));
            z3::expr lowerBound = (v2 + f2->size) <= ((v1 / 8) * 8);
            LOG6("Solitary constraint: " << v1 << " and " << v2);
            solver.add(upperBound || lowerBound);
        }

        std::stringstream str;
        str << "(";
        str << f1->name;
        str << ")";
        debug_info[hdr]["solitary"_cs].insert(str.str());
    }
}

// certain TM metadata has a configurable shift amount, it must be placed in the bottom
// byte of a container. Within the byte, the valid start bit is 0 to (8 - size_of(f)).
// This fields are: qid, icos, meter_color, deflect_on_drop, copy_to_cpu_cos, bypass_egr,
// ct_disable, ct_mcast.
void ConstraintSolver::add_deparsed_to_tm_constraints(cstring hdr,
                                                      ordered_set<const PHV::Field *> &fields) {
    for (auto f : fields) {
        if (f->size > 8) continue;
        if (!f->deparsed_to_tm()) continue;
        if (!f->no_split()) continue;
        z3::expr v = context.bv_const(f->name, 16);
        z3::expr mustFitSingleByte = ((v / 8) * 8) == (((v + f->size - 1) / 8) * 8);
        LOG6("NoSplit constraint: " << f);
        solver.add(mustFitSingleByte);

        std::stringstream str;
        str << "(";
        str << f->name;
        str << ")";
        debug_info[hdr]["fit_one_byte"_cs].insert(str.str());
    }
}

// add no_split if related fields has no_split flag set.
void ConstraintSolver::add_no_split_constraints(cstring hdr,
                                                ordered_set<const PHV::Field *> &fields) {
    for (auto f : fields) {
        if (!f->no_split()) continue;
        z3::expr v = context.bv_const(f->name, 16);
        if (f->size <= 8) {
            if (f->no_split_container_size() != -1) continue;
            z3::expr mustFitSingleByte = ((v / 8) * 8) == (((v + f->size - 1) / 8) * 8);
            LOG6("NoSplit constraint: " << f);
            solver.add(mustFitSingleByte);

            std::stringstream str;
            str << "(";
            str << f->name;
            str << ")";
            debug_info[hdr]["fit_one_byte"_cs].insert(str.str());
        } else if (f->size <= 16) {
            z3::expr mustFitTwoBytes = ((v / 8 + 1) * 8) == (((v + f->size - 1) / 8) * 8);
            LOG6("NoSplit constraint: " << f);
            solver.add(mustFitTwoBytes);

            std::stringstream str;
            str << "(";
            str << f->name;
            str << ")";
            debug_info[hdr]["fit_two_bytes"_cs].insert(str.str());
        }
    }

    // NOTE(1):
    // Sometimes, 'no_split' bridged field has to be allocated to a large
    // container. For instance, a 9-bit bridged field is marked as 'no_split',
    // and it is also used to set value in another 32-bit 'no_split' field.  We
    // must generate extra padding after the 'no_split' bridged field during
    // packing, such that the 'no_split' bridged field is part of a 32-bit
    // cluster.  This is equivalent to generate a 'solitary' constraint on the
    // 'no_split' field assuming the field size rounds up to the large
    // container size.
    //    byte1      byte2      byte3     byte4      byte5      byte6
    // | ....... | ..1111111 | 1pppppp | pppppppp | pppppppp | ....... |
    for (auto f1 : fields) {
        if (!f1->no_split()) continue;
        if (f1->no_split_container_size() == -1) continue;
        for (auto f2 : fields) {
            if (f1->id == f2->id) continue;
            // skip if f1 and f2 has a copack constraint
            if (phv.are_bridged_extracted_together(f1, f2)) continue;
            z3::expr v1 = context.bv_const(f1->name, 16);
            z3::expr v2 = context.bv_const(f2->name, 16);

            z3::expr upperBound = (v2 >= (((v1 + f1->no_split_container_size()) / 8) * 8));
            z3::expr lowerBound = (v2 + f2->size) <= ((v1 / 8) * 8);
            LOG6("Solitary constraint for no_split field: " << f1->no_split_container_size() << " "
                                                            << v1 << " and " << v2);
            solver.add(upperBound || lowerBound);
        }

        std::stringstream str;
        str << "(";
        str << f1->name;
        str << ")";
        debug_info[hdr]["solitary"_cs].insert(str.str());
    }
}

void ConstraintSolver::add_no_pack_constraints(cstring hdr,
                                               ordered_set<const PHV::Field *> &fields) {
    if (!fields.size()) return;

    // no_pack constraint
    using const_iterator = ordered_set<const PHV::Field *>::const_iterator;
    for (const_iterator it = fields.begin(); it != fields.end(); it++) {
        for (const_iterator it2 = it; ++it2 != fields.end();) {
            if (phv.isFieldNoPack(*it, *it2)) {
                z3::expr v1 = context.bv_const((*it)->name, 16);
                z3::expr v2 = context.bv_const((*it2)->name, 16);
                LOG6("NoPack constraint: " << v1 << " and " << v2);
                z3::expr noPack = (((v1 + (*it)->size) / 8) * 8) < ((v2 / 8) * 8) ||
                                  (((v2 + (*it2)->size) / 8) * 8) < ((v1 / 8) * 8);

                packingConstraints[(*it)->name].push_back(noPack);

                std::stringstream str;
                str << "(";
                str << (*it)->name;
                str << " ";
                str << (*it2)->name;
                str << ")";
                debug_info[hdr]["copack"_cs].insert(str.str());
            }
        }
    }
}

const PHV::Field *ConstraintSolver::create_padding(int size) {
    auto padding = new PHV::Field();
    padding->padding = true;
    padding->size = size;
    return padding;
}

/**
 * Initializes the Z3 solver with constraints from PhvInfo (except for the mutually aligned
 * constraint, which is handled with ConstraintSolver::add_mutually_aligned_constraints).
 */
void ConstraintSolver::add_constraints(cstring hdr, ordered_set<const PHV::Field *> &fields) {
    for (auto f : fields) LOG6("add constraints for " << f);
    /// Computes upper bound of the overall size of all fields, which is used as a constraint
    /// to Z3 solver in the ConstraintSolver::add_field_alignment_constraints method.
    /// It assumes that each field is padded to next byte boundary.
    int upper_bound = 0;
    for (auto f : fields) {
        /// When a 'small' bridged field must be packed into a large container,
        /// we must extend the upper bound by the size of the large container,
        /// instead of the round-up value based on the bridge field size.
        if (f->no_split_container_size() != -1) {
            upper_bound += f->no_split_container_size();
        } else {
            upper_bound += f->size + (8 - f->size % 8);
        }
    }

    for (auto f : fields) add_field_alignment_constraints(hdr, f, upper_bound);
    add_non_overlap_constraints(hdr, fields);
    add_solitary_constraints(hdr, fields);
    add_extract_together_constraints(hdr, fields);
    add_deparsed_to_tm_constraints(hdr, fields);
    add_no_pack_constraints(hdr, fields);
    add_no_split_constraints(hdr, fields);
}

void ConstraintSolver::print_assertions() {
    if (LOGGING(5)) {
        LOG6("constraints " << solver.assertions());
    }
}

std::vector<const PHV::Field *> ConstraintSolver::insert_padding(
    std::vector<std::pair<unsigned, std::string>> &placement) {
    // sort by first element in pair
    std::sort(placement.begin(), placement.end());

    // At this point, we may have:
    // 3: f[0..1]
    // 7: g[0..1]
    //
    // We need to fill position 0..2, and 5..6 with padding
    std::vector<std::pair<unsigned, const PHV::Field *>> offset_and_fields;
    auto iter = placement.begin();
    if (iter->first != 0) {
        auto padding_size = iter->first;
        auto padding = create_padding(padding_size);
        offset_and_fields.push_back(std::make_pair(0, padding));
    }
    do {
        iter = std::adjacent_find(
            iter, placement.end(),
            [&](std::pair<unsigned, std::string> a, std::pair<unsigned, std::string> b) {
                auto f = phv.field(a.second);
                CHECK_NULL(f);
                return a.first + f->size != b.first;
            });
        if (iter != placement.end()) {
            auto f = phv.field(iter->second);
            CHECK_NULL(f);
            auto padding_size = (iter + 1)->first - iter->first - f->size;
            auto padding = create_padding(padding_size);
            offset_and_fields.push_back(std::make_pair(iter->first + f->size, padding));
            LOG6("Placing padding at [" << iter->first + f->size << " .. "
                                        << iter->first + f->size + padding->size << "]");
            iter++;
        }
    } while (iter != placement.end());

    // append the rest of the fields.
    ordered_set<std::string> output;
    for (auto p : placement) {
        auto f = phv.field(p.second);
        offset_and_fields.push_back(std::make_pair(p.first, f));

        BUG_CHECK(f != nullptr, "Dereferencing invalid field %s", p.second);
        // debug info
        if (LOGGING(3)) {
            auto pos = p.second.find("$");
            if (pos != std::string::npos) {
                LOG6("Placing " << p.second << " at [" << p.first << ".." << p.first + f->size - 1
                                << "]");
            }
        }
    }

    // sort by first element in pair
    std::sort(offset_and_fields.begin(), offset_and_fields.end());

    // if last element is not byte aligned, add padding
    if (offset_and_fields.size() > 0) {
        auto last = offset_and_fields.back();
        auto offset = last.first + last.second->size;
        if (offset % 8 != 0) {
            auto padding = create_padding(8 - offset % 8);
            offset_and_fields.push_back(std::make_pair(offset, padding));
            LOG6("Placing padding at [" << offset << " .. " << offset + padding->size - 1 << "]");
        }
    }

    std::vector<const PHV::Field *> packedFields;
    for (auto f : offset_and_fields) {
        packedFields.push_back(f.second);
    }
    return packedFields;
}

void ConstraintSolver::dump_unsat_core() {
    z3::solver debug(context);
    unsigned i = 0;
    ordered_map<std::string, z3::expr> assertions;
    for (auto e : solver.assertions()) {
        cstring name = cstring("p"_cs + std::to_string(i++));
        debug.add(e, name.c_str());
        assertions.emplace(name.c_str(), e);
    }
    LOG3("debugger assertions " << debug.assertions());
    auto res = debug.check();
    LOG3("debug " << res);
    z3::expr_vector core = debug.unsat_core();
    LOG3("unsat " << core);
    for (unsigned i = 0; i < core.size(); i++) {
        LOG3(assertions.at(core[i].to_string()));
    }
}

/**
 * Executes the Z3 solver.
 *
 * \param[in] fields For each header, an ordered set of fields to be packed based on
 *                   the solution of the Z3 solver.
 *
 * \return For each header, a vector of fields whose offset is adjusted based
 * on the solution of the Z3 solver with padding fields inserted where necessary.
 */
ordered_map<cstring, std::vector<const PHV::Field *>> ConstraintSolver::solve(
    ordered_map<cstring, ordered_set<const PHV::Field *>> &fields) {
    auto result = solver.check();
    if (result == z3::unsat) {
        if (LOGGING(3)) dump_unsat_core();
        ::fatal_error("Unable to pack bridged header ");
    }

    if (LOGGING(3)) {
        LOG6("model " << solver.get_model());
    }

    ordered_map<std::string, unsigned> offsets;
    for (unsigned i = 0; i < solver.get_model().num_consts(); i++) {
        auto const_decl = solver.get_model().get_const_decl(i);
        auto const_val = solver.get_model().get_const_interp(const_decl).get_numeral_uint();
        offsets.emplace(const_decl.name().str(), const_val);
    }

    ordered_map<cstring, std::vector<const PHV::Field *>> packedFields;

    for (auto fs : fields) {
        std::vector<std::pair<unsigned, std::string>> placement;
        for (auto f : fs.second) {
            if (!offsets.count(f->name.c_str()))
                error("Cannot find field name %1%", f->name.c_str());
            auto offset = offsets.at(f->name.c_str());
            placement.push_back(std::make_pair(offset, f->name.c_str()));
        }
        auto packed = insert_padding(placement);
        packedFields.emplace(fs.first, packed);
    }

    return packedFields;
}

Visitor::profile_t PackWithConstraintSolver::init_apply(const IR::Node *root) {
    // PHV::Field* from previous CollectPhvInfo are invalidated by now.
    // Must clear these maps.
    nonByteAlignedFieldsMap.clear();
    byteAlignedFieldsMap.clear();
    phvFieldToStructFieldMap.clear();
    return Inspector::init_apply(root);
}

/**
 * Looks for \@flexible header/metadata fields. For those whose witdh is a multiple
 * of eight bits, it updates the PackWithConstraintSolver::byteAlignedFieldsMap map.
 * For those whose width is not a multiple of eight bits, it updates the
 * PackWithConstraintSolver::nonByteAlignedFieldsMap map.
 *
 * Also stores the mapping from a PHV::Field to the corresponding IR::StructField
 * into the PackWithConstraintSolver::phvFieldToStructFieldMap map
 * and the mapping from a header/metadata name to the corresponding IR::Type_StructLike
 * into the PackWithConstraintSolver::headerMap map.
 */
bool PackWithConstraintSolver::preorder(const IR::HeaderOrMetadata *hdr) {
    // if we choose to only pack 'candidates' and
    // current header type is not one of the 'candidates', skip.
    for (auto c : candidates) LOG5("with candidate " << c);

    LOG5(" checking " << hdr->type->name);
    if (candidates.size() != 0 && !candidates.count(hdr->type->name)) {
        LOG5("  skip " << hdr);
        return false;
    }

    ordered_set<const PHV::Field *> nonByteAlignedFields;
    ordered_map<const PHV::Field *, const IR::StructField *> phvFieldToStructField;
    auto isNonByteAlignedFlexibleField = [&](const IR::StructField *f) {
        return f->getAnnotation("flexible"_cs) != nullptr && f->type->width_bits() % 8 != 0;
    };
    for (auto f : *hdr->type->fields.getEnumerator()->where(isNonByteAlignedFlexibleField)) {
        cstring fieldName = hdr->name + "."_cs + f->name;
        const auto *field = phv.field(fieldName);
        nonByteAlignedFields.insert(field);
        phvFieldToStructField.emplace(field, f);
        packingCandidateFields[hdr->type->name].insert(fieldName);
    }

    ordered_set<const PHV::Field *> byteAlignedFields;
    auto isByteAlignedFlexibleField = [&](const IR::StructField *f) {
        return f->getAnnotation("flexible"_cs) != nullptr && f->type->width_bits() % 8 == 0;
    };
    for (auto f : *hdr->type->fields.getEnumerator()->where(isByteAlignedFlexibleField)) {
        cstring fieldName = hdr->name + "."_cs + f->name;
        const auto *field = phv.field(fieldName);
        byteAlignedFields.insert(field);
        phvFieldToStructField.emplace(field, f);
    }

    if ((nonByteAlignedFields.size() + byteAlignedFields.size()) == 0) return false;

    if (nonByteAlignedFields.size() != 0)
        nonByteAlignedFieldsMap.emplace(hdr->type->name, nonByteAlignedFields);

    if (byteAlignedFields.size() != 0)
        byteAlignedFieldsMap.emplace(hdr->type->name, byteAlignedFields);

    if (phvFieldToStructField.size() != 0)
        phvFieldToStructFieldMap.emplace(hdr->type->name, phvFieldToStructField);

    headerMap.emplace(hdr->type->name, hdr->type);

    return false;
}

/**
 * Looks for \@flexible fields of digest field lists whose with is, and is not a multiple
 * of eight bits and updates the PackWithConstraintSolver::byteAlignedFieldsMap,
 * and PackWithConstraintSolver::nonByteAlignedFieldsMap maps, respectively.
 *
 * Also stores the mapping from a PHV::Field to the corresponding IR::StructField
 * into the PackWithConstraintSolver::phvFieldToStructFieldMap map
 * and the mapping from a header/metadata name to the corresponding IR::Type_StructLike
 * into the PackWithConstraintSolver::headerMap map.
 */
bool PackWithConstraintSolver::preorder(const IR::BFN::DigestFieldList *d) {
    if (!d->type) return false;

    // if we choose to only pack 'candidates' and
    // current header type is not one of the 'candidates', skip.
    if (candidates.size() != 0 && !candidates.count(d->type->name)) {
        LOG5("skip " << d);
        return false;
    }

    LOG6("Packing " << d->type << " fieldlist " << d);
    std::vector<std::tuple<const IR::StructField *, const IR::BFN::FieldLVal *>> sources;
    // TODO: mirror digest has 'session_id' in the first element, which is not
    // part of the mirror field list.
    auto digest = findContext<const IR::BFN::Digest>();
    if (!digest) return false;
    auto iter = d->sources.begin();
    if (digest->name == "mirror") {
        iter++;
    }
    std::transform(d->type->fields.begin(), d->type->fields.end(), iter,
                   std::back_inserter(sources),
                   [](const IR::StructField *aa, const IR::BFN::FieldLVal *bb) {
                       return std::make_tuple(aa, bb);
                   });

    ordered_set<const PHV::Field *> nonByteAlignedFields;
    ordered_map<const PHV::Field *, const IR::StructField *> phvFieldToStructField;
    auto isNonByteAlignedFlexibleField = [&](const IR::StructField *f) {
        return f->getAnnotation("flexible"_cs) != nullptr && f->type->width_bits() % 8 != 0;
    };
    for (auto s : sources) {
        if (isNonByteAlignedFlexibleField(std::get<0>(s))) {
            const auto *field = phv.field(std::get<1>(s)->field);
            nonByteAlignedFields.insert(field);
            phvFieldToStructField.emplace(field, std::get<0>(s));
        }
    }

    ordered_set<const PHV::Field *> byteAlignedFields;
    auto isByteAlignedFlexibleField = [&](const IR::StructField *f) {
        return f->getAnnotation("flexible"_cs) != nullptr && f->type->width_bits() % 8 == 0;
    };
    for (auto s : sources) {
        if (isByteAlignedFlexibleField(std::get<0>(s))) {
            const auto *field = phv.field(std::get<1>(s)->field);
            byteAlignedFields.insert(field);
            phvFieldToStructField.emplace(field, std::get<0>(s));
        }
    }

    if ((nonByteAlignedFields.size() + byteAlignedFields.size()) == 0) return false;

    if (nonByteAlignedFields.size() != 0)
        nonByteAlignedFieldsMap.emplace(d->type->name, nonByteAlignedFields);

    if (byteAlignedFields.size() != 0)
        byteAlignedFieldsMap.emplace(d->type->name, byteAlignedFields);

    if (phvFieldToStructField.size() != 0)
        phvFieldToStructFieldMap.emplace(d->type->name, phvFieldToStructField);

    headerMap.emplace(d->type->name, d->type);
    return false;
}

/**
 * Triggers adding constraints for all \@flexible fields whose width is not a multiply
 * of eight bits to the constraint solver.
 *
 * There are two steps. The first one adds constraints for individual
 * headers/metadata/digest field lists using ConstraintSolver::add_constraints.
 * The other step adds mutually aligned constraints for fields of all
 * headers/metadata/digest field lists using ConstraintSolver::add_mutually_aligned_constraints.
 *
 * If corresponding logging is enabled, the Z3 solver assertions are printed out to the log.
 *
 * \see ConstraintSolver for logging options to log the Z3 %solver assertions.
 */
void PackWithConstraintSolver::optimize() {
    for (auto f : nonByteAlignedFieldsMap) {
        for (auto g : f.second) {
            LOG4(TAB1 << f.first << " " << g->name);
        }
    }
    // add constraints related to a single field list
    for (auto v : nonByteAlignedFieldsMap) {
        solver.add_constraints(v.first, v.second);
    }

    // add constraints between multiple field lists
    ordered_set<const PHV::Field *> allNonByteAlignedFields;
    for (auto v : nonByteAlignedFieldsMap)
        allNonByteAlignedFields.insert(v.second.begin(), v.second.end());
    solver.add_mutually_aligned_constraints(allNonByteAlignedFields);

    solver.print_assertions();
}

/**
 * Runs optimizer to trigger adding constraints to the constraint solver.
 */
void PackWithConstraintSolver::end_apply() { optimize(); }

/**
 * Runs the constraint solver to get adjusted packing of \@flexible fields whose width is not
 * a multiply of eight bits and creates adjusted headers/metadata/digest field lists
 * containing these fields.
 *
 * Adjusted header/metadata/digest field list is composed as follows:
 *
 * 1. Original non-\@flexbile and non-\@padding fields
 * 2. Original \@flexible fields whose width is a multiply of eight bits
 * 3. Adjusted \@flexible fields whose width is not a multiply of eigth bits
 *    including possible padding fields obtained from the constraint solver.
 *    The padding fields are annotated with \@padding and \@overlayable.
 *
 * Adjusted headers/metadata/digest field lists are stored
 * in PackWithConstraintSolver::repackedTypes.
 */
void PackWithConstraintSolver::solve() {
    auto optimizedPackings = solver.solve(nonByteAlignedFieldsMap);

    auto create_repacked_type = [&](cstring name, std::vector<const PHV::Field *> &optimized) {
        std::vector<const PHV::Field *> packedFields;
        if (optimized.size() != 0) {
            packedFields.insert(packedFields.end(), optimized.begin(), optimized.end());
        }

        if (byteAlignedFieldsMap.count(name)) {
            auto byteAlignedFields = byteAlignedFieldsMap.at(name);
            for (auto f : byteAlignedFields) packedFields.push_back(f);
        }

        // TODO: note that the output is in reverse order.
        std::reverse(packedFields.begin(), packedFields.end());

        IR::IndexedVector<IR::StructField> fields;
        // Add non-flexible fields from the header.
        auto type = headerMap.at(name);
        for (auto f : type->fields) {
            if (!f->getAnnotation("flexible"_cs) && !f->getAnnotation("padding"_cs)) {
                fields.push_back(f);
                LOG6("Pushing original field " << f);
                continue;
            }
        }

        // Add repacked flexible fields and padding.
        unsigned padFieldId = 0;
        auto phvFieldToStructField = phvFieldToStructFieldMap.at(name);
        for (auto f : packedFields) {
            if (phvFieldToStructField.count(f)) {
                auto field = phvFieldToStructField.at(f);
                fields.push_back(field);
                LOG6("Pushing field " << field);
            } else {
                cstring padFieldName = "__pad_"_cs + cstring::to_cstring(padFieldId++);
                auto *fieldAnnotations =
                    new IR::Annotations({new IR::Annotation(IR::ID("padding"), {}),
                                         new IR::Annotation(IR::ID("overlayable"), {})});
                const IR::StructField *padField = new IR::StructField(
                    padFieldName, fieldAnnotations, IR::Type::Bits::get(f->size));

                fields.push_back(padField);
                LOG6("Pushing field " << padField << ", overlayable: " << f->overlayable);
            }
        }

        auto *newtype = new IR::Type_Header(type->name, type->annotations, fields);
        LOG6("Repacked header: " << newtype);

        repackedTypes.emplace(type->name, newtype);
    };

    for (auto op : optimizedPackings) {
        create_repacked_type(op.first, op.second);
    }
}

const IR::Node *ReplaceFlexibleType::postorder(IR::HeaderOrMetadata *h) {
    if (!repackedTypes.count(h->type->name)) return h;
    auto newType = repackedTypes.at(h->type->name);
    auto *repackedHeaderType =
        new IR::Type_Header(h->type->name, h->type->annotations, newType->fields);
    return new IR::Header(h->name, repackedHeaderType);
}

const IR::Node *ReplaceFlexibleType::postorder(IR::Type_StructLike *h) {
    if (!repackedTypes.count(h->name)) return h;
    return repackedTypes.at(h->name);
}

const IR::BFN::DigestFieldList *ReplaceFlexibleType::repackFieldList(
    cstring digest, std::vector<FieldListEntry> repackedFieldIndices,
    const IR::Type_StructLike *repackedHeaderType,
    const IR::BFN::DigestFieldList *origFieldList) const {
    // map index -> fieldlval in the field list.
    ordered_map<int, const IR::BFN::FieldLVal *> origFieldOrder;
    // repacked field list initializer
    IR::Vector<IR::BFN::FieldLVal> newSources;
    // mirror digest sources contains session_id at the beginning
    // which is not part of the p4 field list.
    auto iter = origFieldList->sources.begin();
    if (digest == "mirror") {
        newSources.push_back(*iter);
        iter++;
    }

    int index = 0;
    for (; iter != origFieldList->sources.end(); iter++) {
        origFieldOrder.emplace(index++, *iter);
    }

    // reorder field from the original field list initializer
    // according to the new field list order.
    std::map<int, const IR::BFN::FieldLVal *> repackedFieldOrder;
    index = 0;
    for (auto entry : repackedFieldIndices) {
        if (entry.first == -1) {
            repackedFieldOrder.emplace(index++,
                                       new IR::BFN::FieldLVal(new IR::Padding(entry.second)));
            continue;
        }
        repackedFieldOrder.emplace(index++, origFieldOrder.at(entry.first));
    }

    for (auto f : repackedFieldOrder) {
        IR::BFN::FieldLVal *newSource = new IR::BFN::FieldLVal(f.second->field);
        newSources.push_back(newSource);
    }
    IR::BFN::DigestFieldList *repackedFieldList =
        new IR::BFN::DigestFieldList(origFieldList->srcInfo, origFieldList->idx, newSources,
                                     repackedHeaderType, origFieldList->controlPlaneName);
    LOG6("Repacked digest: " << digest << " " << repackedFieldList);
    return repackedFieldList;
}

const IR::Node *ReplaceFlexibleType::postorder(IR::BFN::DigestFieldList *d) {
    if (!repackedTypes.count(d->type->name)) return d;
    LOG6("Replacing " << d);
    auto repackedHeaderType = repackedTypes.at(d->type->name);
    std::map<cstring, int> original_field_indices;
    int index = 0;
    for (auto f : d->type->fields) {
        original_field_indices.emplace(f->name, index++);
    }

    std::vector<FieldListEntry> repacked_field_indices;
    for (auto f : repackedHeaderType->fields) {
        if (f->getAnnotation("padding"_cs)) {
            repacked_field_indices.push_back(std::make_pair(-1, f->type));
            continue;
        }
        repacked_field_indices.push_back(
            std::make_pair(original_field_indices.at(f->name), f->type));
        LOG6("\tRepacked field " << f->name);
    }

    if (LOGGING(3)) {
        for (auto i : repacked_field_indices) {
            LOG6("\t\trepacking " << i.first << " to " << i.second);
        }
    }

    auto digest = findContext<IR::BFN::Digest>();
    BUG_CHECK(digest != nullptr, "Unable to find digest for %1%", d);
    auto rd = repackFieldList(digest->name, repacked_field_indices, repackedHeaderType, d);
    LOG6("Repacked digest type: " << repackedHeaderType);
    LOG6("Repacked digest " << rd);
    return rd;
}

const IR::Node *ReplaceFlexibleType::postorder(IR::StructExpression *expr) {
    auto isPadding = [&](const IR::StructField *f) -> bool {
        if (f->getAnnotation("padding"_cs)) return true;
        return false;
    };

    auto type = expr->type->to<IR::Type_StructLike>();
    if (!type) return expr;
    if (!repackedTypes.count(type->name)) return expr;
    auto newType = repackedTypes.at(type->name);
    auto comp = new IR::IndexedVector<IR::NamedExpression>;
    for (auto f : newType->fields) {
        if (isPadding(f)) {
            auto ne = new IR::NamedExpression(f->name, new IR::Constant(f->type, 0));
            comp->push_back(ne);
            continue;
        }

        auto decl = expr->components.getDeclaration(f->name);
        if (decl) {
            auto c = decl->to<IR::NamedExpression>();
            comp->push_back(c);
        }
    }
    return new IR::StructExpression(expr->structType, *comp);
}

// helper function
bool findFlexibleAnnotation(const IR::Type_StructLike *header) {
    for (auto f : header->fields) {
        auto anno = f->getAnnotation("flexible"_cs);
        if (anno != nullptr) return true;
    }
    return false;
}

// We can do better with these IR::BFN::DeparserParameters in terms of packing. This IR type is only
// used to represent intrinsic metadata in ingress and egress at the moment. As noted in the
// comments in line 698 in phv_fields.cpp, the constraints on bottom_bit for some of these TM
// metadata is not necessary. We can theoretically place certain TM metadata field, such as
// bypass_egress to any bit in a byte. The implication for bridge metadata packing is that
// bypass_egress could be packed with any field anywhere. There are a few other TM metadata with the
// same property: ct_disable, ct_mcast, qid, icos, meter_color, deflect_on_drop, copy_to_cpu_cos.
bool GatherDeparserParameters::preorder(const IR::BFN::DeparserParameter *p) {
    BUG_CHECK(p->source, "Parameter %1% missing source", p->name);
    const PHV::Field *f = phv.field(p->source->field);
    BUG_CHECK(f, "Field %1% not found while gathering deparser parameters", p->source->field);
    LOG5("    Deparser parameter identified: " << f);
    params.insert(f);
    return true;
}

// TODO: Remove reliance on phase0 parser state name by visiting Phase0 node in
// IR::BFN::Parser and use phase0->fields to gather Phase0 fields. The fields
// must be converted from an IR::StructField to a PHV::Field.
// TODO: Another chance for optimization is by allowing flexible packing of
// fields. Right now, the fields are packed to Phase0 bit width (Tofino = 64 and
// JBAY = 128) with padding inserted. This does not allow flexibility while
// packing fields into containers. The packing is specified in tna.cpp &
// arch/phase0.cpp - canPackDataIntoPhase0() method. The packing code can be
// removed and we can work with only the fields in Phase0. Packing width can be
// determined by Device::pardeSpec().bitPhase0Width()
bool GatherPhase0Fields::preorder(const IR::BFN::ParserState *p) {
    // Not a phase 0 parser state
    if (p->name != PHASE0_PARSER_STATE_NAME) return true;
    // For phase 0 parser state, detect the fields written to.
    for (auto s : p->statements) {
        auto *e = s->to<IR::BFN::Extract>();
        if (!e) continue;
        auto *dest = e->dest->to<IR::BFN::FieldLVal>();
        if (!dest) continue;
        const auto *f = phv.field(dest->field);
        if (!f) continue;
        LOG6("CONSTRAINT must be packed alone A " << f);
        noPackFields.insert(f);
    }
    return true;
}

// TODO: if a field is used in both bridging and mirror/resubmit/digest, we do
// not pack these fields with other fields. The compiler should be able to do
// better.
bool GatherPhase0Fields::preorder(const IR::BFN::DigestFieldList *d) {
    for (auto source : d->sources) {
        const auto *field = phv.field(source->field);
        if (!field) continue;
        LOG6("CONSTRAINT must be packed alone B " << field);
        noPackFields.insert(field);
    }
    return false;
}

std::string LogRepackedHeaders::strip_prefix(cstring str, std::string pre) {
    std::string s = str + "";
    // Find the first occurence of the prefix
    size_t first = s.find(pre, 0);

    // If it's not at 0, then we haven't found a prefix
    if (first != 0) return s;

    // Otherwise, we have a match and can trim it
    return s.substr(pre.length(), std::string::npos);
}

// Collect repacked headers
bool LogRepackedHeaders::preorder(const IR::HeaderOrMetadata *h) {
    // Check if we have visited this header's ingress/egress version before
    cstring hname = h->name;
    std::string h_in = strip_prefix(hname, "ingress::");
    std::string h_out = strip_prefix(hname, "egress::");
    if (hdrs.find(h_in) != hdrs.end() || hdrs.find(h_out) != hdrs.end()) return false;
    // Otherwise, add the shorter one (the one with the prefix stripped) to our set
    std::string h_strip;
    if (h_in.length() > h_out.length())
        h_strip = h_out;
    else
        h_strip = h_in;
    hdrs.emplace(h_strip);

    // Check if this header may have been repacked by looking for flexible fields
    bool isRepacked = false;
    for (auto f : *h->type->fields.getEnumerator()) {
        if (f->getAnnotation("flexible"_cs)) {
            isRepacked = true;
            break;
        }
    }

    // Add it to our vector if it is repacked
    if (isRepacked) {
        std::pair<const IR::HeaderOrMetadata *, std::string> pr(h, h_strip);
        repacked.push_back(pr);
    }

    // Prune this branch because headers won't have headers as children
    return false;
}

// Pretty print all of the headers
void LogRepackedHeaders::end_apply() {
    // Check if we should be logging anything
    if (LOGGING(1))
        // Iterate through the headers and print all of their fields
        for (auto h : repacked) LOG1(pretty_print(h.first, h.second));
}

// TODO: Currently outputting backend name for each field. Should be changed to user facing name.
std::string LogRepackedHeaders::getFieldName(std::string hdr, const IR::StructField *f) const {
    auto nm = hdr + "." + f->name;
    auto *fi = phv.field(nm);
    auto name = fi ? fi->name : nm;
    std::string s = name + "";
    return s;
}

std::string LogRepackedHeaders::pretty_print(const IR::HeaderOrMetadata *h, std::string hdr) const {
    // Number of bytes we have used
    unsigned byte_ctr = 0;
    // Number of bits in the current byte we have used
    unsigned bits_ctr = 0;

    std::stringstream out;
    out << "Repacked header " << hdr << ":" << std::endl;

    // Create our table printer
    TablePrinter *tp =
        new TablePrinter(out, {"Byte #", "Bits", "Field name"}, TablePrinter::Align::CENTER);
    tp->addSep();

    // Run through the fields. We divide each field into 3 sections: (1) the portion that goes into
    // the current byte; (2) the portion that goes into byte(s) in the middle; and (3) the portion
    // that goes into the last byte. As we process each field, we may want to change how previous
    // portions were allocated. For example, if we realize (2) will be a range and (1) was a full
    // byte, then (1) should get merged into (2). Thus, we will capture the printing (to
    // TablePrinter) of each portion as a copy-capture lambda. This allows us to do "lazy
    // evaluation" and effectively modify what is printed after in earlier portions.
    for (auto f : h->type->fields) {
        // PORTION(1): This section will always exist, but it may end up getting included into
        // portion 2
        std::function<void(void)> write_beg = [=]() { return; };
        // PORTION(2): This section may be a range (of full bytes), a single full byte or be empty.
        std::function<void(void)> write_mid = [=]() { return; };
        // PORTION(3): This section is necessary when the field doesn't end up completely filling
        // the last byte of the mid.
        std::function<void(void)> write_end = [=]() { return; };

        // Get the field name. If it's a pad, change to *PAD*
        std::string name = getFieldName(hdr, f);
        if (name.find("__pad_", 0) != std::string::npos) name = "*PAD*";

        // Need to calculate how many bytes and what bits this field takes up
        unsigned width = f->type->width_bits();
        // Remaining bits in the current byte
        unsigned rem_bits = 8 - bits_ctr;
        // True if this field overflows the current byte
        bool ofByte = width > rem_bits;

        // PORTION (1): First, we add to the current byte.
        // Last bit is the next open bit
        unsigned last_bit = ofByte ? 8 : bits_ctr + width;
        // If this first byte is full, we'll need to print a separator.
        bool first_full = last_bit == 8;
        // If this byte is completely occupied by this field, it may need to be a range
        bool first_occu = first_full && bits_ctr == 0;
        write_beg = [=]() {
            tp->addRow({std::to_string(byte_ctr),
                        "[" + std::to_string(bits_ctr) + " : " + std::to_string(last_bit - 1) + "]",
                        name});
            if (first_full) tp->addSep();
        };

        // Update bits/byte counter and width after finishing 1st byte
        bits_ctr = last_bit % 8;
        byte_ctr = first_full ? byte_ctr + 1 : byte_ctr;
        width = width - rem_bits;

        // PORTION (2)/(3): Only need to handle this portion if we did overflow
        if (ofByte) {
            // See what byte/bit we'll fill up to. The last bit is the next open bit
            unsigned end_byte = byte_ctr + width / 8;
            last_bit = width % 8;

            // PORTION(2): Now, we want to handle any bytes that are completely filled by this
            // field. We want to put multiple bytes that are occupied by the same field into a range
            // instead of explicitly printing out each one
            if (end_byte - byte_ctr >= 1) {
                // If we're in this conditional, we know that we have at least 1 full byte, but
                // that's not enough to print a range, so:
                if (end_byte - byte_ctr >= 2 || first_occu) {
                    // Now, we have at least 2 bytes that are full, so we can print a range.
                    unsigned beg_byte = byte_ctr;
                    // If the first was completely occupied by this field, include it in the range
                    // and don't do anything in write_beg
                    if (first_occu) {
                        beg_byte--;
                        write_beg = [=]() { return; };
                    }
                    // Add the range row
                    write_mid = [=]() {
                        tp->addRow(
                            {std::to_string(beg_byte) + " -- " + std::to_string(end_byte - 1),
                             "[" + std::to_string(0) + " : " + std::to_string(7) + "]", name});
                        tp->addSep();
                        return;
                    };
                } else {
                    // Here we know our mid portion is going to be just the single full byte we have
                    write_mid = [=]() {
                        tp->addRow({std::to_string(byte_ctr),
                                    "[" + std::to_string(0) + " : " + std::to_string(7) + "]",
                                    name});
                        tp->addSep();
                        return;
                    };
                }
            }

            // PORTION(3): We now need to handle the partial byte that might be leftover
            if (last_bit != 0) {
                write_end = [=]() {
                    tp->addRow(
                        {std::to_string(end_byte),
                         "[" + std::to_string(0) + " : " + std::to_string((last_bit - 1)) + "]",
                         name});
                    return;
                };
            }

            // Now, we update our counters
            byte_ctr = end_byte;
            bits_ctr = last_bit;
        }
        // Finally, write everything to the table printer
        write_beg();
        write_mid();
        write_end();
    }

    // Print the table to the stream out and return it
    tp->print();
    out << std::endl;
    return out.str();
}

GatherPackingConstraintFromSinglePipeline::GatherPackingConstraintFromSinglePipeline(
    PhvInfo &p, const PhvUse &u, DependencyGraph &dg, const BFN_Options &o,
    PackWithConstraintSolver &sol)
    : options(o),
      pa_no_pack(p),
      packConflicts(p, dg, tMutex, table_alloc, aMutex, pa_no_pack),
      actionConstraints(p, u, packConflicts, tableActionsMap, dg),
      packWithConstraintSolver(sol) {
    addPasses({
        new FindDependencyGraph(p, dg, &options),
        new PHV_Field_Operations(p),
        new PragmaContainerSize(p),
        new PragmaAtomic(p),
        new PragmaSolitary(p),
        &pa_no_pack,
        &tMutex,
        &aMutex,
        &packConflicts,
        &tableActionsMap,
        &actionConstraints,
        new GatherDeparserParameters(p, deparserParams),
        new GatherPhase0Fields(p, noPackFields),
        new GatherAlignmentConstraints(p, actionConstraints),
        &packWithConstraintSolver,
    });
}

// Return a Json representation of flexible headers to be saved in .bfa/context.json
std::string LogRepackedHeaders::asm_output() const {
    if (repacked.empty()) return std::string("");

    std::ostringstream out;
    bool first_header = true;
    out << "flexible_headers: [\n";  // list of headers
    for (auto h : repacked) {
        if (!first_header) {
            out << ",\n";
        } else {
            out << "\n";
            first_header = false;
        }

        out << "  { name: \"" << h.second << "\",\n"
            << "    fields: [\n";  // list of fields

        bool first_field = true;
        for (auto f : h.first->type->fields) {
            auto name = f->name.name;  // getFieldName(h.second, f);
            // for now all the fields are full fields not slices
            unsigned width = f->type->width_bits();
            unsigned start_bit = 0;  // so all fields start at 0.

            if (!first_field) {
                out << ",\n";
            } else {
                out << "\n";
                first_field = false;
            }

            out << "      { name: \"" << name << "\", slice: { " << "start_bit: " << start_bit
                << ", bit_width: "
                << width
                // << ", slice_name:" << "the name of the slice"
                << " } }";
        }
        out << "    ]\n";  // list of fields
        out << "  }";      // header
    }
    out << "]\n";  // list of headers
    return out.str();
}

/**
 * candidates: if non-empty, indicate the set of header types to pack.
 *             if empty, all eligible header types are packed.
 */
PackFlexibleHeaders::PackFlexibleHeaders(const BFN_Options &options,
                                         ordered_set<cstring> &candidates, RepackedHeaderTypes &map)
    : uses(phv),
      defuse(phv),
      solver(context),
      constraint_solver(phv, context, solver, packingConstraints, constraints, debug_info),
      packWithConstraintSolver(phv, constraint_solver, candidates, map, packingCandidateFields,
                               packingConstraints) {
    flexiblePacking = new GatherPackingConstraintFromSinglePipeline(phv, uses, deps, options,
                                                                    packWithConstraintSolver);
    flexiblePacking->addDebugHook(options.getDebugHook(), true);
    PragmaNoOverlay *noOverlay = new PragmaNoOverlay(phv);
    PragmaAlias *pragmaAlias = new PragmaAlias(phv, *noOverlay);
    addPasses({new CreateThreadLocalInstances, new BFN::CollectHardwareConstrainedFields,
               new CheckForUnimplementedFeatures(), new RemoveEmptyControls,
               // new MultipleApply(options, std::nullopt, false, false),
               new AddSelectorSalu, new FixupStatefulAlu,
               new CollectHeaderStackInfo,  // Needed by CollectPhvInfo.
               new CollectPhvInfo(phv), &defuse,
               Device::hasMetadataPOV() ? new AddMetadataPOV(phv) : nullptr,
               new CollectPhvInfo(phv), &defuse,
               new CollectHeaderStackInfo,  // Needs to be rerun after CreateThreadLocalInstances,
                                            // but cannot be run after InstructionSelection.
               new RemovePushInitialization, new StackPushShims,
               new CollectPhvInfo(phv),  // Needs to be rerun after CreateThreadLocalInstances.
               new HeaderPushPop, new GatherReductionOrReqs(deps.red_info),
               new InstructionSelection(options, phv, deps.red_info),
               new FindDependencyGraph(phv, deps, &options, "program_graph"_cs,
                                       "Pack Flexible Headers"_cs),
               new CollectPhvInfo(phv), pragmaAlias, new AutoAlias(phv, *pragmaAlias, *noOverlay),
               new Alias(phv, *pragmaAlias), new CollectPhvInfo(phv),
               // Run after InstructionSelection, before deadcode elimination.
               flexiblePacking});

    if (options.excludeBackendPasses) {
        try {
            removePasses(options.passesToExcludeBackend);
        } catch (const std::runtime_error &e) {
            // Ignore this error since this is only a subset of the backend passes
            LOG6(e.what());
        }
    }
}

void PackFlexibleHeaders::check_conflicting_constraints() {
    for (auto &[hdr, fields] : packingCandidateFields) {
        for (auto &f : fields) {
            if (packingConstraints.count(f) == 0) continue;
            z3::expr_vector constraints(context);
            for (auto c : packingConstraints.at(f)) {
                constraints.push_back(c);
            }
            if (solver.check(constraints) == z3::unsat) {
                LOG6("ignore conflicting constraints: " << constraints);
            } else {
                for (const auto &c : constraints) {
                    solver.add(c);
                }
            }
        }
    }
}

void PackFlexibleHeaders::end_apply() {
    if (LOGGING(2)) {
        for (auto hdr : debug_info) {
            LOG6("(header " << hdr.first);
            for (auto constr = hdr.second.begin(); constr != hdr.second.end(); ++constr) {
                LOG6("  (constraint " << (*constr).first);
                for (auto iter = (*constr).second.begin(); iter != (*constr).second.end(); ++iter) {
                    LOG6("    " << *iter << ((std::next(iter) != (*constr).second.end()) ? "" : ")")
                                << ((std::next(constr) != hdr.second.end()) ? "" : ")"));
                }
            }
        }
    }
}

// Collect the 'emit' and 'extract' call on @flexible headers. We need to
// consider constraints both briding and mirror/resubmit, as a field may
// be used in both places.
void CollectBridgedFieldsUse::postorder(const IR::MethodCallExpression *expr) {
    auto mi = P4::MethodInstance::resolve(expr, refMap, typeMap);
    if (!mi) return;
    auto em = mi->to<P4::ExternMethod>();
    if (!em) return;
    if (em->method->name != "emit" && em->method->name != "extract") return;

    auto type_name = em->originalExternType->name;
    if (type_name != "packet_in" && type_name != "packet_out" && type_name != "Resubmit" &&
        type_name != "Mirror")
        return;

    std::optional<gress_t> thread = std::nullopt;
    auto parser = findContext<IR::BFN::TnaParser>();
    if (parser) thread = parser->thread;

    auto deparser = findContext<IR::BFN::TnaDeparser>();
    if (deparser) thread = deparser->thread;

    // neither parser or deparser
    if (thread == std::nullopt) return;

    // expected number of argument
    size_t arg_count = 1;
    if (type_name == "Mirror") arg_count = 2;

    if (expr->arguments->size() < arg_count) return;

    auto dest = (*expr->arguments)[arg_count - 1]->expression;
    const IR::Type *type = nullptr;
    if (auto *hdr = dest->to<IR::HeaderRef>()) {
        type = hdr->type;
    } else if (auto *hdr = dest->to<IR::StructExpression>()) {
        type = hdr->type;
    }

    if (type == nullptr) return;

    bool need_packing = false;
    if (auto ty = type->to<IR::Type_StructLike>()) {
        if (findFlexibleAnnotation(ty)) {
            need_packing = true;
        }
    } else if (type->is<IR::BFN::Type_FixedSizeHeader>()) {
        need_packing = true;
    }
    if (!need_packing) return;

    Use u;
    u.type = type;
    u.method = em->method->name;
    u.thread = *thread;

    if (auto ty = type->to<IR::Type_StructLike>()) u.name = ty->name;

    bridge_uses.insert(u);
}

class PostMidEndLast : public PassManager {
 public:
    PostMidEndLast() { setName("PostMidEndLast"); }
};

/**
 * TODO: some code duplication with extract_mau_pipe, needs to refactor ParseTna to
 * provide a mechanism to iterate directly on TNA pipelines.
 */
bool ExtractBridgeInfo::preorder(const IR::P4Program *program) {
    BFN::ApplyEvaluator eval(refMap, typeMap);
    auto new_program = program->apply(eval);

    auto toplevel = eval.getToplevelBlock();
    BUG_CHECK(toplevel, "toplevel cannot be nullptr");

    auto main = toplevel->getMain();
    auto arch = new BFN::ParseTna(refMap, typeMap);
    main->apply(*arch);

    /// SimplifyReferences passes are fixup passes that modifies the visited IR tree.
    /// Unfortunately, the modifications by simplifyReferences will transform IR tree towards
    /// the backend IR, which means we can no longer run typeCheck pass after applying
    /// simplifyReferences to the frontend IR.
    auto simplifyReferences = new SimplifyReferences(bindings, refMap, typeMap);

    // collect and set global_pragmas
    new_program->apply(collect_pragma);

    auto npipe = 0;
    for (auto pkg : main->constantValue) {
        // collect per pipe bridge uses
        CollectBridgedFieldsUse collectBridgedFields(refMap, typeMap);
        if (!pkg.second) continue;
        if (!pkg.second->is<IR::PackageBlock>()) continue;
        std::vector<gress_t> gresses = {INGRESS, EGRESS};
        for (auto gress : gresses) {
            if (!arch->pipelines.getPipeline(npipe).threads.count(gress)) {
                error("Unable to find thread %1%", npipe);
                return false;
            }
            auto thread = arch->pipelines.getPipeline(npipe).threads.at(gress);
            thread = thread->apply(*simplifyReferences);
            for (auto p : thread->parsers) {
                if (auto parser = p->to<IR::BFN::TnaParser>()) {
                    parser->apply(collectBridgedFields);
                }
            }
            if (auto dprsr = dynamic_cast<const IR::BFN::TnaDeparser *>(thread->deparser)) {
                dprsr->apply(collectBridgedFields);
            }
        }
        all_uses.emplace(npipe, collectBridgedFields.bridge_uses);
        npipe++;
    }

    return false;
}

std::vector<const IR::BFN::Pipe *> *ExtractBridgeInfo::generate_bridge_pairs(
    std::vector<BridgeContext> &all_context) {
    std::vector<BridgeContext> all_emits;
    std::vector<BridgeContext> all_extracts;

    for (auto context : all_context) {
        if (context.use.method == "emit") {
            all_emits.push_back(context);
        } else if (context.use.method == "extract") {
            all_extracts.push_back(context);
        }
    }

    auto pipes = new std::vector<const IR::BFN::Pipe *>();
    for (auto emit : all_emits) {
        for (auto extract : all_extracts) {
            // cannot bridge from ingress to ingress or egress to egress
            if (emit.use.thread == extract.use.thread) continue;
            // can only bridge from egress to ingress in the same pipe
            if (emit.use.thread == EGRESS && extract.use.thread == INGRESS &&
                emit.pipe_id != extract.pipe_id)
                continue;
            LOG6("pipe " << emit.pipe_id << " " << emit.use << " pipe " << extract.pipe_id << " "
                         << extract.use);
            auto pipe = new IR::BFN::Pipe;
            // emit and extract has the same pointer to pragma
            pipe->global_pragmas = collect_pragma.global_pragmas();
            pipe->thread[INGRESS] = emit.thread;
            pipe->thread[EGRESS] = extract.thread;
            pipes->push_back(pipe);
        }
    }

    return pipes;
}

void ExtractBridgeInfo::end_apply(const IR::Node *) {
    ordered_map<cstring, ordered_set<std::pair<int, CollectBridgedFieldsUse::Use>>> uses_by_name;
    for (auto kv : all_uses) {
        for (auto u : kv.second) {
            uses_by_name[u.name].insert(std::make_pair(kv.first, u));
        }
    }
    ordered_set<cstring> candidates;
    for (auto kv : uses_by_name) candidates.insert(kv.first);

    auto pack = new PackFlexibleHeaders(options, candidates, map);

    for (auto kv : uses_by_name) {
        std::vector<BridgeContext> bridge_infos;
        for (auto u : kv.second) {
            auto pipe_id = u.first;
            auto pipe = conv->getPipes().at(pipe_id);
            auto thread = pipe->thread[u.second.thread];
            BridgeContext ctxt = {pipe_id, u.second, thread};
            bridge_infos.push_back(ctxt);
        }

        auto bridgePath = generate_bridge_pairs(bridge_infos);

        for (auto p : *bridgePath) {
            LOG6("Collect flexible fields for " << kv.first);
            pack->set_pipeline_name(p->canon_name());
            p->apply(*pack);
        }
    }

    pack->check_conflicting_constraints();
    pack->solve();
}

// Invariants:
//  - must be applied to IR::P4Program node that can be type-checked
//  - must generated IR::P4Program node that can still be type-checked.
// Post condition:
//  - vector of backend pipes for the corresponding p4 program.
//  - if bridge_packing is enabled, a transformed IR::P4Program
//        with @flexible header repacked.
// SimplifyReference transforms IR::P4Program towards the
// backend IR representation, as a result, the transformed
// P4Program no longer type-check.
BridgedPacking::BridgedPacking(BFN_Options &options, RepackedHeaderTypes &map,
                               CollectSourceInfoLogging &sourceInfoLogging)
    : map(map) {
    refMap.setIsV1(true);
    bindings =
        new ParamBinding(&typeMap, options.langVersion == CompilerOptions::FrontendVersion::P4_14);
    conv = new BFN::BackendConverter(&refMap, &typeMap, bindings, pipe, pipes, sourceInfoLogging);
    evaluator = new BFN::ApplyEvaluator(&refMap, &typeMap);
    extractBridgeInfo = new ExtractBridgeInfo(options, &refMap, &typeMap, conv, bindings, map);
    auto typeChecking = new BFN::TypeChecking(&refMap, &typeMap, true);

    addPasses({
        new P4::ClearTypeMap(&typeMap),
        typeChecking,
        new BFN::ConvertSizeOfToConstant(),
        new PassRepeated({new P4::ConstantFolding(&typeMap, true, typeChecking),
                          new P4::StrengthReduction(&typeMap, typeChecking),
                          new P4::Reassociation(), new P4::UselessCasts(&typeMap)}),
        typeChecking,
        new RenameArchParams(&refMap, &typeMap),
        new FillFromBlockMap(&refMap, &typeMap),
        evaluator,
        bindings,
        conv,  // map name to IR::BFN::Pipe
        extractBridgeInfo,
        new PadFixedSizeHeaders(map),
    });
}

SubstitutePackedHeaders::SubstitutePackedHeaders(BFN_Options &options,
                                                 const RepackedHeaderTypes &map,
                                                 CollectSourceInfoLogging &sourceInfoLogging) {
    refMap.setIsV1(true);
    bindings =
        new ParamBinding(&typeMap, options.langVersion == CompilerOptions::FrontendVersion::P4_14);
    conv = new BFN::BackendConverter(&refMap, &typeMap, bindings, pipe, pipes, sourceInfoLogging);
    evaluator = new BFN::ApplyEvaluator(&refMap, &typeMap);
    auto typeChecking = new BFN::TypeChecking(&refMap, &typeMap, true);

    addPasses({
        new ReplaceFlexibleType(map),
        new P4::ClearTypeMap(&typeMap),
        typeChecking,
        new BFN::ConvertSizeOfToConstant(),
        new PassRepeated({new P4::ConstantFolding(&typeMap, true, typeChecking),
                          new P4::StrengthReduction(&typeMap, typeChecking),
                          new P4::Reassociation(), new P4::UselessCasts(&typeMap)}),
        typeChecking,
        new RenameArchParams(&refMap, &typeMap),
        new FillFromBlockMap(&refMap, &typeMap),
        evaluator,
        bindings,
        conv,
        new PostMidEndLast,
    });
}

std::ostream &operator<<(std::ostream &out, const CollectBridgedFieldsUse::Use &u) {
    out << u.method;
    if (auto type = u.type->to<IR::Type_StructLike>()) {
        out << " " << type->name.name;
    }
    out << " in " << u.thread << ";";
    return out;
}
