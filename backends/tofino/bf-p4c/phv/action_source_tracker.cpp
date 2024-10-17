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

#include "bf-p4c/phv/action_source_tracker.h"
#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/phv/cluster_phv_operations.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/log.h"

namespace PHV {

namespace {

/// slice all sourceOp in @p sources by @p start, @p len.
ActionClassifiedSources slice(const ActionClassifiedSources& sources, int start, int len) {
    ActionClassifiedSources rv;
    for (const auto& kv : sources) {
        for (const auto& op : kv.second) {
            rv[kv.first].push_back(op.slice(start, len));
        }
    }
    return rv;
}

}  // namespace

SourceOp SourceOp::slice(int start, int len) const {
    if (ad_or_const) {
        return *this;
    } else {
        PHV::FieldSlice slice =
            PHV::FieldSlice(phv_src->field(), StartLen(phv_src->range().lo + start, len));
        auto clone = *this;
        clone.phv_src = slice;
        return clone;
    }
}

bool ActionSourceTracker::preorder(const IR::MAU::Action *act) {
    auto *tbl = findContext<IR::MAU::Table>();
    ActionAnalysis aa(phv, false, false, tbl, red_info);
    ActionAnalysis::FieldActionsMap field_actions_map;
    aa.set_field_actions_map(&field_actions_map);
    act->apply(aa);
    add_sources(act, field_actions_map);
    return true;
}

void ActionSourceTracker::add_sources(const IR::MAU::Action* act,
                                      const ActionAnalysis::FieldActionsMap& instructions) {
    using ActionParam = ActionAnalysis::ActionParam;
    for (const auto& field_action : Values(instructions)) {
        le_bitrange dest_range;
        const auto *dest = phv.field(field_action.write.expr, &dest_range);
        BUG_CHECK(dest != nullptr, "action does not have a write: %1%", act);

        for (const auto &read : field_action.reads) {
            SourceOp src_op;
            if (read.type == ActionParam::PHV) {
                le_bitrange read_range;
                auto* f_read = phv.field(read.expr, &read_range);
                // TODO: There are some unsupported cases in ActionAnalysis
                // see issue983-bmv2.p4 for example.
                if (read_range.size() != dest_range.size()) {
                    LOG1("skip incorrect (size-mismatch) source range: " << read.expr);
                    continue;
                }
                src_op.phv_src = PHV::FieldSlice(f_read, read_range);
            } else if (read.type == ActionParam::ACTIONDATA ||
                       read.type == ActionParam::CONSTANT) {
                src_op.ad_or_const = true;
            } else {
                BUG("Read must either be of a PHV, action data, or constant.");
            }

            if (field_action.name == "set") {
                src_op.t = SourceOp::OpType::move;
            } else if (PHV_Field_Operations::BITWISE_OPS.count(field_action.name)) {
                src_op.t = SourceOp::OpType::bitwise;
            } else {
                src_op.t = SourceOp::OpType::whole_container;
            }
            sources[dest][dest_range][act].push_back(src_op);
            LOG5(dest << " " << dest_range << " source from " << src_op << " in "
                      << act->externalName());
        }
    }
}

void ActionSourceTracker::end_apply() {
    // pre-compute the fine slicing for all fields. This is a speed optimization that
    // later when the user calls get_sources, there will be only one range that contains
    // the requested range, as long as the argument of fieldslice is fine-sliced.
    ordered_map<const PHV::Field*, ordered_map<le_bitrange, ActionClassifiedSources>> fine_sliced;
    for (const auto& source_kv : sources) {
        const auto* f = source_kv.first;
        std::set<int> bounds = {0, f->size};
        for (const auto& range_sources : source_kv.second) {
            const auto& range = range_sources.first;
            // convert this range to a half-open range, i.e. [lo, hi + 1), for fine-slicing.
            // Half open range allows us to split ranges simply based on the numers of lo and hi.
            // For example, if we have two ranges from a 32-bit field, [0, 15], [24, 31]
            // the fine-slicing of the field would be [0, 16), [16, 24), [24, 32).
            bounds.insert(range.lo);
            bounds.insert(range.hi + 1);
        }
        for (auto itr = bounds.begin();
             itr != bounds.end() && std::next(itr) != bounds.end();
             itr++) {
            // convert half open range back to closed range.
            auto new_range = le_bitrange(FromTo(*itr, *std::next(itr) - 1));
            fine_sliced[f][new_range] = get_sources(PHV::FieldSlice(f, new_range));
        }
    }
    sources = fine_sliced;

    LOG3("action source tracker result: ");
    LOG3(*this);
}

ActionClassifiedSources ActionSourceTracker::get_sources(const PHV::FieldSlice& fs) const {
    if (!sources.count(fs.field())) {
        return {};
    }
    ActionClassifiedSources rv;
    for (const auto& range_sources : sources.at(fs.field())) {
        const auto& range = range_sources.first;
        if (range.contains(fs.range())) {
            auto sliced = slice(
                    range_sources.second, fs.range().lo - range.lo, fs.range().size());
            for (const auto& kv : sliced) {
                rv[kv.first].insert(rv[kv.first].end(), kv.second.begin(), kv.second.end());
            }
        } else {
            BUG_CHECK(range.intersectWith(fs.range()).empty(), "%1% is not fine-sliced. "
                      "Saved range is %2%", fs, range);
        }
    }
    return rv;
}


std::ostream &operator<<(std::ostream &out, const SourceOp& src) {
    out << "(";
    if (src.t == SourceOp::OpType::move) {
        out << "move";
    } else if (src.t == SourceOp::OpType::bitwise){
        out << "bitwise";
    } else if (src.t == SourceOp::OpType::whole_container) {
        out << "whole_container";
    } else {
        BUG("unknown op: %1%", (int)src.t);
    }
    out << " ";
    if (src.ad_or_const) {
        out << "AD_OR_CONST";
    } else {
        out << *src.phv_src;
    }
    out << ")";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionClassifiedSources& sources) {
    for (const auto& action_sources : sources) {
        out << "{ " << canon_name(action_sources.first->externalName()) << ": ";
        std::string sep = "";
        for (const auto& s : action_sources.second) {
            out << sep << s;
            sep = ", ";
        }
        out << " }";
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionSourceTracker& tracker) {
    for (const auto& source_kv : tracker.sources) {
        const auto* f = source_kv.first;
        for (const auto& range_sources : source_kv.second) {
            const auto& range = range_sources.first;
            const ActionClassifiedSources& action_sources = range_sources.second;
            if (action_sources.empty()) continue;
            out << f->name << " [" << range.lo << ":" << range.hi << "] ";
            out << action_sources;
            out << "\n";
        }
    }
    return out;
}

}  // namespace PHV
