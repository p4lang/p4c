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

#ifndef BF_P4C_PHV_INIT_IN_MAU_H_
#define BF_P4C_PHV_INIT_IN_MAU_H_

#include "ir/visitor.h"
#include "ir/ir-generated.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/add_initialization.h"


// *****************************************************************************************
// AddInitTable class adds a new table called __mau_inits_table__ in the beginning of the
// first Table Sequence in the IR with one action that contains
// zero-initializations to selected field-slices. The table is added
// to handle initializations that cannot be done in the parser.
//
// Currently it handles initializations for non-extracted field which
// are packed with extracted fields in the same slice list, thus need
// to be allocated in the same container. An example of this happens
// in Arista's profile 'obfucscated-map.p4' where 2 header fields are
// aliased with 2 metadata fields. One of the metadata fields is
// defined in the parser whereas the other is not. The profile is
// quite complex and removing the alias results in non-fitting on
// Tofino-1 device.
//
// The field-slices that get initialized are contained in
// "mauInitFields" set. This container is populated during phv
// allocation by "ParserPackingValidator" which checks for packing
// constraints due to parsing. During Trivial allocation where packing
// is minimal due to infinite phv resources we mark pairs of field-slices that
// belong in the same slice list and one of the slices is extracted
// whereas the other is not.
// During regular PHV-Allocation, field-slices will not be added to "mauInitFields".
// The packing will be rejected unless Trivial PHV-Allocation marked
// the pair of fields for initialization which most likely means that
// Trivial Allocation was not able to pack fields in a way that
// initializations were not required.
//
// The "AddInitsInMAU" pass is run twice in the backend list of passes:
// Once before PHV-Allocation and once after PHV-Allocation (before Table-Placement)
// The reason for running this pass twice is due to the fact that the
// added table is removed when progressing from Table-Placement to
// real PHV-Allocation in the alt-phv-alloc flow and thus we lose its
// physical stage info as well.
//
// *TODO* Eliminate the first instance of "AddInitsInMAU" before
// PHV-Allocation by handling the physical-stage info issues that
// arise.
//
// *TODO* Use "mauInitFields" to also handle initialization for fields
// that are upcasted in the parser.
// *****************************************************************************************
class AddInitTable : public Transform {
 private:
    PhvInfo& phv;
    const MapFieldToExpr&       fieldToExpr;
    std::set<PHV::FieldRange>&  mauInitFields;
    std::vector<IR::Slice*>     mauInitSlices;
    bool ingress_inits = false, egress_inits = false;
    bool check_container;

    bool same_container(PHV::FieldSlice sl1, PHV::FieldSlice sl2) {
        if (!check_container) return true;

        for (const auto& alloc_sl1 : sl1.field()->get_alloc()) {
            // Look for corresponding allocated slice of FieldSlice sl1
            if (sl1.range().overlaps(alloc_sl1.field_slice()) &&
                alloc_sl1.container() != PHV::Container()) {
                for (const auto& alloc_sl2 : sl2.field()->get_alloc()) {
                    // Look for corresponding allocated slice of FieldSlice sl2
                    if (sl2.range().overlaps(alloc_sl2.field_slice())) {
                        if (alloc_sl1.container() == alloc_sl2.container()) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    profile_t init_apply(const IR::Node* root) override {
        auto rv = Transform::init_apply(root);

        LOG3("AddInitTable:  init_apply");
        mauInitSlices.clear();
        // Collect set of FieldRange's which do not need initialization to remove
        std::set<PHV::FieldRange> removeInitFields;
        for (const auto& mau_init : mauInitFields) {
            // Get the field requiring mau init
            if (const auto* fld1 = phv.field(mau_init.field_name)) {
                // Get the extracted field that is possibly allocated
                // to the same container as fld1
                for (auto& f_range : mau_init.conflicts) {
                    const auto* fld2 = phv.field(f_range.field_name);
                    if (fld2 && same_container(PHV::FieldSlice(fld1, mau_init.range),
                                               PHV::FieldSlice(fld2, f_range.range))) {
                        ingress_inits = (fld1->gress == INGRESS);
                        egress_inits  = (fld1->gress == EGRESS);
                        auto slice = new IR::Slice(fieldToExpr.getExpr(fld1),
                                                   mau_init.range.hi, mau_init.range.lo);
                        mauInitSlices.push_back(slice);
                        LOG4("\t Added slice: " << slice << " for " << mau_init.field_name <<
                             " " << mau_init.range);
                    } else {
                        removeInitFields.insert(mau_init);
                    }
                }
            }
        }

        for (auto remInit : removeInitFields) {
            mauInitFields.erase(remInit);
            LOG5("\t Removing mau init " << remInit.field_name << " " << remInit.range);
        }

        return rv;
    }

    IR::MAU::TableSeq* preorder(IR::MAU::TableSeq* tSeq) override {
        LOG3("AddInitTable: preorder TableSeq " << tSeq);
        gress_t seq_gress;
        cstring tableName;
        cstring actionName;
        const IR::MAU::Table* init_table = nullptr;

        if (tSeq->tables.empty()) {
            LOG3("AddInitTable: empty TableSeq - skipping");
            return tSeq;
        } else {
            LOG3("\t Table: " << tSeq->tables.front()->name);
            seq_gress = tSeq->tables.front()->gress;
            tableName = "__mau_inits_table__" + toString(seq_gress);
            actionName = "__mau_inits_action__" + toString(seq_gress);
            for (auto *tbl : tSeq->tables) {
                if (tbl->name == tableName) {
                    init_table = tbl;
                    BUG_CHECK(tbl->actions.count(actionName), "Table %1% does not have action %2%?",
                              tableName, actionName);
                }
            }
        }

        // Check that this Table Sequence is the first one
        const IR::MAU::Table *parent_table = nullptr;
        parent_table = findContext<IR::MAU::Table>();
        BUG_CHECK(!parent_table, "This Table Sequence has parent table %1% ?", parent_table->name);

        prune();

        if (((seq_gress == INGRESS) && !ingress_inits) ||
            ((seq_gress == EGRESS) && !egress_inits)) {
            LOG3("AddInitTable: Table Sequence of gress " << seq_gress <<
                 " without fields of that gress requiring init - nothing to do");
            return tSeq;
        }

        // No need to do anything if initialization table exist
        // *TODO* This is meant to handle re-creation of the initialization
        //        table during real PHV Allocation already exists
        //        from trivial PHV-Allocation. Though this would not happen
        //        in the current implementation. But in the future if we
        //        want to update the set of fields initialized we should
        //        need to check if initialization table exists and also if
        //        if field-slices are already initialized. Thus leaving this as is for now
        if (init_table)
            return tSeq;

        // Create the table and action that will contain the slice inits
        auto createDummyTableAndAction = [&]() -> std::pair<IR::MAU::Table*, IR::MAU::Action*> {
            LOG3("AddInitTable: " << "new table name " << tableName);
            auto table = new IR::MAU::Table(tableName, seq_gress);
            table->gress = seq_gress;
            auto action = new IR::MAU::Action(actionName);
            table->match_table = new IR::P4Table(tableName.c_str(), new IR::TableProperties());
            table->is_compiler_generated = true;
            action->init_default = true;
            action->default_allowed = true;

            for (auto& slice : mauInitSlices) {
                auto type = IR::Type_Bits::get(slice->getH() - slice->getL() + 1);
                auto instr = new IR::MAU::Instruction("set"_cs, {slice, new IR::Constant(type, 0)});
                LOG3("AddInitTable: " << "adding " << instr);
                action->action.push_back(instr);
            }

            table->actions[action->name] = action;

            return {table, action};
        };

        // Create init table if it has not been created before
        auto [table, action] = createDummyTableAndAction();

        tSeq->tables.insert(tSeq->tables.begin(), table);
        LOG3("AddInitTable: the first tableSeq now looks like this " << tSeq);
        return tSeq;
    }

 public:
    explicit AddInitTable(PhvInfo& p, MapFieldToExpr& f2exp,
                          std::set<PHV::FieldRange>& to_be_inited,
                          bool container_info) :
        phv(p), fieldToExpr(f2exp), mauInitFields(to_be_inited),
        check_container(container_info) {}
};

class AddInitsInMAU : public PassManager {
 public:
    explicit AddInitsInMAU(PhvInfo& phv, std::set<PHV::FieldRange>& to_be_inited,
                           bool container_info) {
        auto mapFieldToExpr = new MapFieldToExpr(phv);
        addPasses({
                mapFieldToExpr,
                new AddInitTable(phv, *mapFieldToExpr, to_be_inited, container_info),
            });
    }
};

#endif /* BF_P4C_PHV_INIT_IN_MAU_H_ */
