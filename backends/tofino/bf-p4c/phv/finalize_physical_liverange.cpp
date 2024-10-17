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

#include "bf-p4c/phv/finalize_physical_liverange.h"
#include <sstream>
#include <utility>
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/device.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/parde/clot/clot.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/slice_alloc.h"
#include "lib/exceptions.h"
#include "lib/safe_vector.h"

namespace PHV {

namespace {

/// return all overlapping AllocSlices without taking subslice based on range. We need to use
/// these AllocSlices as keys for updating live ranges.
safe_vector<AllocSlice> find_all_overlapping_alloc_slices(
        const Field* f, const le_bitrange range, const PHV::AllocContext* ctx, bool is_write) {
    safe_vector<AllocSlice> rst;
    const PHV::FieldUse use(is_write ? PHV::FieldUse::WRITE : PHV::FieldUse::READ);
    f->foreach_alloc(StartLen(0, f->size), ctx, &use, [&](const PHV::AllocSlice& slice) {
        if (slice.field_slice().overlaps(range)) {
            rst.push_back(slice);
        }
    });
    return rst;
}

StageAndAccess to_stage_and_access(const IR::BFN::Unit* unit,
                                   const bool is_write,
                                   const PHV::Field* f) {
    if (unit->is<IR::BFN::ParserState>() || unit->is<IR::BFN::Parser>() ||
        unit->is<IR::BFN::GhostParser>()) {
        return {-1, FieldUse(FieldUse::WRITE)};
    } else if (unit->is<IR::BFN::Deparser>()) {
        return {Device::numStages(), FieldUse(FieldUse::READ)};
    } else if (const auto* t = unit->to<IR::MAU::Table>()) {
        return {t->stage(), FieldUse(is_write ? FieldUse::WRITE : FieldUse::READ)};
    } else {
        BUG("unknown unit: %1%, field: %2%", unit, f);
    }
}

}  // namespace

void FinalizePhysicalLiverange::update_liverange(const safe_vector<AllocSlice>& slices,
                                                 const StageAndAccess& op) {
    for (const auto& slice : slices) {
        // update AllocSlice-based live range.
        if (slice.field()->pov) {
            // POV fields are marked as live through the whole pipeline, do not adjust based on
            // usage.
            continue;
        }
        LOG1("update live range of " << slice << ": " << op);
        if (auto it = live_ranges_i.find(slice); it != live_ranges_i.end()) {
            it->second.extend(op);
        } else {
            live_ranges_i.emplace(slice, LiveRange(op, op));
        }
    }
}


bool FinalizePhysicalLiverange::preorder(const IR::BFN::Pipe* pipe) {
    BUG_CHECK(pipe->headerStackInfo != nullptr,
              "Running FinalizePhysicalLiverange without running CollectHeaderStackInfo first?");
    headerStacks = pipe->headerStackInfo;
    return true;
}

/// collect table to stage.
bool FinalizePhysicalLiverange::preorder(const IR::MAU::Table* t) {
    tables_i.insert(t);
    table_stages_i[TableSummary::getTableIName(t)].insert(t->stage());
    return true;
}

void FinalizePhysicalLiverange::mark_access(const PHV::Field* f, le_bitrange bits,
                                            const IR::BFN::Unit* unit, bool is_write,
                                            bool allow_unallocated) {
    // skip clot-allocated unused fields.
    if (clot_i.fully_allocated(FieldSlice(f, bits))) {
        return;
    }
    const StageAndAccess access = to_stage_and_access(unit, is_write, f);
    const auto alloc_slices =
        find_all_overlapping_alloc_slices(f, bits, AllocContext::of_unit(unit), is_write);
    if (alloc_slices.empty() && !allow_unallocated) {
        // (1) Temp vars will be allocated later.
        // (2) It is safe to ignore access in deparser when
        //     clot_i.allocated_unmodified_undigested(f) is true.
        //     It means that the live range of the field is shrunken due to clot allocation.
        // (3) We did not handle digest type field perfectly so that there might be
        //     duplicated padding fields added, although allocation are still correct.
        //     An example case:
        //     remove `(f->padding && f->is_digest())` and run p4_16_samples/issue1043-bmv2.p4
        BUG_CHECK(
            phv_i.isTempVar(f) ||
            (unit->is<IR::BFN::Deparser>() && clot_i.allocated_unmodified_undigested(f)) ||
            (f->padding && f->is_digest()),
            "cannot find corresponding slice, field: %1%, range: %2%, is_write: %3%.\n unit: %4%",
            f, bits, is_write, unit);
        // memorize live range of unallocated temp vars.
        if (phv_i.isTempVar(f)) {
            if (temp_var_live_ranges_i.count(f)) {
                temp_var_live_ranges_i.at(f).extend(access);
            } else {
                temp_var_live_ranges_i.emplace(f, LiveRange(access, access));
            }
        }
        return;
    }

    update_liverange(alloc_slices, access);

    // Handle aliased fields
    if (const PHV::Field* alias_dest = phv_i.getAliasDestination(f)) {
        LOG2("   field " << f->name << " has alias dest: " << alias_dest->name);
        const auto dest_slices =
            find_all_overlapping_alloc_slices(alias_dest, bits, AllocContext::of_unit(unit),
                                              is_write);
        update_liverange(dest_slices, access);
    }
}

// The reason to find all of extract 2^n to xxx.$stkvalid is that, in ValidToStkvalid
// IR::Node* postorder(IR::BFN::Extract* extract), it did an irreversible alias replacement.
// All of `extract 1 to xxx[n].$valid` is replaced by `extract 2^n to xxx.$stkvalid`. And later on
// ReinstateAliasSources does not recover this part of replacement. I have tried to replace all
// `extract 2^n to xxx.$stkvalid` to `extract 1 to xxx[n].$valid` in ReinstateAliasSources, but it
// breaks parser and causes stf failures. Therefore, I need to recover this part of live range
// information here. Once I see a `extract 2^n to xxx.$stkvalid`, I construct a `xxx[n].valid` expr
// and mark field as being written in parser. As for xxx.$stkvalid's live range, it is captured in
// FinalizePhysicalLiverange::preorder(const IR::Expression* e).
bool FinalizePhysicalLiverange::preorder(const IR::BFN::Extract* extract) {
    auto* fieldLVal = extract->dest->to<IR::BFN::FieldLVal>();
    if (!fieldLVal) return true;
    auto* member = fieldLVal->field->to<IR::Member>();
    if (!member) return true;
    if (member->member.toString() != "$stkvalid") return true;

    LOG9("$stkvalid extract: " << extract);
    auto* src = extract->source->to<IR::BFN::ConstantRVal>();
    BUG_CHECK(src, "extract non-constant to validity bit");
    BUG_CHECK(src->constant->fitsUint(), "Constant too big in extract: %1%", extract);
    const auto src_value = bitvec(src->constant->asUnsigned());

    BUG_CHECK(headerStacks != nullptr, "No HeaderStackInfo; was FinalizePhysicalLiverange "
              "applied to a non-Pipe node?");
    auto stack = member->expr->toString();
    auto* stackInfo = headerStacks->get(stack);
    BUG_CHECK(stackInfo, "No HeaderStackInfo for %1% which is needed for %2%",
              stack, extract);

    // Parde/StackPushShims can create writes to $stkvalid which are not power of 2 constant.
    // These writes are writes of "push_bits" that are used to implement stack push.
    // @see HeaderPushPop for more details.
    for (auto stkvalid_bit_index : src_value) {
        // prepare the valid bit expression as xxx[n].$valid
        // The layout of $stkvalid is [push_bits . valid_bits . pop_bits] so the index 0 of stack
        // corresponds to bit position 1 << stackInfo->maxpop.
        auto stack_index = stkvalid_bit_index - stackInfo->maxpop;
        if (stack_index < 0 || stack_index > stackInfo->size)
            continue;  // write to popbits/pushbits -> does not correspond to stack index
        auto valid_bit_expr = new IR::Member(
            IR::Type_Bits::get(1),
            new IR::HeaderStackItemRef(member->expr, new IR::Constant(stack_index)),
            IR::ID("$valid"));
        const auto* unit = findContext<IR::BFN::Unit>();
        le_bitrange bits;
        auto f = phv_i.field(valid_bit_expr, &bits);
        if (!f)
            continue;
        const bool allow_unallocated = unit->is<IR::BFN::ParserState>()
            || unit->is<IR::BFN::Parser>()
            || unit->is<IR::BFN::GhostParser>();
        LOG5("$stkvalid to index " << stack_index << " for " << extract);
        mark_access(f, bits, unit, true, allow_unallocated);
    }
    return true;
}

bool FinalizePhysicalLiverange::preorder(const IR::Expression* e) {
    LOG5("FinalizePhysicalLiverange preorder : " << e);;
    le_bitrange bits;
    const PhvInfo& phv = phv_i;  // const ref to ensure that we do not create new fields.
    auto* f = phv.field(e, &bits);
    if (!f) {
        LOG5("Found non-field expr: " << e);
        return true;
    }
    LOG5("Found " << f->name << " " << bits << " from " << e);
    const auto* unit = findContext<IR::BFN::Unit>();
    BUG_CHECK(unit, "cannot find unit: %1%", e);
    const bool is_write = isWrite();
    // There could be unallocated parser temp vars that are not eliminated before lowering.
    const bool allow_unallocated = unit->is<IR::BFN::ParserState>() ||
                                   unit->is<IR::BFN::Parser>() ||
                                   unit->is<IR::BFN::GhostParser>();
    mark_access(f, bits, unit, is_write, allow_unallocated);
    // NOTE: We do not need to visit alias source/destinations because we have already reinstate
    // all aliased field in IR.
    return false;
}

void FinalizePhysicalLiverange::end_apply() {
    for (auto field : defuse_i.getUninitializedFields()) {
        if (pa_no_init.getFields().count(field)) continue;
        bool found_implicit_parser_init = false;
        for (auto locpair : defuse_i.getAllDefs(field->id)) {
            auto location = locpair.first;
            auto exp = locpair.second;
            if (exp->is<ImplicitParserInit>()) {
                BUG_CHECK(location->is<IR::BFN::ParserState>(), "ImplicitParserInit not in parser");
                mark_access(field, le_bitrange(0, field->size - 1), location, true, true);
                found_implicit_parser_init = true;
            }
        }
        BUG_CHECK(found_implicit_parser_init, "ImplicitParserInit not found");
    }

    // mark parser_err as written in parser stage
    for (const auto &fieldName : FieldDefUse::write_by_parser) {
        const PHV::Field *field = phv_i.field(fieldName);
        if (!field) continue;
        for (const auto &locpair : defuse_i.getAllDefs(field->id)) {
            if (locpair.first->is<IR::BFN::ParserState>() || locpair.first->is<IR::BFN::Parser>()) {
                mark_access(field, le_bitrange(0, field->size - 1), locpair.first, true, true);
            }
        }
    }

    // update AllocSlices' live range.
    for (auto& f : phv_i) {
        bool have_read_to_read_live_range = false;
        std::vector<AllocSlice> live_start_rw;
        std::set<AllocSlice> uninit_read_allocs;
        std::multiset<AllocSlice> all_slices;
        for (auto& slice : f.get_alloc()) {
            BUG_CHECK(slice.isPhysicalStageBased(), "FinalizePhysicalLiverange can only be applied"
                    " on physical-stage AllocSlices (%1%)", slice.toString());
            // Although it should be safe to drop allocated but unreferenced slices,
            // but because most analysis pass were built on field-level, dropping them
            // will make some later passes fail. For example,
            // the CheckForUnallocated pass, which is using phv_uses.
            // LOG here is a good place to see how many bits we can save by updating those
            // passed to fieldslice-level.
            if (!live_ranges_i.count(slice)) {
                LOG1("Found allocated but unreferenced AllocSlice: " << slice);
            } else {
                if (slice.getEarliestLiveness().second.isOnlyReadAndNotLive() &&
                    slice.getLatestLiveness().second.isOnlyReadAndNotLive()) {
                    have_read_to_read_live_range = true;
                }
                const auto old = LiveRange(slice.getEarliestLiveness(), slice.getLatestLiveness());
                const auto& updated = live_ranges_i.at(slice);
                LOG3("Adjusting " << old << " => " << updated << " : " << slice);
                slice.setLiveness(updated.start, updated.end);
            }
            all_slices.emplace(slice);
        }

        // Merge slices with overlapping live ranges
        safe_vector<PHV::AllocSlice> new_slices;
        if (all_slices.size()) {
            int merge_count = 0;

            PHV::AllocSlice merged(*all_slices.begin());
            const auto invalid_liveness =
                std::make_pair(-2 /* never used stage */, PHV::FieldUse(PHV::FieldUse::READ));
            merged.setEarliestLiveness(invalid_liveness);
            merged.setLatestLiveness(invalid_liveness);

            auto record_slice = [&new_slices](const PHV::AllocSlice& slice, int count) {
                if (slice.getEarliestLiveness().first >= -1) {
                    new_slices.push_back(slice);
                    if (count > 1) LOG3("Merging " << count << " slices to produce " << slice);
                }
            };

            for (auto& slice : all_slices) {
                if (slice.container() == merged.container() &&
                    slice.representsSameFieldSlice(merged) && !slice.isLiveRangeDisjoint(merged)) {
                    merged.setEarliestLiveness(
                        std::min(slice.getEarliestLiveness(), merged.getEarliestLiveness()));
                    merged.setLatestLiveness(
                        std::max(slice.getLatestLiveness(), merged.getLatestLiveness()));
                    merge_count++;
                } else {
                    record_slice(merged, merge_count);
                    merged = slice;
                    merge_count = 1;
                }
            }
            record_slice(merged, merge_count);
            f.set_alloc(new_slices);
            f.sort_alloc();
        }

        for (auto& slice : f.get_alloc()) {
            if (slice.getEarliestLiveness().second.isReadAndWrite()) {
                LOG5("Found RW start of liverange for" << slice);
                live_start_rw.push_back(slice);
                int stg = slice.getEarliestLiveness().first;
                slice.setEarliestLiveness(std::make_pair(stg, FieldUse(FieldUse::WRITE)));
            }
        }

        // For the liveranges starting with RW determine if other liveranges cover the R part
        // - If not then create read-only slice for the uninitialized R liverange
        for (auto& sl : live_start_rw) {
            auto rng = sl.field_slice();
            int stg = sl.getEarliestLiveness(). first;
            bool covered = false;

            for (auto& slice : f.get_alloc()) {
                if (sl == slice) continue;
                if (!rng.overlaps(slice.field_slice())) continue;

                // Check if slice's liverange covers stage stg
                if ((slice.getEarliestLiveness().first < stg ||
                    (slice.getEarliestLiveness().first == stg &&
                     slice.getEarliestLiveness().second.isRead())) &&
                    slice.getLatestLiveness().first >= stg) {
                    covered = true;
                    BUG_CHECK(sl.field_slice() == slice.field_slice(),
                              "Partial range coverage of %1% by %2% in stage %3%", sl, slice, stg);
                    break;
                }
            }

            if (!covered) {
                PHV::AllocSlice uninit_alloc(sl);
                uninit_alloc.setIsPhysicalStageBased(true);
                StageAndAccess singleRd = std::make_pair(stg, FieldUse(FieldUse::READ));
                uninit_alloc.setLiveness(singleRd, singleRd);
                LOG4("\t  creating slice: " << uninit_alloc);
                uninit_read_allocs.insert(uninit_alloc);\
            }
        }

        // Add the uninitialized read-only AllocSlice's to the respective field
        for (auto uninit_sl : uninit_read_allocs) {
            f.add_alloc(uninit_sl);
        }

        // A corner case is that table a has a instruction a = a | 1 and a is uninitialized and
        // deparser reads a. Then there will be two live ranges, first is a read to read live range
        // (from table a to table a) and second is a normal write to read live range(from table a
        // to deparser). If table a is allocated to stage 0, 1 and 2, then after finalizing physical
        // live range, these two live ranges will become [0r, 2r] and [0w, deparser]. And this is
        // invalid, because AllocSlice should not have overlapping live ranges. To fix it,
        // the first live range should become [0r, 0r], because it is an invalid live range at first
        // place and shrinking this live range will cause no damage, because the second will access
        // stage 1 and 2.
        if (have_read_to_read_live_range) {
            bitvec normal_live_range;
            // It fills the bitvec with all normal live ranges. This bitvec keeps tracks of the
            // read status of the live range. If stage 1 is marked as occupied, it means that it
            // is possible that this fieldslice is read on stage 1. For the example I used in
            // the comment, the normal live range is [0w, deparser], I will fill stage 1 to the
            // end as occupied. The reason not to fill stage 0 is because in the calculation I
            // only care about read. [0w, deparser] means that is not read on stage 0, but
            // possibly being read on stage 1 onwards. And [..., 0r] and [0w, ...] do not count
            // for overlapping live range, because in tofino, it always reads then writes a
            // fieldslice.
            for (auto& slice : f.get_alloc()) {
                if (!(slice.getEarliestLiveness().second.isOnlyReadAndNotLive() &&
                    slice.getLatestLiveness().second.isOnlyReadAndNotLive())) {
                    normal_live_range.setrange(slice.getEarliestLiveness().first + 1,
                        slice.getLatestLiveness().first - slice.getEarliestLiveness().first);
                }
            }
            // It fills a bitvec for every read to read live range, calculates the live range that
            // is not overlapping with the normal live range and updates the live range. The reason
            // that the read to read live range can be shrunk arbitrarily(from [0r, 2r] to [0r, 0r])
            // is because read to read live range is generated due to uninitialized read and
            // hardware can have random behavior for uninitialized read. The part that strictly
            // checks read/write access is in assembler. If assembler sees a field is read on
            // stage x(even it is an uninitialized read), but phv definition in bfa file does not
            // correctly define the live range to include stage x, it will complain. Therefore, we
            // give uninitialized read a seemingly correct live range so that assembler can pass the
            // check. As long as there is a normal live range reading the value on the stage (in
            // this case, it is stage 1, 2) that is also included in read to read live range, read
            // to read live range can remove this stage from its live range, since the read access
            // will be correctly represented in bfa file by the normal live range.
            for (auto& slice : f.get_alloc()) {
                // only consider uninit read on table that is allocated on multiple stages.
                if (slice.getEarliestLiveness().second.isOnlyReadAndNotLive() &&
                    slice.getLatestLiveness().second.isOnlyReadAndNotLive() &&
                    (slice.getEarliestLiveness().first < slice.getLatestLiveness().first)) {
                    bitvec uninit_read_range(
                        slice.getEarliestLiveness().first,
                        slice.getLatestLiveness().first - slice.getEarliestLiveness().first + 1);
                    if (uninit_read_range.intersects(normal_live_range)) {
                        uninit_read_range =
                            (uninit_read_range ^ normal_live_range) & uninit_read_range;
                    }
                    BUG_CHECK(uninit_read_range.popcount() > 0,
                        "uninit read live range should not be totally eliminated");
                    BUG_CHECK(uninit_read_range.is_contiguous(),
                        "uninit read live range should be contiguous");
                    slice.setLiveness({uninit_read_range.min().index(), FieldUse(FieldUse::READ)},
                        {uninit_read_range.max().index(), FieldUse(FieldUse::READ)});
                }
            }
        }

        // sort alloc by MSB and earliest live range.
        f.sort_alloc();
    }
    // update physical stage info.
    PhvInfo::clearPhysicalStageInfo();
    for (const auto* table : tables_i) {
        BUG_CHECK(table_stages_i.count(TableSummary::getTableIName(table)),
                  "cannot find stage info of table: %1%", table);
        PhvInfo::setPhysicalStages(table, table_stages_i.at(TableSummary::getTableIName(table)));
    }

    // BUG_CHECK for overlapped live ranges of overlaying but non-mutex fields.
    // Slice Filtering Function
    std::function<bool(const PHV::AllocSlice* sl)> need_skip_slices =
        [&](const PHV::AllocSlice* sl) -> bool {
        if (!sl) return true;
        if (!sl->field()) return true;
        // skip alias destination fields and deparsed-zero fields
        if (sl->field()->aliasSource != nullptr || sl->field()->is_deparser_zero_candidate())
            return true;
        // skip
        return (sl->isUninitializedRead());
    };

    // Temporarily disabled (print warnings instead of BUG_CHECK),
    // because post-table-placement table-mutex pass seems to be incorrect.
    // if (!ipv6_host.apply().hit) {
    //     ipv6_lpm.apply();
    // }

    // BUG_CHECK that all non-mutex nor liverange-disjoint but overlaid fields are accessed
    // mutex tables only.
    // Two non-mutex fields, f_a and f_b, are overlaid in B0.
    // f_a's live range: [-1w, 4r]
    // f_b's live range: [3w, 7r]
    // It's not a BUG, because when the table t_a that writes f_a are mutex
    // with table t_b that reads f_b, hence they will not cause read / write violations
    //
    //             stage 3         stage 4
    //    |---- t_a writes B0
    // ---|
    //    |--------------------- t_b reads B0
    //
    for (const auto& c_slices : phv_i.getContainerToSlicesMap(nullptr, &need_skip_slices)) {
        const auto& slices = c_slices.second;
        if (slices.size() <= 1) continue;
        for (auto i = slices.begin(); i != (slices.end() - 1); i++) {
            for (auto j = i + 1; j != slices.end(); j++) {
                const auto& si = *i;
                const auto& sj = *j;
                if (!si.container_slice().overlaps(sj.container_slice())) {
                    continue;
                }
                if (phv_i.field_mutex()(si.field()->id, sj.field()->id)) {
                    continue;
                }
                if (si.isLiveRangeDisjoint(sj)) {
                    continue;
                }
                bool is_i_before_j = si.getEarliestLiveness() < sj.getEarliestLiveness();
                const PHV::AllocSlice& from = is_i_before_j ? si : sj;
                const PHV::AllocSlice& to   = is_i_before_j ? sj : si;
                for (const auto& from_locpair : defuse_i.getAllUses(from.field()->id)) {
                    const auto from_table = from_locpair.first->to<IR::MAU::Table>();
                    if (!from_table) continue;
                    for (const auto& to_locpair : defuse_i.getAllDefs(to.field()->id)) {
                        const auto to_table = to_locpair.first->to<IR::MAU::Table>();
                        if (!to_table) continue;
                        if (TableSummary::getTableIName(from_table) ==
                            TableSummary::getTableIName(to_table))
                            continue;  // same table is okay!
                        if (from_table->stage() <= to_table->stage()) continue;
                        // BUG_CHECK(tb_mutex_i(from_table, to_table),
                        //           "Overlaying slices with overlapped live range without "
                        //           "non-mutex table access is not allowed. table %1% reads %2% "
                        //           "at stage %3%, while table %4% writes %5% at stage %6%",
                        //           from_table->name, from, from_table->stage(), to_table->name,
                        //           to, to_table->stage());
                        if (!tb_mutex_i(from_table, to_table)) {
                            warning(
                                "Overlaying slices with overlapped live range are not allowed to "
                                "be accessed by non-mutex tables. table %1% reads %2% "
                                "at stage %3%, while table %4% writes %5% at stage %6%."
                                "This could be a false-positive alarm due to imprecise table mutex "
                                "analysis in post-table-placement phase. If two tables above are "
                                "mutually exclusive, it is safe to ignore this warning.",
                                from_table->externalName(), cstring::to_cstring(from),
                                from_table->stage(), to_table->externalName(),
                                cstring::to_cstring(to), to_table->stage());
                        }
                    }
                }
            }
        }
    }

    if (LOGGING(3)) {
        // log final phv allocation
        LOG3("PHV Allocation Result After Live Range Finalization");
        for (const auto& f : phv_i) {
            for (const auto& alloc_sl : f.get_alloc()) {
                LOG3(alloc_sl);
            }
        }
        // log table to physical stages
        LOG3("Table Physical Liverange Map After Live Range Finalization");
        for (const auto& kv : PhvInfo::table_to_physical_stages) {
            const cstring table = kv.first;
            std::stringstream ss;
            ss << table << ": ";
            std::string sep = "";
            for (const auto& v : kv.second) {
                ss << sep << v;
                sep = ", ";
            }
            LOG3(ss.str());
        }
        // log temp var physical liveranges
        for (const auto& kv : temp_var_live_ranges_i) {
            LOG3("liverange of temp var " << kv.first << ":" << kv.second);
        }
    }
}

}  // namespace PHV
