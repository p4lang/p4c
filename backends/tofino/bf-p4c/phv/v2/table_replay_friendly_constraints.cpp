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

#include "table_replay_friendly_constraints.h"

std::ostream& operator<<(std::ostream &out, const AllocInfo &ai ) {
    Log::TempIndent indent;
    out << "AllocInfo [ " << ai.length << " " << ai.container_size << " " << ai.perfectly_aligned <<
        " ]" << std::endl << indent;
    out << "\tpack_with: [ ";
    for (auto pw : ai.pack_with) {
        out << pw << " ";
    }
    out << "]";
    return out;
}

namespace PHV {
namespace v2 {
const IR::Node *TableReplayFriendlyPhvConstraints::preorder(IR::BFN::Pipe * pipe) {
    field_candidates.clear();
    action_to_fields.clear();
    if (mau_backtracker.get_table_summary()->getActualState() !=
        State::ALT_FINALIZE_TABLE_SAME_ORDER_TABLE_FIXED) {
        // if it is not in the state for table replay fitting, clear pragmas and return.
        prune();
        add_pa_container_size.clear();
        add_pa_no_pack.clear();
    } else {
        problematic_table =
            mau_backtracker.get_table_summary()->get_table_replay_problematic_table();
        LOG1("problematic table is " << problematic_table->name << " due to " <<
            mau_backtracker.get_table_summary()->get_table_replay_result());
    }
    return pipe;
}

const IR::Node *TableReplayFriendlyPhvConstraints::preorder(IR::Expression *expr) {
    auto table_replay_result = mau_backtracker.get_table_summary()->get_table_replay_result();
    const IR::MAU::Table *table = findContext<IR::MAU::Table>();
    if (!table) return expr;
    const IR::MAU::Action *action = findContext<IR::MAU::Action>();
    // If it failed due to adb or memory, we only care about fields in actions.
    if ((table_replay_result == TableSummary::FAIL_ON_ADB
        || table_replay_result == TableSummary::FAIL_ON_MEM) && !action) return expr;
    // If it failed due to ixbar, we only care about table keys.
    const IR::MAU::TableKey *key = findContext<IR::MAU::TableKey>();
    if (table_replay_result == TableSummary::FAIL_ON_IXBAR && !key) return expr;
    LOG5("parent table is " + table->name);
    if (problematic_table->name != mau_backtracker.get_table_summary()->getTableIName(table)) {
        return expr;
    }
    const auto *f = phv.field(expr);
    if (!f) {
        return expr;
    }
    // collect fields that are used in the same action.
    if (action) action_to_fields[action].insert(f->name);

    if (add_pa_container_size.find(f->name) != add_pa_container_size.end()) {
        return expr;
    }
    if (trivial_allocation_info.find(f->name) == trivial_allocation_info.end()) {
        return expr;
    }
    // For now, we only care about fields that are perfectly aligned in trivial allocation.
    if (!trivial_allocation_info.at(f->name).at(0).perfectly_aligned) {
        return expr;
    }
    // If trivial allocation and real phv allocation is the same, we do not need to add
    // pa_container_size pragmas for this field.
    bool container_size_is_same = true;
    if (trivial_allocation_info.at(f->name).size() == real_allocation_info.at(f->name).size()) {
        for (auto [index, trivial_alloc] : trivial_allocation_info.at(f->name)) {
            if (!real_allocation_info.at(f->name).count(index)) {
                container_size_is_same = false;
                break;
            }
            auto real_alloc = real_allocation_info.at(f->name).at(index);
            if (!(trivial_alloc.length == real_alloc.length &&
                trivial_alloc.container_size == real_alloc.container_size)) {
                container_size_is_same = false;
                break;
            }
        }
    } else {
        container_size_is_same = false;
    }
    if (container_size_is_same) container_size_ok.insert(f);

    if (f->exact_containers()) {
        LOG3(f << " has exact container constraints");
        container_size_ok.insert(f);
    }

    // For now, we only care about field size <= 16
    if (f->size > 16) {
        return expr;
    }
    field_candidates.insert(f);
    return expr;
}

void TableReplayFriendlyPhvConstraints::end_apply(const IR::Node *) {
    auto table_replay_result = mau_backtracker.get_table_summary()->get_table_replay_result();
    // Create a map from a field to a set of fields that are in the same action. This information
    // is useful because phv allocation may pack them togethor to save phv space, but have bad
    // impact on adb allocation. By using pa_no_pack on these relevant fields following the trivial
    // phv allocation, adb allocation can be better.
    ordered_map<cstring, ordered_set<cstring>> fields_in_same_action;
    for (auto packed_fields : Values(action_to_fields)) {
        for (auto field : packed_fields) {
            fields_in_same_action[field] = packed_fields;
        }
    }

    for (auto field_candidate : field_candidates) {
        // For this field candidate, fields_pack_with_trivial_alloc records all fields that are
        // packed with this field in trivial allocation.
        ordered_set<cstring> fields_pack_with_trivial_alloc;
        if (!trivial_allocation_info.count(field_candidate->name)) {
            BUG("trivial allocation not found for %1%", field_candidate->name);
        }
        auto alloc_info = trivial_allocation_info.at(field_candidate->name);
        LOG5("\tTrivial alloc info for field: ");
        for (auto &ai : alloc_info) {
            LOG5("\t\t[ " << ai.first);
            LOG5(ai.second << " ]");
        }

        // add pa_container_size pragma for this field_candidate
        std::vector<PHV::Size> size_vec;
        for (int index = 0; index < field_candidate->size;) {
            BUG_CHECK(alloc_info.find(index) != alloc_info.end(), "index not found");
            size_vec.push_back(PHV::Size(alloc_info[index].container_size));
            fields_pack_with_trivial_alloc.insert(
                alloc_info[index].pack_with.begin(), alloc_info[index].pack_with.end());
            if (alloc_info[index].length != (int)alloc_info[index].container_size) {
                break;
            } else {
                // have a BUG_CHECK to see alloc_info[index] exist, so cannot put this in the third
                // part of for loop.
                index += alloc_info[index].length;
            }
        }
        if (!container_size_ok.count(field_candidate)) {
            add_pa_container_size[field_candidate->name] = size_vec;
            std::stringstream size_vec_str;
            for (auto size : size_vec)
                size_vec_str << " " << size;
            LOG3("adding pa_container_size for" << field_candidate << size_vec_str.str());
        }
        // pa_no_pack pragmas only for FAIL_ON_ADB and FAIL_ON_MEM
        if (table_replay_result == TableSummary::FAIL_ON_ADB ||
            table_replay_result == TableSummary::FAIL_ON_MEM) {
            LOG1("Fails on : " << table_replay_result);
            // For this field candidate, fields_pack_with_trivial_alloc records all fields that are
            // packed with this field in trivial allocation.
            ordered_set<cstring> fields_pack_with_real_alloc;
            if (!real_allocation_info.count(field_candidate->name)) {
                BUG("real allocation not found for %1%", field_candidate->name);
            }
            auto alloc_info = real_allocation_info.at(field_candidate->name);
            for (auto it : alloc_info) {
                fields_pack_with_real_alloc.insert(
                    it.second.pack_with.begin(), it.second.pack_with.end());
            }
            for (auto field : fields_pack_with_real_alloc) {
                LOG5("\tFields pack with real alloc : " << field);
                // if a field is packed with field_candidate in real phv allocation and is not
                // packed with field_candidate in trivial allocation and they are in the same action
                // apply a pa_no_pack pragma.
                if (!fields_pack_with_trivial_alloc.count(field)) {
                    // only no pack field that are in the same action
                    if (fields_in_same_action[field_candidate->name].count(field)) {
                        add_pa_no_pack[field_candidate->name].insert(field);
                        LOG3("adding pa_no_pack for " << field_candidate->name << " and " << field);
                    }
                }
            }
        }
    }
}

void CollectPHVAllocationResult::end_apply(const IR::Node *) {
    auto state = mau_backtracker.get_table_summary()->getActualState();
    auto &allocation_info = state == State::ALT_INITIAL ?
        trivial_allocation_info : real_allocation_info;
    allocation_info.clear();
    for (auto &field : phv) {
        for (auto &alloc : field.get_alloc()) {
            LOG5(alloc.field() << " " << alloc.field_slice() << " is in " << alloc.container());
            AllocInfo info = {(int)alloc.field_slice().size(), alloc.container().size(), {}, true};
            auto fields = phv.fields_in_container(alloc.container());
            for (auto other_field : fields) {
                info.pack_with.insert(other_field->name);
            }
            allocation_info[field.name][alloc.field_slice().lo] = info;
        }
    }

    // perfectly_aligned means that except for the last AllocSlice, every AllocSlice of a fieldslice
    // can occupy a container entirely.
    for (const auto &[ field_name, alloc_align ] : allocation_info) {
        bool perfectly_aligned = true;
        auto field = phv.field(field_name);
        LOG5("Checking field: " << field);
        BUG_CHECK(field, "field not found");
        for (int index = 0; index < field->size; index += alloc_align.at(index).length) {
            // if allocated in clot, then we cannot find this field
            if (alloc_align.find(index) == alloc_align.end()) {
                perfectly_aligned = false;
                break;
            }
            const auto &alloc_info = alloc_align.at(index);
            // if AllocSlice does not occupy a container entirely
            if (alloc_info.length != (int)alloc_info.container_size) {
                // it is perfectly aligned if it is the last AllocSlice.
                if (index + alloc_info.length == field->size) {
                    perfectly_aligned = true;
                } else {
                    perfectly_aligned = false;
                }
                break;
            }
        }
        for (auto &alloc : Values(allocation_info[field_name])) {
            alloc.perfectly_aligned = perfectly_aligned;
        }
    }
}

}  // namespace v2
}  // namespace PHV
