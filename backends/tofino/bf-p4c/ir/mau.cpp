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

#include <assert.h>
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/mau/instruction_memory.h"
#include "bf-p4c/mau/table_layout.h"
#include "bf-p4c/mau/stateful_alu.h"
#include "ir/ir.h"
#include "gateway_control_flow.h"

// FIXME -- This is gcc specific, but allows p4test to link without breaking p4c-barefoot
Device* Device::instance_ __attribute__((weak)) = nullptr;

namespace P4 {
namespace IR {
namespace MAU {

// These must be implemented here because C++ requires the definitions and instantiations of
// function templates to be in the same compilation unit.
void Table::visit_children(Visitor& v) { visit_children(this, v); }
void Table::visit_children(Visitor& v) const { visit_children(this, v); }

/**
 * The potential control flow(s) through an IR::MAU::Table object are quite complex, which
 * causes its visit_children methods to be quite complex as well.
 *
 * An IR::MAU::Table object corresponds to a Tofino hardware table, which includes a
 * match table (which may hit or miss) and a gateway that may override the match table
 * or not.  After the match or gateway result, there may be an action to run which can
 * trigger attached tables, or attached tables may be triggered unconditionally.
 * Finally, a next table may trigger (enable) other tables.
 *
 * Conceptually, the match and gateway portions of the table run simultaneously.  If a
 * gateway row matches, the highest priority (first) matching gateway_row determines the
 * result -- either allow the match table to act normally, or override it, specifying
 * both a payload (action to run) and a next table.  If all the gateway rows miss, the
 * final gateway miss result controls whether the match table runs or is overridden.  A
 * table with no gateway rows has a notional gateway with a miss action that does not
 * override the match table (so it runs normally)
 *
 * note that all gateway override actions (icluding the gateway "miss" action) count as
 * hit actions (follow the hit pathway in match central).
 *
 * To follow this control flow in visit_children, we first visit all the gateway expressions
 * since they are evaluated simultaneously.  In theory, if a high priority row matches, the
 * later rows don't actually matter, so we could visit them sequentially (treating the later
 * ones as dead when the earlier ones match), but we currently do not (fixing this could allow
 * more/better dead code elimination).
 *
 * We then find the gateway rows that override the match table and save them in the
 * payload_info_t saved state.  We then visit the match keys (so match keys will be treated
 * as dead on paths that involve gateway overrides -- even though the match is actually
 * evaluated simultaneously as the gateway in the hardware, it is safe to treat as dead as
 * the match result will be completely ignored if the gateway overrides.
 *
 * After the visiting the match key, we visit each action with union of state from the gateway
 * payloads and match result that can trigger that action.  After all actions have been
 * visited, we visit the next tables with the union of the actions that can trigger each
 * next, which may be from gateway override, match action chain, or match hit/miss.
 *
 * finally, we visit all the attached tables.  This is incorrect for the flow order, so flow
 * analysis through attached tables will not be correct
 */

struct Table::payload_info_t {
    struct info_t {
        Visitor                 *flow_state;
        std::set<cstring>       tags;   // gateway tags that run this action
    };
    std::map<cstring, info_t>   action_info;
    Visitor                     *post_payload = nullptr;
};

template<class THIS>
void Table::visit_children(THIS* self, Visitor& v) {
    // We visit the table in a way that reflects its control flow: at branches, we visit with
    // clones of the visitor; at join points, we merge the clones back together. IMPORTANT: we must
    // ensure that each node is visited exactly once.
    //
    // A table is executed as follows. First, the gateway conditions in "gateway_rows" are
    // evaluated.
    //
    // If no gateway condition is satisfied, the table keys are evaluated. If "next" has a
    // "$try_next_stage" entry, then we nondeterministically choose whether to execute that entry.
    // If there is no "$try_next_stage", or if we choose not to execute it, then we execute a table
    // action, according to the control-plane program. The entry corresponding to "$hit" or "$miss"
    // in "next" is then executed, according to whether the table hit or miss; if neither entry
    // exists, then the "$default" entry in "next" is executed, if it exists.
    //
    // If any condition is satisfied, we have a gateway-inhibited table. The payload in
    // "gateway_payload" is executed, before executing the entry in "next" corresponding to the
    // satisfied condition.

    // Visit gateway conditions.
    for (auto& gw : self->gateway_rows) {
        auto& cond = gw.first;
        v.visit(cond, "gateway_row");
    }

    // Now, we fall into one of four cases, depending on whether the table uses its gateway
    // payload, and on whether the table has a match component.
    bool have_gateway_payload = self->uses_gateway_payload();
    bool have_match_table = !self->conditional_gateway_only();
    payload_info_t      payload_info;

    if (have_gateway_payload && have_match_table) {
        auto& gateway_visitor = v.flow_clone();
        visit_gateway_inhibited(self, gateway_visitor, payload_info);
        visit_match_table(self, v, payload_info);
        v.flow_merge(gateway_visitor);
    } else if (have_gateway_payload) {
        visit_gateway_inhibited(self, v, payload_info);
        BUG_CHECK(payload_info.action_info.empty(),
                  "non-empty action info on conditional_gateway_only");
        BUG_CHECK(payload_info.post_payload == nullptr || payload_info.post_payload == &v,
                  "inconsistent post-payload for conditional_gateway_only");
    } else if (have_match_table) {
        visit_match_table(self, v, payload_info);
    } else {
        // Have neither a gateway payload nor a match table. This table is a no-op; fall through to
        // attached tables.
    }

    // FIXME -- attached tables are not properly visited here in the control-flow order,
    // FIXME -- because we don't really know which actions will trigger them.  They
    // FIXME -- should be visited after the action(s) that trigger them and before
    // FIXME -- the next tables that happen after those actions?
    // FIXME -- If the actions contain references to them, then they'll be visited when
    // FIXME -- the action is visited, and this will be a 'revisit'
    self->attached.visit_children(v);
}

template<class THIS>
void Table::visit_gateway_inhibited(THIS* self, Visitor& v, payload_info_t &payload_info) {
    // Now, visit actions for when the table is gateway-inhibited.

    // Save the control-flow state. We use v to visit the first execution path through the gateway
    // actions. On subsequent paths, we visit with a copy of this saved state, and merge the result
    // into v.
    Visitor* saved = &v.flow_clone();

    // This is the visitor we will use to visit the various gateway actions. Initially, this is
    // v. After the first execution path, this becomes nullptr, and will be lazily instantiated
    // with a copy of "saved", as needed.  However, if there's a match table, we need to save
    // v to visit the match table actions with.
    Visitor* current = &v;

    std::set<cstring> gw_tags_seen;
    bool fallen_through = false;
    for (auto& gw : self->gateway_rows) {
        auto tag = gw.second;
        if (!tag || gw_tags_seen.count(tag)) continue;

        gw_tags_seen.emplace(tag);

        // Each row of the gateway may have a next table, which would inhibit the match table
        // from running.  When a gateway inhibits, it can possibly run an action.  This is the
        // action mapped in the gateway payload, from next to action
        if (self->gateway_payload.count(tag)) {
            if (cstring act_name = self->gateway_payload.at(tag).first) {
                if (!current) current = &saved->flow_clone();
                for (auto &con : self->gateway_payload.at(tag).second)
                    current->visit(con, "gateway_payload");
                if (payload_info.action_info.count(act_name))
                    payload_info.action_info.at(act_name).flow_state->flow_merge(*current);
                else
                    payload_info.action_info[act_name].flow_state = current;
                payload_info.action_info.at(act_name).tags.insert(tag);
                current = nullptr;
                continue;
            }
        }

        if (self->next.count(tag)) {
            if (!current) current = &saved->flow_clone();
            if (auto *gwcf = dynamic_cast<::BFN::GatewayControlFlow *>(current))
                gwcf->pre_visit_table_next(self, tag);
            current->visit(self->next.at(tag), tag);
            if (payload_info.post_payload) {
                if (current != payload_info.post_payload)
                    payload_info.post_payload->flow_merge(*current);
            } else {
                payload_info.post_payload = current;
            }
            current = nullptr;
            continue;
        }

        // No matching action or next, so fall through from "saved" if we haven't done so already.
        if (!fallen_through) {
            if (current) {
                // "current" hasn't been used yet, so just consume it.
                BUG_CHECK(!payload_info.post_payload, "inconsitent gateway post-payload");
                BUG_CHECK(current == &v, "inconsistent gateway current");
                payload_info.post_payload = current;
                current = nullptr;
            } else {
                if (payload_info.post_payload)
                    payload_info.post_payload->flow_merge(*saved);
                else
                    payload_info.post_payload = &saved->flow_clone(); }

            fallen_through = true;
        }
    }
    BUG_CHECK(current == nullptr, "gateway visitor state has not be consumed");
}

/** Custom SplitFlowVisit subclass to visit the next chains in parallel.
 * We compute the initial state (after actions) for each next chain and insert
 * them in the SplitFlowVisitor_base, so it is set up to run each of them as a
 * "coroutine", so the Visitor infra can visit the later ones if needed to get
 * all the parents of some subsequent table that is mulitply applied */
template<class THIS> class SplitFlowVisitTableNext : public SplitFlowVisit_base {
    THIS *table;
    Visitor *saved;
    ordered_map<cstring, int> visitor_for_tag;
    std::vector<cstring> tag_for_visitor;

    void do_visit() override {
        if (!finished()) {
            int idx = visit_next++;
            auto next_action_key = tag_for_visitor.at(idx);
            auto &next_visitor = visitors.at(idx);

            if (table->next.count(next_action_key)) {
                if (!next_visitor) next_visitor = &saved->flow_clone();
                next_visitor->visit(table->next.at(next_action_key), next_action_key,
                                    start_index + idx);
            }
        }
    }

 public:
    // FIXME -- update SplitFlowVisit_base::run_visit to match this, then delete it.
    void run_visit() override {
        BUG_CHECK(visit_next == -1, "started SplitFlowVisit more than once");
        visit_next = 0;
        auto *ctxt = v.getChildContext();
        start_index = ctxt ? ctxt->child_index : 0;
        while (!finished()) do_visit();
        for (auto *cl : visitors) {
            if (cl && cl != &v)
                v.flow_merge(*cl);
        }
    }

    SplitFlowVisitTableNext(THIS *t, Visitor &v, Visitor *s)
        : SplitFlowVisit_base(v), table(t), saved(s) { visit_next = -1; }
    int count(cstring act) { return visitor_for_tag.count(act); }
    Visitor *&at(cstring act) { return visitors.at(visitor_for_tag.at(act)); }
    Visitor *&operator[](cstring act) {
        if (count(act)) return at(act);
        visitor_for_tag[act] = visitors.size();
        visitors.push_back(nullptr);
        tag_for_visitor.push_back(act);
        BUG_CHECK(visitors.size() == tag_for_visitor.size(), "size mismatch");
        return visitors.back(); }

    void dbprint(std::ostream &out) const override {
        out << "SplitFlowVisitTableNext(" << table->name << ") " << (void *)table
            << " processed " << visit_next << " of " << visitors.size(); }
};


template<class THIS>
void Table::visit_match_table(THIS* self, Visitor& v, payload_info_t &payload_info) {
    // Visit match keys.
    self->match_key.visit_children(v);

    // Save the current control-flow state. We use v to visit the first execution path through the
    // table. On subsequent paths, we visit with a copy of this saved state, and merge the result
    // into v.
    Visitor* saved = &v.flow_clone();

    // This is the visitor we will use to visit the various parts of the table. Initially, this is
    // v. After the first execution path, this becomes nullptr, and will be lazily instantiated
    // with a copy of "saved", as needed.
    Visitor* current = &v;

    // Handle all exiting actions. For these actions, we don't want to merge the control-flow back
    // into v.
    for (auto& kv : self->actions) {
        auto action_name = kv.first;
        auto& action = kv.second;
        if (!action->exitAction) continue;
        auto exit_visitor = &saved->flow_clone();
        if (payload_info.action_info.count(action_name))
            exit_visitor->flow_merge(*payload_info.action_info.at(action_name).flow_state);
        exit_visitor->visit(action, "actions"_cs);
        exit_visitor->flow_merge_global_to("-EXIT-"_cs);
    }

    // Handle non-exiting actions, while being careful to avoid visiting "next" entries multiple
    // times.

    SplitFlowVisitTableNext<THIS> next_visitors(self, v, saved);
    bool have_hit_miss = self->next.count("$hit"_cs) || self->next.count("$miss"_cs);
    if (have_hit_miss) {
        next_visitors["$hit"_cs] = nullptr;
        next_visitors["$miss"_cs] = nullptr;
    } else {
        next_visitors["$default"_cs] = nullptr;
    }

    // Visit all actions to populate next_visitors with visitors to visit "next".
    for (auto& kv : self->actions) {
        auto action_name = kv.first;
        auto& action = kv.second;

        if (action->exitAction) continue;

        auto *pinfo = ::getref(payload_info.action_info, action_name);
        if (!action->hit_allowed && !action->default_allowed) {
            // can't be invoked from the match table
            if (pinfo)
                current = pinfo->flow_state;
            else
                continue;
        } else {
            if (!current) current = &saved->flow_clone();
            if (pinfo)
                current->flow_merge(*pinfo->flow_state); }

        // Visit the action.
        current->visit(action, "actions"_cs);

        // Figure out which keys in the next_visitors table need updating.
        if (pinfo) {
            for (auto tag : pinfo->tags) {
                BUG_CHECK(next_visitors.count(tag) == 0, "gateway tag duplication");
                next_visitors[tag] = &current->flow_clone(); } }

        // Can separate table using hit/miss from tables using action chaining, as in P4
        // semantically, a table can not use both hit/miss and action chaining. Potentially if that
        // changes, we will have to support it with a layered approach to the next table
        // propagation
        std::vector<cstring> keys;
        if (have_hit_miss) {
            // Table uses hit/miss chaining.
            if (!action->miss_only()) keys.push_back("$hit"_cs);
            if (!action->hit_only()) keys.push_back("$miss"_cs);
        } else {
            // Table uses action chaining.
            if (self->next.count(action_name)) {
                keys.push_back(action_name);
                BUG_CHECK(next_visitors.count(action_name) == 0, "duplicate action name");
                next_visitors[action_name] = nullptr;
            } else {
                // The action has no "next" entry. It chains to the entry for "$default".
                keys.push_back("$default"_cs);
            }
        }

        // Merge the current visitor into the next_visitors table.
        bool current_used = false;
        for (auto key : keys) {
            BUG_CHECK(next_visitors.count(key), "Unexpected 'next' key: %s", key);
            auto& next_visitor = next_visitors.at(key);
            if (next_visitor) {
                next_visitor->flow_merge(*current);
            } else if (current_used) {
                next_visitor = &current->flow_clone();
            } else {
                next_visitor = current;
                current_used = true;
            }
        }

        current = nullptr;
    }

    // Visit $try_next_stage, if it exists.
    if (self->next.count("$try_next_stage"_cs)) {
        BUG_CHECK(next_visitors.count("$try_next_stage"_cs) == 0, "invalid");
        if (!current) current = &saved->flow_clone();
        next_visitors["$try_next_stage"_cs] = current;
        current = nullptr;
    }

    // Now visit "next".
    next_visitors.run_visit();

    if (payload_info.post_payload && payload_info.post_payload != &v)
        v.flow_merge(*payload_info.post_payload);
}

bool IR::MAU::Table::operator==(const IR::MAU::Table &a) const {
    return name == a.name &&
           gress == a.gress &&
           gateway_cond == a.gateway_cond &&
           gateway_result_tag == a.gateway_result_tag &&
           stage_ == a.stage_ &&
           logical_id == a.logical_id &&
           gateway_rows == a.gateway_rows &&
           gateway_payload == a.gateway_payload &&
           match_table == a.match_table &&
           attached == a.attached &&
           actions == a.actions &&
           next == a.next &&
           match_key == a.match_key &&
           has_dark_init == a.has_dark_init &&
           always_run == a.always_run &&
           suppress_context_json == a.suppress_context_json &&
           layout == a.layout &&
           ways == a.ways &&
           resources == a.resources &&
           run_before_exit == a.run_before_exit;
}

cstring IR::MAU::Table::get_table_type_string() const {
    cstring tbl_type = "gateway"_cs;
    bool no_match_hit = layout.no_match_hit_path() && !conditional_gateway_only();
    // FIXME -- "no_match_hit" is a bit of a misnomer -- it includes "direct" lookup
    // match tables where the match has been changed to an identity hash used to index
    // an action data table.  It really just means a table with no match rams or tcams
    // using a gateway to supply a hit signal
    if (!conditional_gateway_only())
        tbl_type = layout.ternary || layout.no_match_miss_path()
                   ? "ternary_match"_cs : "exact_match"_cs;
    if (layout.proxy_hash)
        tbl_type = "proxy_hash"_cs;
    if (no_match_hit)
        tbl_type = "hash_action"_cs;
    if (layout.atcam)
        tbl_type = "atcam_match"_cs;
    return tbl_type;
}

IR::MAU::Table::Layout &IR::MAU::Table::Layout::operator +=(const IR::MAU::Table::Layout &a) {
    gateway             |= a.gateway;
    ternary             |= a.ternary;
    gateway_match       |= a.gateway_match;
    hash_action         |= a.hash_action;
    atcam               |= a.atcam;
    has_range           |= a.has_range;
    proxy_hash          |= a.proxy_hash;
    requires_versioning |= a.requires_versioning;
    is_lamb             |= a.is_lamb;
    is_direct           |= a.is_direct;
    is_local_tind       |= a.is_local_tind;
    if (a.action_data_bytes > action_data_bytes)
        action_data_bytes = a.action_data_bytes;
    if (a.action_data_bytes_in_table > action_data_bytes_in_table)
        action_data_bytes_in_table = a.action_data_bytes_in_table;
    total_actions       += a.total_actions;
    entries             += a.entries;
    ixbar_bytes         += a.ixbar_bytes;
    ixbar_width_bits    += a.ixbar_width_bits;
    match_width_bits    += a.match_width_bits;
    overhead_bits       += a.overhead_bits;
    immediate_bits      += a.immediate_bits;
    meter_addr          += a.meter_addr;
    stats_addr          += a.stats_addr;
    action_addr         += a.action_addr;
    ghost_bytes         += a.ghost_bytes;
    partition_bits      += a.partition_bits;
    partition_count     += a.partition_count;
    entries_per_set     += a.entries_per_set;
    sets_per_word       += a.sets_per_word;
    return *this;
}
/*
std::ostream &operator<<(std::ostream &out, IR::MAU::Table::Layout &layout) {
    Log::TempIndent indent;
    out << "layout: " << layout.entries << indent;
    if (layout.pre_classifier) out << 'c';
    if (layout.gateway) out << 'g';
    if (layout.exact) out << 'e';
    if (layout.ternary) out << 't';
    if (layout.hash_action) out << 'h';
    if (layout.gateway_match) out << 'G';
    if (layout.atcam) out << 'T';
    if (layout.alpm) out << 'L';
    if (layout.has_range) out << 'r';
    if (layout.proxy_hash) out << 'P';
    if (layout.requires_versioning) out << 'V';
    out << " ixbar:" << layout.ixbar_bytes << "B/" << layout.ixbar_width_bits << "b";
    out << " match:" << layout.match_bytes << "B/" << layout.match_width_bits << "b";
    if (layout.ghost_bytes) out << " gh:" << layout.ghost_bytes;
    if (layout.action_data_bytes || layout.action_data_bytes_in_table) {
        out << " adb:" << layout.action_data_bytes;
        if (layout.action_data_bytes_in_table)
            out << "/" << layout.action_data_bytes_in_table; }
    if (layout.overhead_bits) out << " ov:" << layout.overhead_bits;
    if (layout.immediate_bits) out << " imm:" << layout.immediate_bits;
    if (layout.partition_bits) out << " part:" << layout.partition_bits;
    return out;
}

/// same function with const parameter
std::ostream &operator<<(std::ostream &out, const IR::MAU::Table::Layout &layout) {
    Log::TempIndent indent;
    out << "layout: " << layout.entries << indent;
    if (layout.pre_classifier) out << 'c';
    if (layout.gateway) out << 'g';
    if (layout.exact) out << 'e';
    if (layout.ternary) out << 't';
    if (layout.hash_action) out << 'h';
    if (layout.gateway_match) out << 'G';
    if (layout.atcam) out << 'T';
    if (layout.alpm) out << 'L';
    if (layout.has_range) out << 'r';
    if (layout.proxy_hash) out << 'P';
    if (layout.requires_versioning) out << 'V';
    out << " ixbar:" << layout.ixbar_bytes << "B/" << layout.ixbar_width_bits << "b";
    out << " match:" << layout.match_bytes << "B/" << layout.match_width_bits << "b";
    if (layout.ghost_bytes) out << " gh:" << layout.ghost_bytes;
    if (layout.action_data_bytes || layout.action_data_bytes_in_table) {
        out << " adb:" << layout.action_data_bytes;
        if (layout.action_data_bytes_in_table)
            out << "/" << layout.action_data_bytes_in_table; }
    if (layout.overhead_bits) out << " ov:" << layout.overhead_bits;
    if (layout.immediate_bits) out << " imm:" << layout.immediate_bits;
    if (layout.partition_bits) out << " part:" << layout.partition_bits;
    return out;
}
*/
IR::MAU::Table::IndirectAddress
    &IR::MAU::Table::IndirectAddress::operator +=(const IR::MAU::Table::IndirectAddress &a) {
    shifter_enabled |= a.shifter_enabled;
    address_bits += a.address_bits;
    per_flow_enable |= a.per_flow_enable;
    meter_type_bits += a.meter_type_bits;
    return *this;
}

/** This function can only be used before TablePlacement, or essentially when the IR::MAU::Table
 *  resources object is not yet saved.
 *
 *  This is currently created as a key during the try_place_table algorithm.  These keys,
 *  after the IR is updated in the TablePlacement preorders, should be identical to the keys
 *  after table placement
 */
UniqueId IR::MAU::Table::pp_unique_id(const IR::MAU::AttachedMemory *at,
        bool is_gw, int stage_table, int logical_table,
        UniqueAttachedId::pre_placed_type_t ppt) const {
    BUG_CHECK(!is_placed(), "Illegal call of the pp_unique_id function on %s", this);

    UniqueId rv;
    rv.name = name;
    if (for_dleft())
        rv.speciality = UniqueId::DLEFT;
    if (layout.atcam)
        rv.speciality = UniqueId::ATCAM;

    // Note that for ATCAM/DLEFT tables, if the logical table isn't provided, then the
    // logical table is initialized to 0.  This is because all indirect attached tables
    // for an ATCAM are attached to logical table 0 in the assembler.  It is a lot easier
    // to call particular functions with this known ahead of time
    if (rv.speciality != UniqueId::NONE && logical_table == -1)
        rv.logical_table = 0;
    else
        rv.logical_table = logical_table;

    rv.stage_table = stage_table;
    rv.is_gw = is_gw;
    if (ppt != UniqueAttachedId::NO_PP)
        rv.a_id = UniqueAttachedId(ppt);
    else if (at != nullptr)
        rv.a_id = at->unique_id();

    return rv;
}

/** Building a unique id for a table after table placement.  Stage Table and Logical Table
 *  information as well as speciality information come directly from the IR.
 *
 *  If creating a unique id for an AttachedMemory table underneath a table in the DAG,
 *  then an AttachedMemory can be passed to the function.  The same is true for gateway information
 *
 *  A unique_id was created every time this function is called.  The reason these stay
 *  consistent through all of the passes is that the name of the table, the stage split of
 *  the table and the logical split of the table do not change after table placement.
 *
 *  For the AttachedMemory ids, these stay consistent if the name and type of the IR class
 *  remain consistent throughout the entirety of the allocation, which is the current invariant
 */
UniqueId IR::MAU::Table::unique_id(const IR::MAU::AttachedMemory *at, bool is_gw) const {
    BUG_CHECK(is_placed(), "Illegal call of the unique_id function");

    UniqueId rv;
    rv.name = name;
    rv.stage_table = stage_split;
    rv.logical_table = logical_split;

    if (for_dleft())
        rv.speciality = UniqueId::DLEFT;
    if (layout.atcam)
        rv.speciality = UniqueId::ATCAM;

    rv.is_gw = is_gw;
    if (at != nullptr)
        rv.a_id = at->unique_id();

    return rv;
}

UniqueId IR::MAU::Table::get_uid(const IR::MAU::AttachedMemory *at, bool is_gw) const {
    return is_placed() ? unique_id(at, is_gw) : pp_unique_id(at, is_gw);
}

const IR::MAU::BackendAttached *IR::MAU::Table::get_attached(UniqueId id) const {
    for (auto *at : attached)
        if (unique_id(at->attached) == id)
            return at;
    return nullptr;
}

const IR::MAU::BackendAttached *IR::MAU::Table::get_attached(const AttachedMemory *am) const {
    for (auto *at : attached)
        if (at->attached == am)
            return at;
    return nullptr;
}

bool IR::MAU::Table::hit_miss_p4() const {
    for (auto &n : next) {
        if (n.first == "$hit" || n.first == "$miss")
            return true;
    }
    return false;
}

bool IR::MAU::Table::action_chain() const {
    for (auto &n : next) {
        if (n.first[0] != '$') {
            return true;
        }
    }
    return has_exit_action() && has_non_exit_action();
}

bool IR::MAU::Table::has_default_path() const {
    for (auto &n : next) {
        if (n.first == "$default")
            return true;
    }
    return false;
}

bool IR::MAU::Table::has_exit_action() const {
    for (auto &n : actions)
        if (n.second->exitAction) return true;
    return false;
}

bool IR::MAU::Table::is_exit_table() const {
    if (has_exit_action() && !has_non_exit_action()) {
        return true;
    }
    return false;
}

const IR::MAU::Action *IR::MAU::Table::get_default_action() const {
    for (auto *act : Values(actions))
        if (act->init_default) return act;
    return nullptr;
}

std::vector<const IR::MAU::Action*> IR::MAU::Table::get_exit_actions() const {
    std::vector<const IR::MAU::Action*> exit_actions;
    for (auto &n : actions)
        if (n.second->exitAction)
            exit_actions.push_back(n.second);
    return exit_actions;
}

// FIXME -- consider memoizing these boolean predicate functions for speed...
bool IR::MAU::Table::has_exit_recursive() const {
    if (has_exit_action()) return true;
    for (auto *n : Values(next))
        for (auto *t : n->tables)
            if (t->has_exit_recursive()) return true;
    return false;
}

bool IR::MAU::Table::has_non_exit_action() const {
    for (auto &n : actions)
        if (!n.second->exitAction) return true;
    return false;
}

int IR::MAU::Table::action_next_paths() const {
    int action_paths = 0;
    for (auto &n : next) {
        if (n.first == "$default" || n.first[0] != '$')
            action_paths++;
    }
    if (has_exit_action())
        action_paths++;
    return action_paths;
}

/**
 * If the gateway is a standard conditional and does not use the gateway payload to
 * run an action
 */
bool IR::MAU::Table::conditional_gateway_only() const {
    return match_key.empty() && actions.empty();
}

/**
 * This will return true if the table is either a conditional gateway or a gateway that
 * is enabling a payload, for a hash action table
 */
bool IR::MAU::Table::is_a_gateway_table_only() const {
    if (conditional_gateway_only()) return true;
    if (match_key.empty() && !gateway_payload.empty())
        return true;
    return false;
}

const IR::Annotations *IR::MAU::Table::getAnnotations() const {
    return match_table ? match_table->getAnnotations() : IR::Annotations::empty;
}

const IR::Annotation *IR::MAU::Table::getAnnotation(cstring name) const {
    return match_table ? match_table->getAnnotation(name) : nullptr;
}

const IR::Expression *IR::MAU::Table::getExprAnnotation(cstring name) const {
    if (auto annot = getAnnotation(name)) {
        if (annot->expr.size() == 1)
            return annot->expr.at(0);
        error(ErrorType::ERR_UNEXPECTED, "%1%: %2% pragma provided to table %3% has multiple "
              "parameters, while only one is expected", annot, name, externalName()); }
    return nullptr;
}

bool IR::MAU::Table::getAnnotation(cstring name, int &val) const {
    if (auto *expr = getExprAnnotation(name)) {
        if (auto constant = expr->to<IR::Constant>()) {
            val = constant->asInt();
            return true;
        } else {
            error(ErrorType::ERR_INVALID, "Invalid annotation%1%: %2% pragma provided to "
                "table %3% is not a constant", expr->srcInfo, name, externalName()); } }
    return false;
}

bool IR::MAU::Table::getAnnotation(cstring name, bool &val) const {
    int iVal;
    val = false;
    if (getAnnotation(name, iVal)) {
        if (iVal == 1) {
            val = true;
        } else if (iVal != 0) {
            warning(::BFN::ErrorType::WARN_PRAGMA_USE,
                "Annotation %1% on table %2% ignored because parameter %3% is invalid. "
                "Only the values 0 and 1 are valid.", name, externalName(), iVal);
            return false;
        }
    }
    return true;
}

bool IR::MAU::Table::getAnnotation(cstring name, IR::ID &val) const {
    if (auto *expr = getExprAnnotation(name)) {
        if (auto v = expr->to<IR::StringLiteral>()) {
            val = *v;
            return true;
        } else {
            error(ErrorType::ERR_INVALID, "Invalid annotation%1%: %2% pragma provided to "
                  "table %3% is not a string literal", expr->srcInfo, name, externalName()); } }
    return false;
}

bool IR::MAU::Table::getAnnotation(cstring name, std::vector<IR::ID> &val) const {
    if (!match_table) return false;
    bool rv = false;
    for (auto *annot : match_table->getAnnotations()->annotations) {
        if (annot->name != name) continue;
        rv = true;  // found at least 1
        for (auto *expr : annot->expr) {
            if (auto v = expr->to<IR::StringLiteral>())
                val.push_back(*v);
            else
                error(ErrorType::ERR_INVALID, "Invalid %1%: %2% pragma provided to table %3% is "
                      "not a string literal", annot, name, externalName()); } }
    return rv;
}

int IR::MAU::Table::get_placement_priority_int() const {
    int val = 0;
    if (!match_table) return val;
    bool val_set = false;
    for (auto &annot : match_table->getAnnotations()->annotations) {
        if (annot->name != "placement_priority") continue;
        for (auto *expr : annot->expr) {
            if (auto constant = expr->to<IR::Constant>()) {
                if (val_set)
                    error(ErrorType::ERR_INVALID, "Invalid %1%: Only one integer value is "
                            "allowed for a placement_priority on table %2% for its global score",
                            annot, externalName());
                val = constant->asInt();
                val_set = true;
            }
        }
    }
    return val;
}

std::set<cstring> IR::MAU::Table::get_placement_priority_string() const {
    std::set<cstring> rv;
    if (!match_table) return rv;
    for (auto &annot : match_table->getAnnotations()->annotations) {
        if (annot->name != "placement_priority") continue;
        for (auto *expr : annot->expr) {
            if (auto sl = expr->to<IR::StringLiteral>()) {
                rv.insert(sl->value);
            }
        }
    }
    return rv;
}

int IR::MAU::Table::get_provided_stage(int geq_stage, int *req_entries, int *flags) const {
    if (conditional_gateway_only()) {
        int min_stage = -1;
        for (auto *seq : Values(next)) {
            int i = -1;
            for (auto *tbl : seq->tables) {
                if (seq->deps[++i]) continue;  // ignore tables dependent on earlier tables in seq
                int stage = tbl->get_provided_stage();
                if (stage < 0) return -1;  // no minimum stage
                if (stage < min_stage || min_stage == -1)
                    min_stage = stage; } }
        return min_stage; }
    if (!match_table) return -1;

    auto checkPragma = [](const IR::Annotation *annot) {
        bool valid_pragma = true;
        int intvals = 0;
        int idx = -1;
        for (auto *e : annot->expr) {
            ++idx;
            if (auto *k = e->to<IR::Constant>()) {
                if (k->asInt() < 0)
                    valid_pragma = false;
                ++intvals;
            } else if (e->is<IR::StringLiteral>())  {
                if (idx == 0)
                    valid_pragma = false;
            } else {
                valid_pragma = false; } }
        if (intvals < 1 || intvals > 2)
            valid_pragma = false;
        if (!valid_pragma)
            error(ErrorType::ERR_INVALID, "Invalid %1%: Stage pragma provided can have only "
                    "one or two constant parameters >= 0", annot);
        return valid_pragma;
    };

    const IR::Annotation *stage_annot = nullptr;
    auto stage_annotations = match_table->annotations->where([](const IR::Annotation *annot)
                                                             { return annot->name == "stage"; });
    if (!stage_annotations)
        return -1;

    for (auto *annot : stage_annotations->annotations) {
        if (!checkPragma(annot))
            return -1;

        int curr_stage = annot->expr.at(0)->to<IR::Constant>()->asInt();
        if (curr_stage >= geq_stage) {
            if (stage_annot == nullptr) {
                stage_annot = annot;
            } else {
                int min_stage = stage_annot->expr.at(0)->to<IR::Constant>()->asInt();
                stage_annot = curr_stage < min_stage ? annot : stage_annot;
            }
        }
    }

    if (!stage_annot)
        return -1;

    if (req_entries) {
        *req_entries = -1;
        for (size_t i = 1; i < stage_annot->expr.size(); ++i) {
            if (auto *k = stage_annot->expr.at(i)->to<IR::Constant>())
                *req_entries = k->asInt(); } }
    if (flags) {
        *flags = 0;
        for (auto *e : stage_annot->expr) {
            if (auto *l = e->to<IR::StringLiteral>()) {
                if (l->value == "immediate")
                    *flags |= StageFlag::Immediate;
                else if (l->value == "noimmediate")
                    *flags |= StageFlag::NoImmediate;
                else
                    error(ErrorType::ERR_INVALID, "Invalid flag %s in @stage annotation", l);
            }
        }
    }

    return stage_annot->expr.at(0)->to<IR::Constant>()->asInt();
}

int IR::MAU::Table::get_random_seed() const {
    int val = -1;
    if (getAnnotation("random_seed"_cs, val))
        ERROR_CHECK(val >= 0, "%s: random_seem pragma provided to table %s must be >= 0",
                    srcInfo, externalName());
    return val;
}

int IR::MAU::Table::get_pragma_max_actions() const {
    int pragma_val;
    if (getAnnotation("max_actions"_cs, pragma_val)) {
        int num_actions = actions.size();
        int max_limit = Device::imemSpec().rows() * Device::imemSpec().colors();
        if (pragma_val < num_actions) {
            error(ErrorType::ERR_INVALID, "%1%Invalid max_actions pragma usage on table %2%.  "
                  "The maximum actions (%3%) specified cannot be less than the number of "
                  "callable actions listed (%4%).",
                  srcInfo, externalName(), pragma_val, num_actions);
            return -1;
        } else if (pragma_val > max_limit) {
            error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%Invalid max_actions pragma usage on "
                  "table %2%.  The maximum actions specified (%3%) cannot exceed %4%.",
                  srcInfo, externalName(), pragma_val, max_limit);
            return -1;
        } else if (pragma_val > num_actions) {
            return pragma_val;
        }
    }
    return -1;
}

unsigned IR::MAU::Table::get_match_key_width() const {
    unsigned match_key_width = 0;
    for (auto &m : match_key) {
        if (m->for_match()) {
            match_key_width += m->expr->type->width_bits();
        }
    }
    return match_key_width;
}

IR::MAU::Table::ImmediateControl_t IR::MAU::Table::get_immediate_ctrl() const {
    int force_pragma_val = 0;
    int imm_pragma_val = 1;
    if (getAnnotation("force_immediate"_cs, force_pragma_val)) {
            if (force_pragma_val != 0 && force_pragma_val != 1) {
              error(ErrorType::ERR_INVALID, "%1%Invalid force_immediate pragma usage on "
                    "table %2%.  Only 0 and 1 are allowed.", srcInfo, externalName());
              return IR::MAU::Table::COMPILER; } }
    if (getAnnotation("immediate"_cs, imm_pragma_val)) {
            if (imm_pragma_val != 0 && imm_pragma_val != 1) {
              error(ErrorType::ERR_INVALID, "%1%Invalid immediate pragma usage on "
                    "table %2%.  Only 0 and 1 are allowed.", srcInfo, externalName());
              return IR::MAU::Table::COMPILER; } }

    if (force_pragma_val)
        return IR::MAU::Table::FORCE_IMMEDIATE;
    else if (imm_pragma_val == 0)
        return IR::MAU::Table::FORCE_NON_IMMEDIATE;
    else
        return IR::MAU::Table::COMPILER;
}

bool IR::MAU::Table::has_match_data() const {
    for (auto key : match_key) {
        if (key->for_match())
            return true;
    }
    return false;
}

int IR::MAU::Table::hit_actions() const {
    int _hit_actions = 0;
    for (auto act : Values(actions)) {
        if (!act->miss_only())
            _hit_actions++;
    }
    int pragma_max_actions = get_pragma_max_actions();
    if (pragma_max_actions > 0) return pragma_max_actions;
    return _hit_actions;
}

bool IR::MAU::Table::for_dleft() const {
    for (auto &k : match_key)
        if (k->for_dleft()) return true;
    return false;
}

cstring IR::MAU::Table::externalName() const {
    return match_table ? match_table->externalName() : name;
}

void IR::MAU::Table::remove_gateway() {
    for (auto &gw : gateway_rows)
        next.erase(gw.second);
    gateway_rows.clear();
    gateway_name = cstring();
    gateway_cond = cstring();
    gateway_payload.clear();
}

int IR::MAU::TableSeq::uid_ctr = 0;

cstring IR::MAU::Action::externalName() const {
    if (auto *name_annot = annotations->getSingle("name"_cs))
        return name_annot->getName();
    return name.toString();
}

int IR::MAU::HashGenExpression::nextId = 0;

const IR::MAU::SaluAction *IR::MAU::StatefulAlu::calledAction(
        const IR::MAU::Table* tbl,
        const IR::MAU::Action* act) const {
    auto ta_pair = tbl->name + "-" + act->name.originalName;
    if (!action_map.count(ta_pair)) return nullptr;
    auto *rv = instruction.get<SaluAction>(action_map.at(ta_pair));
    BUG_CHECK(rv, "No action %s in %s", action_map.at(ta_pair), this);
    return rv;
}

int IR::MAU::StatefulAlu::source_width() const {
    return std::min(Device::statefulAluSpec().MaxPhvInputWidth, width / (dual + 1));
}

}  // namespace MAU
}  // namespace IR
}  // namespace P4
