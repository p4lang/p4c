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

#include "fieldslice_live_range.h"
#include <sstream>
#include <string>
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/device.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/exceptions.h"
#include "lib/safe_vector.h"

namespace PHV {

namespace {

std::vector<std::string> to_string_vec(const LiveRangeInfo& info) {
    std::vector<std::string> ops(Device::numStages() + 2);
    for (int i = 0; i < Device::numStages() + 2; i++) {
        std::ostringstream s;
        s << info.vec()[i];
        ops[i] = s.str();
    }
    return ops;
}

void pretty_print_live_range_info(std::ostream& out, const LiveRangeInfo& info,
                                  const LiveRangeInfo* access_logs = nullptr) {
    static const std::vector<std::string>* tp_format = nullptr;
    if (tp_format == nullptr) {
        auto format = new std::vector<std::string>(Device::numStages() + 2);
        format->at(0) = "P";
        format->at(Device::numStages() + 1) = "D";
        for (int i = 1; i <= Device::numStages(); i++) {
            format->at(i) = std::to_string(i - 1);
        }
        tp_format = format;
    }
    TablePrinter tp(out, *tp_format, TablePrinter::Align::RIGHT);
    tp.addRow(to_string_vec(info));
    if (access_logs) {
        tp.addRow(to_string_vec(*access_logs));
    }
    tp.print();
}

}  // namespace

int LiveRangeInfo::num_table_stages = 0;

// result table
//    D  R  W  RW L
// D  D  R  W  RW L
// R  R  R  RW RW L
// W  W  RW W  RW L
// RW RW RW RW RW L
// L  L  L  L  L  L
LiveRangeInfo::OpInfo
operator|(const LiveRangeInfo::OpInfo& a, const LiveRangeInfo::OpInfo& b) {
    using OpInfo = LiveRangeInfo::OpInfo;
    // (1) either dead, the other
    if (a == OpInfo::DEAD) {
        return b;
    } else if (b == OpInfo::DEAD) {
        return a;
    }
    // (2) either live, return live
    if (a == OpInfo::LIVE || b == OpInfo::LIVE) {
        return OpInfo::LIVE;
    }
    // (3) then either READ_WRITE, return READ_WRITE
    if (a == OpInfo::READ_WRITE || b == OpInfo::READ_WRITE) {
        return OpInfo::READ_WRITE;
    }
    // (4) at this point, @p a and @p b can only be read or write.
    if (a != b) {
        return OpInfo::READ_WRITE;
    } else {
        return a;
    }
}

LiveRangeInfo::OpInfo& operator|=(LiveRangeInfo::OpInfo& a, const LiveRangeInfo::OpInfo& b) {
    a = a | b;
    return a;
}

bool LiveRangeInfo::can_overlay(const LiveRangeInfo& other) const {
    BUG_CHECK(vec().size() == other.vec().size(),
              "LiveRangeInfo must have the same number of stages.");
    const int n = vec().size();
    for (int i = 0; i < n; i++) {
        const auto& op = vec()[i];
        const auto& other_op = other.vec()[i];
        // When one of the OpInfo is dead, we can overlay.
        // Even if one op might be Write, check of the next
        // stage will be cover this case.
        if (op == OpInfo::DEAD || other_op == OpInfo::DEAD) {
            continue;
        }
        // LIVE can only be overlaid when the other is DEAD.
        if (op == OpInfo::LIVE || other_op == OpInfo::LIVE) {
            return false;
        }
        // the only case when READ can be overlaid with non-Dead
        // Other's write and mine will be dead next stage.
        const auto read_ok = [&]() {
            const bool last_read = i < n - 1 && vec()[i + 1] == OpInfo::DEAD;
            if (!(other_op == OpInfo::WRITE && last_read)) {
                return false;
            }
            return true;
        };
        // the only case when a write can be overlaid with non-Dead
        // others are dead next stage.
        const auto write_ok = [&]() {
            const bool other_last_read = i < n - 1 && other.vec()[i + 1] == OpInfo::DEAD;
            if (!other_last_read) {
                return false;
            }
            return true;
        };
        // Check the special case of write-after-read overlaying.
        switch (op) {
            case OpInfo::READ_WRITE:
                if (!read_ok() || !write_ok()) {
                    return false;
                }
                break;
            case OpInfo::READ:
                if (!read_ok()) {
                    return false;
                }
                break;
            case OpInfo::WRITE:
                if (!write_ok()) {
                    return false;
                }
                break;
            default:
                continue;  // surpass warnings.
        }
    }
    return true;
}

std::vector<LiveRange> LiveRangeInfo::disjoint_ranges() const {
    std::vector<LiveRange> rst;
    std::optional<int> last_defined_read;
    for (int i = 0; i < int(lives_i.size()); i++) {
        if (lives_i[i] == OpInfo::DEAD) {
            continue;
        }
        BUG_CHECK(lives_i[i] != OpInfo::LIVE, "live without definition: %1%", *this);

        // uninitialized read, not even caught by parser implicit init,
        // field can be considered as not live, we will add a short live range of [x, x).
        // NOTE that we ignore uninitialized reads in parser (i != 0) because parser reads
        // values from input buffer instead of PHV containers.
        if (i != 0 && (lives_i[i] == OpInfo::READ || lives_i[i] == OpInfo::READ_WRITE)) {
            const auto uninit_read = std::make_pair(i - 1, PHV::FieldUse(PHV::FieldUse::READ));
            const LiveRange empty_liverange{uninit_read, uninit_read};
            if (lives_i[i] == OpInfo::READ) {
                rst.emplace_back(empty_liverange);
                LOG6("uninitialized read: " << uninit_read);
                continue;
            } else {
                // The read in this READ_WRITE must be not actually initialized.
                // DO NOT skip this WRITE of READ_WRITE starting from i.
                if (!last_defined_read || last_defined_read != i) {
                    rst.emplace_back(empty_liverange);
                    LOG6("uninitialized read: " << uninit_read);
                }
            }
        }

        auto start = std::make_pair(i - 1, PHV::FieldUse(PHV::FieldUse::WRITE));
        int j = i + 1;
        while (j < int(lives_i.size()) && lives_i[j] == OpInfo::LIVE) ++j;
        if (j == int(lives_i.size()) ||
            lives_i[j] == OpInfo::DEAD || lives_i[j] == OpInfo::WRITE) {
            rst.emplace_back(start, start);
            i = j - 1;
            continue;
        }
        BUG_CHECK(lives_i[j] != OpInfo::WRITE, "invalid end of live range: %1% of %2%", j, *this);
        auto end = std::make_pair(j - 1, PHV::FieldUse(PHV::FieldUse::READ));
        rst.emplace_back(start, end);
        i = j;
        // when the end is READ_WRITE, it is also a start of the next live range.
        if (lives_i[j] == OpInfo::READ_WRITE) {
            last_defined_read = i;
            i--;
        }
    }
    return rst;
}

std::vector<LiveRange> LiveRangeInfo::merge_invalid_ranges(const std::vector<LiveRange>& ranges) {
    // premise bug check.
    for (auto itr = ranges.begin(); itr != ranges.end() && std::next(itr) != ranges.end(); ++itr) {
        auto next = std::next(itr);
        BUG_CHECK((*itr).end < (*next).start,
                  "live ranges are not sorted or not disjoint: %1%, %2%", *itr, *next);
    }
    std::vector<LiveRange> rst;
    const auto is_invalid = [&](const LiveRange& lr) { return lr.start == lr.end; };
    for (auto itr = ranges.begin(); itr != ranges.end(); ++itr) {
        if (!is_invalid(*itr)) {
            rst.emplace_back(*itr);
            continue;
        }
        // find consecutive invalid range of the same type (read-only or write-only).
        auto start = (*itr).start;
        auto next = std::next(itr);
        while (next != ranges.end() && is_invalid(*next) && start.second == (*next).start.second) {
            next = std::next(next);
        }
        next--;
        rst.emplace_back(LiveRange{start, (*next).end});
        itr = next;
    }
    return rst;
}

std::ostream &operator<<(std::ostream &out, const LiveRangeInfo::OpInfo& opinfo) {
    if (opinfo == LiveRangeInfo::OpInfo::DEAD)
        out << "";
    else if (opinfo == LiveRangeInfo::OpInfo::READ)
        out << "R";
    else if (opinfo == LiveRangeInfo::OpInfo::WRITE)
        out << "W";
    else if (opinfo == LiveRangeInfo::OpInfo::READ_WRITE)
        out << "RW";
    else if (opinfo == LiveRangeInfo::OpInfo::LIVE)
        out << "L";
    else
        BUG("unknown LiveRangeInfo::OpInfo: %1%", int(opinfo));
    return out;
}

std::ostream &operator<<(std::ostream &out, const LiveRangeInfo& info) {
    pretty_print_live_range_info(out, info);
    return out;
}

Visitor::profile_t FieldSliceLiveRangeDB::init_apply(const IR::Node *root) {
    profile_t rv = PassManager::init_apply(root);
    live_range_map.clear();
    return rv;
}

Visitor::profile_t FieldSliceLiveRangeDB::MapFieldSliceToAction::init_apply(const IR::Node *root) {
    profile_t rv = Inspector::init_apply(root);
    action_to_writes.clear();
    action_to_reads.clear();
    return rv;
}

bool FieldSliceLiveRangeDB::MapFieldSliceToAction::preorder(const IR::MAU::Action *act) {
    auto *tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "unable to find a table in context");
    ActionAnalysis aa(phv, false, false, tbl, red_info);
    ActionAnalysis::FieldActionsMap field_actions_map;
    aa.set_field_actions_map(&field_actions_map);
    act->apply(aa);

    for (const auto &field_action : Values(field_actions_map)) {
        le_bitrange write_range;
        auto *write_field = phv.field(field_action.write.expr, &write_range);
        BUG_CHECK(write_field != nullptr, "Action does not have a write?");
        PHV::FieldSlice write(write_field, write_range);
        action_to_writes[act].insert(write);
        for (const auto &read : field_action.reads) {
            if (read.type == ActionAnalysis::ActionParam::PHV) {
                le_bitrange read_range;
                auto f_used = phv.field(read.expr, &read_range);
                PHV::FieldSlice read_fs(f_used, read_range);
                action_to_reads[act].insert(read_fs);
            }  // don't care about others
        }
    }
    return false;
}

std::optional<FieldSliceLiveRangeDB::DBSetter::Location>
FieldSliceLiveRangeDB::DBSetter::to_location(const PHV::Field* field,
                                             const FieldDefUse::locpair& unit_expr,
                                             bool is_read) const {
    const auto* unit = unit_expr.first;
    const auto* expr = unit_expr.second;
    Location loc;
    if (unit->is<IR::BFN::ParserState>() || unit->is<IR::BFN::Parser>() ||
        unit->is<IR::BFN::GhostParser>()) {
        if (!is_read) {
            // Ignore implicit parser init when the field is marked as no_init.
            if (expr->is<ImplicitParserInit>() && not_implicit_init_fields().count(field))
                return std::nullopt;
        }
        loc.u = Location::PARSER;
    } else if (unit->is<IR::BFN::Deparser>()) {
        loc.u = Location::DEPARSER;
    } else if (unit->is<IR::MAU::Table>()) {
        const auto* t = unit->to<IR::MAU::Table>();
        loc.u = Location::TABLE;
        loc.stages = backtracker->stage(t, true);
    } else {
        BUG("unknown use unit: %1%, field: %2%", unit, field);
    }
    return loc;
}

std::pair<int, int> FieldSliceLiveRangeDB::DBSetter::update_live_status(
        LiveRangeInfo& liverange, const Location& loc, bool is_read) const {
    using OpInfo = LiveRangeInfo::OpInfo;
    const LiveRangeInfo::OpInfo op = is_read ? OpInfo::READ : OpInfo::WRITE;
    std::pair<int, int> updated_range;
    if (loc.u == Location::PARSER) {
        liverange.parser() |= op;
        updated_range = std::make_pair(-1, -1);
        LOG5(op << " on parser");
    } else if (loc.u == Location::DEPARSER) {
        liverange.deparser() |= op;
        updated_range = std::make_pair(Device::numStages(), Device::numStages());
        LOG5(op << " on deparser");
    } else if (loc.u == Location::TABLE) {
        BUG_CHECK(loc.stages.size() > 0, "table not allocated");
        const auto min_max = std::minmax_element(loc.stages.begin(), loc.stages.end());
        for (int i = *min_max.first; i <= *min_max.second; i++) {
            LOG5(op << " on stage " << i);
            liverange.stage(i) |= op;
        }
        updated_range = std::make_pair(*min_max.first, *min_max.second);
    } else {
        BUG("unknown Location Type");
    }
    return updated_range;
}

void FieldSliceLiveRangeDB::DBSetter::update_live_range_info(const PHV::FieldSlice& fs,
                                                             const Location& use_loc,
                                                             const Location& def_loc,
                                                             LiveRangeInfo& liverange) const {
    LOG3("update_live_range_info " << fs << ": " << def_loc.u << "," << use_loc.u);
    std::pair<int, int> use_range = update_live_status(liverange, use_loc, true);
    std::pair<int, int> def_range = update_live_status(liverange, def_loc, false);

    // mark(or) LIVE for stages in between.
    using OpInfo = LiveRangeInfo::OpInfo;
    if (def_range.second < use_range.first) {
        // NOTE: for fieldslice the its def or use table are split across multiple stages.
        // The live range starts at the first def table stage and end at the last read stage.
        for (int i = def_range.first + 1; i <= use_range.second - 1; i++) {
            liverange.stage(i) |= OpInfo::LIVE;
        }
    } else {
        // read write within parser does not matter.
        if (use_range.first == -1 && def_range.second == -1) {
            return;
        }
        // TODO: we need to handle this case more carefully. For example,
        // There could be a table that has a dominating write of this read in control flow
        // but because of ignore_table_dependency pragma, the write table is placed after
        // the read table. Then we need to find write of the read earlier in the pipeline.
        warning(
            "Because of ignore_table_dependency pragma, for %1% field, the read in stage %2% "
            "cannot source its definition of the write in stage %3%. Unexpected value might be "
            "read and physical live range analysis will set its liverange to the whole pipeline,"
            " regardless of the actual physical live range.",
            cstring::to_cstring(fs), use_range.first, def_range.second);
        liverange.parser() = OpInfo::WRITE;
        for (int i = 0; i <= Device::numStages() - 1; i++) {
            liverange.stage(i) = OpInfo::LIVE;
        }
        liverange.deparser() = OpInfo::READ;
    }
}

void FieldSliceLiveRangeDB::DBSetter::end_apply() {
    // skip this when table has not been placed or alt_phv_alloc is not enabled.
    // because when alt_phv_alloc is not enabled, table placement might be incomplete.
    if (!backtracker->hasTablePlacement() || !BFNContext::get().options().alt_phv_alloc) {
        LOG1("alt-phv-alloc not enabled or no table placement found, skip FieldSliceLiveRangeDB");
        return;
    }
    using OpInfo = LiveRangeInfo::OpInfo;
    // collect all field slice and identify the max stage used by a table.
    // Note that because defuse analysis is still based on
    // whole fields instead of slices, it is okay to use the whole field.
    // TODO:
    //   (1) use MakeSlices info from make_clsuters.
    //   (2) update defuse pass to use slice-level read/write analysis.
    ordered_set<PHV::FieldSlice> fs_set;
    ordered_map<FieldSlice, LiveRangeInfo> fs_info_map;
    ordered_map<FieldSlice, LiveRangeInfo> table_access_logs;

    int max_stage = -1;
    auto update_max_stage = [&max_stage](const Location& loc) {
        if (loc.u == Location::TABLE) {
            int loc_max = *std::max_element(loc.stages.begin(), loc.stages.end());
            max_stage = std::max(max_stage, loc_max);
        }
    };

    for (const auto& kv : phv.get_all_fields()) {
        const auto fs = PHV::FieldSlice(&kv.second);
        fs_set.insert(fs);

        const auto* field = fs.field();
        for (const FieldDefUse::locpair& use : defuse->getAllUses(field->id)) {
            auto loc = to_location(field, use, true);
            update_max_stage(*to_location(field, use, true));
            const auto& defs_of_use = defuse->getDefs(use);
            for (const auto& def : defs_of_use) {
                auto loc = to_location(field, def, false);
                if (loc) update_max_stage(*loc);
            }
        }
    }
    LiveRangeInfo::set_num_table_stages(max_stage + 1);

    // Set up data structures _after_ the number of table stages has been identified
    // to ensure that the LiveRangeInfo objects are correctly sized
    for (const auto& fs : fs_set) {
        fs_info_map.emplace(fs, LiveRangeInfo());
        table_access_logs.emplace(fs, LiveRangeInfo());
    }

    for (const auto& fs : fs_set) {
        LOG5("compute physical live range: " << fs);
        LiveRangeInfo& liverange = fs_info_map[fs];
        const auto* field = fs.field();

        // set (W..L...R) for all paired defuses.
        for (const FieldDefUse::locpair& use : defuse->getAllUses(fs.field()->id)) {
            LOG5("found use: " << use.second << " in " << DBPrint::Brief << use.first);
            const auto use_loc = to_location(field, use, true);
            BUG_CHECK(use_loc, "use cannot be ignored");

            // Ignore deparser reads for clot-allocated, unused or read_only, and non-digested
            // fields. Deparser can directly read from clot instead of PHV unless they are packed
            // with modified fields in a container. In that case, caller need to extend
            // physical live ranges of packed header fields to deparser.
            if (use_loc->u == Location::DEPARSER && clot.allocated_unmodified_undigested(field)) {
                LOG3("ignore clot-allocated unmodified " << field->name << " read: " << use.second);
                continue;
            }

            // update table access logs.
            if (use_loc->u == Location::unit::TABLE) {
                for (const auto& s : use_loc->stages) {
                    table_access_logs[fs].stage(s) |= OpInfo::READ;
                }
            }

            // Always update uses. It is possible for field to be read without def.
            // For example,
            // (1) fields added by compiler as padding for deparsed(digested) metadata.
            // (2) a = a & 1, when a has not been written before, including auto-init-metadata
            //     is disabled on this field.
            update_live_status(liverange, *use_loc, true);

            // mark(or) W and R and all stages in between to LIVE.
            const auto& defs_of_use = defuse->getDefs(use);
            for (const auto& def : defs_of_use) {
                LOG5("found paired def: " << def.second << " in " << DBPrint::Brief << def.first);
                const auto* field = fs.field();
                const auto def_loc = to_location(field, def, false);
                if (def_loc) {
                    update_live_range_info(fs, *use_loc, *def_loc, fs_info_map[fs]);
                    if (def_loc->u == Location::unit::TABLE) {
                        for (const auto& s : def_loc->stages) {
                            table_access_logs[fs].stage(s) |= OpInfo::WRITE;
                        }
                    }
                } else {
                    LOG5("ignoring parser init of " << field->name
                         << ", because @pa_auto_init_metadata is not enabled for this field.");
                }
            }
        }

        // set (w) for tailing writes (write without read)
        for (const FieldDefUse::locpair& def : defuse->getAllDefs(fs.field()->id)) {
            const auto& uses_of_def = defuse->getUses(def);
            // TODO: There is a bug in field_defuse and IR::BFN::GhostParser that because
            // ghost parser write all ghost metadata in one expression, later uses of
            // ghost metadata are paired with this def. For example, in many ghost tests,
            // although ghost::gh_intr_md.pipe_id has no read, and only one def, if you invoke
            // defuse->getUses(def) on the only def of IR::BFN::GhostParser, you will get
            // some uses from other ghost metadata like qid, being incorrectly paired with this
            // def. To fix this, we should change IR::BFN::GhostParser to write individual fields.
            if (!uses_of_def.empty() && !def.first->is<IR::BFN::GhostParser>()) {
                continue;
            }
            // implicit parser init, is not an actual write. If there is no paired
            // uses, it is safe to ignore.
            if (def.second->is<ImplicitParserInit>()) {
                continue;
            }
            const auto def_loc = to_location(field, def, false);
            if (!def_loc) {
                continue;
            }
            LOG5("found tailing write: " << def.first);
            update_live_status(liverange, *def_loc, false);
        }
        if (fs.field()->pov) {
            // POV fields should be live throughout the whole pipeline
            update_live_range_info(fs,
                                   Location{Location::DEPARSER, {}},
                                   Location{Location::PARSER, {}},
                                   liverange);
        }
    }

    LOG3("LiveRange Result:");
    for (auto it : fs_info_map) {
        self.set_liverange(it.first, it.second);
        LOG3(it.first);
        std::ostringstream ss;
        pretty_print_live_range_info(ss, it.second, &table_access_logs.at(it.first));
        LOG3(ss.str());
    }
}

void
FieldSliceLiveRangeDB::set_liverange(const PHV::FieldSlice& fs, const LiveRangeInfo& info) {
    live_range_map[fs.field()][fs] = info;
}

const LiveRangeInfo*
FieldSliceLiveRangeDB::get_liverange(const PHV::FieldSlice& fieldslice) const {
    if (live_range_map.count(fieldslice.field())) {
        if (live_range_map.at(fieldslice.field()).count(fieldslice)) {
            return &live_range_map.at(fieldslice.field()).at(fieldslice);
        } else {
            for (const auto& pair : live_range_map.at(fieldslice.field())) {
                const auto& fs = pair.first;
                const auto& info = pair.second;
                if (fs.range().contains(fieldslice.range())) return &info;
            }
        }
    }
    return nullptr;
}

const LiveRangeInfo* FieldSliceLiveRangeDB::default_liverange() const {
    static LiveRangeInfo* whole_pipe = nullptr;
    if (whole_pipe) {
        return whole_pipe;
    }
    whole_pipe = new LiveRangeInfo();
    whole_pipe->parser() = LiveRangeInfo::OpInfo::WRITE;
    for (int i = 0; i < Device::numStages(); i++) {
        whole_pipe->stage(i) = LiveRangeInfo::OpInfo::LIVE;
    }
    whole_pipe->deparser() = LiveRangeInfo::OpInfo::READ;
    return whole_pipe;
}

}  // namespace PHV
