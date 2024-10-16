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

#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir-generated.h"
#include "lib/log.h"
#include "lib/stringref.h"

//***********************************************************************************
//
// class Phv_Parde_Mau_Use
//
// preorder walk on IR tree to compute use information
//
//***********************************************************************************

Visitor::profile_t Phv_Parde_Mau_Use::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);
    in_mau = false;
    in_dep = false;
    use_i.clear();
    for (auto &x : deparser_i) x.clear();
    for (auto &x : extracted_i) x.clear();
    for (auto &x : extracted_from_pkt_i) x.clear();
    for (auto &x : extracted_from_const_i) x.clear();
    written_i.clear();
    used_alu_i.clear();
    ixbar_read_i.clear();
    learning_reads_i.clear();
    return rv;
}

bool Phv_Parde_Mau_Use::preorder(const IR::BFN::Parser *p) {
    LOG5("PREORDER Parser ");
    in_mau = false;
    in_dep = false;
    thread = p->gress;
    revisit_visited();
    return true;
}

bool Phv_Parde_Mau_Use::preorder(const IR::BFN::Extract *e) {
    LOG5("PREORDER Extract ");
    auto lval = e->dest->to<IR::BFN::FieldLVal>();
    if (!lval) return true;
    auto* f = phv.field(lval->field);
    BUG_CHECK(f, "Extract to non-PHV destination: %1%", e);
    extracted_i[thread][f->id] = true;

    auto ibufRVal = e->source->to<IR::BFN::PacketRVal>();
    if (ibufRVal) {
        extracted_from_pkt_i[thread][f->id] = true;
        // std::cerr << "EFP: " << f->id << std::endl;
    }
    auto crval = e->source->to<IR::BFN::ConstantRVal>();
    if (crval)
        extracted_from_const_i[thread][f->id] = true;
    return true;
}

bool Phv_Parde_Mau_Use::preorder(const IR::BFN::ParserChecksumWritePrimitive *e) {
    LOG5("PREORDER Checksum " << e);
    auto lval = e->getWriteDest();
    if (!lval) return true;
    auto* f = phv.field(lval->field);
    BUG_CHECK(f, "Checksum write to non-PHV destination: %1%", e);
    extracted_i[thread][f->id] = true;

    // TODO: something like extracted_from_checksum needed?
    return true;
}

bool Phv_Parde_Mau_Use::preorder(const IR::BFN::Deparser *d) {
    LOG5("PREORDER Deparser ");
    thread = d->gress;
    in_mau = false;
    in_dep = true;
    revisit_visited();
    return true;
}

bool Phv_Parde_Mau_Use::preorder(const IR::BFN::Digest* digest) {
    if (digest->name == "learning") {
        const PHV::Field* selector = phv.field(digest->selector->field);
        learning_reads_i.insert(selector);
        for (auto fieldList : digest->fieldLists) {
            for (auto flval : fieldList->sources) {
                if (const auto *f = phv.field(flval->field)) {
                    learning_reads_i.insert(f);
                    LOG1("save learning: " << f);
                }
            }
        }
    }
    return true;
}

bool Phv_Parde_Mau_Use::preorder(const IR::MAU::TableSeq *) {
    LOG5("PREORDER TableSeq ");
    // FIXME -- treat GHOST thread as ingress for PHV allocation
    if ((thread = VisitingThread(this)) == GHOST)
        thread = INGRESS;
    in_mau = true;
    in_dep = false;
    revisit_visited();
    return true;
}

bool Phv_Parde_Mau_Use::preorder(const IR::MAU::TableKey * key) {
    LOG5("PREORDER TableKey: " << *key);
    const auto* table = findContext<IR::MAU::Table>();
    BUG_CHECK(table, "no table for key: %1%", key);
    le_bitrange bits;
    const PHV::Field* f = phv.field(key->expr, &bits);
    if (!f) return true;
    LOG5("Found table key field: " << f->name << bits);
    ixbar_read_i[f][bits].insert({table, key});
    return true;
}

bool Phv_Parde_Mau_Use::preorder(const IR::Expression *e) {
    LOG5("PREORDER Expression " << *e);
    if (auto *hr = e->to<IR::HeaderRef>()) {
        for (auto id : phv.struct_info(hr).field_ids()) {
            auto* field = phv.field(id);
            CHECK_NULL(field);
            use_i[id][in_mau].insert(StartLen(0, field->size));
            deparser_i[thread][id] = in_dep; } }

    le_bitrange bits;
    if (auto* info = phv.field(e, &bits)) {
        // Used in MAU.
        LOG5("use " << info->name << " in " << thread << (in_mau ? " mau" : ""));
        use_i[info->id][in_mau].insert(bits);
        LOG5("is_used_mau: " << is_used_mau(info));

        // Used in ALU instruction.
        if (findContext<IR::MAU::Instruction>() != nullptr) {
            LOG5("use " << info->name << " in ALU instruction");
            used_alu_i[info->id].insert(bits);
        }

        // Used in deparser.
        LOG5("dep " << info->name << " in " << thread << (in_dep ? " dep" : ""));
        deparser_i[thread][info->id] = in_dep;

        if (isWrite() && in_mau) {
            written_i[info->id].insert(bits);
        }
        return false;
    } else if (e->is<IR::Member>()) {  // prevent descent into IR::Member objects
        LOG5("  ... member expr");
        return false;
    }
    return true;
}

//
// Phv_Parde_Mau_Use routines that check use_i[]
//

bool Phv_Parde_Mau_Use::is_referenced(const PHV::Field *f) const {      // use in mau or parde
    BUG_CHECK(f, "Null field");
    if (f->bridged) {
        // bridge metadata
        return true;
    }
    return is_used_mau(f) || is_used_parde(f);
}

bool Phv_Parde_Mau_Use::is_deparsed(const PHV::Field *f) const {      // use in deparser
    BUG_CHECK(f, "Null field");
    bool use_deparser = deparser_i[f->gress][f->id] || f->deparsed();
    return use_deparser;
}

bool Phv_Parde_Mau_Use::is_used_mau(const PHV::Field *f) const {      // use in mau
    BUG_CHECK(f, "Null field");

    if (!use_i.count(f->id)) return false;
    return use_i.at(f->id).count(MAU);
}

bool Phv_Parde_Mau_Use::is_used_mau(const PHV::Field* f, le_bitrange range) const {
    BUG_CHECK(f, "Null field");
    if (!use_i.count(f->id)) return false;
    if (!use_i.at(f->id).count(MAU)) return false;
    for (auto r : use_i.at(f->id).at(MAU)) {
        if (range.overlaps(r))
            return true;
    }
    return false;
}

bool Phv_Parde_Mau_Use::is_used_alu(const PHV::Field *f) const {      // use in alu
    BUG_CHECK(f, "Null field");
    return used_alu_i.count(f->id);
}

bool Phv_Parde_Mau_Use::is_written_mau(const PHV::Field *f) const {
    BUG_CHECK(f, "Null field");
    return written_i.count(f->id);;
}

bool Phv_Parde_Mau_Use::is_used_parde(const PHV::Field *f) const {    // use in parser/deparser
    BUG_CHECK(f, "Null field");
    bool use_i_pd;
    if (!use_i.count(f->id))
        use_i_pd = false;
    else
        use_i_pd = use_i.at(f->id).count(PARDE);
    bool use_pd = use_i_pd || extracted_i[f->gress][f->id];
    return use_pd;
}

bool Phv_Parde_Mau_Use::is_extracted(const PHV::Field *f, std::optional<gress_t> gress) const {
    BUG_CHECK(f, "Null field");
    if (!gress)
        return extracted_i[INGRESS][f->id] || extracted_i[EGRESS][f->id];
    else
        return extracted_i[*gress][f->id];
}

bool Phv_Parde_Mau_Use::is_extracted_from_pkt(const PHV::Field *f,
        std::optional<gress_t> gress) const {
    BUG_CHECK(f, "Null field");
    if (!gress)
        return extracted_from_pkt_i[INGRESS][f->id] || extracted_from_pkt_i[EGRESS][f->id];
    else
        return extracted_from_pkt_i[*gress][f->id];
}

bool Phv_Parde_Mau_Use::is_extracted_from_constant(const PHV::Field *f,
        std::optional<gress_t> gress) const {
    BUG_CHECK(f, "Null field");
    if (!gress)
        return extracted_from_const_i[INGRESS][f->id] || extracted_from_const_i[EGRESS][f->id];
    else
        return extracted_from_const_i[*gress][f->id];
}

bool Phv_Parde_Mau_Use::is_allocation_required(const PHV::Field *f) const {
    return !f->is_ignore_alloc() && (is_referenced(f) || f->isGhostField());
}

ordered_set<std::pair<const IR::MAU::Table *, const IR::MAU::TableKey *>>
Phv_Parde_Mau_Use::ixbar_read(const PHV::Field *f, le_bitrange range) const {
    if (!ixbar_read_i.count(f)) return {};
    ordered_set<std::pair<const IR::MAU::Table *, const IR::MAU::TableKey *>> rv;
    for (const auto& kv : ixbar_read_i.at(f)) {
        if (kv.first.contains(range)) {
            rv.insert(kv.second.begin(), kv.second.end());
        } else {
            BUG_CHECK(!range.overlaps(kv.first), "not fine-sliced range of %1% %2%, saved: %3%", f,
                      range, kv.first);
        }
    }
    return rv;
}

//***********************************************************************************
//
// class PhvUse
//
// preorder walk on IR tree to set compute use information
//
//***********************************************************************************

bool PhvUse::preorder(const IR::BFN::Deparser *d) {
    LOG5("PREORDER Deparser PhvUse");
    thread = d->gress;
    in_dep = true;
    in_mau = false;

    revisit_visited();
    visit(d->digests, "digests");
    visit(d->params, "params");

    revisit_visited();
    visit(d->emits, "emits");

    return false;
}

bool PhvUse::preorder(const IR::BFN::DeparserParameter*) {
    LOG5("PREORDER DeparserParameter PhvUse");
    // Treat fields which are used to set intrinsic deparser parameters as if
    // they're used in the MAU, because they can't go in TPHV or CLOT.
    in_mau = true;
    LOG5("\t\t Turn on MAU for DeparserParameter");
    return true;
}

void PhvUse::postorder(const IR::BFN::DeparserParameter*) {
    LOG5("POSTORDER Deparser PhvUse");
    in_mau = false;
    LOG5("\t\t Turn off MAU for DeparserParameter");
}

bool PhvUse::preorder(const IR::BFN::Digest* digest) {
    Phv_Parde_Mau_Use::preorder(digest);
    LOG5("PREORDER Digest PhvUse");
    // Treat fields which are used in digests as if they're used in the MAU,
    // because they can't go in TPHV or CLOT.
    in_mau = true;
    LOG5("\t\t Turn on MAU for Digest");
    return true;
}

void PhvUse::postorder(const IR::BFN::Digest*) {
    LOG5("POSTORDER Digest PhvUse");
    in_mau = false;
    LOG5("\t\t Turn off MAU for Digest");
}
