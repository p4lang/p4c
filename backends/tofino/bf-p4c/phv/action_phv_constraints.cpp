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

#include <algorithm>
#include <sstream>
#include <tuple>
#include <boost/optional/optional_io.hpp>

#include "bf-p4c/phv/phv_fields.h"
#include "lib/algorithm.h"
#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/common/scc_toposort.h"
#include "bf-p4c/phv/solver/action_constraint_solver.h"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/phv/cluster_phv_operations.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/phv/utils/slice_alloc.h"
#include "lib/exceptions.h"

namespace {

using namespace P4;

// return a solver based on contaienr type.
solver::ActionSolverBase* make_solver(const PHV::Container& c) {
    switch (c.type().kind()) {
        case PHV::Kind::normal: {
            LOG3("initializing normal container move solver");
            return new solver::ActionMoveSolver();
        }
        case PHV::Kind::mocha: {
            LOG3("initializing mocha container move solver");
            return new solver::ActionMochaSolver();
        }
        case PHV::Kind::dark: {
            LOG3("initializing dark container move solver");
            return new solver::ActionDarkSolver();
        }
        case PHV::Kind::tagalong: {
            BUG("creating action solver on TPHV container is invalid: %1%", c);
        }
        default:
            BUG("unknown container type: %1%", c);
    }
}

/// compute_dest_live_bv returns a bitvec represents bits are live after applying
/// @p action of @p stage to @p container_state.
/// @p action can be nullptr when it's an always_run_table action.
/// @param container_state is a set of field slices that are non-mutex with some fieldslice
/// in the candidate allocslice list.
bitvec compute_dest_live_bv(const PhvUse& uses, const PHV::Allocation& alloc,
                            const PHV::Allocation::LiveRangeShrinkingMap& initActions,
                            const PHV::Allocation::MutuallyLiveSlices& container_state,
                            const int stage, const IR::MAU::Action* action) {
    bitvec dest_live;
    for (const auto& slice : container_state) {
        const auto* field = slice.field();
        if (!uses.is_referenced(field) || field->padding) continue;
        // TODO: for physical-stage-based liverange, we can actually only do precise
        // live range checks for non-mutex slices of (@p stage, write), instead of slices that
        // live at least at @p stage. The precise check can allows us to overwrite fields
        // that their last read is at @p stage.
        if (slice.isLiveAt(stage, PHV::FieldUse(PHV::FieldUse::READ)) ||
            slice.isLiveAt(stage, PHV::FieldUse(PHV::FieldUse::WRITE))) {
            dest_live.setrange(slice.container_slice().lo, slice.container_slice().size());
        }
        // there seems to be a bug that a field @f is initialized in action @a
        // but the earliest liverange of @f is somehow wrongly later then min_stage of @a;
        // Add this additional check here to avoid compiler bug.
        // We can remove this after we fix this bug.
        // To reproduce, remove these checks and rerun CI tests.
        if (action) {
            if ((initActions.count(field) && initActions.at(field).count(action)) ||
                alloc.getMetadataInits(action).count(field)) {
                dest_live.setrange(slice.container_slice().lo, slice.container_slice().size());
            } else {
                const auto& dark_prim = slice.getInitPrimitive();
                if (!dark_prim.isAlwaysRunActionPrim()) {
                    if (dark_prim.getInitPoints().count(action)) {
                        dest_live.setrange(slice.container_slice().lo,
                                           slice.container_slice().size());
                    }
                }
            }
        }
    }
    return dest_live;
}

}  // namespace

int ActionPhvConstraints::ConstraintTracker::current_action = 0;

Visitor::profile_t ActionPhvConstraints::init_apply(const IR::Node *root) {
    LOG1("CLEARING ActionPhvConstraints");
    profile_t rv = Inspector::init_apply(root);
    meter_color_destinations.clear();
    meter_color_destinations_8bit.clear();
    special_no_pack.clear();
    constraint_tracker.clear();
    prelim_constraints_ok = true;
    same_byte_fields.clear();
    determine_same_byte_fields();
    return rv;
}

bool ActionPhvConstraints::preorder(const IR::MAU::Action *act) {
    auto *tbl = findContext<IR::MAU::Table>();
    ActionAnalysis aa(phv, false, false, tbl, dg.red_info);
    ActionAnalysis::FieldActionsMap field_actions_map;
    aa.set_field_actions_map(&field_actions_map);
    act->apply(aa);
    constraint_tracker.add_action(act, field_actions_map, tbl);
    prelim_constraints_ok &= early_check_ok(act);
    return true;
}

void ActionPhvConstraints::end_apply() {
    LOG7("*****Printing  ActionPhvConstraints Maps*****");
    constraint_tracker.printMapStates();
    LOG7("*****End Print ActionPhvConstraints Maps*****");
}

void ActionPhvConstraints::ConstraintTracker::clear() {
    LOG1("CLEARING ActionPhvConstraints::ConstraintTracker");
    current_action = 0;
    field_writes_to_actions.clear();
    action_to_writes.clear();
    action_to_reads.clear();
    action_to_table.clear();
    write_to_reads_per_action.clear();
    read_to_writes_per_action.clear();
    statefulWrites.clear();
}

void ActionPhvConstraints::ConstraintTracker::add_action(
        const IR::MAU::Action *act,
        const ActionAnalysis::FieldActionsMap field_actions_map,
        const IR::MAU::Table *tbl) {
    LOG5("Action PHV Constraints: Analyzing " << act << " in table " << tbl->name);
    action_to_table[act] = tbl;
    for (auto &field_action : Values(field_actions_map)) {
        le_bitrange write_range;
        auto *write_field = phv.field(field_action.write.expr, &write_range);
        if (write_field == nullptr)
            BUG("Action does not have a write?");
        PHV::FieldSlice write(write_field, write_range);
        field_writes_to_actions[write_field][write_range].insert(act);
        OperandInfo fw(write, current_action);
        fw.operation = field_action.name;
        if (field_action.name == "set") {
            fw.flags |= OperandInfo::MOVE;
        } else if (field_action.container_write_type().first !=
                   ActionAnalysis::FieldAction::ALL_BITS) {
            fw.flags |= OperandInfo::PART_OF_CONTAINER;
        } else if (PHV_Field_Operations::BITWISE_OPS.count(field_action.name)) {
            fw.flags |= OperandInfo::BITWISE;
        } else {
            fw.flags |= OperandInfo::WHOLE_CONTAINER;
        }

        fw.action_name = field_action.name;
        LOG5("    ...write: " << fw);
        action_to_writes[act].insert(fw);
        for (auto &read : field_action.reads) {
            OperandInfo fr;
            fr.unique_action_id = current_action;
            if (read.type == ActionAnalysis::ActionParam::PHV) {
                le_bitrange read_range;
                auto* f_used = phv.field(read.expr, &read_range);
                fr.phv_used = PHV::FieldSlice(f_used, read_range);
                read_to_writes_per_action[f_used][act][read_range].insert(write);
                action_to_reads[act].insert(*fr.phv_used);
            } else if (read.type == ActionAnalysis::ActionParam::ACTIONDATA) {
                fr.ad = true;
                fr.special_ad = read.speciality;
                if (LOGGING(5) && read.speciality != ActionAnalysis::ActionParam::NO_SPECIAL)
                    LOG5("      ...speciality action data read: " << fr);
            } else if (read.type == ActionAnalysis::ActionParam::CONSTANT) {
                if (field_action.is_funnel_shift()) {
                    // Funnel shift have the format src1(PHV), src2(PHV), shift(Constant) and the
                    // shift constant is part of the instruction parameter. This is why it should
                    // not be seen as one of the source operand.
                    LOG5("      ...skipping funnel shift constant");
                    continue;
                }
                fr.constant = true;
                if (read.expr->is<IR::Constant>()) {
                    const auto* constExpr = read.expr->to<IR::Constant>();
                    if (constExpr->fitsLong())
                        fr.const_value = constExpr->asLong();
                    else
                        fr.const_value = -1;
                } else {
                    fr.const_value = -1;
                }
            } else {
                BUG("Read must either be of a PHV, action data, or constant."); }

            // Originally this condition was to satisfy the current table placement requirement that
            // any destination written by meter colors must be allocated to an 8-bit PHV. This
            // constraint is now relaxed by only enforcing this destination to
            // an 8-bit container if the operation can't be rotated. This is to work around the
            // limitation from which the color is automatically being set as part of the bit 24..31
            // of the immediate which can also only be alligned to the high part of an 16-bit or
            // 32-bit container. To simplify things, we are actually only enforcing strict alignment
            // operation on 8-bit container. Using a deposit-field instruction to copy the
            // meter color to any container size should work fine.
            if (read.speciality == ActionAnalysis::ActionParam::METER_COLOR) {
                if (fw.flags != OperandInfo::MOVE)
                    self.meter_color_destinations_8bit.insert(write.field());

                self.meter_color_destinations.insert(write.field());
            }

            // xxx(Deep): This condition is to satisfy the current table placement requirement that
            // any destination written by METER_ALU, METER_COLOR, HASH_DIST, or RANDOM must not be
            // packed with other fields written in the same action. To enable this, maintain a list
            // of all actions where such writes happen for the given field.
            if (read.speciality != ActionAnalysis::ActionParam::NO_SPECIAL) {
                self.special_no_pack[write.field()].insert(act);
                if (read.speciality == ActionAnalysis::ActionParam::METER_ALU) {
                    int lo, hi;
                    if (auto sl = read.expr->to<IR::Slice>()) {
                        lo = sl->getL();
                        hi = sl->getH();
                    } else {
                        lo = 0;
                        hi = read.size() - 1;
                    }
                    std::pair<int, int> read_range_pair = std::make_pair(lo, hi);
                    statefulWrites[write_field][write_range][act].insert(read_range_pair);
                    LOG5("\t  ...adding stateful read range [" << lo << ", " << hi << "] and "
                         "write range [" << write_range.lo << ", " << write_range.hi << "] "
                         "for action " << act->name << " in table " << tbl->name);
                }
            }

            if (field_action.reads.size() > 1) {
                fr.flags |= OperandInfo::ANOTHER_OPERAND;
            }
            fr.operation = field_action.name;
            if (field_action.name == "set") {
                fr.flags |= OperandInfo::MOVE;
            } else if (PHV_Field_Operations::BITWISE_OPS.count(field_action.name)) {
                fr.flags |= OperandInfo::BITWISE;
            } else {
                fr.flags |= OperandInfo::WHOLE_CONTAINER; }
            fr.action_name = field_action.name;
            LOG5("    ...read: " << fr);
            write_to_reads_per_action[write_field][act][write_range].push_back(fr); } }

    current_action++;
}

bool ActionPhvConstraints::check_speciality_packing(
    const PHV::Allocation::MutuallyLiveSlices& container_state) const {
    ordered_set<const PHV::Field*> fields;
    for (auto& s : container_state) fields.insert(s.field());

    ordered_set<const PHV::Field*> special_writes;
    // Detect all the speciality writes in the container
    for (const PHV::Field* f : fields) {
        if (special_no_pack.count(f))
            special_writes.insert(f); }

    // If no special writes, detected return true
    if (special_writes.size() == 0) return true;

    // If special writes present, check against all other actions
    for (const PHV::Field* f : special_writes) {
        for (const IR::MAU::Action* act : special_no_pack.at(f)) {
            for (auto& fo_wr : constraint_tracker.writes(act)) {
                if (!fo_wr.phv_used) continue;
                const PHV::Field* f_wr = fo_wr.phv_used->field();
                if (f_wr == f) continue;
                if (fields.count(f_wr))
                    return false; } } }

    return true;
}

std::vector<ActionPhvConstraints::OperandInfo> ActionPhvConstraints::ConstraintTracker::sources(
        const PHV::FieldSlice& dst,
        const IR::MAU::Action* act) const {
    std::vector<OperandInfo> rv;

    // Check whether this field is ever a dst.
    if (write_to_reads_per_action.find(dst.field()) == write_to_reads_per_action.end())
        return rv;

    auto& by_action = write_to_reads_per_action.at(dst.field());
    if (by_action.find(act) == by_action.end())
        return rv;

    // Find the range containing @dst, if any.
    auto& by_range = by_action.at(act);
    const le_bitrange *containing = nullptr;
    for (auto& kv : by_range) {
        if (kv.first.contains(dst.range())) {
            BUG_CHECK(containing == nullptr,
                      "Field %1% written to more than once in action %2%",
                      dst.field()->name, act);
            containing = &(kv.first); } }
    if (containing == nullptr)
        return rv;

    // Shrink each source field operand to correspond to the slice of the
    // destination, if any.  The offset of `dst` in `containing` must
    // correspond to the shrunken range to the source operand range.
    unsigned offset = dst.range().lo - containing->lo;    // Positive because containing.

    for (const auto& field_op : by_range.at(*containing)) {
        // Skip non-PHV operands.
        if (!field_op.phv_used) {
            rv.push_back(field_op);
            continue; }
        auto& used = field_op.phv_used;
        le_bitrange shifted = StartLen(used->range().lo + offset, dst.range().size());
        le_bitinterval src_int = used->range().intersectWith(shifted);
        if (auto src_range = toClosedRange(src_int)) {
            // Copy field_op, then update `phv_used` in the copy.
            rv.push_back(field_op);
            rv.back().phv_used = PHV::FieldSlice(used->field(), *src_range); } }

    return rv;
}

ordered_set<PHV::FieldSlice> ActionPhvConstraints::ConstraintTracker::destinations(
        PHV::FieldSlice src,
        const IR::MAU::Action* act) const {
    ordered_set<PHV::FieldSlice> rv;

    // Check whether this field is ever a source.
    if (read_to_writes_per_action.find(src.field()) == read_to_writes_per_action.end())
        return rv;

    auto& by_action = read_to_writes_per_action.at(src.field());
    if (by_action.find(act) == by_action.end())
        return rv;

    // Find the range containing @src, if any.
    for (auto& kv : by_action.at(act)) {
        if (!kv.first.contains(src.range()))
            continue;

        // Shrink each destination field operand to correspond to the slice of the
        // source, if any.  The offset of `src` in `kv.first` must
        // correspond to the shrunken range to the destination operand range.
        unsigned offset = src.range().lo - kv.first.lo;    // Positive because containing.
        for (const auto& slice : kv.second) {
            le_bitrange shifted = StartLen(slice.range().lo + offset, src.range().size());
            le_bitinterval src_int = slice.range().intersectWith(shifted);
            if (auto src_range = toClosedRange(src_int))
                rv.insert(PHV::FieldSlice(slice.field(), *src_range)); } }

    return rv;
}

const ordered_set<ActionPhvConstraints::OperandInfo>&
ActionPhvConstraints::ConstraintTracker::writes(const IR::MAU::Action* act) const {
    static ordered_set<OperandInfo> empty;
    auto it = action_to_writes.find(act);
    if (it == action_to_writes.end())
        return empty;
    return it->second;
}

const ordered_set<PHV::FieldSlice>&
ActionPhvConstraints::ConstraintTracker::reads(const IR::MAU::Action* act) const {
    static ordered_set<PHV::FieldSlice> empty;
    auto it = action_to_reads.find(act);
    if (it == action_to_reads.end())
        return empty;
    return it->second;
}

ordered_set<const IR::MAU::Action*>
ActionPhvConstraints::ConstraintTracker::read_in(PHV::FieldSlice src) const {
    ordered_set<const IR::MAU::Action*> rv;
    if (read_to_writes_per_action.find(src.field()) == read_to_writes_per_action.end())
        return rv;
    for (const auto& by_action : read_to_writes_per_action.at(src.field())) {
        for (const auto& by_range : by_action.second)
            if (by_range.first.contains(src.range()))
                rv.insert(by_action.first); }
    return rv;
}

ordered_set<const IR::MAU::Action*>
ActionPhvConstraints::ConstraintTracker::written_in(PHV::FieldSlice dst) const {
    ordered_set<const IR::MAU::Action*> rv;
    if (!field_writes_to_actions.count(dst.field())) return rv;
    for (auto& kv : field_writes_to_actions.at(dst.field())) {
        if (kv.first.overlaps(dst.range()))
            rv.insert(kv.second.begin(), kv.second.end()); }
    return rv;
}

ordered_set<int> ActionPhvConstraints::ConstraintTracker::source_alignment(
        PHV::AllocSlice dst,
        PHV::FieldSlice src) const {
    ordered_set<int> rv;
    for (auto* act : written_in(dst)) {
        for (auto& opInfo : sources(dst, act)) {
            if (!opInfo.phv_used || *opInfo.phv_used != src)
                continue;
            LOG6("\t\t\t\tSource alignment " << dst << " / ");
            LOG6("\t\t\t\t\t\t" << src);
            LOG6("\t\t\t\t\t\t" << "(opInfo: "
                 << *opInfo.phv_used << ") @ " << dst.container_slice().lo);
            LOG6("\t\t\t\t...induced by action " << act->name);
            rv.insert(dst.container_slice().lo); } }

    return rv;
}

std::optional<int> ActionPhvConstraints::ConstraintTracker::can_be_both_sources(
        const std::vector<PHV::AllocSlice> &slices, ordered_set<PHV::FieldSlice> &packing_slices,
        PHV::FieldSlice src) const {
    ordered_set<const IR::MAU::Action *> two_aligned_actions;
    bitvec alignment;

    int single_alignment_point = -1;
    struct LocalAlignmentInfo {
        bitvec alignment;
        bool has_ad = false;
    };


    ordered_map<const IR::MAU::Action *, LocalAlignmentInfo> act_to_alignment;

    for (auto dst : slices) {
        for (auto* act : written_in(dst)) {
            auto local_alignment = act_to_alignment[act];
            for (auto& opInfo : sources(dst, act)) {
                if (!opInfo.phv_used)
                    local_alignment.has_ad = true;
                else if (*opInfo.phv_used == src)
                    local_alignment.alignment.setbit(dst.container_slice().lo);
            }
            alignment |= local_alignment.alignment;
        }
    }

    for (auto entry : act_to_alignment) {
        auto loc_align = entry.second;
        if (loc_align.alignment.popcount() == 2) {
            if (loc_align.has_ad)
                return std::nullopt;
            two_aligned_actions.insert(entry.first);
        } else if (loc_align.alignment.popcount() == 1) {
            if (single_alignment_point >= 0 &&
                loc_align.alignment.min().index() != single_alignment_point)
                return std::nullopt;
            single_alignment_point = loc_align.alignment.min().index();
        }
    }

    for (auto *act : two_aligned_actions) {
        std::map<PHV::FieldSlice, std::vector<le_bitrange>> src_to_dst_bits;
        bitvec written_bits;
        PHV::Container container;
        for (auto dst : slices) {
            for (auto &opInfo : sources(dst, act)) {
                if (!opInfo.phv_used) continue;
                auto it = std::find(packing_slices.begin(), packing_slices.end(), *opInfo.phv_used);
                if (it == packing_slices.end()) continue;
                le_bitrange container_bits = dst.container_slice();
                src_to_dst_bits[*it].push_back(container_bits);
                written_bits.setrange(container_bits.lo, container_bits.size());
            }
            container = dst.container();
            if (dst.field()->is_padding())
                written_bits.setrange(dst.container_slice().lo, dst.width());
        }

        if (written_bits.popcount() != int(container.size()))
            return std::nullopt;

        for (auto entry : src_to_dst_bits) {
            if (entry.second.size() == 1) {
                continue;
            } else if (entry.second.size() > 2) {
                return std::nullopt;
            } else {
                if (entry.first != src)
                    return std::nullopt;
            }
        }
    }
    return single_alignment_point >= 0 ? single_alignment_point : alignment.min().index();
}

void ActionPhvConstraints::ConstraintTracker::print_field_ordering(
        const std::vector<PHV::AllocSlice>& slices) const {
    ordered_map<PHV::AllocSlice, size_t> field_slices_to_writes;
    ordered_map<PHV::AllocSlice, size_t> field_slices_to_reads;
    for (auto sl : slices) {
        PHV::FieldSlice slice(sl.field(), sl.field_slice());
        field_slices_to_writes[sl] = written_in(slice).size();
        field_slices_to_reads[sl] = read_in(slice).size(); }

    LOG6("\t\t\t\t\t\t\tField Ordering Map");
    for (auto sl : slices) {
        LOG6("\t\t\t\t\t\t\t" << sl << "\t" << field_slices_to_writes[sl] << "\t" <<
                field_slices_to_reads[sl]); }
}

const IR::MAU::Table *ActionPhvConstraints::ConstraintTracker::action_table(
        const IR::MAU::Action *act) const {
    if (action_to_table.count(act)) return action_to_table.at(act);
    return nullptr;
}

void ActionPhvConstraints::sort(std::list<const PHV::SuperCluster::SliceList*>& slice_lists) const {
    if (LOGGING(6)) {
        LOG6("Slice list on input");
        for (auto sl : slice_lists) {
            LOG6("  " << sl);
        }
    }

    ordered_map<const PHV::Field*, std::vector<PHV::FieldSlice>> fields;
    for (const auto* sl : slice_lists) {
        for (const auto& fs : *sl) {
            fields[fs.field()].push_back(fs);
        }
    }
    auto priorities = compute_sources_first_order(fields);
    ordered_map<const PHV::SuperCluster::SliceList*, int> list_priority;
    for (const auto* sl : slice_lists) {
        for (const auto& fs : *sl) {
            if (list_priority.count(sl)) {
                list_priority[sl] = std::min(list_priority[sl], priorities.at(fs.field()));
            } else {
                list_priority[sl] = priorities.at(fs.field());
            }
        }
    }

    if (LOGGING(6)) {
        LOG6("Identified field priorities");
        for (auto fld_prior : list_priority) {
            LOG6("  Field = " << fld_prior.first << " --> priority = " << fld_prior.second);
        }
    }

    auto SliceListComparator = [&](const PHV::SuperCluster::SliceList* l,
                                   const PHV::SuperCluster::SliceList* r) {
        // LO slice needs to be before HI - make that check before going to read/write status
        // analysis. The check can be performed on first element in the slice list only
        // because the wide arith slices are arranged like this
        auto first_l = *l->begin();
        auto first_r = *r->begin();
        bool same_field_names = first_l.field() == first_r.field();
        bool fields_in_wide_arith =
            first_l.field()->used_in_wide_arith() && first_r.field()->used_in_wide_arith();
        if (same_field_names && fields_in_wide_arith) {
            return first_l.range().lo < first_r.range().lo;
        }
        // We can continue with the priority criteria if non-wide arith condition is met
        if (list_priority.at(l) != list_priority.at(r)) {
            return list_priority.at(l) < list_priority.at(r);
        }
        // Slice lists aren't at the same level and LO/HI slices are not available in compared
        // lists.
        auto l_reads = 0;
        auto l_writes = 0;
        auto r_reads = 0;
        auto r_writes = 0;

        for (auto& sl : *l) {
            l_reads += this->constraint_tracker.read_in(sl).size();
            l_writes += this->constraint_tracker.written_in(sl).size();
        }

        for (auto &sl : *r) {
            r_reads += this->constraint_tracker.read_in(sl).size();
            r_writes += this->constraint_tracker.written_in(sl).size();
        }

        if (l_writes < r_writes) {
            return true;
        } else if (l_writes > r_writes) {
            return false;
        } else {
            if (l_reads > r_reads) {
                return true;
            } else {
                return false;
            }
        }
    };
    slice_lists.sort(SliceListComparator);
    if (LOGGING(6)) {
        LOG6("Slice list on output");
        for (auto sl : slice_lists) {
            LOG6("  " << sl);
        }
    }
}

void ActionPhvConstraints::sort(std::vector<PHV::FieldSlice>& slices) const {
    ordered_map<const PHV::Field*, std::vector<PHV::FieldSlice>> fields;
    for (const auto& fs : slices) {
        fields[fs.field()].push_back(fs);
    }
    auto priorities = compute_sources_first_order(fields);

    std::sort(slices.begin(), slices.end(),
        [&](PHV::FieldSlice l, PHV::FieldSlice r) {
            if (priorities.at(l.field()) != priorities.at(r.field())) {
                return priorities.at(l.field()) < priorities.at(r.field());
            }
            auto l_reads = this->constraint_tracker.read_in(l).size();
            auto l_writes = this->constraint_tracker.written_in(l).size();
            auto r_reads = this->constraint_tracker.read_in(r).size();
            auto r_writes = this->constraint_tracker.written_in(r).size();
            if (l_writes != r_writes)
                return l_writes < r_writes;
            return l_reads > r_reads; });
}

void ActionPhvConstraints::dest_first_sort(
    std::vector<const PHV::SuperCluster::SliceList*>& slice_lists) const {
    ordered_map<const PHV::Field*, std::vector<PHV::FieldSlice>> fields;
    for (const auto* sl : slice_lists) {
        for (const auto& fs : *sl) {
            fields[fs.field()].push_back(fs);
        }
    }
    auto priorities = compute_sources_first_order(fields);
    ordered_map<const PHV::SuperCluster::SliceList*, int> priority;
    for (const auto* sl : slice_lists) {
        for (const auto& fs : *sl) {
            if (priority.count(sl)) {
                priority[sl] = std::max(priority[sl], priorities.at(fs.field()));
            } else {
                priority[sl] = priorities.at(fs.field());
            }
        }
    }

    ordered_map<const PHV::SuperCluster::SliceList*, int> num_action_write;
    for (const auto* sl : slice_lists) {
        for (const auto& fs : *sl) {
            num_action_write[sl] += this->constraint_tracker.written_in(fs).size();
        }
    }

    const auto SliceListComparator = [&](const PHV::SuperCluster::SliceList* l,
                                         const PHV::SuperCluster::SliceList* r) {
        // NOTE: wide_arithmetic slice lists are handled differently in the new allocator
        // so that we do not need to care about their ordering here like before.

        // by topological order first.
        if (priority.at(l) != priority.at(r)) {
            return priority.at(l) > priority.at(r);
        }
        // by number of actions that write them.
        if (num_action_write.at(l) != num_action_write.at(r)) {
            return num_action_write.at(l) > num_action_write.at(r);
        }
        //  by lo if same field.
        const auto& l_front = l->front();
        const auto& r_front = r->front();
        if (l_front.field() == r_front.field()) {
            BUG_CHECK(l_front.range().lo != r_front.range().lo,
                      "same field slice in different slice lists: %1% and %2%", l, r);
            return l_front.range().lo < r_front.range().lo;
        }
        return false;
    };
    std::stable_sort(slice_lists.begin(), slice_lists.end(), SliceListComparator);
}

void ActionPhvConstraints::determine_same_byte_fields() {
    ordered_map<const PHV::Field*, le_bitrange> header_bytes;
    for (const auto& f : phv) {
        if (f.metadata || f.pov) continue;
        le_bitrange hdr_byte_range = f.byteAlignedRangeInBits();
        LOG7("\t  Range for " << f.name << " : " << hdr_byte_range);
        for (auto kv : header_bytes) {
            if (kv.first->header() != f.header()) continue;
            if (kv.second.overlaps(hdr_byte_range)) {
                LOG6("\t" << kv.first->name << " and " << f.name << " share a byte");
                same_byte_fields[kv.first].insert(&f);
                same_byte_fields[&f].insert(kv.first);
            }
        }
        header_bytes[&f] = hdr_byte_range;
    }
}

const ordered_set<PHV::FieldSlice>
ActionPhvConstraints::get_slices_in_same_byte(const PHV::FieldSlice& slice) const {
    ordered_set<PHV::FieldSlice> rv;
    rv.insert(PHV::FieldSlice(slice.field(), slice.range()));
    if (!same_byte_fields.count(slice.field())) return rv;
    le_bitrange limit = slice.byteAlignedRangeInBits();
    LOG7("    Range for slice " << limit);
    for (const auto* f : same_byte_fields.at(slice.field())) {
        LOG7("    Checking same byte field: " << f->name);
        le_bitrange f_limit = f->byteAlignedRangeInBits();
        if (limit.contains(f_limit)) {
            rv.insert(PHV::FieldSlice(f, StartLen(0, f->size)));
        } else if (limit.overlaps(f_limit)) {
            LOG7("      Field " << f->name << " overlaps with " << slice);
            // TODO: Need to handle this case.
        }
    }
    return rv;
}

bool ActionPhvConstraints::early_check_ok(const IR::MAU::Action* act) {
    const auto& writes_in_act = constraint_tracker.writes(act);
    ordered_map<PHV::FieldSlice, OperandInfo> slice_to_info;
    for (auto& info : writes_in_act) {
        BUG_CHECK(info.phv_used != std::nullopt, "Write slice cannot be NULL.");
        slice_to_info[*(info.phv_used)] = info;
    }

    std::stringstream error_msg;
    bool rv = true;
    ordered_set<PHV::FieldSlice> slicesConsidered;
    for (auto& slice : Keys(slice_to_info)) {
        // We have already checked constraints related to @slice if the slice is in slicesConsidered
        // set. Therefore, go to the next slice.
        if (slicesConsidered.count(PHV::FieldSlice(slice.field(), slice.range()))) continue;
        std::stringstream ss;
        const auto slices_in_same_byte = get_slices_in_same_byte(slice);
        if (!valid_container_operation_type(act, slices_in_same_byte, ss)) {
            rv = false;
            error_msg << ss.str();
        }
        slicesConsidered.insert(slices_in_same_byte.begin(), slices_in_same_byte.end());
    }
    if (!rv) error("%1%", error_msg.str());
    return rv;
}

std::set<int> ActionPhvConstraints::stages(const IR::MAU::Action* action,
                                           bool physical_stage) const {
    const auto tbl = tableActionsMap.getTableForAction(action);
    if (!tbl) BUG("Action %1% not defined as part of a table.", action->name);
    if (physical_stage) {
        return PhvInfo::physicalStages(*tbl);
    } else {
        return {dg.min_stage(*tbl)};
    }
}

ActionPhvConstraints::NumContainers ActionPhvConstraints::num_container_sources(
        const PHV::Allocation &alloc,
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const IR::MAU::Action* action) const {
    // source containers used in @p action
    ordered_set<PHV::Container> source_containers;
    // source -> destinations mapping.
    ordered_map<PHV::FieldSlice, ordered_set<PHV::AllocSlice>> source_to_dest;
    // unallocated slices, grouped by field
    ordered_map<const PHV::Field*, ordered_set<PHV::FieldSlice>> unalloc_slices;
    int num_unallocated = 0;
    // TODO: these num_double_* were introduced in
    // https://github.com/barefootnetworks/bf-p4c-compilers/pull/3559/files
    // to capture a cornor case: slices alive at the same time in a container
    // written by same source, the source should count as two sources.
    // NOTE: although named as double_*, can be multiple.
    int num_double_allocated = 0;
    int num_double_unallocated = 0;
    for (const auto& slice : container_state) {
        // when computing num container sources, we only need to verify this action in one
        // stage, because actions are the same across all stages.
        const int stage = *stages(action, slice.isPhysicalStageBased()).begin();
        auto reads = constraint_tracker.sources(slice, action);
        // No need to include metadata initialization here because metadata initialized always
        // happens with a constant/action data as source.
        if (reads.size() == 0)
            LOG5("\t\t\t\tSlice " << slice << " is not written in action " << action->name);
        for (auto operand : reads) {
            if (operand.ad || operand.constant || !operand.phv_used) {
                LOG6("\t\t\t\t" << operand << " doesn't count as a container source");
                continue;
            }
            const PHV::FieldSlice& source = *(operand.phv_used);
            // compute allocated slices for this source, which is
            // current allocation object + in this container.
            ordered_set<PHV::AllocSlice> source_allocated_slices =
                alloc.slices(source.field(), source.range(), stage,
                             PHV::FieldUse(PHV::FieldUse::READ));
            for (const auto& container_source : container_state) {
                if (container_source.field() == source.field() &&
                    container_source.field_slice().overlaps(source.range())) {
                    source_allocated_slices.insert(container_source);
                }
            }

            // compute allocated container of this source.
            ordered_set<PHV::Container> source_allocated_containers;
            for (auto source_slice : source_allocated_slices) {
                source_allocated_containers.insert(source_slice.container());
                LOG5("\t\t\t\t\tSource slice for " << slice << " : " << source_slice);
            }

            // double_read check, note that slices with disjoint liveranges should
            // not be considered as double_read.
            bool source_was_read_before = false;
            if (source_to_dest.count(source)) {
                for (const auto& prev_dest : source_to_dest.at(source)) {
                    if (!prev_dest.isLiveRangeDisjoint(slice)) {
                        LOG5("\t\t\t\t" << source.field()->name << source.range()
                                        << " has already been read by "
                                        << source_to_dest.at(source));
                        source_was_read_before = true;
                    }
                }
            }
            source_to_dest[source].insert(slice);

            if (source_allocated_containers.size() == 0) {
                LOG5("\t\t\t\tSource " << source << " has not been allocated yet.");
                unalloc_slices[source.field()].insert(source);
                if (source_was_read_before) ++num_double_unallocated;
            } else {
                source_containers.insert(source_allocated_containers.begin(),
                                         source_allocated_containers.end());
                if (source_was_read_before) ++num_double_allocated;
            }
        }
    }

    // compute number of containers needed for unallocated slices.
    for (auto kv : unalloc_slices) {
        LOG5("\t\t\t\tSource field: " << kv.first->name);
        if (kv.first->no_split() || kv.first->no_holes()) {
            // If fields were
            // (1) no_split: they will end up in one container.
            // (2) no_holes: used in special instructions like funnel-shift
            //     and wide_arithmetic ops, we can treat them as sourcing
            //     from one container. TODO: need to verify this.
            ++num_unallocated;
        } else {
            for (const auto& slice : kv.second) {
                LOG5("\t\t\t\t  Unallocated slice: " << slice);
                ++num_unallocated;
            }
        }
    }

    // TODO: not sure why we add num_double_unallocated to this?
    const int total_allocated = source_containers.size() + num_double_allocated;
    const int total_unallocated = num_unallocated + num_double_unallocated;

    if (LOGGING(5)) {
        LOG5("\t\t\t\tDouble allocation, allocated: " << num_double_allocated <<
             ", unallocated: " << num_double_unallocated);
        LOG5("\t\t\t\tNumber of allocated sources  : " << total_allocated);
        LOG5("\t\t\t\tNumber of unallocated sources: " << total_unallocated);
        LOG5("\t\t\t\tTotal number of sources      : " << total_allocated + total_unallocated);
    }

    NumContainers rv(total_allocated, total_unallocated);
    rv.double_unallocated = (num_double_unallocated > 0);
    return rv;
}

std::optional<PHV::AllocSlice> ActionPhvConstraints::getSourcePHVSlice(
    const PHV::Allocation& alloc, const std::vector<PHV::AllocSlice>& slices,
    const PHV::AllocSlice& dst, const IR::MAU::Action* action, const int stage) const {
    LOG5("\t\t\t\tgetSourcePHVSlices for action: " << action->name << " and slice " << dst);
    auto *field = dst.field();
    auto reads = constraint_tracker.sources(dst, action);

    if (reads.size() == 0)
        LOG5("\t\t\t\tField " << field->name << " is not written in action " << action->name);
    else
        LOG5("\t\t\t\tField " << field->name << " is written in action "  << action->name <<
             " using " << reads.size() << " operands");

    // There should be only one source per dest slice.
    std::optional<PHV::AllocSlice> source_slice = std::nullopt;
    for (auto operand : reads) {
        if (operand.ad || operand.constant) continue;
        const PHV::Field* fieldRead = operand.phv_used->field();
        const le_bitrange rangeRead = operand.phv_used->range();
        LOG5("\t\t\t\t\tSlice read: " << PHV::FieldSlice(fieldRead, rangeRead));
        ordered_set<PHV::FieldSlice> seen;  // to filter out duplicated slices.
        ordered_set<PHV::AllocSlice> source_slices =
            alloc.slices(fieldRead, rangeRead, stage, PHV::FieldUse(PHV::FieldUse::READ));
        for (auto& allocated_slice : source_slices) {
            LOG5("\t\t\t\t\tAllocated Slice: " << allocated_slice);
            seen.insert(PHV::FieldSlice(allocated_slice.field(), allocated_slice.field_slice()));
        }

        // Add any source slices found in @slices, which are the proposed packing.
        // any overlapping slice will be added.
        for (auto& candidate_slice : slices) {
            if (candidate_slice.field() == fieldRead &&
                candidate_slice.field_slice().overlaps(rangeRead)) {
                if (seen.count(
                        PHV::FieldSlice(candidate_slice.field(), candidate_slice.field_slice()))) {
                    LOG5("\t\t\t\t\tignore duplicated Candidate Slice: " << candidate_slice);
                    continue;
                }
                if (!candidate_slice.isLiveAt(stage, PHV::FieldUse(PHV::FieldUse::READ))) {
                    LOG5("\t\t\t\t\tignore dead Candidate Slice: " << candidate_slice);
                    continue;
                }
                LOG5("\t\t\t\t\tCandidate Slice: " << candidate_slice);
                source_slices.insert(candidate_slice);
            }
        }

        // source not found or not allocated.
        if (source_slices.empty()) {
            continue;
        }

        // check and split out the allocSlice that matches the source field range.
        if (source_slices.size() > 1) {
            const auto sources_str = [&]() {
                std::stringstream ss;
                for (const auto& slice : source_slices) {
                    ss << slice << ";";
                }
                return ss.str();
            };
            BUG_CHECK(are_adjacent_field_slices(source_slices),
                      "Multiple source slices found in getSourcePHVSlice() for %1%: %2%",
                      fieldRead->name, sources_str());
            const bool all_adjacent_container_slices =
                std::adjacent_find(source_slices.begin(), source_slices.end(),
                                   [&](const PHV::AllocSlice& a, const PHV::AllocSlice& b) {
                                       return a.container() != b.container() ||
                                              a.container_slice().hi + 1 != b.container_slice().lo;
                                   }) == source_slices.end();
            BUG_CHECK(all_adjacent_container_slices,
                      "Multiple container found for one proposed allocSlice for %1%: %2%",
                      fieldRead->name, sources_str());
        }
        // Alloc object might return merged slices that its range is wider than readRange.
        // We need to cut the range to match the destination slice.
        const auto& first = source_slices.front();
        const int container_range_start =
            rangeRead.lo - first.field_slice().lo + first.container_slice().lo;
        PHV::AllocSlice rst =
            PHV::AllocSlice(first.field(), first.container(), rangeRead,
                            le_bitrange(StartLen(container_range_start, rangeRead.size())));
        rst.setIsPhysicalStageBased(first.isPhysicalStageBased());
        rst.setLiveness(first.getEarliestLiveness(), first.getLatestLiveness());
        // a corner case:
        // f1 = f1 | f2, when f1 and f2 are allocated (overlaid) to the same container
        // because f1 was uninitialized, it is okay to return any source here.
        BUG_CHECK(!source_slice || (source_slice->container() == rst.container() &&
                                    source_slice->container_slice() == rst.container_slice()),
                  "multiple source slice found for one dest: %1% and %2%", *source_slice, rst);
        source_slice = rst;
    }

    return source_slice;
}

//  Note: If both action data and constant are used in the same action as operands on the same
//  container, action data allocation folds them into one action data parameter to ensure a
//  legal Tofino action. Same is true when multiple action data and/or multiple constants are used
//  as operands on the same container in the same action.
bool ActionPhvConstraints::has_ad_or_constant_sources(
        const PHV::Allocation::MutuallyLiveSlices& slices,
        const IR::MAU::Action* action,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    for (const auto& slice : slices) {
        for (auto operand : constraint_tracker.sources(slice, action)) {
            if (operand.ad || operand.constant) {
                LOG5("\t\t\t\t  Field " << slice.field()->name <<
                     " written using action data/constant in action " << action->name);
                return true; } }
        // If the field is initialized due to metadata initialization in @action, then add
        // constant/action data source for the field.
        if (initActions.count(slice.field()) && initActions.at(slice.field()).count(action)) {
            LOG5("\t\t\t\t  Field " << slice.field()->name << " initialized for live range "
                 "shrinking in action " << action->name);
            return true; }
    }
    return false;
}

ActionPhvConstraints::ActionDataUses ActionPhvConstraints::all_or_none_constant_sources(
        const PHV::Allocation::MutuallyLiveSlices& slices,
        const IR::MAU::Action* action,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    ordered_set<PHV::AllocSlice> slices_written_by_ad;
    ordered_set<PHV::AllocSlice> slices_written_by_special_ad;
    ordered_set<PHV::AllocSlice> padding_slices;
    bool has_non_special_slices_written_by_constant_only = true;
    unsigned speciality_type = ActionAnalysis::ActionParam::NO_SPECIAL;
    for (const auto& slice : slices) {
        for (auto operand : constraint_tracker.sources(slice, action)) {
            if (operand.ad || operand.constant) {
                slices_written_by_ad.insert(slice);
                if (operand.special_ad) {
                    speciality_type |= operand.special_ad;
                    slices_written_by_special_ad.insert(slice);
                } else {
                    has_non_special_slices_written_by_constant_only &= operand.constant;
                }
            }
        }
        if (initActions.count(slice.field()) && initActions.at(slice.field()).count(action))
            slices_written_by_ad.insert(slice);
        bool is_padding = !uses.is_referenced(slice.field()) || slice.field()->padding;
        if (is_padding)
            padding_slices.insert(slice);
    }
    if (LOGGING(5))
        for (const auto& slice : slices_written_by_special_ad)
            LOG5("\t\t\t\t\t  Special AD slice: " << slice);
    unsigned num_slices_written_by_special_ad = slices_written_by_special_ad.size();
    unsigned num_slices_written_by_ad = slices_written_by_ad.size();
    unsigned num_slices_padding = padding_slices.size();
    LOG5("\t\t\t\t\tSpecial AD slices: " << num_slices_written_by_special_ad <<
         ", AD slices: " << num_slices_written_by_ad << ", Padding slices: " << num_slices_padding);
    BUG_CHECK(num_slices_written_by_special_ad <= num_slices_written_by_ad,
              "Slices written by speciality action data cannot be greater than slices written by "
              "action data");
    if (num_slices_written_by_ad == 0) {
        LOG5("\t\t\t\t  No slice in proposed packing written by action data/constant in action "
             << action->name);
        return ALL_AD_CONSTANT; }

    // If there is a slice written by speciality action data and another slice written by other
    // action data that must always be packed in the same container, then do not return
    // COMPLEX_AD_PACKING_REQ. Instead, allow the failure to happen in table placement later.
    bool check_speciality_packing = true;
    for (auto& sl_ad : slices_written_by_ad) {
        int offset_ad = sl_ad.field()->offset;
        int left_sl_ad = (offset_ad + sl_ad.field_slice().lo) / 8;
        int right_sl_ad = ROUNDUP(offset_ad + sl_ad.field_slice().hi, 8);
        for (auto& sl_special : slices_written_by_special_ad) {
            // Only applicable for header fields as the strict parser alignment requirements are
            // only for headers.
            if (sl_ad.field()->metadata || sl_special.field()->metadata) continue;
            // If the slices are of fields belonging to different headers, then we do not have any
            // pack-together constraints from the parser.
            if (sl_ad.field()->header() != sl_special.field()->header()) continue;
            int offset_special = sl_special.field()->offset;
            int left_sl_special = (offset_special + sl_special.field_slice().lo) / 8;
            int right_sl_special = ROUNDUP(offset_special + sl_special.field_slice().hi, 8);
            LOG6("\t\t\t\t  Slice ad: " << sl_ad << ", offset: " << offset_ad << ", lo: " <<
                    left_sl_ad << ", hi: " << right_sl_ad);
            LOG6("\t\t\t\t  Slice special: " << sl_special << ", offset: " << offset_special <<
                    ", lo: " << left_sl_special << ", hi: " << right_sl_special);
            // If there is an overlap between two different slices (they share the same byte), then
            // the left limit of the later slice is going to be 1 less than the right limit of the
            // earlier slice.
            if (sl_ad != sl_special &&
               (left_sl_special + 1 == right_sl_ad || left_sl_ad + 1 == right_sl_special)) {
                LOG5("\t\t\t\t  Slices " << sl_ad << " and " << sl_special << " must be packed in "
                     "the same container and share a byte.");
                check_speciality_packing = false;
            }
        }
    }

    if (check_speciality_packing && num_slices_written_by_special_ad != 0 &&
            num_slices_written_by_ad != num_slices_written_by_special_ad) {
        // For an action where some slices are written by a HASH_DIST operation and the other slices
        // are all written by constants (not action data), packing is possible.
        if (speciality_type == ActionAnalysis::ActionParam::HASH_DIST &&
                has_non_special_slices_written_by_constant_only) {
            LOG5("\t\t\t\t  Can combine HASH_DIST sources with constant data.");
        } else {
            // We currently disable packing of field slices if there is a mixture of speciality
            // action data and normal action data reads in the same action. This may be relaxed in
            // the future when action data packing becomes more efficient.
            LOG5("\t\t\t\t  This packing will require combining a speciality action data with "
                 "other action data for action " << action->name << ". The compiler currently does "
                 "not support this feature.");
            return COMPLEX_AD_PACKING_REQ; }
    }
    if (num_slices_written_by_ad + num_slices_padding == slices.size()) {
        LOG5("\t\t\t\t  All slices in proposed packing written by action data/constant in action "
             << action->name);
        return ALL_AD_CONSTANT; }
    LOG5("\t\t\t\t  Only " << num_slices_written_by_ad << " out of " << slices.size() << " slices "
         " in proposed packing are written by action data/constant in action " << action->name);
    return SOME_AD_CONSTANT;
}

int ActionPhvConstraints::unallocated_bits(PHV::Allocation::MutuallyLiveSlices slices,
        const PHV::Container c) const {
    int size_used = 0;
    for (const auto& slice : slices)
        size_used += slice.width();
    if (int(c.size()) < size_used)
        LOG4("Total size of mutually live slices is greater than the size of the container");
    return (c.size() - size_used);
}

bool ActionPhvConstraints::valid_container_operation_type(
        const IR::MAU::Action* action,
        const ordered_set<PHV::FieldSlice>& slices,
        std::stringstream& ss) const {
    unsigned type_of_operation = 0;
    ordered_set<PHV::FieldSlice> fields_not_written;
    ordered_map<cstring, ordered_set<PHV::FieldSlice>> operations_to_fields;
    cstring operation;
    cstring action_name = canon_name(action->externalName());

    cstring hdr = (*(slices.begin())).field()->header();

    ss << "This program violates action constraints imposed by " << Device::name() <<
        "." << std::endl << std::endl;
    ss << "  The following field slices must be allocated in the same container as they are "
          "present within the same byte of header " << hdr << ":" << std::endl;
    for (auto& slice : slices) {
        ss << "\t" << slice.shortString() << std::endl;
        std::optional<OperandInfo> fw = constraint_tracker.is_written(slice, action);
        bool is_padding = !uses.is_referenced(slice.field()) || slice.field()->padding;
        // Unreferenced fields may be overwritten no issues.
        if (!fw) {
            if (!is_padding)
                fields_not_written.insert(slice);
        } else if (fw->flags & OperandInfo::MOVE) {
            type_of_operation |= OperandInfo::MOVE;
            operations_to_fields["assignment"_cs].insert(slice);
            operation = "assignment"_cs;
        } else if (fw->flags & OperandInfo::BITWISE) {
            type_of_operation |= OperandInfo::BITWISE;
            operations_to_fields[fw->operation].insert(slice);
            operation = fw->operation;
        } else if (fw->flags & OperandInfo::WHOLE_CONTAINER) {
            type_of_operation |= OperandInfo::WHOLE_CONTAINER;
            operations_to_fields[fw->operation].insert(slice);
            operation = fw->operation;
        }
    }

    // If there are multiple instruction types for the same container in the same action, then
    // violates action constraints. Therefore, return false.
    if (operations_to_fields.size() > 1) {
        ss << std::endl;
        ss << "  However, the program requires multiple instruction types for the same container "
              "in the same action (" << action_name << "):" << std::endl;
        for (auto kv : operations_to_fields) {
            ss << "\tThe following slice(s) are written using " << kv.first << " instruction." <<
                std::endl;
            for (auto& slice : kv.second)
                ss << "\t  " << slice.shortString() << std::endl;
        }
        ss << std::endl << "Therefore, the program requires an action impossible to synthesize for "
           << Device::name() << " ALU. Rewrite action " << action_name << " to use the same " <<
           "instruction for all the above field slices that must be in the same container."<<
           std::endl;
        return false;
    }

    // If there is a WHOLE_CONTAINER or a BITWISE operation writing to this container, and some
    // slice in the container is not written, then the unwritten slice would be overwritten
    // illegally. Therefore, flag an error message for this, and return false.
    if (((type_of_operation & OperandInfo::WHOLE_CONTAINER) ||
                (type_of_operation & OperandInfo::BITWISE)) && fields_not_written.size() > 0) {
        ss << std::endl;
        ss << "  However, the following fields slice(s) are written in action " << action_name <<
              " by the " << operation << " instruction, which operates on the entire container:" <<
              std::endl;
        for (auto& slice : operations_to_fields.at(operation))
            ss << "\t" << slice.shortString() << std::endl;
        ss << std::endl << "  As a result, the following field slice(s) not written in action " <<
            action_name << " will be overwritten illegally:" << std::endl;
        for (auto& slice : fields_not_written)
            ss << "\t" << slice.shortString() << std::endl;
        ss << std::endl << "Therefore, the program requires an action impossible to synthesize for "
           << Device::name() << " ALU." << std::endl;
        return false;
    }

    return true;
}

unsigned
ActionPhvConstraints::container_operation_type(
    const IR::MAU::Action* action, const PHV::Allocation::MutuallyLiveSlices& slices,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    LOG5("\t\t\tChecking container operation type for action: " << action->name);
    unsigned type_of_operation = 0;
    size_t num_fields_not_written = 0;
    ordered_set<const PHV::Field*> observed_fields;
    cstring operation;

    for (const auto& slice : slices) {
        auto field_slice = PHV::FieldSlice(slice.field(), slice.field_slice());
        std::optional<OperandInfo> fw = constraint_tracker.is_written(field_slice, action);
        bool is_padding = !uses.is_referenced(slice.field()) || slice.field()->padding;
        if (!fw) {
            if (initActions.count(slice.field()) && initActions.at(slice.field()).count(action)) {
                // This slice is written by metadata initialization for this action.
                type_of_operation |= OperandInfo::MOVE;
            } else if (!is_padding) {
                num_fields_not_written++;
            }
        } else if (fw->flags & OperandInfo::MOVE) {
            type_of_operation |= OperandInfo::MOVE;
        } else if (fw->flags & OperandInfo::BITWISE) {
            type_of_operation |= OperandInfo::BITWISE;
            if (operation == cstring()) {
                LOG5("\t\t\t\t  First bitwise operation found: " << fw->operation);
                operation = fw->operation;
            } else if (operation == fw->operation) {
                LOG5("\t\t\t\t  Subsequent bitwise operation found: " << fw->operation);
            } else {
                LOG5("\t\t\t\t  Action " << action->name << " uses multiple different bitwise "
                        "operations for slices in the proposed packing.");
                return OperandInfo::MIXED;
            }
        } else if (fw->flags & OperandInfo::WHOLE_CONTAINER) {
            type_of_operation |= OperandInfo::WHOLE_CONTAINER;
            // Check if it a whole container operation on adjacent slices of the same field
            observed_fields.insert(slice.field());
        } else if (fw->flags & OperandInfo::PART_OF_CONTAINER) {
            type_of_operation |= OperandInfo::PART_OF_CONTAINER;
            observed_fields.insert(slice.field());
        } else {
            warning("Detected a write that is neither move nor whole container "
                    "operation.");
        }
    }

    // If there is a WHOLE_CONTAINER operation present, check if the slices written by the whole
    // container operations belong to the same field. If yes, then return
    // OperandInfo::WHOLE_CONTAINER_SAME_FIELD. If not, then return
    // OperandInfo::WHOLE_CONTAINER, which means that the proposed packing is not valid.
    // For debugging purposes, we distinguish cases where we have a mixture between bitwise/move
    // and whole container operations. Additionally, if a whole container operation is detected
    // in the action and we find that there is a slice not written in the same action
    // (num_fields_not_written > 0), then the proposed packing is not valid, which is indicated
    // by returning OperandInfo::MIXED (mix of not written and whole container write).
    // If there is a PART_OF_CONTAINER operation present, the checks are the same as in case
    // of WHOLE_CONTAINER. However, the returned operation type is PART_OF_CONTAINER to indicate
    // that the debugging check in valid_container_operation_type shouldn't apply, because
    // only a part of container is supposed to be overwritten. We then rely on
    // ContainerAction::verify_overwritten to catch packing that lead to incorrectly overwritten
    // fields.
    if (type_of_operation & (OperandInfo::WHOLE_CONTAINER | OperandInfo::PART_OF_CONTAINER)) {
        if (type_of_operation & OperandInfo::WHOLE_CONTAINER) {
            if (num_fields_not_written) {
                LOG5("\t\t\t\tAction " << action->name << " uses a whole container operation but "
                        << num_fields_not_written << " slices are not written in this action.");
                return OperandInfo::MIXED; }
        }

        if (type_of_operation & OperandInfo::MOVE) {
            LOG5("\t\t\t\tAction " << action->name << " uses both whole container and move "
                    "operations for slices in the proposed packing.");
            return OperandInfo::MIXED; }

        if (type_of_operation == (OperandInfo::WHOLE_CONTAINER | OperandInfo::PART_OF_CONTAINER)) {
            LOG5("\t\t\t\tAction " << action->name << " uses multiple whole container "
                   "operations for slices in the proposed packing.");
            return OperandInfo::MIXED;
        }

        LOG5("\t\t\t\tNumber of fields written to by this whole container operation: " <<
                observed_fields.size());
        if (observed_fields.size() == 1) {
            return (type_of_operation & OperandInfo::WHOLE_CONTAINER) ?
                          OperandInfo::WHOLE_CONTAINER_SAME_FIELD
                        : OperandInfo::PART_OF_CONTAINER;
        }

        if (type_of_operation == OperandInfo::PART_OF_CONTAINER)
            return OperandInfo::PART_OF_CONTAINER;

        return OperandInfo::WHOLE_CONTAINER;
    }

    // For BITWISE operations, we have already checked above that the bitwise operation used per
    // action is the same for all slices in the proposed packing. We also must make sure that no
    // field slice in the proposed packing is unwritten when the bitwise operation is used.
    // Finally, for debug purposes, we explicitly flag cases where there is a mixture of MOVE
    // and BITWISE operations.
    if (type_of_operation & OperandInfo::BITWISE) {
        if (num_fields_not_written) {
            LOG5("\t\t\t\tAction " << action->name << " uses a bitwise operation but " <<
                    num_fields_not_written << " slices are not written in this action.");
            return OperandInfo::MIXED; }
        if (type_of_operation & OperandInfo::MOVE) {
            LOG5("\t\t\t\tAction " << action->name << " uses both bitwise and move operations "
                    "for slices in the proposed packing.");
            return OperandInfo::MIXED; }
        return OperandInfo::BITWISE; }

    return OperandInfo::MOVE;
}

bool ActionPhvConstraints::are_adjacent_field_slices(
        const PHV::Allocation::MutuallyLiveSlices& container_state) const {
    // ignore prepending padding or unused fieldslices.
    auto itr = container_state.begin();
    while (itr != container_state.end() &&
           (!uses.is_referenced(itr->field()) || itr->field()->padding)) {
        itr++;
    }
    itr = std::adjacent_find(
        itr, container_state.end(), [](const PHV::AllocSlice& a, const PHV::AllocSlice& b) {
            return a.field() != b.field() || a.field_slice().hi + 1 != b.field_slice().lo;
        });
    // ignore postpending padding or unused fieldslices.
    if (itr != container_state.end()) {
        itr++;
        while (itr != container_state.end() &&
               (!uses.is_referenced(itr->field()) || itr->field()->padding)) {
            itr++;
        }
    }
    return itr == container_state.end();
}

unsigned ActionPhvConstraints::count_container_holes(
    const PHV::Allocation::MutuallyLiveSlices& container_state) const {
    le_bitrange last;
    bool firstSlice = true;
    cstring lastField;
    unsigned numBreaks = 0;
    for (const auto& slice : container_state) {
        le_bitrange range = slice.container_slice();
        // No checks for the first slice
        if (firstSlice) {
            lastField = slice.field()->name;
            last = range;
            firstSlice = false;
            continue; }
        if (last.hi + 1 != range.lo) {
            LOG7("\t\t\t\t\tSlices [" << last.lo << ", " << last.hi << "] of field " <<
                    lastField << " and [" << range.lo << ", " << range.hi << "] of field " <<
                    slice.field() << " are not adjacent.");
            numBreaks += 1; }
        last = range;
        lastField = slice.field()->name; }
    return numBreaks;
}

bool ActionPhvConstraints::pack_slices_together(
        const PHV::Allocation &alloc,
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        PackingConstraints* packing_constraints,
        const IR::MAU::Action* action,
        bool pack_unallocated_only,  /*If true, only unallocated slices will be packed together*/
        bool ad_source_present /* action data/constant source present */) const {
    const PHV::Field* no_pack_source_field = nullptr;
    if (pack_unallocated_only)
        LOG5("\t\t\t\t\tPack all unallocated slices together. All bits in container are occupied.");
    else
        LOG5("\t\t\t\t\tPack all slices together.");
    ordered_set<PHV::FieldSlice> pack_together;
    ordered_set<PHV::FieldSlice> pack_together_no_pack;
    ordered_set<const PHV::Field*> pack_together_fields;
    bool pack_together_has_no_pack = false;
    for (const auto& slice : container_state) {
        // copack constraints are not dependent on stages, so just use the first one.
        const int stage = *stages(action, slice.isPhysicalStageBased()).begin();
        for (auto operand : constraint_tracker.sources(slice, action)) {
            if (operand.ad || operand.constant)
                continue;
            const PHV::Field* fieldRead = operand.phv_used->field();
            le_bitrange rangeRead = operand.phv_used->range();
            if (pack_unallocated_only) {
                ordered_set<PHV::Container> containers;
                ordered_set<PHV::AllocSlice> per_source_slices =
                    alloc.slices(fieldRead, rangeRead, stage, PHV::FieldUse(PHV::FieldUse::READ));

                // Add any source slices found in @slices, which are the proposed packing.
                for (auto &packed_slice : container_state)
                    // TODO: Should this be overlaps() or contains()?
                    if (packed_slice.field() == fieldRead &&
                            packed_slice.field_slice().overlaps(rangeRead))
                        per_source_slices.insert(packed_slice);

                for (const auto& slice : per_source_slices)
                    containers.insert(slice.container());
                if (containers.size() != 0)
                    continue; }

            // Insert the slices to be packed together into the UnionFind structure
            LOG3("\t\t\t\t\tInserting " << fieldRead->name << " [" << rangeRead.lo << ", " <<
                    rangeRead.hi << "] into copacking_constraints for action " << action->name);
            if (fieldRead->is_solitary() &&
               (no_pack_source_field == nullptr || fieldRead == no_pack_source_field)) {
                // If the source is no-pack and there is no other no-pack field encountered, then
                // add this to the no-pack slice list. Also do the same if this slice belongs to the
                // already noted no-pack field.
                no_pack_source_field = fieldRead;
                LOG6("\t\t\t\t\tFound a no-pack field, so we need a second source");
                pack_together_no_pack.insert(PHV::FieldSlice(fieldRead, rangeRead));
            } else {
                // Note if we have a second no-pack source.
                if (fieldRead->is_solitary()) {
                    pack_together_fields.insert(fieldRead);
                    pack_together_has_no_pack = true;
                }
                LOG3("pack together " << fieldRead);
                pack_together.insert(PHV::FieldSlice(fieldRead, rangeRead));
            }
        }
    }

    // The first no-pack source always goes by itself in the pack_together_no_pack set. If
    // pack_together_has_no_pack is set, then it means we have at least a second no-pack source. If
    // there are more than two no-pack sources or if another field without no-pack is also required
    // to have a conditional constraint, then pack_together_fields will have more than one member.
    // However, if there is a second no-pack source, pack_together_fields cannot have more than one
    // member either. Therefore, throw an error.
    if (pack_together_has_no_pack && pack_together_fields.size() > 1) {
        LOG5("\t\t\t\t  Conditional constraints require use of more than two PHV containers.");
        return false;
    }

    // If there are multiple containers required for the conditional constraints--indicated by both
    // pack_together and pack_together_no_pack sets having members present, and the allocation also
    // has an action data/constant source, then this allocation is invalid.
    if (ad_source_present && pack_together.size() > 0 && pack_together_no_pack.size() > 0) {
        LOG5("\t\t\t\t  Conditional constraints require use of two PHV containers as sources. "
             "However, this is impossible because one of the two allowed sources is action "
             "data/constant.");
        return false;
    }

    if (LOGGING(5)) {
        std::stringstream ss;
        for (const auto& slice : pack_together)
            ss << slice;
        LOG5("\t\t\t\t\tPack together: " << ss.str()); }

    PHV::FieldSlice *firstSlice = nullptr;
    // Pack all the non no-pack slices together.
    for (const auto& slice : pack_together) {
        if (firstSlice == nullptr) {
            LOG5("\t\t\t\t\t\tSetting first slice to  " << slice);
            firstSlice = new PHV::FieldSlice(slice.field(), slice.range()); }
        LOG5("\t\t\t\t\tUnion " << *firstSlice << " with " << slice);
        packing_constraints->at(action).makeUnion(*firstSlice, slice); }
    // Pack the no-pack slices together in the second source.
    firstSlice = nullptr;
    for (const auto& slice : pack_together_no_pack) {
        if (firstSlice == nullptr) {
            LOG5("\t\t\t\t\t\tSetting first slice to " << slice);
            firstSlice = new PHV::FieldSlice(slice.field(), slice.range()); }
        LOG5("\t\t\t\t\tUnion " << *firstSlice << " with " << slice);
        packing_constraints->at(action).makeUnion(*firstSlice, slice); }
    return true;
}

// At this point, any packing is valid, having passed the can_pack() method. Also, if fields are
// mutually exclusive, they are written by different actions and in different tables, so the writes
// to those mutually exclusive fields should not have an effect on the number of bitmasked-set
// instructions detected.
int ActionPhvConstraints::count_bitmasked_set_instructions(
        const std::vector<PHV::AllocSlice>& slices,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    int numBitmaskedSet = 0;
    if (slices.size() == 0) return 0;
    // Create a set out of the vector of slices, because has_ad_or_constant_sources() only takes the
    // set.
    ordered_set<PHV::AllocSlice> setOfSlices;
    for (auto& slice : slices)
        setOfSlices.insert(slice);

    // Merge actions for all candidate fields into a set.
    PHV::ActionSet allActionsForSlices;
    for (auto& slice : slices) {
        const auto& writingActions = constraint_tracker.written_in(slice);
        allActionsForSlices.insert(writingActions.begin(), writingActions.end());
        const auto it = initActions.find(slice.field());
        if (it != initActions.end()) {
            allActionsForSlices.insert(it->second.begin(), it->second.end());
        }
    }
    // For every action, check if bitmasked-set would be synthesized for the writes to slices.
    for (auto& action : allActionsForSlices) {
        bool has_ad_constant_sources = has_ad_or_constant_sources(setOfSlices, action, initActions);
        // Bitmasked-set instructions require an action data source.
        if (!has_ad_constant_sources)
            continue;
        // Determine the set of fields in the packing (slices) that are not written by action.
        ordered_set<PHV::AllocSlice> fieldsNotWrittenTo;
        for (const auto& slice : slices) {
            if (!constraint_tracker.is_written(slice, action) &&
                !(initActions.count(slice.field()) && initActions.at(slice.field()).count(action)))
                fieldsNotWrittenTo.insert(slice);
        }
        if (is_bitmasked_set(slices, fieldsNotWrittenTo))
            numBitmaskedSet++;
    }
    return numBitmaskedSet;
}

bool ActionPhvConstraints::is_bitmasked_set(
        const std::vector<PHV::AllocSlice>& container_state,
        const ordered_set<PHV::AllocSlice>& fields_not_written_to) const {
    bitvec written;
    for (auto& slice : container_state) {
        if (fields_not_written_to.count(slice))
            continue;
        auto container_range = slice.container_slice();
        bitvec writtenThisSlice(container_range.lo, container_range.size());
        written |= writtenThisSlice;
    }
    // Contiguity is enough because we don't currently support making action data rotationally
    // equivalent. If the bits written are contiguous, then this instruction is going to be realized
    // using deposit-field rather than bitmasked-set.
    if (written.is_contiguous())
        return false;
    return true;
}

bool ActionPhvConstraints::pack_conflicts_present(
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const std::vector<PHV::AllocSlice>& slices) const {
    // Check that none of the new slices have pack conflicts with the already allocated slices
    // (container_state).
    for (auto sl1 : container_state) {
        for (auto sl2 : slices) {
            if (sl1.field() == sl2.field()) continue;
            BUG_CHECK(sl1.isPhysicalStageBased() == sl2.isPhysicalStageBased(),
                      "mixed physical-stage-based and min-stage-based slices");
            if (sl1.isPhysicalStageBased() && sl2.isPhysicalStageBased()) {
                if (sl1.isLiveRangeDisjoint(sl2)) {
                    continue;
                }
            }
            PHV::FieldSlice fs1(sl1.field(), sl1.field_slice());
            PHV::FieldSlice fs2(sl2.field(), sl2.field_slice());
            if (hasPackConflict(fs1, fs2)) {
                LOG5("\t\t\t" << fs1 << " cannot be packed in the same stage with " << fs2);
                return true;
            }
        }
    }

    // Check no pack conflicts for fields not in a same byte.
    // Skip fields in the same byte, because no_pack is a soft constraints, when
    // fields are in a same byte, then it's impossible to allocate them without
    // violating this constraint.
    // For example, assume there is a pack conflict between f1 and f2.
    // For header [f1<1>, f2<3>, f3<12>]
    // Previous implementation allows nonSliceable supercluster to bypass the check, but does
    // not take same-byte fields into account.
    // So it allows [f1<1>, f2<3>, f3<12>[0:3]], [f3<12>[4:11]]
    // but disallow [f1<1>, f2<3>, f3<12>].
    // However, both of them will create the same pack conflict on f1 and f2.
    // This is one of the previous bug that make allocation be short of 8-bit containers.
    // we check the pack conflicts between slices within the candidate set.
    for (const auto& sl1 : slices) {
        for (const auto& sl2 : slices) {
            if (phv.must_alloc_same_container(
                    PHV::FieldSlice(sl1.field(), sl1.field_slice()),
                    PHV::FieldSlice(sl2.field(), sl2.field_slice()))) {
                continue;
            }
            auto* f1 = sl1.field();
            auto* f2 = sl2.field();
            if (f1 == f2) continue;
            if (hasPackConflict(PHV::FieldSlice(sl1.field(), sl1.field_slice()),
                PHV::FieldSlice(sl2.field(), sl2.field_slice()))) {
                LOG5("\t\t\tAllocation candidate " << sl1.field()->name
                                                   << " cannot be packed in "
                                                      "the same stage with "
                                                   << sl2.field()->name);
                return true;
            }
        }
    }
    return false;
}

bool ActionPhvConstraints::stateful_destinations_constraints_violated(
        const PHV::Allocation::MutuallyLiveSlices& container_state) const {
    BUG_CHECK(container_state.size() > 0, "No slices in candidate container allocation?");
    size_t size = static_cast<size_t>(container_state.begin()->container().size());
    LOG6("\t\t\tContainer size: " << size);
    const auto& statefulWrites = constraint_tracker.getStatefulWrites();
    ordered_map<const PHV::Field*,
                ordered_map<le_bitrange, ordered_map<const IR::MAU::Action*, int>>> fslice_to_rot;
    for (const auto& slice : container_state) {
        if (!statefulWrites.count(slice.field())) continue;
        for (auto kv : statefulWrites.at(slice.field())) {
            auto write_range = kv.first;
            LOG6("\t\t\tChecking range " << write_range);
            auto written_field_slice = slice.field_slice();
            if (!kv.first.contains(written_field_slice)) continue;
            for (auto limit : kv.second) {
                auto *act = limit.first;
                for (auto read_range : limit.second) {
                    if (write_range.lo != written_field_slice.lo || write_range.hi !=
                        written_field_slice.hi)
                        LOG6("\t\t\t  Range " << written_field_slice << " contained in [" <<
                             write_range.lo << ", " << write_range.hi << "] but not the same as "
                             "it.");
                    // Because of the way slicing is done, write_range will always be a superset of
                    // written_field_slice. Therefore, written_field_slice's lo will always be
                    // greater than or equal to write_range's left coordinate (lo) and
                    // written_field_slice's hi will always be lesser than or equal to
                    // write_range's right coordinate (hi).
                    LOG7("\t\t\t\tlo: " << read_range.first << ", hi: " << read_range.second);
                    if (write_range.lo != written_field_slice.lo) {
                        read_range.first -= (write_range.lo - written_field_slice.lo);
                        LOG7("\t\t\t\tlo factor: " << (written_field_slice.lo - write_range.lo));
                    }
                    if (write_range.hi != written_field_slice.hi) {
                        read_range.second -= (write_range.hi - written_field_slice.hi);
                        LOG7("\t\t\t\thi factor: " << (written_field_slice.hi - write_range.hi));
                    }
                    LOG7("\t\t\t\tlo: " << read_range.first << ", hi: " << read_range.second);
                    if (read_range.first / size == read_range.second / size) {
                        // Validate that all of the slice of the same field of the same container
                        // written by a stateful action can share the same rotation.
                        int cur_rot = read_range.first - slice.container_slice().lo;
                        if (fslice_to_rot.count(slice.field()) &&
                            fslice_to_rot[slice.field()].count(write_range) &&
                            fslice_to_rot[slice.field()][write_range].count(act)) {
                            int prev_rot = fslice_to_rot[slice.field()][write_range][act];
                            if (prev_rot != cur_rot) {
                                LOG5("\t\t\tThe rotation (" << cur_rot << ") of " << slice <<
                                     " is not equal to other rotation (" << prev_rot << ") carried"
                                     " in the same container. Thereforce, this packing is not"
                                     " possible.");
                                return true;
                            }
                        } else {
                            fslice_to_rot[slice.field()][write_range][act] = cur_rot;
                        }
                        continue;
                    }
                    LOG5("\t\t\tThe alignment of " << slice << " would force the data for ALU "
                         "operation to go to multiple action data bus slots. Therefore, this "
                         "packing is not possible.");
                    return true;
                }
            }
        }
    }
    return false;
}

bool ActionPhvConstraints::parser_constant_extract_satisfied(
        const PHV::Container& c,
        const PHV::Allocation::MutuallyLiveSlices& container_state) const {
    if (c.is(PHV::Size::b8)) return true;
    const auto& unionFind = phv.getSameSetConstantExtraction();
    for (auto& slice : container_state) {
        // For each slice, if it is not marked as parser constant extract, do nothing.
        if (!phv.hasParserConstantExtract(slice.field())) continue;
        bitvec range(slice.container_slice().lo, slice.width());
        const auto& sliceSet = unionFind.setOf(const_cast<PHV::Field*>(slice.field()));
        for (auto& slice1 : container_state) {
            if (slice == slice1) continue;
            // If these are different slices of the same field, then don't do anything.
            if (slice.field() == slice1.field()) continue;
            // if the other slice is not marked parser constant extract, then we don't need to do
            // anything.
            if (!phv.hasParserConstantExtract(slice1.field())) continue;
            // If the field is not in the same set as the parser constant extract field, then we are
            // good.
            if (sliceSet.find(const_cast<PHV::Field*>(slice1.field())) == sliceSet.end())
                continue;
            // Both slice and slice1's fields are in the same set. Now we need to make sure they can
            // be allocated into the same container.
            range |= bitvec(slice1.container_slice().lo, slice1.width());
            size_t count = 0;
            bool firstOneFound = false;
            size_t prevBit = 0;
            for (auto b : range) {
                if (!firstOneFound) {
                    // First bit. Just set prevBit and exit.
                    ++count;
                    prevBit = b;
                    firstOneFound = true;
                    continue;
                }
                // Later bit. Increase count by the difference between prevBit and the currentBit.
                count += (b - prevBit);
                // Set prevBit to current bit.
                prevBit = b;
            }
            // Need to pack within 4 consecutive bits for 16b fields and within 3 consecutive bits
            // for 32b fields.
            size_t maxAllowed = c.is(PHV::Size::b16) ? 4 : 3;
            if (count <= maxAllowed) continue;
            LOG5("\t\t\tIn container " << c << ", POV slices " << slice << " and " << slice1 <<
                 " are packed " << count << " (maxAllowed: " << maxAllowed << "b) bits apart." <<
                 " Disallowing this packing to save parser extractors.");
            return false;
        }
    }
    return true;
}

bool ActionPhvConstraints::check_and_generate_constraints_for_move_with_unallocated_sources(
    const PHV::Allocation& alloc, const IR::MAU::Action* action, const PHV::Container& c,
    const PHV::Allocation::MutuallyLiveSlices& container_state,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions, ActionContainerProperty* action_prop,
    PackingConstraints* copacking_constraints) const {
    // Special packing constraints are introduced when number of source containers > 2 and
    // number of allocated containers is less than or equal to 2.
    // At this point of the loop, sources.num_allocated <= 2, sources.num_unallocated may be any
    // value.
    const bool mocha_or_dark = c.is(PHV::Kind::dark) || c.is(PHV::Kind::mocha);
    const auto& sources = action_prop->sources;

    PHV::Allocation::MutuallyLiveSlices state_written_to;
    PHV::Allocation::MutuallyLiveSlices state_not_written_to;
    for (const auto& slice : container_state) {
        bool is_padding = slice.field()->padding || !uses.is_referenced(slice.field());
        if (constraint_tracker.is_written(slice, action))
            // If written by normal instruction.
            state_written_to.insert(slice);
        else if (initActions.count(slice.field()) && initActions.at(slice.field()).count(action))
            // If written by metadata initialization.
            state_written_to.insert(slice);
        else if (!is_padding)
            state_not_written_to.insert(slice);
    }
    size_t num_fields_not_written_to = state_not_written_to.size();
    bool has_bits_not_written_to = num_fields_not_written_to > 0;

    if (action_prop->use_ad) {
        // At this point, at least one PHV container is present, so we have both action
        // data/constant source as well as a PHV source.
        // Therefore, no fields can be unwritten in any given action.
        if (num_fields_not_written_to) {
            LOG5("\t\t\t\tSome bits not written in action " << action->name << " will get "
                    "clobbered because there is at least one PHV source and another action"
                    " data/ constant source");
            return false; }
        // Any unallocated PHV slices must all be packed within the same container, as there can
        // only be a maximum of one PHV source when an action data/constant source is present.
        // Generate these conditional constraints for this particular case.
        if (sources.num_unallocated > 0) {
            if (!masks_valid(container_state, action, true /*action data only*/, initActions)) {
                LOG5("\t\t\t\tThe action data used for this packing is not contiguous");
                return false; }
            if (!pack_slices_together(alloc, container_state, copacking_constraints, action,
                        false /* pack both allocated and unallocated together */,
                        true /* action data source present */))
                return false;
        }
        // At this point, analysis determines there is at least 1 PHV source. So
        // must_be_aligned for this action is true.
        LOG6("\t\t\t\t\tSetting must_be_aligned for action " << action->name << " to TRUE");
        action_prop->must_be_aligned = true;
    } else {
        if (action_prop->num_sources == 1) {
            // If both fields were not written to and the fields written to are not contiguous, then
            // there are too many holes in the deposit field operation and so, this packing is not
            // valid. Therefore, always check masks if the number of source containers is 1, and
            // there are fields not written to in this candidate packing. Note that if one of the
            // field sets is contiguous, then a valid deposit-field is possible.
            static PHV::Allocation::LiveRangeShrinkingMap emptyInit;
            if (has_bits_not_written_to) {
                // If the source container is not aligned with the container state being written,
                // then the written container state must be src1, as it would be shifted into the
                // right bit position within the container. This requires the written container
                // state to be contiguous as the mask is expressed as a contiguous [lo, hi] value
                // for the deposit-field. Therefore, for the unaligned case, we should only check
                // the mask validity for the container state written to.
                bool sourceContainerAlignedWithDest = is_aligned(state_written_to, action, alloc);
                bool writtenMaskValid = masks_valid(state_written_to, action,
                        false /* non action data only */, emptyInit);
                if (sourceContainerAlignedWithDest) {
                    if (!writtenMaskValid && !masks_valid(state_not_written_to, action,
                                false /*non action data only*/, emptyInit)) {
                        LOG5("\t\t\t\tMasks found to not be valid for packing");
                        return false;
                    }
                } else if (!writtenMaskValid) {
                    LOG5("\t\t\t\tSource container and destination container are not aligned. "
                         "Therefore, we require a contiguous mask for the container state written. "
                         "However, mask found to not be contiguous.");
                    return false;
                }
            }
            // If the allocation is to a mocha or dark container, and there is an unallocated
            // source, we need to make sure the source and the destination are aligned.
            if (mocha_or_dark && sources.num_unallocated > 0) {
                LOG5("\t\t\t\tSetting phv must be aligned for mocha/dark destination and "
                     "unallocated source");
                action_prop->must_be_aligned = true;
            }
            // No Action data or constant sources and only 1 PHV container as source plus masks for
            // deposit-field found to be valid. So, the packing is valid without any other induced
            // constraints.
            return true;
        }
    }
    bool requires_two_containers =
        sources.num_allocated == 2 || (sources.double_unallocated && sources.num_unallocated == 2);

    // If some field is not written to, then one of the sources for the move has to be the
    // container itself.
    // If this requires two containers, this packing is not possible (TOO_MANY_SOURCES)
    if (num_fields_not_written_to && requires_two_containers) {
        LOG5("\t\t\t\t" << num_fields_not_written_to << " fields not written in action "
                << action->name << " will get clobbered.");
        return false; }

    // If some bits in the container are not written to, then one of the sources of the move has
    // to be the container itself.
    // If requires two containers, this packing is not possible (TOO_MANY_SOURCES)
    if (has_bits_not_written_to && requires_two_containers) {
        LOG5("\t\t\t\tSome unallocated bits in the container will get clobbered by writes in "
                "action" << action->name);
        return false; }

    // One of the PHV must be aligned for the case with 2 sources
    LOG6("\t\t\t\t\tSetting phvMustBeAligned for action " << action->name << " to TRUE");
    action_prop->must_be_aligned = true;

    // If sources.num_allocated == 2 and sources.num_unallocated == 0, then packing is valid and
    // no other packing constraints are induced
    if (sources.num_allocated == 2 && sources.num_unallocated == 0)
        return true;

    // If sources.num_allocated == 2 and sources.num_unallocated > 0, then all unallocated
    // fields have to be packed together with one of the allocated fields
    // TODO: What's the best way to choose which allocated slice to pack with
    if (sources.num_allocated == 2 && sources.num_unallocated > 0)
        if (!pack_slices_together(alloc, container_state, copacking_constraints, action, false))
            return false;

    // For mocha and dark containers, partial container sets are impossible.
    if (mocha_or_dark && sources.num_unallocated > 0) {
        BUG_CHECK(sources.num_allocated <= 1, "Cannot have 2 or more sources for container %1%",
                c);
        // Pack all slices together.
        if (!pack_slices_together(alloc, container_state, copacking_constraints, action, false))
            return false;
    }

    // If sources.num_allocated == 1 and sources.num_unallocated > 0, then
    if (sources.num_allocated <= 1 && sources.num_unallocated > 0) {
        if (num_fields_not_written_to || has_bits_not_written_to) {
            // Pack all slices together (both allocated and unallocated)
            // Can only have src2 as src1 is always the destination container itself
            if (!pack_slices_together(alloc, container_state, copacking_constraints, action, false))
                return false;
            LOG6("\t\t\t\t\tMust pack unallocated fields with allocated fields."
                    " Setting source containers to 1.");
            if (!masks_valid(container_state, action)) {
                LOG5("\t\t\t\tCannot synthesize deposit-field instruction.");
                return false;
            }
            action_prop->num_sources = 1;
        } else {
            // For this case, sources need not be packed together as we may have (at most) 2
            // source containers
            if (action_prop->num_sources == 2) return true;
            // Only pack unallocated slices together
            if (!pack_slices_together(alloc, container_state, copacking_constraints, action, true))
                return false;
        }
    }
    return true;
}

bool ActionPhvConstraints::generate_conditional_constraints_for_bitwise_op(
    const PHV::Allocation::MutuallyLiveSlices& container_state, const IR::MAU::Action* action,
    const ordered_set<PHV::FieldSlice>& sources,
    PackingConstraints* copacking_constraints) const {
    if (sources.size() == 0) return true;
    bool firstSliceProcessed = false;
    bool destSameAsSource = false;
    for (auto& slice : container_state) {
        PHV::FieldSlice sl(slice.field(), slice.field_slice());
        bool sameAsSource = sources.count(sl);
        if (!firstSliceProcessed) {
            destSameAsSource = sameAsSource;
            firstSliceProcessed = true;
        } else {
            if (sameAsSource != destSameAsSource) {
                LOG6("\t\t\t\tCannot generate conditional constraints for bitwise operations.");
                return false;
            }
        }
    }
    if (destSameAsSource) {
        LOG6(
            "\t\t\t\tDo not need to generate conditional constraints for bitwise operations, "
            "as the sources are the same as destination.");
        return true;
    }
    if (copacking_constraints == nullptr) return true;
    LOG6("\t\t\t\t\tPrinting source set for bitwise operation");
    for (auto& slice : sources) {
        LOG6("\t\t\t\t\t  " << slice);
        for (auto& slice1 : sources) {
            if (slice == slice1) continue;
            LOG6("\t\t\t\t\t\tInserting " << slice1 << " into copacking_constraints for " << slice);
            copacking_constraints->at(action).makeUnion(slice, slice1);
        }
    }
    return true;
}

bool ActionPhvConstraints::check_and_generate_constraints_for_bitwise_op_with_unallocated_sources(
    const IR::MAU::Action* action, const PHV::Allocation::MutuallyLiveSlices& container_state,
    const NumContainers& sources, PackingConstraints* copacking_constraints) const {
    // No unallocated sources, so no need to generate any conditional constraints.
    bool rv = true;
    if (sources.num_unallocated == 0) return rv;
    ordered_set<PHV::FieldSlice> source1;
    ordered_set<PHV::FieldSlice> source2;
    for (auto& slice : container_state) {
        // Get all the sources for a given slice in this action.
        auto sourceSlices = constraint_tracker.sources(slice, action);
        unsigned operandNumber = 0;
        for (auto operand : sourceSlices) {
            // Depending on whether this is the first set or the second set, choose the appropriate
            // set.
            ordered_set<PHV::FieldSlice>* copacking_set = (operandNumber == 0) ? &source1 :
                &source2;
            ++operandNumber;
            if (operand.ad || operand.constant) continue;
            const PHV::Field* fieldRead = operand.phv_used->field();
            le_bitrange rangeRead = operand.phv_used->range();
            copacking_set->insert(PHV::FieldSlice(fieldRead, rangeRead));
        }
    }
    rv &= generate_conditional_constraints_for_bitwise_op(container_state, action, source1,
            copacking_constraints);
    rv &= generate_conditional_constraints_for_bitwise_op(container_state, action, source2,
            copacking_constraints);
    return rv;
}

PHV::Allocation::MutuallyLiveSlices make_mutually_live_slices(
    const std::vector<PHV::AllocSlice>& slices,
    const PHV::Allocation::MutuallyLiveSlices& original_container_state) {

    PHV::Allocation::MutuallyLiveSlices container_state;
    LOG4("\t\tOriginal existing container state: " <<
         (original_container_state.size() ? "" : "{}"));
    for (const auto& slice : original_container_state) {
        const auto IsDisjoint = [&](const PHV::AllocSlice& alloc_slice) {
            return slice.isLiveRangeDisjoint(alloc_slice);
        };
        LOG4("\t\t\t" << slice);
        if (std::all_of(slices.begin(), slices.end(), IsDisjoint)) {
            LOG4(
                "\t\t\t  Ignoring original slice because its live range is disjoint with all "
                "candidate slices.");
        } else {
            container_state.insert(slice);
        }
    }
    return container_state;
}

bool ActionPhvConstraints::check_speciality_read_and_bitmask(
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    const auto WriteFromSpeciality = [&](const PHV::AllocSlice& slice) {
        return special_no_pack.count(slice.field());
    };
    if (std::any_of(container_state.begin(), container_state.end(), WriteFromSpeciality)) {
        std::vector<PHV::AllocSlice> existingVector;
        for (auto& sl : container_state) existingVector.push_back(sl);
        if (count_bitmasked_set_instructions(existingVector, initActions) != 0) {
            return true;
        }
    }
    return false;
}

PHV::ActionSet ActionPhvConstraints::make_writing_action_set(
    const PHV::Allocation& alloc, const PHV::Allocation::MutuallyLiveSlices& container_state,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    PHV::ActionSet set_of_actions;
    for (const auto& slice : container_state) {
        const auto& writing_actions = constraint_tracker.written_in(slice);
        set_of_actions.insert(writing_actions.begin(), writing_actions.end());
        // Add metaInit actions of this candidate set.
        if (initActions.count(slice.field())) {
            for (const auto* a : initActions.at(slice.field())) {
                set_of_actions.insert(a);
            }
        }
        // add darkInit actions of this candidate set and previously allocated slices.
        const auto& dark_prim = slice.getInitPrimitive();
        if (!dark_prim.isAlwaysRunActionPrim()) {
            for (const auto* action : dark_prim.getInitPoints()) {
                set_of_actions.insert(action);
            }
        }
        //// TODO: not sure whether the two below were duplicated, keep them
        //// both here to be defensive.
        // add metaInit actions of this candidate set and previously allocated slices.
        for (const auto* dark_init : slice.getInitPoints()) {
            set_of_actions.insert(dark_init);
        }
        // add metaInit actions added before this allocation.
        if (const auto init_actions = alloc.getInitPoints(slice)) {
            set_of_actions.insert(init_actions->begin(), init_actions->end());
        }
    }
    return set_of_actions;
}

/// generate action container properties for @p actions.
ActionPhvConstraints::ActionPropertyMap ActionPhvConstraints::make_action_container_properties(
    const PHV::Allocation& alloc, const PHV::ActionSet& actions,
    const PHV::Allocation::MutuallyLiveSlices& container_state,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions,
    bool is_mocha_or_dark) const {
    ordered_map<const IR::MAU::Action*, ActionPhvConstraints::ActionContainerProperty> rst;
    for (const auto* action : actions) {
        NumContainers sources =
            num_container_sources(alloc, container_state, action);

        ActionContainerProperty prop;
        prop.op_type = container_operation_type(action, container_state, initActions);
        // Is there any action data or constant source for the proposed packing in this action?
        prop.use_ad = has_ad_or_constant_sources(container_state, action, initActions);
        prop.sources = sources;
        prop.num_sources = sources.num_allocated + sources.num_unallocated;
        prop.num_unallocated = sources.num_unallocated;
        prop.must_be_aligned = is_mocha_or_dark;
        rst[action] = prop;
    }
    return rst;
}

ActionPhvConstraints::PackingConstraints ActionPhvConstraints::make_initial_copack_constraints(
    const PHV::ActionSet& actions,
    const PHV::Allocation::MutuallyLiveSlices& container_state) const {
    ActionPhvConstraints::PackingConstraints packing_constraints;
    for (const auto* action : actions) {
        for (const auto& slice : container_state) {
            auto reads = constraint_tracker.sources(slice, action);
            for (auto operand : reads) {
                if (operand.ad || operand.constant || !operand.phv_used) {
                    LOG6("\t\t\t\t" << operand << " doesn't count as a container source");
                    continue;
                }

                // update copacking constraint
                const PHV::FieldSlice& source = *(operand.phv_used);
                if (!packing_constraints[action].contains(source)) {
                    LOG5("\t\t\t\t\tInserting "
                         << source << " into copacking_constraints for action " << action->name);
                    packing_constraints[action].insert(source);
                }
            }
        }
    }
    return packing_constraints;
}

CanPackErrorV2 ActionPhvConstraints::check_bitwise_and_basic_move_constraints(
    const PHV::ActionSet& actions,
    const PHV::Allocation::MutuallyLiveSlices& container_state,
    const ActionPropertyMap* action_props) const {
    for (const auto* action : actions) {
        LOG5("\t\t\tNeed to check container sources now for action " << action->name);
        const int stage = *stages(action, container_state.front().isPhysicalStageBased()).begin();
        PHV::Allocation::MutuallyLiveSlices container_state_of_action;
        for (const auto& slice : container_state) {
            if (slice.isLiveAt(stage, PHV::FieldUse(PHV::FieldUse::WRITE))) {
                container_state_of_action.insert(slice);
            }
        }
        auto& prop = action_props->at(action);
        const auto& op_type = prop.op_type;
        if (op_type == OperandInfo::WHOLE_CONTAINER || op_type == OperandInfo::MIXED)
            return {CanPackErrorCode::MIXED_OPERAND,
                    "mixed operation type in action " + action->externalName()};

        if (op_type == OperandInfo::WHOLE_CONTAINER_SAME_FIELD) {
            if (!are_adjacent_field_slices(container_state_of_action)) {
                return CanPackErrorV2{CanPackErrorCode::NONE_ADJACENT_FIELD};
            } else {
                LOG5("\t\t\t\tMultiple slices involved in whole container operation are adjacent");
            }
        }

        if (op_type == OperandInfo::BITWISE) LOG5("\t\t\t\tDetected bitwise operations");

        // If there is an action data or constant source, are all slices in the proposed packing
        // written using action data or constant sources.
        const auto all_or_none_ad_constant_sources =
            prop.use_ad ? all_or_none_constant_sources(container_state_of_action, action, {})
                        : NO_AD_CONSTANT;

        // If the action requires combining a speciality action data with a non-speciality action
        // data, we return false because the compiler currently does not support such packing.
        if (all_or_none_ad_constant_sources == COMPLEX_AD_PACKING_REQ)
            return CanPackErrorV2{CanPackErrorCode::COMPLEX_AD_PACKING};

        // If the action involves a bitwise operation for the proposed packing in container c, and
        // only some of the field slices are written using action data or constant sources, then
        // this packing is not valid.
        if (op_type == OperandInfo::BITWISE && !all_or_none_ad_constant_sources)
            return CanPackErrorV2{CanPackErrorCode::BITWISE_MIXED_AD};

        // If no PHV containers, then packing is valid
        if (prop.num_sources == 0) continue;

        // If source fields have already been allocated and number of sources greater than 2, then
        // packing is not possible (TOO_MANY_SOURCES)
        if (prop.sources.num_allocated > 2) {
            LOG5("\t\t\t\tAction " << action->name << " uses more than two PHV sources.");
            return CanPackErrorV2{CanPackErrorCode::MORE_THAN_TWO_SOURCES};
        }

        // num_source_containers == 2 if execution gets here
        // If source fields have already been allocated and there are two PHV sources in addition to
        // an action data/constant source then packing is not possible (TOO_MANY_SOURCES)
        if (prop.sources.num_allocated == 2 && prop.use_ad) {
            LOG5("\t\t\t\tAction " << action->name
                                   << " uses action data/constant in addition to "
                                      "two PHV sources");
            return CanPackErrorV2{CanPackErrorCode::TWO_SOURCES_AND_CONSTANT};
        }

        // Check the validity of packing for move operations, and generate intermediate structures
        // that will be used to create conditional constraints.
        if (op_type == OperandInfo::MOVE) {
            // solver will handle all move-based checks.
            continue;
        } else if (op_type == OperandInfo::BITWISE) {
            // Check the validity of bitwise operations and generate intermediate structures that
            // will be used to create conditional constraints.
            // At this point, we have already checked (in container_type_operation) that every
            // single slice in the proposed packing has already been written by the same bitwise
            // operation (for this action).
            if (!check_and_generate_constraints_for_bitwise_op_with_unallocated_sources(
                        action, container_state, prop.sources, nullptr))
                return CanPackErrorV2{CanPackErrorCode::BITWISE_AND_UNALLOCATED_SOURCE};
        } else if (op_type != OperandInfo::WHOLE_CONTAINER_SAME_FIELD &&
                   op_type != OperandInfo::PART_OF_CONTAINER) {
            BUG("Operation type other than BITWISE and MOVE encountered.");
        }
    }
    return CanPackErrorV2{CanPackErrorCode::NO_ERROR};
}

CanPackErrorCode ActionPhvConstraints::check_and_generate_constraints_for_bitwise_or_move(
    const PHV::Allocation& alloc, const PHV::ActionSet& actions,
    const PHV::Allocation::MutuallyLiveSlices& container_state, const PHV::Container& c,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions, ActionPropertyMap* action_props,
    PackingConstraints* copack_constraints) const {
    if (LOGGING(5)) {
        std::stringstream ss;
        ss << "\t\t\tMust check " << actions.size() << " actions: ";
        for (auto* act : actions) ss << act->name << " ";
        LOG5(ss.str());
    }

    const bool mocha_or_dark = c.is(PHV::Kind::dark) || c.is(PHV::Kind::mocha);
    for (const auto* action : actions) {
        LOG5("\t\t\tNeed to check container sources now for action " << action->name);
        const int stage = *stages(action, container_state.front().isPhysicalStageBased()).begin();
        PHV::Allocation::MutuallyLiveSlices container_state_of_action;
        for (const auto& slice : container_state) {
            if (slice.isLiveAt(stage, PHV::FieldUse(PHV::FieldUse::WRITE))) {
                container_state_of_action.insert(slice);
            }
        }
        auto& prop = action_props->at(action);
        const auto& op_type = prop.op_type;
        if (op_type == OperandInfo::WHOLE_CONTAINER || op_type == OperandInfo::MIXED)
            return CanPackErrorCode::MIXED_OPERAND;

        if (op_type == OperandInfo::WHOLE_CONTAINER_SAME_FIELD) {
            if (!are_adjacent_field_slices(container_state_of_action)) {
                return CanPackErrorCode::NONE_ADJACENT_FIELD;
            } else {
                LOG5("\t\t\t\tMultiple slices involved in whole container operation are adjacent");
            }
        }

        if (op_type == OperandInfo::BITWISE) LOG5("\t\t\t\tDetected bitwise operations");

        // If there is an action data or constant source, are all slices in the proposed packing
        // written using action data or constant sources.
        auto all_or_none_ad_constant_sources =
            prop.use_ad
                ? all_or_none_constant_sources(container_state_of_action, action, initActions)
                : NO_AD_CONSTANT;

        // If the action requires combining a speciality action data with a non-speciality action
        // data, we return false because the compiler currently does not support such packing.
        if (all_or_none_ad_constant_sources == COMPLEX_AD_PACKING_REQ)
            return CanPackErrorCode::COMPLEX_AD_PACKING;

        // If the action involves a bitwise operation for the proposed packing in container c, and
        // only some of the field slices are written using action data or constant sources, then
        // this packing is not valid.
        if (op_type == OperandInfo::BITWISE && !all_or_none_ad_constant_sources)
            return CanPackErrorCode::BITWISE_MIXED_AD;

        // If no PHV containers, then packing is valid
        if (prop.num_sources == 0) continue;

        // Dark and mocha containers require the entire container to be written all at once. For
        // dark and mocha containers, ensure that all the field slices in the container are written
        // in every action that writes one of those fields.
        if (mocha_or_dark) {
            // Only one container source for dark/mocha.
            if (prop.sources.num_allocated > 1)
                return CanPackErrorCode::TF2_MORE_THAN_ONE_SOURCE;
            if (!all_field_slices_written_together(container_state, actions, initActions))
                return CanPackErrorCode::TF2_ALL_WRITTEN_TOGETHER;
        }

        // If source fields have already been allocated and number of sources greater than 2, then
        // packing is not possible (TOO_MANY_SOURCES)
        if (prop.sources.num_allocated > 2) {
            LOG5("\t\t\t\tAction " << action->name << " uses more than two PHV sources.");
            return CanPackErrorCode::MORE_THAN_TWO_SOURCES;
        }

        // num_source_containers == 2 if execution gets here
        // If source fields have already been allocated and there are two PHV sources in addition to
        // an action data/constant source then packing is not possible (TOO_MANY_SOURCES)
        if (prop.sources.num_allocated == 2 && prop.use_ad) {
            LOG5("\t\t\t\tAction " << action->name
                                   << " uses action data/constant in addition to "
                                      "two PHV sources");
            return CanPackErrorCode::TWO_SOURCES_AND_CONSTANT;
        }

        // Check the validity of packing for move operations, and generate intermediate structures
        // that will be used to create conditional constraints.
        if (op_type == OperandInfo::MOVE) {
            if (!check_and_generate_constraints_for_move_with_unallocated_sources(
                    alloc, action, c, container_state, initActions, &prop, copack_constraints))
                return CanPackErrorCode::MOVE_AND_UNALLOCATED_SOURCE;
        } else if (op_type == OperandInfo::BITWISE) {
            // Check the validity of bitwise operations and generate intermediate structures that
            // will be used to create conditional constraints.
            // At this point, we have already checked (in container_type_operation) that every
            // single slice in the proposed packing has already been written by the same bitwise
            // operation (for this action).
            if (!check_and_generate_constraints_for_bitwise_op_with_unallocated_sources(
                    action, container_state, prop.sources, copack_constraints))
                return CanPackErrorCode::BITWISE_AND_UNALLOCATED_SOURCE;
        } else if (op_type != OperandInfo::WHOLE_CONTAINER_SAME_FIELD
                   && op_type != OperandInfo::PART_OF_CONTAINER) {
            BUG("Operation type other than BITWISE and MOVE encountered.");
        }
    }
    return CanPackErrorCode::NO_ERROR;
}

CanPackErrorCode ActionPhvConstraints::check_and_generate_rotational_alignment_constraints(
    const PHV::Allocation& alloc,
    const std::vector<PHV::AllocSlice>& slices,
    const PHV::ActionSet& actions,
    const PHV::Allocation::MutuallyLiveSlices& container_state,
    const PHV::Container& c,
    ActionPropertyMap* action_props) const {
    const bool mocha_or_dark = c.is(PHV::Kind::dark) || c.is(PHV::Kind::mocha);
    for (const auto& action : actions) {
        const int stage = *stages(action, container_state.front().isPhysicalStageBased()).begin();
        auto& prop = action_props->at(action);
        LOG5("\t\t\tphvMustBeAligned: " << prop.must_be_aligned << " numSourceContainers: "
                                        << prop.num_sources << " action: " << action->name);

        // If there is no alignment restriction on PHV source, we just need to ensure that the
        // different slices in the source PHV must be aligned at the same offset
        // with respect to the destination.
        if (!prop.must_be_aligned) {
            if (prop.num_sources == 1) {
                if (are_adjacent_field_slices(container_state)) {
                    // If all fields are adjacent slices of the same field, check if all the
                    // sources are adjacent slices of the same field as well
                    ordered_set<PHV::AllocSlice> sources;
                    for (const auto& slice : container_state) {
                        std::optional<PHV::AllocSlice> source =
                            getSourcePHVSlice(alloc, slices, slice, action, stage);
                        if (!source) continue;
                        if (!sources.count(*source)) sources.insert(*source);
                    }
                    // TODO: bug here, it should be are_adjacent_field_slices(sources).
                    if (are_adjacent_field_slices(container_state)) {
                        continue;
                    }
                }
                bool firstSlice = true;
                int firstOffset = 0;
                for (const auto& slice : container_state) {
                    std::optional<PHV::AllocSlice> source =
                        getSourcePHVSlice(alloc, slices, slice, action, stage);
                    if (!source) continue;
                    int offset = getOffset(slice.container_slice(), source->container_slice(),
                                           slice.container());
                    LOG5("\t\t\t\t\tOffset found: " << offset);
                    if (firstSlice) {
                        firstOffset = offset;
                        firstSlice = false;
                    } else {
                        if (offset != firstOffset) {
                            LOG5(
                                "\t\t\t\tSource slices are at different offsets with respect to "
                                "destination slices");
                            return CanPackErrorCode::SLICE_DIFF_OFFSET;
                        }
                    }
                }
            }
            continue;
        }
        // when there is aligment constraint on PHV sources.
        prop.aligned_one_unallocated = false;
        prop.aligned_two_unallocated = false;
        prop.aligned_unallocated_requires_aligment = false;
        if (prop.num_sources == 1) {
            // The single phv source must be aligned
            for (const auto& slice : container_state) {
                LOG5("\t\t\t\tslice: " << slice);
                std::optional<PHV::AllocSlice> source =
                    getSourcePHVSlice(alloc, slices, slice, action, stage);
                if (!source) {
                    if (prop.num_unallocated == 1 && mocha_or_dark) {
                        prop.aligned_one_unallocated = true;
                        LOG6("\t\t\t\tFound one unallocated PHV source for mocha/dark destination");
                    }
                    // Detect case where we have an unallocated PHV source and action data/constant
                    // writing to the same container in the same action. For such a case, enforce
                    // alignment constraints for the unallocated PHV source later (guarded using
                    // aligned_one_unallocated variable).
                    if (container_state.size() > 1 && prop.use_ad && prop.num_unallocated == 1) {
                        prop.aligned_one_unallocated = true;
                        LOG6("\t\t\t\tFound one unallocated PHV source");
                    }
                } else if (slice.container_slice() != source->container_slice()) {
                    LOG5("container slice " << slice << " source " << *source);
                    LOG5("\t\t\t\tContainer alignment for slice and source do not match");
                    return CanPackErrorCode::SLICE_ALIGNMENT;
                }
            }
        } else if (prop.num_sources == 2) {
            // TODO: If must_be_aligned and one of the fields to be
            // packed is in the UnionFind data structure (i.e. is a source), then
            // fail: It's impossible for a source to be aligned and also packed in
            // the same container as its destination (unless it's the same field).
            std::optional<ClassifiedSources> classifiedSources =
                verify_two_container_alignment(alloc, container_state, action, c, &prop);
            if (!classifiedSources)
                return CanPackErrorCode::PACK_AND_ALIGNED;
            if (LOGGING(5)) {
                LOG5("\t\t\t\tFirst container source: " << classifiedSources.value()[0]);
                LOG5("\t\t\t\tSecond container source: " << classifiedSources.value()[1]);
            }
            if (!masks_valid(classifiedSources.value()))
                return CanPackErrorCode::INVALID_MASK;
            if (prop.num_unallocated == 2) {
                prop.aligned_two_unallocated = true;
            } else if (prop.num_unallocated == 1) {
                LOG5("\t\t\t\t  Detected one unallocated source.");
                prop.aligned_one_unallocated = true;
            }
        }
    }
    return CanPackErrorCode::NO_ERROR;
}

CanPackErrorCode ActionPhvConstraints::check_and_generate_copack_set(
    const PHV::Allocation& alloc,
    const PHV::Allocation::MutuallyLiveSlices& container_state,
    const PackingConstraints& copack_constraints,
    const ActionPropertyMap& action_props,
    ordered_map<int, ordered_set<PHV::FieldSlice>>* copacking_set,
    ordered_map<PHV::FieldSlice, PHV::Container>* req_container) const {
    int setIndex = 0;
    for (const auto& kv : copack_constraints) {
        LOG5("\t\t\t  Enforcing copacking constraint for action " << kv.first->name);
        const auto& prop = action_props.at(kv.first);
        for (auto& set : kv.second) {
            // Need to enforce alignment constraints when we have one unallocated PHV source and
            // action data writing to the container in the same action.
            if (set.size() < 2 && (prop.num_sources == 2 || !prop.aligned_one_unallocated))
                continue;
            // If some sources in the same set in copacking_constraints are already allocated, set
            // the container requirement for the unallocated sources in that set.
            if (!assign_containers_to_unallocated_sources(alloc, kv.second, *req_container)) {
                LOG5(
                    "\t\t\t\tMultiple slices that must go into the same container are allocated "
                    "to different containers");
                return CanPackErrorCode::COPACK_UNSATISFIED;
            }
            ordered_set<PHV::FieldSlice> setFieldSlices;
            setFieldSlices.insert(set.begin(), set.end());
            (*copacking_set)[setIndex++] = setFieldSlices;
        }
    }

    // If copacking_constraints has exactly two unallocated PHV source slices, force the smaller
    // slice to be aligned with its destination.
    for (const auto& kv : copack_constraints) {
        const auto& prop = action_props.at(kv.first);
        if (prop.aligned_two_unallocated) {
            LOG5("Detected two unallocated PHV sources for action " << kv.first->name);
            ordered_set<PHV::FieldSlice> field_slices_in_current_container;
            for (auto& sl : container_state) {
                field_slices_in_current_container.insert(
                    PHV::FieldSlice(sl.field(), sl.field_slice()));
                LOG5("\t\t\t\tAdding " << sl << " to field slices in current container");
            }
            std::optional<PHV::FieldSlice> alignedSlice =
                get_smaller_source_slice(alloc, kv.second, field_slices_in_current_container);
            if (alignedSlice) {
                bool foundAlignmentConstraint = false;
                for (auto& set : kv.second) {
                    if (set.size() > 1 && set.count(*alignedSlice)) {
                        foundAlignmentConstraint = true;
                        LOG5("\t\t\t\t  Alignment constraint already found on smaller slice: "
                             << *alignedSlice);
                    }
                }
                if (!foundAlignmentConstraint) {
                    (*copacking_set)[setIndex++] = {*alignedSlice};
                    LOG5("\t\t\t\tEnforcing alignment constraint on smaller slice: "
                         << *alignedSlice);
                }
            } else {
                LOG5("\t\t\t\tSmaller slice not found");
            }
        } else if (prop.num_sources == 2 && prop.aligned_one_unallocated) {
            LOG5("\t\t\t\tNeed to handle the case here.");
            ordered_set<PHV::FieldSlice> field_slices_in_current_container;
            for (auto& sl : container_state) {
                field_slices_in_current_container.insert(
                    PHV::FieldSlice(sl.field(), sl.field_slice()));
                LOG5("\t\t\t\tAdding " << sl << " to field slices in current container");
            }
            auto unallocatedSlice =
                get_unallocated_slice(alloc, kv.second, field_slices_in_current_container);
            if (!unallocatedSlice) {
                LOG5("\t\t\t\tUnallocated slice not found");
            } else {
                LOG5("\t\t\t\tUnallocated slice: " << *unallocatedSlice);
                if (prop.aligned_unallocated_requires_aligment) {
                    LOG5("\t\t\t\t  This unallocated slice must be aligned.");
                    (*copacking_set)[setIndex++] = {*unallocatedSlice};
                    LOG5("\t\t\t\tEnforcing alignment constraint on unallocated slice: "
                         << *unallocatedSlice);
                }
            }
        }
    }
    return CanPackErrorCode::NO_ERROR;
}

CanPackReturnType ActionPhvConstraints::check_and_generate_conditional_constraints(
    const PHV::Allocation& alloc, const std::vector<PHV::AllocSlice>& slices,
    // const ordered_set<const IR::MAU::Action*>& actions,
    const PHV::Allocation::MutuallyLiveSlices& container_state,
    const ordered_map<int, ordered_set<PHV::FieldSlice>>& copacking_set,
    const ordered_map<PHV::FieldSlice, PHV::Container>& req_container) const {
    PHV::Allocation::ConditionalConstraints rv;
    // Find the right alignment for each slice in the copacking constraints.
    for (auto kv_unallocated : copacking_set) {
        PHV::Allocation::ConditionalConstraint per_unallocated_source;
        // All fields in rv must be placed in the same container.  If there are
        // overlaps based on required alignment, return std::nullopt.
        ordered_map<const PHV::FieldSlice, le_bitrange> placements;

        for (auto& packing_slice : kv_unallocated.second) {
            ordered_set<int> req_alignment;
            for (auto& slice : container_state) {
                auto sources = constraint_tracker.source_alignment(slice, packing_slice);
                req_alignment |= sources;
            }

            // PROBLEM: the cross product of slices and copacking_set loses which
            // slices correspond to which copacking.

            // Conservatively reject this packing if an operand is required to be aligned at two
            // different positions.
            // TODO: Possible optimization could be that allocating some other field
            // differently would resolve the multiple requirements for this field's alignment.
            int bitPosition = -1;
            if (req_alignment.size() > 1) {
                auto boost_bitpos = constraint_tracker.can_be_both_sources(
                    slices, kv_unallocated.second, packing_slice);
                if (boost_bitpos == std::nullopt) {
                    LOG5("\t\t\tPacking failed because "
                         << packing_slice
                         << " would (conservatively) need to be aligned at more than one position: "
                         << P4::cstring::to_cstring(req_alignment));
                    return std::make_tuple(CanPackErrorCode::MULTIPLE_ALIGNMENTS, std::nullopt);
                }
                bitPosition = *boost_bitpos;
            } else if (req_alignment.size() == 1) {
                bitPosition = *(req_alignment.begin());
            } else {
                // Alignment requirements could be empty in case the source slices are also
                // unallocated or due to action data/constant writes.
                continue;
            }

            // Check that no other slices are also required to be at this bit
            // position, unless they're mutually exclusive and can be overlaid.
            for (auto& kv : placements) {
                bool isMutex = phv.field_mutex()(kv.first.field()->id, packing_slice.field()->id);
                if (kv.second.overlaps(StartLen(bitPosition, packing_slice.size())) && !isMutex) {
                    LOG5("\t\t\tPacking failed because " << packing_slice << " and " << kv.first
                                                         << " slice would (conservatively) need to "
                                                            "be aligned at the same position in "
                                                            "the same container.");
                    return std::make_tuple(CanPackErrorCode::OVERLAPPING_SLICES, std::nullopt);
                }
            }

            // If a slice that is part of the conditional constraints is already allocated, we do
            // not need to actually add the allocated slice to the conditional constraints list.
            // This is because the constraint related to the allocation of the slice to the relevant
            // container is already captured in the container parameter of the unallocated slices
            // within the conditional constraint data.
            if (alloc.slices(packing_slice.field(), packing_slice.range()).size() != 0) {
                LOG5("\t\t\tPacking slice " << packing_slice << " already been allocated.");
                continue;
            }

            placements[packing_slice] = StartLen(bitPosition, packing_slice.size());

            // Set the required bit position.
            if (req_container.count(packing_slice))
                per_unallocated_source[packing_slice] = PHV::Allocation::ConditionalConstraintData(
                        bitPosition, req_container.at(packing_slice));
            else
                per_unallocated_source[packing_slice] =
                    PHV::Allocation::ConditionalConstraintData(bitPosition);
        }

        // Make sure that the same set of conditional constraints have not been generated for any
        // other source.
        bool conditionalConstraintFound = false;
        for (auto kv1 : rv) {
            if (kv1.second.size() != per_unallocated_source.size()) continue;
            bool individualConditionMatch = true;
            for (auto& kv2 : per_unallocated_source) {
                // If slice not found, then this conditional source does not match.
                individualConditionMatch =
                    kv1.second.count(kv2.first) && (kv1.second.at(kv2.first) == kv2.second);
                if (!individualConditionMatch) break;
            }
            conditionalConstraintFound = individualConditionMatch;
            if (conditionalConstraintFound) break;
        }
        if (!conditionalConstraintFound) rv[kv_unallocated.first] = per_unallocated_source;
    }
    return std::make_tuple(CanPackErrorCode::NO_ERROR, rv);
}

CanPackReturnType ActionPhvConstraints::can_pack(
        const PHV::Allocation& alloc,
        const std::vector<PHV::AllocSlice>& slices,
        const PHV::Allocation::MutuallyLiveSlices& original_container_state,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    // Allocating zero slices always succeeds...
    if (slices.size() == 0)
        return std::make_tuple(CanPackErrorCode::NO_ERROR,
                               PHV::Allocation::ConditionalConstraints());

    // no action will be applied on TPHV container.
    const PHV::Container c = slices.front().container();
    if (c.type().kind() == PHV::Kind::tagalong) {
        return std::make_tuple(CanPackErrorCode::NO_ERROR,
                               PHV::Allocation::ConditionalConstraints());
    }

    if (LOGGING(6)) constraint_tracker.print_field_ordering(slices);

    const bool mocha_or_dark = c.is(PHV::Kind::dark) || c.is(PHV::Kind::mocha);

    // mutual live (against any of @p slices) slices of this container
    auto container_state = make_mutually_live_slices(slices, original_container_state);
    if (LOGGING(4)) {
        LOG4("\t\tChecking whether field slice(s) ");
        for (const auto& slice : slices)
            LOG4("\t\t\t" << slice.field()->name << " (" << slice.width() << "b)");
        LOG4("\t\tcan be packed into container " << container_state << " already containing "
                                                 << container_state.size() << " slices");
    }

    // Check if table placement induced any no pack constraints on fields that are candidates for
    // packing. If yes, packing not possible.
    if (pack_conflicts_present(container_state, slices))
        return std::make_tuple(CanPackErrorCode::PACK_CONSTRAINT_PRESENT, std::nullopt);

    // Create candidate packing
    for (const auto& slice : slices) {
        container_state.insert(slice);
    }

    // Check if any of the fields are stateful ALU writes and check the data bus alignment
    // constraints.
    if (stateful_destinations_constraints_violated(container_state))
        return std::make_tuple(CanPackErrorCode::STATEFUL_DEST_CONSTRAINT, std::nullopt);

#if 0
    // Check for parser constant extract for non 8b containers.
    if (Device::currentDevice() == Device::TOFINO)
       if (!parser_constant_extract_satisfied(c, container_state))
            return std::nullopt;
#endif

    // check speciality read and bitmask set not in same container.
    if (check_speciality_read_and_bitmask(container_state, initActions)) {
        LOG5(
            "\t\tThis packing requires a bitmasked-set instruction for a slice that reads "
            "special action data. Therefore, this packing is not possible.");
        return std::make_tuple(CanPackErrorCode::BITMASK_CONSTRAINT, std::nullopt);
    }

    // xxx(Deep): This function checks if any field that gets its value from METER_ALU, HASH_DIST,
    // RANDOM, or METER_COLOR is being packed with other fields written in the same action.
    if (!check_speciality_packing(container_state))
        return std::make_tuple(CanPackErrorCode::SPECIALTY_DATA, std::nullopt);

    const auto actions = make_writing_action_set(alloc, container_state, initActions);
    auto action_props = make_action_container_properties(alloc, actions, container_state,
                                                         initActions, mocha_or_dark);
    auto copack_constraints = make_initial_copack_constraints(actions, container_state);

    // update action_props and copack_constraints by analyzing bitwise or move instructions.
    CanPackErrorCode err = check_and_generate_constraints_for_bitwise_or_move(
        alloc, actions, container_state, c, initActions, &action_props, &copack_constraints);
    if (err != CanPackErrorCode::NO_ERROR) {
        return std::make_tuple(err, std::nullopt);
    }
    LOG4("after check bitwise_or_move copacking constraint " << copack_constraints.size());

    // TODO: we use solver to check move-based actions as a safe net to ensure
    // we don't create invalid packing.
    auto ara_move_err = check_ara_move_constraints(
            alloc, container_state, c, initActions);
    if (ara_move_err != CanPackErrorCode::NO_ERROR) {
        return std::make_tuple(ara_move_err, std::nullopt);
    }
    for (const auto* action : actions) {
        if (action_props.at(action).op_type != OperandInfo::MOVE) {
            LOG3("skip non-move action on container " << c << ": " << action->name);
            continue;
        }
        auto move_err = check_move_constraints(
                alloc, action, slices, container_state, c, initActions);
        if (!move_err.ok()) {
            return std::make_tuple(move_err.code, std::nullopt);
        }
        if (!mocha_or_dark && move_err.remove_align_req) {
            auto& prop = action_props.at(action);
            if ((prop.sources.num_allocated == 2) ||
                (prop.sources.double_unallocated && prop.sources.num_unallocated == 2)) {
                prop.must_be_aligned = false;
                LOG5("REMOVING must_be_aligned for action " << action->name);
                LOG5("\tnum_alloc: " << prop.sources.num_allocated << " double_unalloc: " <<
                     prop.sources.double_unallocated << " num_unalloc: " <<
                     prop.sources.num_unallocated);
            }
        }
    }
    // check from source side.
    auto read_move_err = check_move_constraints_from_read(alloc, slices, initActions);
    if (!read_move_err.ok()) {
        return std::make_tuple(read_move_err.code, std::nullopt);
    }

    // depending on ^check_and_generate_constraints_for_bitwise_or_move.
    LOG5("\t\t\tChecking rotational alignment");
    err = check_and_generate_rotational_alignment_constraints(
            alloc, slices, actions, container_state, c, &action_props);
    if (err != CanPackErrorCode::NO_ERROR) {
        return std::make_tuple(err, std::nullopt);
    }

    // TODO: If there are conditional constraints---i.e. if these slices
    // can only be packed if some unallocated source operands are packed in the
    // right way---then compute a valid bit position for each source operand.
    // Note that this is overly conservative: It requires source operands to be
    // packed together and directly aligned (not rotationally aligned) with
    // their destinations.  This is what glass does, but we should try to relax
    // this constraint in the future.

    if (LOGGING(5) && copack_constraints.size() > 0) {
        LOG5("Printing copacking constraints");
        for (auto kv : copack_constraints) {
            LOG5("Action: " << kv.first->name);
            LOG5("\tFields: " << kv.second);
        }
    }

    /// check and generate conditional constraints.
    ordered_map<int, ordered_set<PHV::FieldSlice>> copacking_set;
    ordered_map<PHV::FieldSlice, PHV::Container> req_container;
    err = check_and_generate_copack_set(alloc, container_state, copack_constraints, action_props,
                                        &copacking_set, &req_container);
    if (err != CanPackErrorCode::NO_ERROR) {
        return std::make_tuple(err, std::nullopt);
    }

    if (LOGGING(5) && copacking_set.size() > 0) {
        LOG5("\t\t\tPrinting copacking_set");
        for (auto kv : copacking_set) {
            std::stringstream ss;
            ss << "\t\t\t  " << kv.first << " : ";
            for (auto field_slice : kv.second)
                ss << kv.second << " ";
            LOG5(ss.str());
        }
    }

    // final check and generate conditional constraints;
    return check_and_generate_conditional_constraints(
            alloc, slices, container_state, copacking_set, req_container);
}

CanPackErrorV2 ActionPhvConstraints::can_pack_v2(
        const PHV::Allocation& alloc,
        const std::vector<PHV::AllocSlice>& candidates) const {
    Log::TempIndent indent;
    LOG5("Can pack v2 with " << candidates.size() << " candidates " << indent);

    // Allocating zero slices always succeeds...
    if (candidates.size() == 0) return {CanPackErrorCode::NO_ERROR, ""};

    // no action will be applied on TPHV container.
    const PHV::Container c = candidates.front().container();
    if (c.type().kind() == PHV::Kind::tagalong) {
        return {CanPackErrorCode::NO_ERROR, ""};
    }

    /// TODO: checks inherited from legacy code does not have detailed error message.
    /// For example, pack_conflicts_present, stateful_destinations_constraints_violated and ...

    if (LOGGING(6)) constraint_tracker.print_field_ordering(candidates);

    ordered_set<PHV::AllocSlice> container_state =
        alloc.liverange_overlapped_slices(c, candidates);

    // Check if table placement induced any no pack constraints on fields that are candidates for
    // packing. If yes, packing not possible.
    if (pack_conflicts_present(container_state, candidates))
        return {CanPackErrorCode::PACK_CONSTRAINT_PRESENT, ""};
    LOG6("No pack conflicts present");

    // Create after-allocation container state.
    for (const auto& sl : candidates) {
        container_state.insert(sl);
    }

    // Check if any of the fields are stateful ALU writes and check the data bus alignment
    // constraints.
    if (stateful_destinations_constraints_violated(container_state))
        return {CanPackErrorCode::STATEFUL_DEST_CONSTRAINT, ""};
    LOG6("No stateful destinations constraints violated");

    // TODO: we should be able to remove this part by simply disable bit-masked-set
    // instruction if slice has speciality read.
    // check speciality read and bitmask set not in same container.
    if (check_speciality_read_and_bitmask(container_state, {})) {
        LOG5(
            "\t\tThis packing requires a bitmasked-set instruction for a slice that reads "
            "special action data. Therefore, this packing is not possible.");
        return {CanPackErrorCode::BITMASK_CONSTRAINT, ""};
    }
    LOG6("No packing violated for bitmask-set");

    // xxx(Deep): This function checks if any field that gets its value from METER_ALU, HASH_DIST,
    // RANDOM, or METER_COLOR is being packed with other fields written in the same action.
    if (!check_speciality_packing(container_state))
        return {CanPackErrorCode::SPECIALTY_DATA, ""};
    LOG6("No packing violated for speciality data");

    const bool mocha_or_dark = c.is(PHV::Kind::dark) || c.is(PHV::Kind::mocha);
    const auto actions = make_writing_action_set(alloc, container_state, {});
    auto action_props = make_action_container_properties(
            alloc, actions, container_state, {}, mocha_or_dark);

    // check basic constraints of bitwise and move.
    CanPackErrorV2 err =
        check_bitwise_and_basic_move_constraints(
                actions, container_state, &action_props);
    if (!err.ok()) {
        return err;
    }
    LOG6("No packing violated for bitwise move constraints");

    // we use solver to check move-based actions as a safe net to ensure
    // we don't create invalid packing.
    for (const auto* action : actions) {
        if (action_props.at(action).op_type != OperandInfo::MOVE) {
            LOG3("skip non-move action on container " << c << ": " << action->name);
            continue;
        }
        auto move_err = check_move_constraints(
                alloc, action, candidates, container_state, c, {});
        if (!move_err.ok()) {
            std::stringstream ss;
            ss << "In action " << action->externalName()
               << ", allocation candidate is used as instruction destination, move "
               << "instruction constraints will be violated: " << move_err.msg;
            move_err.msg = ss.str();
            return move_err;
        }
    }
    LOG6("No packing violated for action move constraints");

    // check from source side.
    auto read_move_err = check_move_constraints_from_read(alloc, candidates, {});
    if (!read_move_err.ok()) {
        return read_move_err;
    }
    LOG6("No packing violated for read move constraints");

    return CanPackErrorV2{CanPackErrorCode::NO_ERROR};
}


bool ActionPhvConstraints::creates_container_conflicts(
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions,
        const MapTablesToActions& tableActionsMap) const {
    if (container_state.size() == 0) return false;
    LOG5("\tPrinting container conflicts candidates");
    for (auto& slice : container_state)
        LOG5("\t  " << slice);
    ordered_map<const PHV::Field*, ordered_set<const IR::MAU::Table*>> field_to_table_writes;
    for (auto kv : initActions) {
        for (auto* action : kv.second) {
            auto t = tableActionsMap.getTableForAction(action);
            BUG_CHECK(t, "Action %1% does not have a table associated with it.", action->name);
            field_to_table_writes[kv.first].insert(*t);
        }
    }
    for (auto& slice : container_state) {
        auto actions = actions_writing_fields(slice);
        for (const auto* action : actions) {
            auto t = tableActionsMap.getTableForAction(action);
            BUG_CHECK(t, "Action %1% does not have a table associated with it.", action->name);
            field_to_table_writes[slice.field()].insert(*t);
        }
    }

    if (LOGGING(6)) {
        for (auto kv : field_to_table_writes) {
            std::stringstream ss;
            ss << "\t" << kv.first->name << " : ";
            for (const auto* t : kv.second)
                ss << t->name << " ";
            LOG6(ss.str());
        }
    }

    for (auto kv : field_to_table_writes) {
        for (auto kv1 : field_to_table_writes) {
            // If the same field, ignore.
            if (kv.first == kv1.first) continue;
            // If different fields, check their tables.
            for (const auto* t1 : kv.second) {
                for (const auto* t2 : kv1.second) {
                    if (conflicts.writtenInSameStageDifferentTable(t1, t2)) {
                        LOG5("\t\t  Fields " << kv.first->name << " and " << kv1.first->name <<
                             " are written in different tables in the same stage: "
                             << t1->name << " and " << t2->name << std::endl << "\t\t  " <<
                             "This would produce container conflicts.");
                        return true; } } } } }

    return false;
}

std::optional<PHV::FieldSlice> ActionPhvConstraints::get_unallocated_slice(
        const PHV::Allocation& alloc,
        const UnionFind<PHV::FieldSlice>& copacking_constraints,
        const ordered_set<PHV::FieldSlice>& container_state) const {
    // Get unallocated sources, excluding both allocated sources as well as slices that are being
    // packed in the container_State. Since those slices are in the contianer_state, they must be
    // assumed to be allocated already.
    ordered_set<PHV::FieldSlice> unallocatedSlices;
    LOG5("\t\t\t\tGathering unallocated source for this container:");
    for (auto& set : copacking_constraints) {
        for (auto& sl : set) {
            if (!container_state.count(sl) && alloc.slices(sl.field(), sl.range()).size() == 0) {
                LOG5("\t\t\t\t\tAdding " << sl);
                unallocatedSlices.insert(sl);
            }
        }
    }
    if (unallocatedSlices.size() != 1) {
        LOG5("\t\t\t\tFound more than one unallocated slice despite expecting only one such "
             "slice.");
        return std::nullopt;
    }
    return (*(unallocatedSlices.begin()));
}

std::optional<PHV::FieldSlice> ActionPhvConstraints::get_smaller_source_slice(
        const PHV::Allocation& alloc,
        const UnionFind<PHV::FieldSlice>& copacking_constraints,
        const ordered_set<PHV::FieldSlice>& container_state) const {
    // Get unallocated sources, excluding both allocated sources as well as slices that
    // are being packed in the container_state. Since those slices are in the container_state, they
    // are assumed to be allocated already.
    ordered_set<PHV::FieldSlice> twoUnallocatedSlices;
    LOG5("\t\t\t\tGathering unallocated sources for this container:");
    for (auto& set : copacking_constraints) {
        for (auto& sl : set) {
            if (!container_state.count(sl) && alloc.slices(sl.field(), sl.range()).size() == 0) {
                LOG5("\t\t\t\t\tAdding " << sl);
                twoUnallocatedSlices.insert(sl);
            } else {
                LOG5("\t\t\t\t\tIgnoring allocated slice " << sl); } } }

    if (twoUnallocatedSlices.size() > 2) {
        // If we find more than two unallocated slices, return std::nullopt.
        LOG5("\t\t\t\tFound more than two unallocated slices.");
        return std::nullopt; }

    auto min = std::min_element(twoUnallocatedSlices.begin(), twoUnallocatedSlices.end(),
            [&](const PHV::FieldSlice& sl1, const PHV::FieldSlice& sl2) {
        return sl1.size() < sl2.size(); });
    if (min == twoUnallocatedSlices.end())
        return std::nullopt;
    else
        return *min;
}

bool ActionPhvConstraints::masks_valid(const ClassifiedSources& sources) const {
    const int source0_holes = count_container_holes(sources[0].slices);
    const int source1_holes = count_container_holes(sources[1].slices);
    const int total_holes =  source1_holes + source0_holes;
    if (total_holes > 1) {
        LOG5("\t\t\t\tInvalid mask found: more than 1 holes");
        return false;
    }
    if ((!sources[0].aligned && source0_holes > 0) ||
        (!sources[1].aligned && source1_holes > 0)) {
        LOG5("\t\t\t\tInvalid mask found: non-aligned source with holes");
        return false;
    }
    return true;
}

bool ActionPhvConstraints::masks_valid(
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const IR::MAU::Action* action) const {
    bitvec mask;
    int c_size = -1;
    for (auto& slice : container_state) {
        if (c_size == -1) c_size = slice.container().size();
        if (constraint_tracker.is_written(slice, action))
            mask |= bitvec(slice.container_slice().lo, slice.width());
    }
    if (c_size == -1) return true;
    LOG5("\t\t\t\t  Required mask: " << mask);
    if (mask.is_contiguous()) return true;
    bitvec allOnes(0, c_size);
    bitvec complementMask = allOnes - mask;
    LOG5("\t\t\t\t  Complement mask: " << complementMask);
    if (complementMask.is_contiguous()) return true;
    return false;
}

bool ActionPhvConstraints::masks_valid(
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const IR::MAU::Action* action,
        bool actionDataOnly,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    bitvec actionDataConstantMask;
    for (const auto& slice : container_state) {
        if (actionDataOnly && has_ad_or_constant_sources({ slice }, action, initActions)) {
            LOG5("\t\t\t\t  Action data constant mask for " << slice << " : " <<
                    slice.container_slice().lo << ", " << slice.width());
            actionDataConstantMask |= bitvec(slice.container_slice().lo, slice.width());
        }
        if (!actionDataOnly) {
            LOG5("\t\t\t\t  Mask for " << slice << " : " << slice.container_slice().lo
                    << ", " << slice.width());
            actionDataConstantMask |= bitvec(slice.container_slice().lo, slice.width());
        }
    }
    LOG5("\t\t\t\tRequired mask : " << actionDataConstantMask);
    if (actionDataConstantMask.is_contiguous())
        return true;
    return false;
}

bool ActionPhvConstraints::is_aligned(
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const IR::MAU::Action* action,
        const PHV::Allocation& alloc) const {
    for (auto& slice : container_state) {
        auto reads = constraint_tracker.sources(slice, action);
        BUG_CHECK(reads.size() > 0, "Slice %1% must be written in action %2%", slice, action->name);
        for (auto operand : reads) {
            BUG_CHECK(operand.phv_used->field(), "There must be a field read for slice %1%", slice);
            for (const int stage : stages(action, slice.isPhysicalStageBased())) {
                ordered_set<PHV::AllocSlice> per_source_slices =
                    alloc.slices(operand.phv_used->field(), operand.phv_used->range(), stage,
                                 PHV::FieldUse(PHV::FieldUse::READ));
                for (auto source : container_state)
                    if (source.field() == operand.phv_used->field() &&
                        source.field_slice().overlaps(operand.phv_used->range()))
                        per_source_slices.insert(source);
                for (auto source_slice : per_source_slices) {
                    if (source_slice.container_slice().lo != slice.container_slice().lo)
                        return false;
                }
            }
        }
    }
    return true;
}

inline int ActionPhvConstraints::getOffset(le_bitrange a, le_bitrange b, PHV::Container c) const {
    return ((a.lo - b.lo) % c.size());
}

/** Steps in verifying alignment of the two PHV sources:
 *
 * 1. Divide the source AllocSlices corresponding to the packing in
 * @p container_state into the respective containers (firstContainerSet and
 * secondContainerSet). All slices in each respective ContainerSet must be
 * aligned at the same offset with reference to their destination slices.
 * Also, only one container set can be unaligned with the destination for every
 * instruction, and that container set cannot have any hole in allocation, which is not check
 * in this function but should be checked in masks_valid.
 *
 * 2. If both firstContainerAligned and secondContainerAligned are
 * simultaneously false, then packing is impossible due to alignment
 * constraints on the instructions.
 *
 * 3. If packing is possible, return the classification of source slices into
 * respective source containers (modeled as a ClassifiedSources array with 2 items).
 */
std::optional<ActionPhvConstraints::ClassifiedSources>
ActionPhvConstraints::verify_two_container_alignment(
        const PHV::Allocation& alloc,
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const IR::MAU::Action* action,
        const PHV::Container destination,
        ActionContainerProperty* action_prop) const {
    ClassifiedSources sources;
    for (auto dest : container_state) {
        LOG7("\t\t\t\tClassifying source slice for: " << dest);
        for (auto operand : constraint_tracker.sources(dest, action)) {
            if (operand.ad || operand.constant || !operand.phv_used) continue;
            const PHV::Field* fieldRead = operand.phv_used->field();
            // only need to check one stage because action alignments requirement
            // are the same regardless of stage and slices in the container.
            const int stage = *stages(action, dest.isPhysicalStageBased()).begin();
            le_bitrange rangeRead = operand.phv_used->range();
            ordered_set<PHV::AllocSlice> source_slices =
                alloc.slices(fieldRead, rangeRead, stage, PHV::FieldUse(PHV::FieldUse::READ));
            for (auto source_slice : container_state) {
                const bool rangeOverlaps = source_slice.field_slice().overlaps(rangeRead);
                if (source_slice.field() == fieldRead && rangeOverlaps)
                    source_slices.insert(source_slice);
            }

            // Combine multiple adjacent source slices.
            const PHV::AllocSlice* src_slice = nullptr;
            for (auto& slice : source_slices) {
                if (src_slice == nullptr) {
                    src_slice = &slice;
                    continue;
                }

                // TODO: We might be able to handle slices of the same
                // field in different containers in the future.  Needs more
                // thought.
                BUG_CHECK(src_slice->container() == slice.container(),
                          "Source slices of the same field found in different containers");
                BUG_CHECK(src_slice->container_slice().hi + 1 == slice.container_slice().lo,
                          "Non-adjacent container slices of the same field");
                BUG_CHECK(src_slice->field_slice().hi + 1 == slice.field_slice().lo,
                          "Non-adjacent field slices of the same field");
                src_slice = new PHV::AllocSlice(
                    src_slice->field(), src_slice->container(), src_slice->field_slice().lo,
                    src_slice->container_slice().lo,
                    src_slice->field_slice().size() + slice.field_slice().size());
            }
            // For every source slice, check alignment individually and divide it up as part of
            // either the first source container or the second source container
            if (src_slice == nullptr) {
                LOG5("\t\t\t\t\tNo source slice found");
                continue;
            }

            // add this source list to sources.
            const auto& sl = *src_slice;
            if (!sources[0].exist) {
                // first container encountered
                sources[0].exist = true;
                sources[0].container = sl.container();
                sources[0].slices.insert(sl);
                sources[0].aligned = (sl.container_slice() == dest.container_slice());
                sources[0].offset =
                    getOffset(sl.container_slice(), dest.container_slice(), dest.container());
                LOG7("First container is " << sources[0].container
                                           << " aligned: " << sources[0].aligned
                                           << ", offset: " << sources[0].offset);
            } else if (!sources[1].exist) {
                // first container has already been encountered at this point
                if (sources[0].container == sl.container()) {
                    LOG7("\t\t\t\t\tFound first container : " << sl.container() << " in : " <<
                            sl);
                    sources[0].slices.insert(sl);
                    // check if the slice is aligned and whether this is the same as the
                    // previous source slices from this source container
                    const bool slice_aligned = (sl.container_slice() == dest.container_slice());
                    if (sources[0].aligned != slice_aligned) {
                        LOG5("\t\t\t\t\tSource slices are both aligned and unaligned");
                        return std::nullopt;
                    }
                    // if the slices are all unaligned, check if the offset of the source and
                    // destination slices are uniform across all slices. If not, packing is
                    // invalid (enforced by setting firstContainerAligned to false).
                    if (!sources[0].aligned) {
                        const int offset = getOffset(
                                sl.container_slice(), dest.container_slice(), dest.container());
                        if (sources[0].offset != offset) {
                            LOG5("\t\t\t\t\tSource slices are aligned at different offsets.");
                            return std::nullopt;
                        }
                    }
                } else {
                    // at this point, we have encountered the second source container
                    sources[1].exist = true;
                    sources[1].container = sl.container();
                    sources[1].slices.insert(sl);
                    sources[1].aligned = (sl.container_slice() == dest.container_slice());
                    sources[1].offset =
                        getOffset(sl.container_slice(), dest.container_slice(), dest.container());
                    LOG7("Second container is " << sources[0].container
                                                << " aligned: " << sources[0].aligned
                                                << ", offset: " << sources[0].offset);
                }
            } else {
                // two different containers have already been encountered
                const bool sliceAligned = (sl.container_slice() == dest.container_slice());
                int refOffset = 0;
                bool containerAligned;
                if (sources[0].container == sl.container()) {
                    LOG7("\t\t\t\t\tFound first container : " << sl.container() << " in : " <<
                            sl);
                    containerAligned = sources[0].aligned;
                    refOffset = sources[0].offset;
                    sources[0].slices.insert(sl);
                } else if (sources[1].container == sl.container()) {
                    LOG7("\t\t\t\t\tFound second container: " << sl.container() << " in : " <<
                            sl);
                    containerAligned = sources[1].aligned;
                    refOffset = sources[1].offset;
                    sources[1].slices.insert(sl);
                } else {
                    BUG("Found a third container source");
                }

                // If alignment is different for any source slice, either in first or second
                // source container, then packing is invalid.
                if (containerAligned != sliceAligned) {
                    LOG5("\t\t\t\t\tSource slices are both aligned and unaligned.");
                    return std::nullopt;
                }
                // If offset is different for any source slice, either in first or second source
                // container, then packing is invalid.
                const int offset =
                    getOffset(sl.container_slice(), dest.container_slice(), dest.container());
                if (refOffset != offset) {
                    LOG5("\t\t\t\t\tSource slices are aligned at different offsets.");
                    return std::nullopt;
                }
            }
        }
    }

    // If first container is set and unaligned, and the second container is unallocated, then the
    // second container must be aligned, meaning that the unallocated source must be aligned for
    // this action.
    if (sources[0].exist && !sources[0].aligned && !sources[1].exist)
        action_prop->aligned_unallocated_requires_aligment = true;

    // If both source containers are unaligned, then packing is invalid.
    if (!sources[0].aligned && !sources[1].aligned) {
        LOG5("\t\t\t\tBoth source containers cannot be unaligned.");
        return std::nullopt;
    }

    // xxx(Deep): Overly restrictive constraint
    // For deposit-field, if the destination container is also a source, it cannot be the rotated
    // source only.
    // The right way to fix this is to ensure that for fields containers with unallocated bits, all
    // unallocated sources have to be packed in the same container as the allocated sources.
    if (!sources[0].aligned && sources[0].container == destination &&
        sources[1].container != destination) {
        LOG5("\t\t\t\tDestination cannot also be rotated source.");
        return std::nullopt;
    }
    if (!sources[1].aligned && sources[1].container == destination &&
        sources[0].container != destination) {
        LOG5("\t\t\t\tDestination cannot also be rotated source.");
        return std::nullopt;
    }

    return sources;
}

bool ActionPhvConstraints::assign_containers_to_unallocated_sources(
        const PHV::Allocation& alloc,
        const UnionFind<PHV::FieldSlice>& copacking_constraints,
        ordered_map<PHV::FieldSlice, PHV::Container>& req_container) const {
    // For each set in copacking_constraints, check if any sources are allocated and if yes, all
    // unallocated sources in that set have to have the same container number
    for (auto& set : copacking_constraints) {
        PHV::Container c;
        ordered_set<PHV::FieldSlice> unallocated_slices;
        // Find all allocated slices in each set
        for (auto& slice : set) {
            ordered_set<PHV::AllocSlice> per_source_slices = alloc.slices(slice.field(),
                    slice.range());
            // If this is an unallocated source slice, then examine the next slice after adding it
            // to the unallocated slices set
            if (per_source_slices.size() == 0) {
                unallocated_slices.insert(slice);
                continue; }
            // For each alloc slice, note the container used. If we encounter two different
            // containers, then packing in a single container is not possible. Return false.
            for (auto sl : per_source_slices) {
                // No container allocated so far
                if (c == PHV::Container()) {
                    LOG6("\t\t\t\t\tSlice " << sl << " already allocated to container " <<
                            sl.container());
                    c = sl.container();
                    continue; }
                if (sl.container() != c) {
                    LOG5("\t\t\t\t\tSlice " << sl << " allocated to container " << sl.container() <<
                         " even though other slice(s) in the copacking set are allocated to " <<
                         "container "  << c);
                    return false; } } }
        // If all slices are unallocated, go to the next set in copacking_constraints
        if (c == PHV::Container()) continue;
        for (auto& slice : unallocated_slices) {
            LOG5("\t\t\t\t\tSlice " << slice << " must be allocated to container " << c);
            req_container[slice] = c; } }

    return true;
}

bool ActionPhvConstraints::all_field_slices_written_together(
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const PHV::ActionSet& set_of_actions,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    for (auto action : set_of_actions) {
        enum WriteType { NO_INIT, NOT_WRITTEN, WRITTEN } thisActionWrites = NO_INIT;
        // for each AllocSlice in the container, check if it is written by the action.
        for (auto& slice : container_state) {
            std::optional<OperandInfo> writeStatus = constraint_tracker.is_written(slice, action);
            bool metaInit = false;
            // Metadata initialization done in this action for the field of slice.
            if (initActions.count(slice.field()) && initActions.at(slice.field()).count(action))
                metaInit = true;
            if (!writeStatus && !metaInit) {
                if (thisActionWrites == NO_INIT)
                    // First slice encountered, so set status to field not written.
                    thisActionWrites = NOT_WRITTEN;
                else if (thisActionWrites == WRITTEN)
                    // If a field was written previously, this returns false.
                    return false;
            } else {
                if (thisActionWrites == NO_INIT)
                    // first slice encountered, so set status to field written.
                    thisActionWrites = WRITTEN;
                else if (thisActionWrites == NOT_WRITTEN)
                    // If a field was not written previously, this returns false.
                    return false; } } }
    return true;
}

bool ActionPhvConstraints::checkBridgedPackingConstraints(
        const ordered_set<const PHV::Field*>& packing) const {
    // Mapping from a field to the actions in which the field is written.
    ordered_map<const PHV::Field*, ordered_set<const IR::MAU::Action*>> actionToWrittenFieldsMap;
    // Represents the list of all actions where the fields in the candidate packing @packing are
    // written.
    ordered_set<const IR::MAU::Action*> allActions;
    for (auto f : packing) {
        auto actions = actions_writing_fields(f);
        const IR::MAU::Action* act = nullptr;
        // The action with the name `act` is used to initialize the bridged metadata header version
        // of the field to the original program version. We do not need to consider the set
        // operations introduced in this compiler-generated action when checking valid packing for
        // the bridged metadata fields. Therefore, we remove this action from the list of actions
        // writing fields.
        for (auto* a : actions)
            if (a->name == "act")
                act = a;
        if (act != nullptr)
            actions.erase(act);
        actionToWrittenFieldsMap[f] = actions;
        allActions.insert(actionToWrittenFieldsMap[f].begin(), actionToWrittenFieldsMap[f].end()); }

    // If there are bits present in a container that won't be written for a given action, we need to
    // make sure that the fields that are written all have either a PHV write or an action data
    // write.
    for (auto* act : allActions) {
        // true if we have encountered a PHV source for one of the fields in the packing.
        std::optional<bool> foundPHVSource = std::nullopt;
        // true if we have encountered an action data/constant source for one of the fields in the
        // packing.
        std::optional<bool> foundADConstantSource = std::nullopt;
        LOG6("\t\t    Action: " << act->name);
        BUG_CHECK(act->name != "act", "Action %1% should have been removed earlier", act->name);
        // For each field in packing:
        for (auto* f : packing) {
            auto hasPHVSource = constraint_tracker.hasPHVSource(f, act);
            auto hasADSource = constraint_tracker.hasActionDataOrConstantSource(f, act);
            LOG6("\t\t\t  Field: " << f->name);
            std::stringstream ss;
            if (hasPHVSource)
                ss << "\t\t\t\tPHV: " << *hasPHVSource;
            else
                ss << "\t\t\t\tNo PHV";
            if (hasADSource)
                ss << ", AD/Constant: " << *hasADSource;
            else
                ss << ", No AD/Constant.";
            LOG6(ss.str());
            // Mark foundPHVSource as having found the first PHV source.
            if (!foundPHVSource && hasPHVSource) {
                foundPHVSource = *hasPHVSource;
                LOG6("\t\t\t  Setting foundPHVSource to " << *foundPHVSource); }

            // Mark foundADConstantSource as having found the first action data/constant source.
            if (!foundADConstantSource && hasADSource) {
                foundADConstantSource = *hasADSource;
                LOG6("\t\t\t  Setting foundADConstantSource to " << *foundADConstantSource); }

            // If there was a PHV source, and a field does not have a PHV source, then this packing
            // is not possible. (Underlying principle: Do not mix action data/PHV sources in bridged
            // metadata packing because ensuring a valid deposit-field in those cases becomes very
            // complicated, and such a valid packing is not always possible).
            if (hasPHVSource && foundPHVSource && *hasPHVSource != *foundPHVSource) {
                LOG6("\t\t\t  Returning false");
                return false; }

            // Similarly, if there was an action data/constant source and a field does not have an
            // action data/constant source, then this packing is not possible.
            if (hasADSource && foundADConstantSource && *hasADSource != *foundADConstantSource) {
                LOG6("\t\t\t  Returning false");
                return false; } } }
    return true;
}

bool ActionPhvConstraints::written_in(const PHV::Field* f, const IR::MAU::Action* act) const {
    if (LOGGING(6)) {
        LOG6("\t\t\t\tField " << f->name << " is written by:");
        for (const auto* act : actions_writing_fields(f))
            LOG6("\t\t\t\t  " << act->name);
    }
    return actions_writing_fields(f).count(act);
}

bool ActionPhvConstraints::written_by_ad_constant(
        const PHV::Field* f,
        const IR::MAU::Action* act) const {
    // If field is not written in this action, return false.
    if (!written_in(f, act)) return false;
    PHV::FieldSlice slice(f, StartLen(0, f->size));
    for (auto operand : constraint_tracker.sources(slice, act))
        if (operand.ad || operand.constant)
            return true;
    return false;
}

std::optional<ActionPhvConstraints::OperandInfo>
ActionPhvConstraints::ConstraintTracker::is_written(
        PHV::FieldSlice slice,
        const IR::MAU::Action *act) const {
    const auto& all_writes = this->writes(act);
    for (const auto& op : all_writes) {
        if (!op.phv_used)
            continue;
        if (op.phv_used->field() != slice.field())
            continue;
        if (!op.phv_used->range().contains(slice.range()))
            continue;

        ActionPhvConstraints::OperandInfo rv = op;
        rv.phv_used = slice;
        cstring operation = rv.flags & OperandInfo::WHOLE_CONTAINER     ? "WHOLE_CONTAINER"_cs
                            : rv.flags & OperandInfo::BITWISE           ? "BITWISE"_cs
                            : rv.flags & OperandInfo::PART_OF_CONTAINER ? "PART_CONTAINER"_cs
                                                                        : "MOVE"_cs;
        LOG5("\t\t\t\tSlice " << slice << " is written in action " << act->name << " by a "
                              << operation << " operation.");
        return rv; }

    LOG5("\t\t\t\tSlice " << slice << " is not written in action " << act->name);
    return std::nullopt;
}

bool ActionPhvConstraints::cannot_initialize(
        const PHV::Container& c,
        const IR::MAU::Action* action,
        const PHV::Allocation& alloc) const {
    ordered_map<PHV::FieldSlice, ordered_set<PHV::AllocSlice>> destinationsToBeChecked;
    const int stage = *stages(action, false).begin();
    for (auto write : constraint_tracker.writes(action)) {
        // For each write in the action, check if the written slice has been allocated yet, and if
        // yes, whether it has been  allocated in container c.
        if (!write.phv_used) BUG("No PHV FieldSlice corresponding to the write");
        const PHV::Field* fieldWritten = write.phv_used->field();
        le_bitrange rangeWritten = write.phv_used->range();
        ordered_set<PHV::AllocSlice> per_destination_slices = alloc.slices(fieldWritten,
                rangeWritten, stage, PHV::FieldUse(PHV::FieldUse::WRITE));
        for (auto dest : per_destination_slices) {
            if (dest.container() != c) continue;
            // This written slice is allocated to the current container.
            PHV::FieldSlice write(dest.field(), dest.field_slice());
            LOG5("\t\t\t  Field slice " << write << " must be checked for PHV/action data/non-zero "
                 "constant sources.");
            for (auto& read : constraint_tracker.sources(write, action)) {
                if (read.ad) { LOG5("\t\t\t... action data"); return true; }
                if (read.phv_used != std::nullopt) { LOG5("\t\t\t... PHV"); return true; }
                if (read.constant && read.const_value != 0) {
                    LOG5("\t\t\t... action data");
                    return true; }
            }
        }
    }
    return false;
}

ordered_set<const PHV::Field*>
ActionPhvConstraints::actionReads(const IR::MAU::Action* act) const {
    ordered_set<const PHV::Field*> rv;
    for (auto& slice : constraint_tracker.reads(act))
        rv.insert(slice.field());
    return rv;
}

ordered_set<const PHV::Field*>
ActionPhvConstraints::actionWrites(const IR::MAU::Action* act) const {
    ordered_set<const PHV::Field*> rv;
    for (auto fw : constraint_tracker.writes(act)) {
        if (fw.phv_used)
            rv.insert(fw.phv_used->field()); }
    return rv;
}

ordered_set<const PHV::Field*> ActionPhvConstraints::slices_sources(
    const PHV::Field* dest, const std::vector<PHV::FieldSlice>& slices) const {
    ordered_set<const PHV::Field*> rv;
    for (const IR::MAU::Action* act : constraint_tracker.written_in(PHV::FieldSlice(dest))) {
        for (const auto& fs : slices) {
            BUG_CHECK(fs.field() == dest, "field slice %1% is not part of %2%", fs, dest->name);
            auto operands = constraint_tracker.sources(fs, act);
            for (const auto& operand : operands) {
                if (operand.phv_used) {
                    rv.insert(operand.phv_used->field());
                }
            }
        }
    }
    return rv;
}

ordered_set<const PHV::Field*> ActionPhvConstraints::slices_destinations(
    const PHV::Field* src, const std::vector<PHV::FieldSlice>& slices) const {
    ordered_set<const PHV::Field*> rv;
    for (const IR::MAU::Action* act : constraint_tracker.read_in(PHV::FieldSlice(src))) {
        for (const auto& fs : slices) {
            auto destinations = constraint_tracker.destinations(fs, act);
            for (const auto& slice : destinations) rv.insert(slice.field());
        }
    }
    return rv;
}

std::optional<const PHV::Field*> ActionPhvConstraints::field_destination(
        const PHV::Field* f,
        const IR::MAU::Action* action) const {
    ordered_set<const PHV::Field*> rs;
    PHV::FieldSlice source(f, StartLen(0, f->size));
    for (const IR::MAU::Action* act : constraint_tracker.read_in(source)) {
        if (act != action) continue;
        auto destinations = constraint_tracker.destinations(source, act);
        for (const auto& slice : destinations)
            rs.insert(slice.field());
    }
    if (rs.size() == 0) return std::nullopt;
    return (*(rs.begin()));
}

bool ActionPhvConstraints::move_only_operations(const PHV::Field* f) const {
    PHV::FieldSlice dest(f, StartLen(0, f->size));
    // Whenever f is written in an action, if the write is not a MOVE operation, return true
    for (const IR::MAU::Action* act : constraint_tracker.written_in(dest)) {
        auto operands = constraint_tracker.sources(dest, act);
        for (auto it = operands.begin(); it != operands.end(); ++it)
            if (it->flags != OperandInfo::MOVE)
                return false; }
    return true;
}

bool ActionPhvConstraints::can_pack_pov(
        const PHV::SuperCluster::SliceList* slice_list,
        const PHV::Field* f) const {
    LOG3("Considering slice list " << *slice_list);
    ordered_map<const PHV::Field*, size_t> fieldsWithPositions;
    size_t index = 0;
    for (auto& slice : *slice_list)
        fieldsWithPositions[slice.field()] = index++;
    fieldsWithPositions[f] = index++;
    for (auto kv : fieldsWithPositions)
        LOG3(kv.first->name << " : " << kv.second);

    // Conditions:
    // 1. If these fields are all written in the same container with different kind of
    //    operands, then reject this packing.
    ordered_set<const IR::MAU::Action*> set_of_actions;
    for (auto kv : fieldsWithPositions) {
        const auto& writing_actions = constraint_tracker.written_in(PHV::FieldSlice(kv.first));
        set_of_actions.insert(writing_actions.begin(), writing_actions.end());
    }

    unsigned num_fields = fieldsWithPositions.size();
    for (const auto* act : set_of_actions) {
        LOG3("\tChecking action " << act->name);
        ordered_set<const PHV::Field*> written_by_phv;
        ordered_set<const PHV::Field*> written_by_ad;
        ordered_set<const PHV::Field*> not_written;
        for (auto kv : fieldsWithPositions) {
            auto source = constraint_tracker.hasPHVSource(kv.first, act);
            if (!source) {
                not_written.insert(kv.first);
                continue;
            }
            if (*source == true)
                written_by_phv.insert(kv.first);
            else
                written_by_ad.insert(kv.first);
        }

        LOG3("phv: " << written_by_phv.size() << ", ad: " << written_by_ad.size() <<
             ", not written: " << not_written.size());
        if (num_fields % 8 == 0 && not_written.size() == 0) continue;
        if (written_by_phv.size() > 0 && written_by_ad.size() > 0 &&
                (not_written.size() > 0 || num_fields % 8 != 0)) {
            LOG3("Some part of the container will be overwritten.");
            return false;
        }
    }
    return true;
}

std::optional<bool> ActionPhvConstraints::ConstraintTracker::hasPHVSource(
        const PHV::Field* field,
        const IR::MAU::Action* act) const {
    PHV::FieldSlice destination(field, StartLen(0, field->size));
    // Destination field not written in @act, return std::nullopt.
    if (!written_in(destination).count(act))
        return std::nullopt;
    // Check each source for the destination field.
    auto operands = sources(destination, act);
    for (auto& op : operands) {
        // If any source has a non-PHV source, then return false.
        if (op.phv_used == std::nullopt)
            return false;
    }
    return true;
}

std::optional<bool> ActionPhvConstraints::ConstraintTracker::hasActionDataOrConstantSource(
        const PHV::Field* field,
        const IR::MAU::Action* act) const {
    PHV::FieldSlice destination(field, StartLen(0, field->size));
    // Destination field not written in @act, return std::nullopt.
    if (!written_in(destination).count(act))
        return std::nullopt;
    // Check each source for the destination field.
    auto operands = sources(destination, act);
    for (auto& op : operands) {
        // If any source has a non action data or constant source, return false.
        if (op.ad == false && op.constant == false)
            return false;
    }
    return true;
}

UnionFind<PHV::FieldSlice> ActionPhvConstraints::classify_PHV_sources(
        const ordered_set<PHV::FieldSlice>& phvSlices,
        const ordered_map<PHV::FieldSlice, const PHV::SuperCluster::SliceList*>& lists_map) const {
    UnionFind<PHV::FieldSlice> classified_sources;
    ordered_map<const PHV::SuperCluster::SliceList*, PHV::FieldSlice> slice_list_to_slice;
    for (auto& slice : phvSlices) {
        classified_sources.insert(slice);
        // If the slice does not belong to a slice list, then it is to be placed in its own
        // container.
        if (!lists_map.count(slice)) continue;
        // The slice list is already represented in the slice_list_to_slice map. Union the set
        // corresponding to the representative slice with the current slice.
        if (slice_list_to_slice.count(lists_map.at(slice))) {
            classified_sources.makeUnion(slice_list_to_slice.at(lists_map.at(slice)), slice);
            continue;
        }
        // First representative slice of this slice list.
        slice_list_to_slice[lists_map.at(slice)] = slice;
    }
    return classified_sources;
}

void ActionPhvConstraints::throw_too_many_sources_error(
        const ordered_set<PHV::FieldSlice>& actionDataWrittenSlices,
        const ordered_set<PHV::FieldSlice>& notWrittenSlices,
        const UnionFind<PHV::FieldSlice>& phvSources,
        const ordered_map<PHV::FieldSlice, std::pair<int, int>>& phvAlignedSlices,
        const IR::MAU::Action* action,
        std::stringstream& ss) const {
    cstring action_name = canon_name(action->externalName());
    bool adSource = (actionDataWrittenSlices.size() > 0);
    ss << "it requires too many sources.";
    unsigned source_num = 1;
    if (adSource) {
        ss << std::endl << "\tSource " << (source_num++) << " is action data/constant, which "
            "write(s) the following field slices:";
        for (auto& slice : actionDataWrittenSlices) {
            auto sources = constraint_tracker.sources(slice, action);
            std::stringstream ss_source;
            for (auto& source : sources) {
                if (source.ad) {
                    ss_source << ", written by action data";
                    break;
                } else if (source.constant) {
                    ss_source << ", written by constant 0x" << std::hex << source.const_value;
                    break;
                }
            }
            ss << std::endl << "\t  " << slice.shortString() << ss_source.str();
        }
    }
    if (notWrittenSlices.size() > 0) {
        ss << std::endl << "\tSource " << (source_num++) << " is the container itself, imposed "
            "by the following fields that are unwritten in action " << action_name << ":";
        for (auto& slice : notWrittenSlices)
            ss << std::endl << "\t  " << slice.shortString();
    }
    for (const auto& set : phvSources) {
        bool allSlicesUnwritten = true;
        // Ignore the slices in the UnionFind struct that are sources because they are not written
        // in this action. The not-written case has already been handled by the above if-condition.
        for (auto& slice : set)
            if (!notWrittenSlices.count(slice))
                allSlicesUnwritten = false;
        if (allSlicesUnwritten) continue;
        ss << std::endl << "\tSource " << (source_num++) << " contains the following fields:";
        std::optional<int> offset = std::nullopt;
        bool unaligned = false;
        ordered_set<PHV::FieldSlice> printedFields;
        for (auto& slice : set) {
            if (printedFields.count(slice)) continue;
            ss << std::endl << "\t  " << slice.shortString();
            printedFields.insert(slice);
            if (phvAlignedSlices.count(slice)) {
                int start = phvAlignedSlices.at(slice).second;
                int thisOffset = phvAlignedSlices.at(slice).first - start;
                if ((offset != std::nullopt) && (*offset != thisOffset))
                    unaligned = true;
                // Start = -1 indicates that the source does not belong to a slice list and
                // therefore, could potentially have multiple possible alignments within its
                // container.
                // TODO: Potentially improve this by considering bit in byte alignments for
                // fields.
                if (start == -1) continue;
                ss << " @ bits " << start << "-" << (start + slice.size() - 1);
                offset = thisOffset;
            }
        }
        // Also, output second order alignment violations (the programmer first needs to resolve the
        // too-many-sources error).
        // Also, indicate to the user that PHV sources are not aligned (if possible), which violates
        // alignment constraints when action data/constant is present.
        if (adSource && (offset != std::nullopt) && (*offset != 0)) {
            ss << std::endl << "  Slices of source " << source_num-1 << " are not allocated "
                << "at the same offsets within the container as the destination slices." <<
                std::endl;
            ss << "  " << Device::name() << " does not support action data/constant with "
                << "rotated PHV source at the same time." << std::endl;
            continue;
        }
        // If each destination in the container has a different offset relative to its source slice
        // (and all those source slices are part of the same source container), that also violates
        // an action constraint.
        if (unaligned) {
            ss << std::endl << "  Slices of source " << source_num-1 << " do not have the "
                "same rotational alignment with respect to their destination slices." <<
                std::endl;
            continue;
        }
    }

    if (adSource) {
        ss << std::endl << "  However, " << Device::name() << " supports only one PHV source, "
            "when action data/constant is present.";
    } else {
        ss << std::endl << "  However, " << Device::name() << " supports only two PHV sources.";
    }
    ss << std::endl << "  Rewrite action " << action_name << " to use at most 2 PHV sources "
        "(and no action data/constant source) or 1 PHV source (if action data/constant is "
        << "necessary)." << std::endl << std::endl;
}

void ActionPhvConstraints::throw_non_contiguous_mask_error(
        const ordered_set<PHV::FieldSlice>& notWrittenSlices,
        const ordered_map<PHV::FieldSlice, ordered_set<PHV::FieldSlice>>& destToSource,
        const ordered_map<PHV::FieldSlice, unsigned>& fieldAlignments,
        cstring action_name,
        std::stringstream& ss) const {
    ss << " of non-contiguous partial container write(s)." << std::endl;
    ss << "\tAction " << action_name << " writes the following fields in the container:" <<
        std::endl;
    for (const auto& kv : destToSource) {
        ss << "\t  " << kv.first.shortString();
        if (kv.first.size() == 1)
            ss << " @ bit " << fieldAlignments.at(kv.first) << std::endl;
        else
            ss << " @ bits " << fieldAlignments.at(kv.first) << "-" <<
                (fieldAlignments.at(kv.first) + kv.first.size() - 1) << std::endl;
    }
    ss << "\tAction " << action_name << " does not write the following container fields:" <<
        std::endl;
    for (auto& slice : notWrittenSlices) {
        ss << "\t  " << slice.shortString();
        if (slice.size() == 1)
            ss << " @ bit " << fieldAlignments.at(slice) << std::endl;
        else
            ss << " @ bits " << fieldAlignments.at(slice) << "-" << (fieldAlignments.at(slice) +
                    slice.size() - 1) << std::endl;
    }
    ss << "  " << Device::name() << " requires at least one of the set of slices written or "
       "slices not written to be contiguous." << std::endl << std::endl;
}

bool ActionPhvConstraints::diagnoseInvalidPacking(
        const IR::MAU::Action* action,
        const PHV::SuperCluster::SliceList* list,
        const ordered_map<PHV::FieldSlice, unsigned>& fieldAlignments,
        const ordered_map<PHV::FieldSlice, const PHV::SuperCluster::SliceList*>& lists_map,
        std::stringstream& ss) const {
    ordered_map<PHV::FieldSlice, ordered_set<PHV::FieldSlice>> destToSource;
    ordered_map<PHV::FieldSlice, ordered_set<PHV::FieldSlice>> sourceToDest;
    ordered_set<PHV::FieldSlice> notWrittenSlices;
    ordered_set<PHV::FieldSlice> actionDataWrittenSlices;
    ordered_map<PHV::FieldSlice, std::pair<int, int>> phvAlignedSlices;
    ordered_set<PHV::FieldSlice> allPHVSlices;
    cstring action_name = canon_name(action->externalName());
    bitvec writtenBitvec;
    bitvec notWrittenBitvec;
    for (auto& slice : *list) {
        auto sources = constraint_tracker.sources(slice, action);
        if (sources.size() == 0) {
            notWrittenSlices.insert(slice);
            notWrittenBitvec |= bitvec(fieldAlignments.at(slice), slice.size());
            allPHVSlices.insert(slice);
            continue;
        }
        writtenBitvec |= bitvec(fieldAlignments.at(slice), slice.size());
        for (auto& info : sources) {
            if (info.ad || info.constant) {
                actionDataWrittenSlices.insert(slice);
            } else {
                BUG_CHECK(info.phv_used != std::nullopt, "No source associated with OperandInfo?");
                PHV::FieldSlice source_slice = *(info.phv_used);
                destToSource[slice].insert(source_slice);
                sourceToDest[source_slice].insert(slice);
                allPHVSlices.insert(*(info.phv_used));
                if (fieldAlignments.count(source_slice))
                    phvAlignedSlices[source_slice] = std::make_pair(fieldAlignments.at(slice),
                            fieldAlignments.at(source_slice));
                else
                    phvAlignedSlices[source_slice] = std::make_pair(fieldAlignments.at(slice), -1);
            }
        }
    }
    UnionFind<PHV::FieldSlice> phvSources = classify_PHV_sources(allPHVSlices, lists_map);
    size_t numPHVSources = phvSources.numSets();
    LOG4("Number of detected PHV sources: " << numPHVSources);
    LOG4(phvSources);

    bool adSource = (actionDataWrittenSlices.size() > 0);
    size_t allowedPHVSources = adSource ? 1 : 2;
    bool tooManySources = (numPHVSources > allowedPHVSources);

    // Detect the case where multiple PHV sources are required, in addition to action data/constant.
    if (tooManySources) {
        throw_too_many_sources_error(actionDataWrittenSlices, notWrittenSlices, phvSources,
                phvAlignedSlices, action, ss);
        return false;
    }

    // Detect cases where a deposit-field cannot be synthesized because the mask cannot be
    // contiguous.
    if (numPHVSources > 0 && !writtenBitvec.is_contiguous() && !notWrittenBitvec.is_contiguous()) {
        throw_non_contiguous_mask_error(notWrittenSlices, destToSource, fieldAlignments,
                action_name, ss);
        return false;
    }

    auto IsRotation = [](const std::pair<const PHV::FieldSlice, std::pair<int, int>> &alignment) {
        if (alignment.second.second != -1 && alignment.second.first != alignment.second.second)
            return true;
        return false;
    };

    if (adSource && numPHVSources > 0 &&
        std::any_of(phvAlignedSlices.begin(), phvAlignedSlices.end(), IsRotation)) {
        throw_too_many_sources_error(actionDataWrittenSlices, notWrittenSlices, phvSources,
                phvAlignedSlices, action, ss);
        return false;
    }

    return true;
}

bool ActionPhvConstraints::diagnoseSuperCluster(
        const ordered_set<const PHV::SuperCluster::SliceList*>& sc,
        const ordered_map<PHV::FieldSlice, unsigned>& fieldAlignments,
        std::stringstream& error_msg) const {
    // For each slice list, gather all the field slices in it, and the actions that write to that
    // slice list.
    error_msg << "This program violates action constraints imposed by " << Device::name() << "." <<
        std::endl << std::endl;
    ordered_map<PHV::FieldSlice, const PHV::SuperCluster::SliceList*> slice_to_slice_list;
    bool error_found = false;
    for (const auto* list : sc)
        for (auto& slice : *list)
            slice_to_slice_list[slice] = list;
    for (const auto* list : sc) {
        ordered_set<const IR::MAU::Action*> actions;
        ordered_set<PHV::FieldSlice> set_of_slices;
        cstring hdr = (*(list->begin())).field()->header();
        std::stringstream slices;
        slices << "  PHV allocation requires the following field slices to be packed together, "
            "because of the structure of header " << hdr << std::endl;
        for (auto& slice : *list) {
            slices << "\t" << slice.shortString() << std::endl;
            auto actionsWritingSlice = constraint_tracker.written_in(slice);
            actions.insert(actionsWritingSlice.begin(), actionsWritingSlice.end());
            set_of_slices.insert(slice);
        }
        for (const auto* action : actions) {
            cstring action_name = canon_name(action->externalName());
            std::stringstream ss;
            ss << "  Action " << action_name << " must be rewritten.";
            // Check whether the operations in this slice list are of the correct kind.
            if (!valid_container_operation_type(action, set_of_slices, ss)) {
                if (action->srcInfo.isValid())
                    error_msg << action->srcInfo.toPositionString() << ":" << std::endl <<
                            action->srcInfo.toSourceFragment() << std::endl;
                error_msg << ss.str();
                error_found = true;
                continue;
            }
            ss.str(std::string());
            ss << "  Action " << action_name << " must be rewritten, because ";
            // Then check if the packing for this supercluster is valid.
            if (!diagnoseInvalidPacking(action, list, fieldAlignments, slice_to_slice_list, ss)) {
                if (action->srcInfo.isValid())
                    error_msg << action->srcInfo.toPositionString() << ":" << std::endl <<
                            action->srcInfo.toSourceFragment() << std::endl;
                error_msg << slices.str() << std::endl;
                error_msg << ss.str();
                error_found = true;
            }
        }
    }
    return error_found;
}

void ActionPhvConstraints::ConstraintTracker::printMapStates() const {
    if (!LOGGING(7)) return;
    for (auto &act : action_to_writes) {
        LOG7("Action: " << act.first->name << " writes fields: ");
        for (auto &fi : act.second) {
            LOG7("\t\t" << *fi.phv_used << ", written by a MOVE? "
                 << (fi.flags == OperandInfo::MOVE)); } }

        for (auto &f : write_to_reads_per_action) {
            LOG7("Key field: " << f.first << " uses operands: ");
            for (auto &fi : f.second) {
                LOG7("\tAction: " << fi.first->name);
                for (auto &r : fi.second) {
                    LOG7("\t\tRange: " << r.first);
                    for (auto &fii : r.second) {
                        if (!fii.ad && !fii.constant)
                            LOG7("\t\t\tSlice: " << *fii.phv_used);
                        else
                            LOG7("\t\t\tAction data."); } } } }

        for (auto &f : read_to_writes_per_action) {
            LOG7("Key field: " << f.first << " is read by the field(s): ");
            for (auto &fi : f.second) {
                LOG7("\tAction: " << fi.first->name);
                for (auto& r : fi.second) {
                    LOG7("\t\tRange: " << r.first);
                    for (auto &fii : r.second)
                        LOG7("\t\t\tSlice: " << fii); } } }

    for (auto& by_field : field_writes_to_actions) {
        for (auto& by_range : by_field.second) {
            LOG7(PHV::FieldSlice(by_field.first, by_range.first) << " written in:");
            for (auto* act : by_range.second)
                LOG7("    " << act->name); } }
}

bool ActionPhvConstraints::is_in_field_writes_to_actions(
        cstring write,
        const IR::MAU::Action* action) const {
    const PHV::Field* write_field = phv.field(write);
    if (write_field == nullptr)
        return false;
    auto slice = PHV::FieldSlice(write_field, StartLen(0, write_field->size));
    auto acts = constraint_tracker.written_in(slice);
    return (acts.find(action) != acts.end());
}

bool ActionPhvConstraints::is_in_action_to_writes(
        const IR::MAU::Action* action,
        cstring write) const {
    const PHV::Field* write_field = phv.field(write);
    if (write_field == nullptr)
        return false;
    auto ops = constraint_tracker.writes(action);
    return std::any_of(ops.begin(), ops.end(), [&](const OperandInfo& info) {
                            return info.phv_used && info.phv_used->field() == write_field; });
}

bool ActionPhvConstraints::is_in_write_to_reads(
        cstring write,
        const IR::MAU::Action *act,
        cstring read) const {
    const PHV::Field* write_field = phv.field(write);
    const PHV::Field* read_field = phv.field(read);
    if (write_field == nullptr || read_field == nullptr)
        return false;
    auto write_slice = PHV::FieldSlice(write_field, StartLen(0, write_field->size));
    auto reads = constraint_tracker.sources(write_slice, act);
    return std::any_of(reads.begin(), reads.end(), [&](const OperandInfo& info) {
                            return info.phv_used && info.phv_used->field() == read_field; });
}

ordered_map<const PHV::Field*, int> ActionPhvConstraints::compute_sources_first_order(
    const ordered_map<const PHV::Field*, std::vector<PHV::FieldSlice>>& fields) const {
    // forall writes, add them to dep graph.
    ordered_map<const PHV::Field*, int> field_to_nodes;
    SccTopoSorter topo_sorter;
    for (const auto& kv : fields) {
        const auto* f = kv.first;
        const auto& slices = kv.second;
        if (!field_to_nodes.count(f)) {
            field_to_nodes[f] = topo_sorter.new_node();
        }
        for (const auto* src : slices_sources(f, slices)) {
            if (!field_to_nodes.count(src)) {
                field_to_nodes[src] = topo_sorter.new_node();
            }
            // allocation of f depends on its source src.
            topo_sorter.add_dep(field_to_nodes[src], field_to_nodes[f]);
        }
    }
    auto topo_orders = topo_sorter.scc_topo_sort();
    ordered_map<const PHV::Field*, int> rst;
    for (const auto& kv : field_to_nodes) {
        rst[kv.first] = topo_orders[kv.second];
    }
    return rst;
}

ActionPhvConstraints::ActionSources ActionPhvConstraints::getActionSources(
        const IR::MAU::Action* act, const PHV::Container& c,
        ordered_set<PHV::AllocSlice>& new_slices,
        const PHV::Allocation& alloc) const {
    ActionSources rv;

    LOG5("Finding action sources for " << act->name << " writing to " << c);
    for (const auto& fs : actionWritesSlices(act)) {
        std::set<PHV::AllocSlice> dstSlices;
        for (const auto& sl : new_slices) {
            if (sl.field() == fs.field() && sl.field_slice().overlaps(fs.range()) &&
                    sl.container() == c) {
                dstSlices.emplace(sl);
                rv.dest_bits.setrange(sl.container_slice().lo, sl.container_slice().size());
            }
        }
        for (auto sl : alloc.slices(fs.field(), fs.range(), *stages(act, false).begin(),
                                    PHV::FieldUse(PHV::FieldUse::WRITE))) {
            if (sl.container() == c) {
                dstSlices.emplace(sl);
                rv.dest_bits.setrange(sl.container_slice().lo, sl.container_slice().size());
            }
        }

        if (dstSlices.size() == 0) continue;

        for (const auto& op : constraint_tracker.sources(fs, act)) {
            rv.has_ad |= op.ad;
            rv.has_const |= op.constant;
            if (op.phv_used) {
                const auto slices =
                    alloc.slices(op.phv_used->field(), op.phv_used->range(),
                                 *stages(act, false).begin(), PHV::FieldUse(PHV::FieldUse::READ));
                for (const auto& sl : slices) {
                    if (sl.container()) {
                        rv.phv.emplace(sl.container());
                    } else {
                        rv.unalloc++;
                    }
                }
            }
        }
    }

    return rv;
}

CanPackErrorV2 ActionPhvConstraints::check_read_action_move_constraints(
    const PHV::Allocation& alloc,
    const std::vector<PHV::AllocSlice>& candidates,
    const IR::MAU::Action* action,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    const auto action_stages = stages(action, candidates.front().isPhysicalStageBased());
    const auto use = PHV::FieldUse(PHV::FieldUse::WRITE);
    for (const auto& stage : action_stages) {
        // Gather all containers read from any newly allocated slice.
        ordered_map<PHV::Container, ordered_set<PHV::AllocSlice>> container_write_slices;
        for (const auto& slice : candidates) {
            for (const auto& write : constraint_tracker.destinations(slice, action)) {
                // if destination has been allocated, save their containers.
                alloc.foreach_slice(write.field(), write.range(), stage, use,
                    [&] (const auto& write_sl) {
                        container_write_slices[write_sl.container()].insert(write_sl);
                    });
            }
        }
        // Walk through the destination containers one-by-one and check their validity
        for (const auto& kv : container_write_slices) {
            const auto& c = kv.first;
            const auto& write_slices = kv.second;
            const auto is_all_mutex = [&](const PHV::AllocSlice& sl) {
                for (const auto& write_sl : write_slices) {
                    if (!phv.field_mutex()(write_sl.field()->id, sl.field()->id) &&
                        !write_sl.isLiveRangeDisjoint(sl)) {
                        return false;
                    }
                }
                return true;
            };

            ordered_set<PHV::AllocSlice> live_slices;
            for (const auto& sl : alloc.slices(c, stage, use)) {
                if (is_all_mutex(sl)) continue;
                live_slices.insert(sl);
            }
            auto op_type = container_operation_type(action, live_slices, {});
            if (op_type == OperandInfo::MOVE) {
                auto err =
                    check_move_constraints(
                            alloc, action, candidates, live_slices, c, initActions);
                if (!err.ok()) {
                    return err;
                }
            }
        }
    }
    return CanPackErrorV2{CanPackErrorCode::NO_ERROR};
}

CanPackErrorV2 ActionPhvConstraints::check_read_action_num_source_constraints(
    const PHV::Allocation& alloc,
    const std::vector<PHV::AllocSlice>& candidates,
    const IR::MAU::Action* action) const {
    for (const int stage : stages(action, candidates.front().isPhysicalStageBased())) {
        // Gather all containers written by any of the slices in candidates
        //
        // A destination field won't be checked _in this function_ if it's not placed yet.
        // However, an unplaced destination field _will_ be checked in can_pack when they are
        // finally placed. In that case, can_pack looks at all the writes into that container.
        ordered_set<PHV::Container> dests;
        const auto use = PHV::FieldUse(PHV::FieldUse::WRITE);
        for (auto slice : candidates)
            for (auto write : constraint_tracker.destinations(slice, action))
                for (auto writeSlice : alloc.slices(write.field(), write.range(), stage, use))
                    if (writeSlice.container()) dests.emplace(writeSlice.container());

        // Walk through the destination containers one-by-one and check their validity
        for (const auto dest : dests) {
            const bool mocha_or_dark = dest.is(PHV::Kind::mocha) || dest.is(PHV::Kind::dark);
            size_t num_adc = 0;
            ordered_set<PHV::Container> all_source_containers;
            for (auto sl : alloc.slices(dest, stage, use)) {
                LOG5("\tWrite to " << sl.field()->name << " " << sl.field_slice() << " "
                                   << sl.container());
                for (const auto operand : constraint_tracker.sources(sl, action)) {
                    if (operand.ad || operand.constant) {
                        LOG6("\t\t\t\t" << operand << " is action data/constant source");
                        num_adc = 1;
                        continue;
                    }
                    const PHV::Field* src_field = operand.phv_used->field();
                    le_bitrange src_field_range = operand.phv_used->range();
                    ordered_set<PHV::Container> src_containers;
                    ordered_set<PHV::AllocSlice> src_alloc = alloc.slices(
                        src_field, src_field_range, stage, PHV::FieldUse(PHV::FieldUse::READ));
                    for (auto source : candidates) {
                        if (source.field() == src_field &&
                            source.field_slice().overlaps(src_field_range) &&
                            source.isLiveAt(stage, PHV::FieldUse(PHV::FieldUse::READ))) {
                            src_alloc.insert(source);
                        }
                    }

                    for (auto source_slice : src_alloc) {
                        src_containers.insert(source_slice.container());
                        LOG5("\t\t\t\t\tSource slice for " << sl << " : " << source_slice);
                    }

                    if (src_containers.size() == 0) {
                        LOG1("\t\t\t\tSource " << *(operand.phv_used)
                                               << " has not been allocated yet.");
                    } else {
                        all_source_containers.insert(src_containers.begin(), src_containers.end());
                    }
                }
            }

            size_t total_sources = num_adc + all_source_containers.size();
            if (total_sources > 2) {
                std::stringstream ss;
                ss << "Action " << action->name
                   << " uses more than two PHV sources to write to container " << dest << ".";
                LOG5("\t\t\t\t" << ss.str());
                return {CanPackErrorCode::MORE_THAN_TWO_SOURCES, ss.str()};
            } else if (mocha_or_dark && total_sources > 1) {
                std::stringstream ss;
                ss << "Action " << action->name
                   << " uses more than one PHV sources to write to container " << dest << ".";
                LOG5("\t\t\t\tAction " << ss.str());
                return {CanPackErrorCode::TF2_MORE_THAN_ONE_SOURCE, ss.str()};
            }
        }
    }

    return CanPackErrorV2{CanPackErrorCode::NO_ERROR};
}

CanPackErrorV2 ActionPhvConstraints::check_move_constraints_from_read(
    const PHV::Allocation& alloc, const std::vector<PHV::AllocSlice>& candidates,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    ordered_set<const IR::MAU::Action*> verified;
    for (const auto& slice : candidates) {
        for (const auto& action : constraint_tracker.read_in(slice)) {
            if (verified.count(action)) {
                continue;
            }
            verified.insert(action);
            LOG5("\t\t\tCheck action " << action->name << " because its source "
                                       << slice.field()->name << " is being allocated.");
            // check move-based instruction constraints
            auto err = check_read_action_move_constraints(
                    alloc, candidates, action, initActions);
            if (!err.ok()) {
                std::stringstream ss;
                ss << "In action " << action->externalName()
                   << ", allocation candidate is used as source operand, move "
                      "instruction constraints will be violated: "
                   << err.msg;
                err.msg = ss.str();
                return err;
            }
            // check other constraints.
            err = check_read_action_num_source_constraints(alloc, candidates, action);
            if (!err.ok()) {
                return err;
            }
        }
    }
    return {CanPackErrorCode::NO_ERROR, ""};
}

CanPackErrorCode ActionPhvConstraints::check_ara_move_constraints(
    const PHV::Allocation& alloc,
    const PHV::Allocation::MutuallyLiveSlices& container_state, const PHV::Container& c,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    using namespace solver;
    auto* solver = make_solver(c);
    // Unfornately, due to the current implementation, there can be non-mutex alloc slices
    // in container_state, when dark overlay is involved. For example, you might see
    // MH10 bit[15..0] <--f1<16> mocha [0:15] live at [9w, 11r] { always_run; 0 actions }
    // MH10 bit[0]     <--f2<1>  mocha [0:0] live at [5w, 9r]
    // DH9 bit[15..0]  <--f1<16> mocha [0:15] live at [5w, 9r]
    // in container_state when we dark swap out f1 to DH9.
    // So, we need to filter out all ara actions and check fields that lives.
    ordered_set<const PHV::AllocSlice*> ara_slices;
    for (const auto& slice : container_state) {
        const auto& prim = slice.getInitPrimitive();
        // alloc slice without source slices are slices that was swapped out, e.g.,
        // DH9 bit[15..0] <-- f1 live at [5w, 9r] = MH10 f2 live at [-1w, 5r].
        if (prim.isAlwaysRunActionPrim() && prim.getSourceSlice()) {
            ara_slices.insert(&slice);
        }
    }
    for (const auto* ara_slice : ara_slices) {
        LOG3("check ARA on slice: " << ara_slice);
        solver->clear();
        solver->enable_bitmasked_set(false);  // ara cannot do bitmasked set.
        const auto dest_live =
            compute_dest_live_bv(uses, alloc, initActions, container_state,
                                 ara_slice->getEarliestLiveness().first, nullptr);
        LOG5("dest live bits bitvec after ARA: " << dest_live);
        solver->set_container_spec(c.toString(), c.size(), dest_live);
        for (const auto& slice : container_state) {
            // TODO: It seems that we are adding dark init for pure padding fields.
            // We should fix it in darkInit and change this to a bug check?.
            if (!uses.is_referenced(slice.field()) || slice.field()->padding) {
                continue;
            }
            // Because we do not have a fixed ARA stage, we have to use earliest liveness
            // as a guess of the actual stage of ARA.
            if (slice.getEarliestLiveness() != ara_slice->getEarliestLiveness()) {
                continue;
            }
            const auto& prim = slice.getInitPrimitive();
            const auto dest = make_container_operand(c.toString(), slice.container_slice());
            if (prim.isAlwaysRunActionPrim()) {
                if (auto src_slice = prim.getSourceSlice()) {
                    const auto src_container = src_slice->container();
                    const auto src = make_container_operand(src_container.toString(),
                                                            src_slice->container_slice());
                    if (src_container != c) {
                        solver->set_container_spec(src_container.toString(), src_container.size(),
                                                   bitvec());
                    }
                    LOG5("add ARA dark container move to " << c << " from " << src);
                    solver->add_assign(dest, src);
                } else if (prim.destAssignedToZero()) {
                    LOG5("add dark container init from zero");
                    solver->add_assign(dest, make_ad_or_const_operand());
                }
            }
        }
        auto rst = solver->solve();
        if (!rst.ok()) {
            LOG2("ARA Solver Error Reason: " << rst.err->msg);
            return CanPackErrorCode::CONSTRAINT_CHECKER_FAILED;
        }
        if (LOGGING(4)) {
            LOG4("ARA instructions: ");
            for (const auto* inst : rst.instructions) {
                LOG4(inst->to_cstring());
            }
        }
    }
    return CanPackErrorCode::NO_ERROR;
}

CanPackErrorV2 ActionPhvConstraints::check_move_constraints(
    const PHV::Allocation& alloc, const IR::MAU::Action* action,
    const std::vector<PHV::AllocSlice>& slices,
    const PHV::Allocation::MutuallyLiveSlices& container_state, const PHV::Container& c,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions) const {
    if (LOGGING(5)) {
        LOG5("check_move_constraints:");
        LOG5("action: " << action->name);
        for (const auto& slice : container_state) {
            LOG5("slice: " << slice);
        }
    }
    using namespace solver;
    auto* solver = make_solver(c);

    // check metaInit for dest slice, if found, add assignment to solver.
    const auto add_meta_init_assign = [&](const IR::MAU::Action* action,
                                          const PHV::AllocSlice& slice,
                                          const Operand& dest) {
        if (initActions.count(slice.field()) && initActions.at(slice.field()).count(action)) {
            LOG5("add metadata init to " << dest);
            solver->add_assign(dest, make_ad_or_const_operand());
        } else if (alloc.getMetadataInits(action).count(slice.field())) {
            LOG5("add metadata init to " << dest);
            solver->add_assign(dest, make_ad_or_const_operand());
        }
    };

    // check dark swap slices. AlwaysRun inits in getInitPrimitive are ignored here
    // because those slices were not initialized through actions.
    const auto add_dark_init_assign = [&](const IR::MAU::Action* action,
                                          const PHV::AllocSlice& slice,
                                          const Operand& dest) {
        const auto& dark_prim = slice.getInitPrimitive();
        if (!dark_prim.isAlwaysRunActionPrim()) {
            if (dark_prim.getInitPoints().count(action)) {
                auto src_slice = dark_prim.getSourceSlice();
                if (src_slice) {
                    const auto src_container = src_slice->container();
                    const auto src = make_container_operand(
                            src_container.toString(), src_slice->container_slice());
                    if (src_container != c) {
                        solver->set_container_spec(
                                src_container.toString(), src_container.size(), bitvec());
                    }
                    LOG5("add dark container move to " << dest << " from " << src);
                    solver->add_assign(dest, src);
                } else if (dark_prim.destAssignedToZero()) {
                    LOG5("add dark container init from zero");
                    solver->add_assign(dest, make_ad_or_const_operand());
                }
            }
        }
    };

    // add assignment to dest in the action.
    const auto add_action_assign = [&](const IR::MAU::Action* action,
                                       const PHV::AllocSlice& dest_slice,
                                       const Operand& dest,
                                       const int stage) {
        // ignore slices that are not live for this action.
        // earliest is always a write, if write after this stage, ignore
        // latest is always a read, if read at or before this stage, ignore
        if (!dest_slice.isLiveAt(stage, PHV::FieldUse(PHV::FieldUse::WRITE))) {
            return;
        }
        const auto sources = constraint_tracker.sources(dest_slice, action);
        if (sources.empty()) {
            return;
        }
        BUG_CHECK(sources.size() == 1,
            "dest_slice %1% is not fine-sliced. Multiple sources found: %2%\n%3%",
             dest_slice.toString(), sources.size(), sources);
        const auto& operand = sources.front();
        LOG5("has op: " << operand);
        if (operand.ad || operand.constant) {
            LOG5("add ad_or_const move to " << dest);
            solver->add_assign(dest, make_ad_or_const_operand());
        } else {
            const auto src_phv = getSourcePHVSlice(alloc, slices, dest_slice, action, stage);
            if (!src_phv) {
                // TODO: do not add unallocated assignment if src_phv can be
                // overlaid with dest because of disjoint live range.
                // We will need to add physical live range as a dep of action constraints,
                // and potentially the function that checks overlay.
                // Without ^, we might prune some vary rare but possible case.
                solver->add_src_unallocated_assign(dest.container, dest.range);
                return;
            }
            const auto src_container = (*src_phv).container();
            const auto src =
                make_container_operand(src_container.toString(), (*src_phv).container_slice());
            LOG5("add container move to " << dest << " from " << src);
            if (src_container != c) {
                solver->set_container_spec(src_container.toString(), src_container.size(),
                                           bitvec());
            }
            solver->add_assign(dest, src);
        }
    };

    bool clear_align = false;
    bool enable_bitmasked_set = true;
    const auto* tbl = constraint_tracker.action_table(action);
    if (tbl->match_key.empty()) {
        GetActionRequirements ghdr;
        action->apply(ghdr);
        if (ghdr.is_hash_dist_needed() || ghdr.is_rng_needed())
            enable_bitmasked_set = false;  // Keyless table with hash cannot have action data
    }
    const auto action_stages = stages(action, slices.front().isPhysicalStageBased());
    for (const auto& stage : action_stages) {
        solver->clear();
        LOG5("check action " << action->name << " for " << c << " @ stage " << stage);
        solver->enable_bitmasked_set(enable_bitmasked_set);
        const auto dest_live =
            compute_dest_live_bv(uses, alloc, initActions, container_state, stage, action);
        LOG5("dest after action live bitvec: " << dest_live);
        solver->set_container_spec(c.toString(), c.size(), dest_live);
        for (const auto& slice : container_state) {
            // There can be dark slices in container state even if c is a normal container,
            // because of the current implementation of container_states. Those slice
            // should be just ignored. TODO: split them out to dark solver?
            if (slice.container() != c) {
                continue;
            }
            if (!uses.is_referenced(slice.field()) || slice.field()->padding) {
                continue;
            }
            // When placing upcasted source field into dark/mocha, the write must cover the
            // whole container: all (msb) container bits should not be clobbered. Because otherwise
            // the instruction, that read this container for the upcasted field, will read
            // corrupted bits. We check it by setting all bits of destination
            // to be *live*, (meaning cannot be clobbered), to solver.
            if (c.type().kind() == PHV::Kind::mocha || c.type().kind() == PHV::Kind::dark){
                if (slice.field()->is_upcasted()) {
                    const auto sources = constraint_tracker.sources(slice, action);
                    if (!sources.empty()) {
                        const auto& operand = sources.front();
                        // Action Data or Constant set the entire container with padding
                        if (!operand.ad && !operand.constant)
                            solver->set_container_spec(c.toString(), c.size(), bitvec(0, c.size()));
                    }
                }
            }
            const auto dest = make_container_operand(c.toString(), slice.container_slice());
            LOG5("check dest slice: " << slice << " as " << dest);
            add_meta_init_assign(action, slice, dest);
            add_dark_init_assign(action, slice, dest);
            add_action_assign(action, slice, dest, stage);
        }
        auto rst = solver->solve();
        if (!rst.ok()) {
            LOG2("Solver Error Reason: " << rst.err->msg);
            // construct invalid-packing slices, which are slices live in the container
            // for this action.
            auto* invalid_packing = new std::vector<PHV::AllocSlice>();
            for (const auto& sl : container_state) {
                if (sl.container() != c) {
                    continue;
                }
                if (!sl.isLiveAt(stage, PHV::FieldUse(PHV::FieldUse::WRITE))) {
                    continue;
                }
                invalid_packing->push_back(sl);
            }
            return {CanPackErrorCode::CONSTRAINT_CHECKER_FAILED, rst.err->msg, invalid_packing};
        }
        int num_instrs = rst.instructions.size();
        LOG4("instructions: ");
        for (const auto* inst : rst.instructions) {
            clear_align = (num_instrs == 1) && (inst->name() == "byte-rotate-merge");
            LOG4(inst->to_cstring());
        }
    }

    return CanPackErrorV2{CanPackErrorCode::NO_ERROR, clear_align};
}

std::ostream &operator<<(std::ostream &out, const ActionPhvConstraints::ClassifiedSource& src) {
    out << (boost::format("{ exist: %1%, container: %2%, aligned: %3%, offset: %4%, ") %
            src.exist % src.container % src.aligned % src.offset);
    out << "slices: [";
    for (auto sl : src.slices) out << sl << ", ";
    out << "]}";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionPhvConstraints::OperandInfo& info) {
    out << "[ ";
    if (info.ad)
        out << "ACTION DATA ";
    if (info.constant)
        out << "CONST " << info.const_value << " ";
    if (info.phv_used)
        out << *info.phv_used << " ";
    if (!info.ad && !info.constant && !info.phv_used)
        out << "INVALID OPERAND INFO ";
    out << "]";
    out << "[ ";
    if (info.flags & ActionPhvConstraints::OperandInfo::MOVE)
        out << " MOVE ";
    if (info.flags & ActionPhvConstraints::OperandInfo::BITWISE)
        out << " BITWISE ";
    if (info.flags & ActionPhvConstraints::OperandInfo::WHOLE_CONTAINER)
        out << " WHOLE ";
    if (info.flags & ActionPhvConstraints::OperandInfo::PART_OF_CONTAINER)
        out << " PART ";
    if (info.flags & ActionPhvConstraints::OperandInfo::ANOTHER_OPERAND)
        out << " ANOTHER ";
    if (info.flags & ActionPhvConstraints::OperandInfo::MIXED)
        out << " MIXED ";
    if (info.flags & ActionPhvConstraints::OperandInfo::WHOLE_CONTAINER_SAME_FIELD)
        out << " SAME ";
    out << "]";
    return out;
}

std::ostream &
operator<<(std::ostream &out, const safe_vector<ActionPhvConstraints::OperandInfo>& info) {
    for (auto i : info) {
        out << i << "\n";
    }
    return out;
}

cstring can_pack_error_code_str(const CanPackErrorCode &code) {
    switch (code) {
        case CanPackErrorCode::NO_ERROR:
            return "NO_ERROR"_cs;
        case CanPackErrorCode::SLICE_EMPTY:
            return "SLICE_EMPTY"_cs;
        case CanPackErrorCode::PACK_CONSTRAINT_PRESENT:
            return "PACK_CONSTRAINT_PRESENT"_cs;
        case CanPackErrorCode::STATEFUL_DEST_CONSTRAINT:
            return "STATEFUL_DEST_CONSTRAINT"_cs;
        case CanPackErrorCode::BITMASK_CONSTRAINT:
            return "BITMASK_CONSTRAINT"_cs;
        case CanPackErrorCode::SPECIALTY_DATA:
            return "SPECIALTY_DATA"_cs;
        case CanPackErrorCode::MIXED_OPERAND:
            return "MIXED_OPERAND"_cs;
        case CanPackErrorCode::NONE_ADJACENT_FIELD:
            return "NONE_ADJACENT_FIELD"_cs;
        case CanPackErrorCode::COMPLEX_AD_PACKING:
            return "COMPLEX_AD_PACKING"_cs;
        case CanPackErrorCode::BITWISE_MIXED_AD:
            return "BITWISE_MIXED_AD"_cs;
        case CanPackErrorCode::TF2_MORE_THAN_ONE_SOURCE:
            return "TF2_MORE_THAN_ONE_SOURCE"_cs;
        case CanPackErrorCode::TF2_ALL_WRITTEN_TOGETHER:
            return "TF2_ALL_WRITTEN_TOGETHER"_cs;
        case CanPackErrorCode::MORE_THAN_TWO_SOURCES:
            return "MORE_THAN_TWO_SOURCES"_cs;
        case CanPackErrorCode::TWO_SOURCES_AND_CONSTANT:
            return "TWO_SOURCES_AND_CONSTANT"_cs;
        case CanPackErrorCode::MOVE_AND_UNALLOCATED_SOURCE:
            return "MOVE_AND_UNALLOCATED_SOURCE"_cs;
        case CanPackErrorCode::BITWISE_AND_UNALLOCATED_SOURCE:
            return "BITWISE_AND_UNALLOCATED_SOURCE"_cs;
        case CanPackErrorCode::SLICE_ALIGNMENT:
            return "SLICE_ALIGNMENT"_cs;
        case CanPackErrorCode::PACK_AND_ALIGNED:
            return "PACK_AND_ALIGNED"_cs;
        case CanPackErrorCode::INVALID_MASK:
            return "INVALID_MASK"_cs;
        case CanPackErrorCode::SLICE_DIFF_OFFSET:
            return "SLICE_DIFF_OFFSET"_cs;
        case CanPackErrorCode::COPACK_UNSATISFIED:
            return "COPACK_UNSATISFIED"_cs;
        case CanPackErrorCode::MULTIPLE_ALIGNMENTS:
            return "MULTIPLE_ALIGNMENTS"_cs;
        case CanPackErrorCode::OVERLAPPING_SLICES:
            return "OVERLAPPING_SLICES"_cs;
        case CanPackErrorCode::CONSTRAINT_CHECKER_FAILED:
            return "CONSTRAINT_CHECKER_FAILED"_cs;
        default:
            BUG("Invalid packing error code");
    }
}

std::ostream &operator<<(std::ostream &out, const CanPackErrorCode &ec) {
    out << " CanPackErrorCode: ";
    out << can_pack_error_code_str(ec);
    return out;
}
