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

#include "bf-p4c/mau/ixbar_realign.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/hex.h"

class IXBarVerify::GetCurrentUse : public MauInspector {
    IXBarVerify        &self;
    bool preorder(const IR::Expression *) override { return false; }
    bool preorder(const IR::MAU::Table *t) override {
        BUG_CHECK(t->is_always_run_action() || t->global_id(), "Table not placed");
        int gress = INGRESS;
        unsigned stage = t->stage();
        if (!self.ixbar[gress][stage])
            self.ixbar[gress][stage].reset(IXBar::create());
        self.ixbar[gress][stage]->update(t);
        return true; }
 public:
    explicit GetCurrentUse(IXBarVerify &s) : self(s) {}
};

/** For each ixbar byte, this verifies that each IXBar::Use::Byte coordinates to a single
 *  byte within a PHV container.  Furthermore it verifies that each byte is at it's correct
 *  alignment.  Currently the algorithm does not take advantage of the TCAM byte swizzle,
 *  and would have to be expanded, as TCAM bytes don't have to be at their alignment
 */
void IXBarVerify::verify_format(const IXBar::Use *use) {
    BUG_CHECK(currentTable, "No table context found for input crossbar use");
    if (!use) return;
    for (auto &byte : use->use) {
        bitvec byte_use;
        PHV::Container container;
        size_t mod_4_offset = -1;
        bool container_set = false;
        for (auto fi : byte.field_bytes) {
            auto *field = phv.field(fi.field);
            CHECK_NULL(field);
            bool byte_found = false;
            bool single_byte = true;
            PHV::FieldUse use(PHV::FieldUse::READ);
            auto slicesToProcess =
                field->get_combined_alloc_bytes(PHV::AllocContext::of_unit(currentTable), &use,
                                                PHV::SliceMatch::REF_PHYS_LR);
            //  early check to make debugging incorrect PHV live range easier.
            BUG_CHECK(!slicesToProcess.empty(),
                      "cannot find allocation of %1%, read in %2% (%3%)",
                      field, currentTable->name, currentTable->externalName());
            for (const auto &alloc : slicesToProcess) {
                if (fi.lo < alloc.field_slice().lo || fi.hi > alloc.field_slice().hi) continue;
                size_t potential_mod4_offset = alloc.container_slice().lo / 8;
                if (!container_set) {
                    container = alloc.container();
                    mod_4_offset = potential_mod4_offset;
                    container_set = true;
                } else if (container != alloc.container()
                           || mod_4_offset != potential_mod4_offset) {
                    single_byte = false;
                    continue;
                }

                int container_start = (alloc.container_slice().lo % 8) + fi.lo -
                                      alloc.field_slice().lo;
                if (container_start != fi.cont_lo)
                    continue;
                byte_found = true;
                BUG_CHECK((byte_use & fi.cont_loc()).empty(), "Overlapping field bit "
                          "information on an input xbar byte: %s", byte);
                byte_use |= fi.cont_loc();
            }
            if (!byte_found || !single_byte) {
                throw IXBar::failure(-1, byte.loc.group);
            }
        }

        if (use->type != IXBar::Use::TERNARY_MATCH) {
            if ((byte.loc.byte % (container.size() / 8)) != mod_4_offset) {
                throw IXBar::failure(-1, byte.loc.group);
            }
        } else {
            if ((use->ternary_align(byte.loc) % (container.size() / 8)) != mod_4_offset) {
                throw IXBar::failure(-1, byte.loc.group);
            }
        }
    }
}

Visitor::profile_t IXBarVerify::init_apply(const IR::Node *root) {
    auto rv = MauModifier::init_apply(root);
    for (auto gress : { INGRESS, EGRESS })
        ixbar[gress].clear();
    currentTable = nullptr;
    root->apply(GetCurrentUse(*this));
    return rv;
}

void IXBarVerify::postorder(IR::MAU::Table *tbl) {
    currentTable = tbl;
    verify_format(tbl->resources->gateway_ixbar.get());
    verify_format(tbl->resources->match_ixbar.get());
    verify_format(tbl->resources->selector_ixbar.get());
    verify_format(tbl->resources->salu_ixbar.get());
    verify_format(tbl->resources->meter_ixbar.get());
    for (auto &hash_dist_use : tbl->resources->hash_dists) {
        for (auto &ir_alloc : hash_dist_use.ir_allocations) {
            verify_format(ir_alloc.use.get());
        }
    }
}
