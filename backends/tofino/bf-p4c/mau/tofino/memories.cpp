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

#include "bf-p4c/mau/tofino/memories.h"

#include <functional>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/payload_gateway.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "lib/bitops.h"
#include "lib/log.h"
#include "lib/range.h"

namespace Tofino {
// Despite the namespace name, this code is shared for Tofino and JBay (Tofino 1/2)

static const char *use_type_to_str[] = {"EXACT",    "ATCAM",    "TERNARY",   "GATEWAY",
                                        "TIND",     "IDLETIME", "COUNTER",   "METER",
                                        "SELECTOR", "STATEFUL", "ACTIONDATA"};

static const char *sram_group_type_to_str[] = {
    "Exact", "Action", "Stats", "Meter", "Register", "Selector", "Tind", "Idletime", "Atcam"};

/** When anlyzing tables, the order is occasionally important, so we sort in order of
 * this priority.  For now the only important one is anaylyzing dleft tables before any
 * non-dleft table that may share the stateful ALU
 */
int Memories::table_alloc::analysis_priority() const { return table->for_dleft() ? 0 : 1; }

/** Building a UniqueId per table alloc.  The stage table comes from initialization, all other
 *  data must be provided
 */
UniqueId Memories::table_alloc::build_unique_id(const IR::MAU::AttachedMemory *at, bool is_gw,
                                                int logical_table,
                                                UniqueAttachedId::pre_placed_type_t ppt) const {
    return table->pp_unique_id(at, is_gw, stage_table, logical_table, ppt);
}

/**
 * The memuse map holds the allocation of each table that has RAM array requirements.
 * These include:
 *     RAMs/buses dedicated to match data
 *     RAMs/buses dedicated to any of the BackendAttached Objects
 *         - These can exist from the P4 program: (i.e. counter, meter)
 *         - These can be implied by requirements (i.e. direct action data, ternary indirect)
 *     gateways/buses for the gateways
 *
 * The key in the memuse map refers to a unique one of these objects, and the key is then
 * used to build a unique name for the assembly file.
 *
 * The other major issue is that multiple tables can access the same P4 object, (i.e. an
 * action profile).  There must be a unique allocation per P4 based object.
 *
 * In the memory use object, currently the P4 object is attached only to one table.  The table
 * that this P4 object is attached to has a unique id based on that table name.  All other tables
 * that use that P4 object keep a link to that table in the unattached_tables map.
 *
 * Thus the memory object initially decides which shared P4 object the table will be linked
 * to within the map, as well as keeps a map of the namespace around how to find the linked
 * table within the asm_output file.
 */

/**
 * Given the input parameters, this will return all of the UniqueIds associated with this
 * particular IR::MAU::Table object.  Generally this is a one-to-one relationship with
 * a few exceptions.
 *
 * ATCAM tables: The match portion and any direct BackendAttached tables (action data/idletime),
 * have multiple logical table units per allocation
 *
 * DLEFT tables: currently split so that each dleft cache is split as a logical table (though
 * in the long term this could be even stranger).  However by correctly setting up this
 * function, the corner casing only happens here
 *
 * This leaves room for any other odd corner casing we need to support
 */
safe_vector<UniqueId> Memories::table_alloc::allocation_units(
    const IR::MAU::AttachedMemory *at, bool is_gw, UniqueAttachedId::pre_placed_type_t ppt) const {
    safe_vector<UniqueId> rv;
    Log::TempIndent indent;
    LOG5("Allocation unit for memory : gateway: " << is_gw << ", uid: " << ppt);
    if (table->layout.atcam) {
        if (((at && at->direct) || at == nullptr) && is_gw == false) {
            for (int lt = 0; lt < layout_option->logical_tables(); lt++) {
                rv.push_back(build_unique_id(at, is_gw, lt, ppt));
                LOG5("Building unique id for allocation unit: " << rv.back());
            }
        } else {
            rv.push_back(build_unique_id(at, is_gw, 0, ppt));
            LOG5("Building unique id for allocation unit: " << rv.back());
        }
    } else if (table->for_dleft()) {
        for (int lt = 0; lt < layout_option->logical_tables(); lt++) {
            rv.push_back(build_unique_id(at, is_gw, lt, ppt));
            LOG5("Building unique id for allocation unit: " << rv.back());
        }
    } else {
        rv.push_back(build_unique_id(at, is_gw, -1, ppt));
        LOG5("Building unique id for allocation unit: " << rv.back());
    }
    return rv;
}

/**
 * Given the particular input parameters, this will return all of the UniqueIds that will not
 * have a provided allocation, but will be necessary to account for in the unattached_tables
 * map.
 *
 * ATCAM tables: For any indirect BackendAttached Tables, the UniqueId for any logical table > 0
 * will appear in the unattached object
 *
 * DLEFT some point in the future, maybe if other tables are attached, not just the stateful
 * ALU table.
 */
safe_vector<UniqueId> Memories::table_alloc::unattached_units(
    const IR::MAU::AttachedMemory *at, UniqueAttachedId::pre_placed_type_t ppt) const {
    safe_vector<UniqueId> rv;
    if (table->layout.atcam) {
        if (at && at->direct == false) {
            for (int lt = 1; lt < layout_option->logical_tables(); lt++) {
                rv.push_back(build_unique_id(at, false, lt, ppt));
            }
        }
    }
    return rv;
}

/**
 * The union of the allocation_units and the unattached_units
 */
safe_vector<UniqueId> Memories::table_alloc::accounted_units(
    const IR::MAU::AttachedMemory *at, UniqueAttachedId::pre_placed_type_t ppt) const {
    safe_vector<UniqueId> rv = allocation_units(at, false, ppt);
    safe_vector<UniqueId> vec2 = unattached_units(at, ppt);
    safe_vector<UniqueId> intersect;
    std::set_intersection(rv.begin(), rv.end(), vec2.begin(), vec2.end(),
                          std::back_inserter(intersect));
    BUG_CHECK(intersect.empty(),
              "%s: Error when accounting for uniqueness in logical units "
              "to address in memory allocation",
              table->name);
    rv.insert(rv.end(), vec2.begin(), vec2.end());
    return rv;
}

UniqueId Memories::SRAM_group::build_unique_id() const {
    return ta->build_unique_id(attached, false, logical_table, ppt);
}

Memories::result_bus_info Memories::SRAM_group::build_result_bus(int width_sect) const {
    if (ta->table_format->result_bus_words().getbit(width_sect))
        return result_bus_info(build_unique_id().build_name(), width_sect, logical_table);
    return result_bus_info();
}

void Memories::SRAM_group::dbprint(std::ostream &out) const {
    out << "SRAM group: " << ta->table->name << ", type: " << sram_group_type_to_str[type]
        << ", width: " << width << ", number: " << number;
    out << " { is placed: " << placed << ", depth " << depth << ", left to place "
        << left_to_place() << " }";
}

bool Memories::SRAM_group::same_wide_action(const SRAM_group &a) {
    return ta == a.ta && type == ACTION && a.type == ACTION && logical_table == a.logical_table;
}

unsigned Memories::side_mask(RAM_side_t side) const {
    switch (side) {
        case LEFT:
            return (1 << LEFT_SIDE_COLUMNS) - 1;
        case RIGHT:
            return ((1 << SRAM_COLUMNS) - 1) & ~((1 << LEFT_SIDE_COLUMNS) - 1);
        case RAM_SIDES:
            return ((1 << SRAM_COLUMNS) - 1);
        default:
            BUG("Unrecognizable RAM side to allocate");
    }
    return 0;
}

unsigned Memories::partition_mask(RAM_side_t side) {
    unsigned mask = (1 << MAX_PARTITION_RAMS_PER_ROW) - 1;
    if (side == RIGHT) mask <<= MAX_PARTITION_RAMS_PER_ROW;
    return mask;
}

int Memories::mems_needed(int entries, int depth, int per_mem_row, bool is_twoport) {
    int mems_needed = (entries + per_mem_row * depth - 1) / (depth * per_mem_row);
    return is_twoport ? mems_needed + 1 : mems_needed;
}

void Memories::clear_uses() {
    sram_use.clear();
    stash_use.clear();
    memset(sram_inuse, 0, sizeof(sram_inuse));
    tcam_use.clear();
    gateway_use.clear();
    sram_search_bus.clear();
    sram_print_search_bus.clear();
    sram_result_bus.clear();
    sram_print_result_bus.clear();
    memset(tcam_midbyte_use, -1, sizeof(tcam_midbyte_use));
    tind_bus.clear();
    action_data_bus.clear();
    overflow_bus.clear();
    twoport_bus.clear();
    vert_overflow_bus.clear();
    mapram_use.clear();
    memset(mapram_inuse, 0, sizeof(mapram_inuse));
    idletime_bus.clear();
    memset(gw_bytes_reserved, false, sizeof(gw_bytes_reserved));
    stats_alus.clear();
    meter_alus.clear();
    payload_use.clear();
}

void Memories::clear() {
    tables.clear();
    clear_table_vectors();
    clear_uses();
}

void Memories::clear_table_vectors() {
    exact_tables.clear();
    exact_match_ways.clear();
    ternary_tables.clear();
    tind_tables.clear();
    tind_groups.clear();
    action_tables.clear();
    indirect_action_tables.clear();
    selector_tables.clear();
    stats_tables.clear();
    meter_tables.clear();
    stateful_tables.clear();
    action_bus_users.clear();
    synth_bus_users.clear();
    gw_tables.clear();
    no_match_hit_tables.clear();
    no_match_miss_tables.clear();
    tind_result_bus_tables.clear();
    payload_gws.clear();
    normal_gws.clear();
    no_match_gws.clear();
    idletime_tables.clear();
    idletime_groups.clear();
}

void Memories::clear_allocation() {
    for (auto ta : tables) {
        for (auto &kv : *(ta->memuse)) {
            kv.second.clear_allocation();
        }
    }
}

/* Creates a new table_alloc object for each of the tables within the memory allocation */
void Memories::add_table(const IR::MAU::Table *t, const IR::MAU::Table *gw,
                         TableResourceAlloc *resources, const LayoutOption *lo,
                         const ActionData::Format::Use *af, ActionData::FormatType_t ft,
                         int entries, int stage_table, attached_entries_t attached_entries) {
    table_alloc *ta;
    auto gw_ixbar = resources->gateway_ixbar.get();
    // auto gw_ixbar = dynamic_cast<const IXBar::Use *>(resources->gateway_ixbar.get());
    if (!t->conditional_gateway_only()) {
        auto match_ixbar = resources->match_ixbar.get();
        // auto match_ixbar = dynamic_cast<const IXBar::Use *>(resources->match_ixbar.get());
        if (lo->layout.gateway_match || !ft.matchThisStage()) match_ixbar = gw_ixbar;
        ta = new table_alloc(t, match_ixbar, &resources->table_format, &resources->instr_mem, af,
                             &resources->memuse, lo, ft, entries, stage_table,
                             std::move(attached_entries));
    } else {
        ta = new table_alloc(t, gw_ixbar, nullptr, nullptr, nullptr, &resources->memuse, lo, ft,
                             entries, stage_table, std::move(attached_entries));
    }
    LOG2("Adding table " << ta->table->name << " with " << entries << " entries");
    tables.push_back(ta);
    if (gw != nullptr) {
        auto *ta_gw = new table_alloc(gw, gw_ixbar, nullptr, nullptr, nullptr, &resources->memuse,
                                      lo, ft, -1, stage_table, {});
        LOG2("Adding gateway table " << ta_gw->table->name << " to table " << ta_gw->table->name);
        ta_gw->link_table(ta);
        ta->link_table(ta_gw);
        tables.push_back(ta_gw);
    }
}

/** Due to gateways potentially requiring search buses that could may be used
 *  by exact match tables, the memories allocation can be run multiple times.  The difference
 *  factor would be the total number of RAMs per row exact match tables can be allocated to.
 *  If an exact match table can fit in a single row, only one search bus is required, which
 *  will provide access for gateways.
 *
 *  The algorithm is making a trade-off between search buses, and balance for other tables
 *  such as tind, synth2port, and action tables later.
 */
bool Memories::single_allocation_balance(mem_info &mi, unsigned row) {
    if (!allocate_all_atcam(mi)) return false;
    if (!allocate_all_exact(row)) return false;
    if (!allocate_all_ternary()) return false;
    if (!allocate_all_tind()) return false;
    if (!allocate_all_swbox_users()) return false;
    if (!allocate_all_idletime()) return false;
    if (!allocate_all_gw()) return false;
    if (!allocate_all_tind_result_bus_tables()) return false;
    if (!allocate_all_no_match_miss()) return false;
    return true;
}

/* Function that tests whether all added tables can be allocated to the stage */
bool Memories::allocate_all() {
    mem_info mi;

    failure_reason = cstring();
    LOG3("Analyzing tables " << tables << IndentCtl::indent);
    if (!analyze_tables(mi)) {
        LOG3_UNINDENT;
        if (!failure_reason) failure_reason = "analyze_tables failed"_cs;
        return false;
    }
    unsigned row = 0;
    bool column_balance_init = false;
    bool finished = false;
    calculate_entries();

    do {
        clear_uses();
        clear_allocation();
        calculate_column_balance(mi, row, column_balance_init);
        LOG2(" Column balance 0x" << hex(row));
        if (single_allocation_balance(mi, row)) {
            finished = true;
        }

        if (!finished) LOG2(" Increasing balance");
    } while (bitcount(row) < SRAM_COLUMNS && !finished);

    LOG3_UNINDENT;
    if (!finished) {
        if (!failure_reason) failure_reason = "unknown failure"_cs;
        return false;
    }

    LOG2("Memory allocation fits");
    return true;
}

bool Memories::allocate_all_dummies() {
    for (auto ta : tables) {
        gw_tables.push_back(ta);
    }
    return allocate_all_gw();
}

/* This class is responsible for filling in all of the particular lists with the corresponding
   twoport tables, as well as getting the sharing of indirect action tables and selectors correct
*/
class SetupAttachedTables : public MauInspector {
    Memories &mem;
    Memories::table_alloc *ta;
    int entries;
    const attached_entries_t &attached_entries;
    Memories::mem_info &mi;
    bool stats_pushed = false, meter_pushed = false, stateful_pushed = false;

    profile_t init_apply(const IR::Node *root) {
        profile_t rv = MauInspector::init_apply(root);
        if (ta->layout_option == nullptr) return rv;
        bool tind_check =
            ta->layout_option->layout.ternary && !ta->layout_option->layout.no_match_miss_path() &&
            !ta->layout_option->layout.gateway_match && ta->format_type.matchThisStage();
        if (tind_check) {
            if (ta->table_format->has_overhead()) {
                for (auto u_id : ta->allocation_units(nullptr, false, UniqueAttachedId::TIND_PP)) {
                    auto &alloc = (*ta->memuse)[u_id];
                    alloc.type = Memories::Use::TIND;
                    alloc.used_by = ta->table->externalName();
                    mi.tind_tables++;
                }
                int per_row = TernaryIndirectPerWord(&ta->layout_option->layout, ta->table);
                mem.tind_tables.push_back(ta);
                mi.tind_RAMs += mem.mems_needed(entries, Memories::SRAM_DEPTH, per_row, false);
            } else {
                mem.tind_result_bus_tables.push_back(ta);
            }
        }

        LOG5("Table: " << *ta << ", layout: " << ta->layout_option);
        if (ta->layout_option->layout.direct_ad_required()) {
            for (auto u_id : ta->allocation_units(nullptr, false, UniqueAttachedId::ADATA_PP)) {
                auto &alloc = (*ta->memuse)[u_id];
                alloc.type = Memories::Use::ACTIONDATA;
                alloc.used_by = ta->table->externalName();
                if (ta->layout_option->layout.pre_classifier) alloc.used_by += "_preclassifier";
                alloc.used_by += "$action";

                mi.action_tables++;
            }
            LOG5("Adding " << mi.action_tables << " action tables");
            mem.action_tables.push_back(ta);
            int width = 1;
            int per_row = ActionDataPerWord(&ta->layout_option->layout, &width);
            int depth = mem.mems_needed(entries, Memories::SRAM_DEPTH, per_row, false);
            mi.action_bus_min += width;
            mi.action_RAMs += depth * width;
        }
        return rv;
    }

    /** For shared attached tables, this initializes the unattached maps, if the attached
     *  table is already associated with a table
     */
    bool determine_unattached(const IR::MAU::AttachedMemory *at) {
        bool attached = false;
        safe_vector<UniqueId> attached_object_id;
        safe_vector<UniqueId> unattached_object_ids;
        if (!mem.shared_attached.count(at)) {
            // This P4 object is going to be attached to this table
            attached_object_id = ta->allocation_units(at);
            unattached_object_ids = ta->unattached_units(at);
            attached = true;
        } else {
            // This P4 object is already attached to a different IR::MAU::Table object
            attached_object_id = mem.shared_attached.at(at)->allocation_units(at);
            unattached_object_ids = ta->accounted_units(at);
        }

        safe_vector<UniqueId> intersect;
        std::set_intersection(attached_object_id.begin(), attached_object_id.end(),
                              unattached_object_ids.begin(), unattached_object_ids.end(),
                              std::back_inserter(intersect));
        BUG_CHECK(intersect.empty(),
                  "%s: Error when accounting for uniqueness in logical units "
                  "to address in memory allocation",
                  ta->table->name);

        for (auto unattached_id : unattached_object_ids) {
            // auto tbl_id = ta->build_unique_id(nullptr, false, unattached_id.logical_table);
            auto tbl_id = unattached_id.base_match_id();
            for (auto ao_id : attached_object_id)
                (*ta->memuse)[tbl_id].unattached_tables[unattached_id].insert(ao_id);
        }
        return !attached;
    }

    /* In order to only visit the attached tables of the current table */
    bool preorder(const IR::MAU::TableSeq *) { return false; }
    bool preorder(const IR::MAU::Action *) { return false; }

    bool preorder(const IR::MAU::ActionData *ad) {
        BUG_CHECK(!ad->direct, "No direct action data tables before table placement");
        if (determine_unattached(ad)) return false;

        for (auto u_id : ta->allocation_units(ad)) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Memories::Use::ACTIONDATA;
            alloc.used_by = ad->name.name;
            mi.action_tables++;
        }

        mem.indirect_action_tables.push_back(ta);
        mem.shared_attached[ad] = ta;
        int width = 1;
        int per_row = ActionDataPerWord(&ta->table->layout, &width);
        int depth =
            mem.mems_needed(attached_entries.at(ad).entries, Memories::SRAM_DEPTH, per_row, false);
        mi.action_bus_min += width;
        mi.action_RAMs += depth * width;
        return false;
    }

    bool preorder(const IR::MAU::Meter *mtr) {
        if (determine_unattached(mtr)) return false;

        for (auto u_id : ta->allocation_units(mtr)) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Memories::Use::METER;
            alloc.used_by = mtr->name.name;
            mi.meter_tables++;
        }

        mem.shared_attached[mtr] = ta;
        if (mtr->direct)
            mi.meter_RAMs += mem.mems_needed(entries, Memories::SRAM_DEPTH, 1, true);
        else
            mi.meter_RAMs +=
                mem.mems_needed(attached_entries.at(mtr).entries, Memories::SRAM_DEPTH, 1, true);

        if (!meter_pushed) {
            mem.meter_tables.push_back(ta);
            meter_pushed = true;
        }
        return false;
    }

    bool preorder(const IR::MAU::Counter *cnt) {
        if (determine_unattached(cnt)) return false;
        for (auto u_id : ta->allocation_units(cnt)) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Memories::Use::COUNTER;
            alloc.used_by = cnt->name.name;
            mi.stats_tables++;
        }
        mem.shared_attached[cnt] = ta;
        int per_row = CounterPerWord(cnt);
        if (cnt->direct)
            mi.stats_RAMs += mem.mems_needed(entries, Memories::SRAM_DEPTH, per_row, true);
        else
            mi.stats_RAMs += mem.mems_needed(attached_entries.at(cnt).entries, Memories::SRAM_DEPTH,
                                             per_row, true);

        if (!stats_pushed) {
            mem.stats_tables.push_back(ta);
            stats_pushed = true;
        }
        return false;
    }

    void init_dleft_learn_and_match(const IR::MAU::StatefulAlu *salu) {
        if (mem.shared_attached.count(salu)) {
            BUG("Currently sharing a dleft table between two tables is impossible");
        }
        auto u_ids = ta->accounted_units(salu);
        for (size_t i = 0; i < u_ids.size(); i++) {
            // Learn from every logical table before you
            for (size_t j = 0; j < i; j++) {
                (*ta->memuse)[u_ids[i]].dleft_learn.push_back(u_ids[j]);
            }
            // Match with all of the dleft hash tables in that stage
            for (size_t j = 0; j < u_ids.size(); j++) {
                if (i == j) continue;
                (*ta->memuse)[u_ids[i]].dleft_match.push_back(u_ids[j]);
            }
        }
    }

    bool preorder(const IR::MAU::StatefulAlu *salu) {
        if (determine_unattached(salu)) return false;

        for (auto u_id : ta->allocation_units(salu)) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Memories::Use::STATEFUL;
            alloc.used_by = salu->name.name;
            mi.stateful_tables++;
        }

        if (ta->table->for_dleft()) init_dleft_learn_and_match(salu);

        mem.shared_attached[salu] = ta;
        int per_row = RegisterPerWord(salu);
        if (salu->direct)
            mi.stateful_RAMs += mem.mems_needed(entries, Memories::SRAM_DEPTH, per_row, true);
        else
            mi.stateful_RAMs += mem.mems_needed(attached_entries.at(salu).entries,
                                                Memories::SRAM_DEPTH, per_row, true);

        if (!stateful_pushed) {
            mem.stateful_tables.push_back(ta);
            stateful_pushed = true;
        }
        return false;
    }

    bool preorder(const IR::MAU::TernaryIndirect *) {
        BUG("Should be no Ternary Indirect before table placement is complete");
    }

    bool preorder(const IR::MAU::Selector *as) {
        if (determine_unattached(as)) return false;

        for (auto u_id : ta->allocation_units(as)) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Memories::Use::SELECTOR;
            alloc.used_by = as->name.name;
        }

        mem.selector_tables.push_back(ta);
        mem.shared_attached[as] = ta;
        mi.selector_RAMs += 2;
        return false;
    }

    bool preorder(const IR::MAU::Table *tbl) {
        visit(tbl->attached);
        return false;
    }

    bool preorder(const IR::MAU::IdleTime *idle) {
        for (auto u_id : ta->allocation_units(idle)) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Memories::Use::IDLETIME;
            alloc.used_by = ta->table->externalName();
        }
        mem.idletime_tables.push_back(ta);
        int per_row = IdleTimePerWord(idle);
        mi.idletime_RAMs += mem.mems_needed(entries, Memories::SRAM_DEPTH, per_row, false);
        return false;
    }

 public:
    explicit SetupAttachedTables(Memories &m, Memories::table_alloc *t, int e,
                                 const attached_entries_t &ae, Memories::mem_info &i)
        : mem(m), ta(t), entries(e), attached_entries(ae), mi(i) {}
};

void Memories::set_logical_memuse_type(table_alloc *ta, Use::type_t type) {
    for (auto u_id : ta->allocation_units()) {
        (*ta->memuse)[u_id].type = type;
        (*ta->memuse)[u_id].used_by = ta->table->externalName();
    }
}

/* Run a quick analysis on all tables added by the table placement algorithm,
   and add the tables to their corresponding lists */
bool Memories::analyze_tables(mem_info &mi) {
    mi.clear();
    clear_table_vectors();
    std::stable_sort(tables.begin(), tables.end(), [](table_alloc *a, table_alloc *b) {
        return a->analysis_priority() < b->analysis_priority();
    });
    for (auto *ta : tables) {
        auto table = ta->table;
        int entries = ta->provided_entries;
        if (table->always_run == IR::MAU::AlwaysRun::ACTION) {
            continue;
        } else if (entries == -1 || entries == 0) {
            auto unique_id = ta->build_unique_id(nullptr, true);
            if (ta->table_link != nullptr) {
                unique_id = ta->table_link->build_unique_id(nullptr, true);
            } else {
                mi.independent_gw_tables++;
                mi.logical_tables++;
            }
            (*ta->memuse)[unique_id].type = Use::GATEWAY;
            gw_tables.push_back(ta);
            LOG4("Gateway table for " << ta->table->name);
            if (any_of(Values(ta->attached_entries),
                       [](attached_entries_element_t ent) { return ent.entries > 0; })) {
                // at least one attached table so need a no match table
                no_match_hit_tables.push_back(ta);
                ta->link_table(ta);  // link to itself to produce payload
                set_logical_memuse_type(ta, Use::EXACT);
                mi.no_match_tables++;
            } else {
                continue;
            }
        } else if (ta->layout_option->layout.no_match_rams()) {
            mi.logical_tables += ta->layout_option->logical_tables();
            if (ta->layout_option->layout.no_match_hit_path() ||
                ta->layout_option->layout.gateway_match) {
                // payload gateway tables are realy "no_match_hit_and_miss" -- they need
                // both the hit path from the gateway and the miss path.  So they really should
                // be allocated as empty exact match tables, but this is the closest we have
                no_match_hit_tables.push_back(ta);
                set_logical_memuse_type(ta, Use::EXACT);
            } else {
                BUG_CHECK(!ta->table->for_dleft(),
                          "DLeft tables can only be "
                          "part of the hit path");
                set_logical_memuse_type(ta, Use::TERNARY);
                // In order to potentially provide potential sizes for attached tables,
                // must at least have a size of 1
                no_match_miss_tables.push_back(ta);
            }
            mi.no_match_tables++;
        } else if (table->layout.atcam) {
            atcam_tables.push_back(ta);
            mi.atcam_tables++;
            mi.logical_tables += ta->layout_option->logical_tables();
            int width = ta->layout_option->way.width;
            int groups = ta->layout_option->way.match_groups;
            set_logical_memuse_type(ta, Use::ATCAM);
            int depth = mems_needed(entries, SRAM_DEPTH, groups, false);
            mi.match_RAMs += depth * width;
            mi.result_bus_min += ta->layout_option->logical_tables();
        } else if (!table->layout.ternary) {
            set_logical_memuse_type(ta, Use::EXACT);
            exact_tables.push_back(ta);
            mi.match_tables += ta->layout_option->logical_tables();
            mi.logical_tables += ta->layout_option->logical_tables();
            int width = ta->layout_option->way.width;
            int groups = ta->layout_option->way.match_groups;
            int depth = mems_needed(entries, SRAM_DEPTH, groups, false);
            mi.result_bus_min += width;
            mi.match_RAMs += depth;
        } else {
            set_logical_memuse_type(ta, Use::TERNARY);
            ternary_tables.push_back(ta);
            mi.ternary_tables += ta->layout_option->logical_tables();
            mi.logical_tables += ta->layout_option->logical_tables();
            int bytes = table->layout.match_bytes;
            int TCAMs_needed = 0;
            while (bytes > 11) {
                bytes -= 11;
                TCAMs_needed += 2;
            }

            if (bytes > 6)
                TCAMs_needed += 2;
            else
                TCAMs_needed += 1;

            int depth = mems_needed(entries, TCAM_DEPTH, 1, false);
            mi.ternary_TCAMs += TCAMs_needed * depth;
        }
        SetupAttachedTables setup(*this, ta, entries, ta->attached_entries, mi);
        ta->table->apply(setup);
    }
    return mi.constraint_check(logical_tables_allowed, failure_reason);
}

bool Memories::mem_info::constraint_check(int lt_allowed, cstring &failure_reason) const {
    if (match_tables + no_match_tables + ternary_tables + independent_gw_tables >
        Memories::TABLES_MAX) {
        LOG6(" match_tables(" << match_tables << ") + no_match_tables(" << no_match_tables
                              << ") + ternary_tables(" << ternary_tables
                              << ") + independent_gw_tables(" << independent_gw_tables
                              << ") > Memories::TABLES_MAX(" << Memories::TABLES_MAX << ")");
        failure_reason = "too many tables total"_cs;
        return false;
    }

    if (tind_tables > Memories::TERNARY_TABLES_MAX) {
        LOG6(" tind_tables(" << tind_tables << ") > Memories::TERNARY_TABLES_MAX("
                             << Memories::TERNARY_TABLES_MAX << ")");
        failure_reason = "too many tind tables"_cs;
        return false;
    }

    if (action_tables > Memories::ACTION_TABLES_MAX) {
        LOG6(" action_tables(" << action_tables << ") > Memories::ACTION_TABLES_MAX("
                               << Memories::ACTION_TABLES_MAX << ")");
        failure_reason = "too many action tables"_cs;
        return false;
    }

    if (action_bus_min > Memories::SRAM_ROWS * Memories::BUS_COUNT) {
        LOG6(" action_bus_min(" << action_bus_min << ") > Memories::SRAM_ROWS("
                                << Memories::SRAM_ROWS << ") * Memories::BUS_COUNT("
                                << Memories::BUS_COUNT << ")");
        failure_reason = "too many action busses"_cs;
        return false;
    }

    if (match_RAMs + action_RAMs + tind_RAMs > Memories::SRAM_ROWS * Memories::SRAM_COLUMNS) {
        LOG6(" match_RAMs(" << match_RAMs << ") + action_RAMs(" << action_RAMs << ") + tind_RAMs("
                            << tind_RAMs << ") > Memories::SRAM_ROWS(" << Memories::SRAM_ROWS
                            << ") * Memories::SRAM_COLUMNS(" << Memories::SRAM_COLUMNS << ")");
        failure_reason = "too many srams"_cs;
        return false;
    }

    if (ternary_tables > Memories::TERNARY_TABLES_MAX) {
        LOG6(" ternary_tables(" << ternary_tables << ") > Memories::TERNARY_TABLES_MAX("
                                << Memories::TERNARY_TABLES_MAX << ")");
        failure_reason = "too many ternary tables"_cs;
        return false;
    }

    if (stats_tables > Memories::STATS_ALUS) {
        LOG6(" stats_tables(" << stats_tables << ") > Memories::STATS_ALUS(" << Memories::STATS_ALUS
                              << ")");
        failure_reason = "too many stats tables"_cs;
        return false;
    }

    if (meter_tables + stateful_tables + selector_tables > Memories::METER_ALUS) {
        LOG6(" meter_tables(" << meter_tables << ") + stateful_tables(" << stateful_tables
                              << ") + selector_tables(" << selector_tables
                              << ") > Memories::METER_ALUS(" << Memories::METER_ALUS << ")");
        failure_reason = "too many meter alu users"_cs;
        return false;
    }

    if (meter_RAMs + stats_RAMs + stateful_RAMs + selector_RAMs + idletime_RAMs >
        Memories::MAPRAM_COLUMNS * Memories::SRAM_ROWS) {
        LOG6(" meter_RAMs(" << meter_RAMs << ") + stats_RAMs(" << stats_RAMs << ") + stateful_RAMs("
                            << stateful_RAMs << ") + selector_RAMs(" << selector_RAMs
                            << ") + idletime_RAMs(" << idletime_RAMs
                            << ") > Memories::MAPRAM_COLUMNS(" << Memories::MAPRAM_COLUMNS
                            << ") * Memories::SRAM_ROWS(" << Memories::SRAM_ROWS << ")");
        failure_reason = "too many map rams"_cs;
        return false;
    }

    if (logical_tables > lt_allowed) {
        LOG6(" logical_tables(" << logical_tables << ") > lt_allowed(" << lt_allowed << ")");
        failure_reason = "too many logical tables"_cs;
        return false;
    }

    return true;
}

void Memories::calculate_entries() {
    for (auto ta : exact_tables) {
        // Multiple entries if the table is using an SRAM to hold the dleft initial key
        BUG_CHECK(ta->allocation_units().size() == size_t(ta->layout_option->logical_tables()),
                  "Logical table mismatch on %s", ta->table->name);
        ta->calc_entries_per_uid.resize(ta->layout_option->logical_tables(), ta->provided_entries);
    }

    for (auto ta : atcam_tables) {
        BUG_CHECK(ta->allocation_units().size() == size_t(ta->layout_option->logical_tables()),
                  "Logical table mismatch on %s", ta->table->name);
        for (int lt = 0; lt < ta->layout_option->logical_tables(); lt++) {
            int search_bus_per_lt =
                (ta->table->layout.partition_count + SRAM_DEPTH - 1) / SRAM_DEPTH;
            /* we cannot use total_depth since the real number of allocations is max_partition_rams
             * times the number of ways the match happens */
            int lt_entries = search_bus_per_lt * ta->layout_option->partition_sizes[lt] *
                             ta->layout_option->way.match_groups * SRAM_DEPTH;
            ta->calc_entries_per_uid.push_back(lt_entries);
        }
    }

    for (auto ta : ternary_tables) {
        BUG_CHECK(ta->allocation_units().size() == size_t(ta->layout_option->logical_tables()),
                  "Logical table mismatch on %s", ta->table->name);
        ta->calc_entries_per_uid.resize(ta->layout_option->logical_tables(), ta->provided_entries);
    }

    for (auto ta : no_match_hit_tables) {
        BUG_CHECK(ta->allocation_units().size() == size_t(ta->layout_option->logical_tables()),
                  "Logical table mismatch on %s", ta->table->name);
        ta->calc_entries_per_uid.resize(ta->layout_option->logical_tables(), ta->provided_entries);
    }

    for (auto ta : no_match_miss_tables) {
        BUG_CHECK(ta->allocation_units().size() == size_t(ta->layout_option->logical_tables()),
                  "Logical table mismatch on %s", ta->table->name);
        ta->calc_entries_per_uid.resize(ta->layout_option->logical_tables(), 1);
    }
}

/* Calculate the size of the ways given the number of RAMs necessary */
safe_vector<int> Memories::way_size_calculator(int ways, int RAMs_needed) {
    safe_vector<int> vec;
    if (ways == -1) {
        // FIXME: If the number of ways are not provided, not yet considered
    } else {
        for (int i = 0; i < ways; i++) {
            if (RAMs_needed < ways - i) {
                RAMs_needed = ways - i;
            }

            int depth = (RAMs_needed + ways - i - 1) / (ways - i);
            int log2sz = floor_log2(depth);
            if (depth != (1 << log2sz)) depth = 1 << (log2sz + 1);

            RAMs_needed -= depth;
            vec.push_back(depth);
        }
    }
    return vec;
}

/**
 * Find the rows of SRAMs that can hold the table, verified as well by the busses set in SRAM
 */
safe_vector<std::pair<int, int>> Memories::available_SRAMs_per_row(unsigned mask, SRAM_group *group,
                                                                   int width_sect) {
    safe_vector<std::pair<int, int>> available_rams;
    auto group_search_bus = group->build_search_bus(width_sect);
    auto group_result_bus = group->build_result_bus(width_sect);
    for (int i = 0; i < SRAM_ROWS; i++) {
        if (!search_bus_available(i, group_search_bus)) continue;
        if (group_result_bus.init && !result_bus_available(i, group_result_bus)) continue;
        available_rams.push_back(std::make_pair(i, bitcount(mask & ~sram_inuse[i])));
    }

    std::sort(available_rams.begin(), available_rams.end(),
              [&](const std::pair<int, int> a, const std::pair<int, int> b) {
                  int t;
                  if (a.second != 0 && b.second == 0) return true;
                  if (a.second == 0 && b.second != 0) return false;

                  // Must fully place a partition
                  if (group->type == SRAM_group::ATCAM) {
                      if (group->left_to_place() <= a.second && group->left_to_place() > b.second)
                          return true;
                      if (group->left_to_place() > a.second && group->left_to_place() <= b.second)
                          return false;
                  }

                  // Prefer sharing a search bus/result bus if there is one to share
                  auto bus = sram_search_bus[a.first];
                  bool a_matching_bus = bus[0] == group_search_bus || bus[1] == group_search_bus;
                  bus = sram_search_bus[b.first];
                  bool b_matching_bus = bus[0] == group_search_bus || bus[1] == group_search_bus;
                  if (a_matching_bus != b_matching_bus) return a_matching_bus;

                  if ((t = a.second - b.second) != 0) return t > 0;
                  return a > b;
              });
    return available_rams;
}

/* Simple now.  Just find rows with the available RAMs that it is asking for */
safe_vector<int> Memories::available_match_SRAMs_per_row(unsigned selected_columns_mask,
                                                         unsigned total_mask,
                                                         std::set<int> used_rows, SRAM_group *group,
                                                         int width_sect) {
    safe_vector<int> matching_rows;
    auto group_search_bus = group->build_search_bus(width_sect);
    auto group_result_bus = group->build_result_bus(width_sect);

    for (int i = 0; i < SRAM_ROWS; i++) {
        if (used_rows.find(i) != used_rows.end()) continue;
        if (!search_bus_available(i, group_search_bus)) continue;
        if (group_result_bus.init && !result_bus_available(i, group_result_bus)) continue;

        if (bitcount(selected_columns_mask & ~sram_inuse[i]) == bitcount(selected_columns_mask))
            matching_rows.push_back(i);
    }

    std::sort(matching_rows.begin(), matching_rows.end(), [=](const int a, const int b) {
        int t;
        if ((t = bitcount(~sram_inuse[a] & total_mask) - bitcount(~sram_inuse[b] & total_mask)) !=
            0)
            return t < 0;

        auto bus = sram_search_bus[a];
        bool a_matching_bus = bus[0] == group_search_bus || bus[1] == group_search_bus;
        bus = sram_search_bus[b];
        bool b_matching_bus = bus[0] == group_search_bus || bus[1] == group_search_bus;

        if (a_matching_bus != b_matching_bus) return a_matching_bus;

        return a > b;
    });
    return matching_rows;
}

/* Based on the number of ways provided by the layout option, the ways sizes and
   select masks are initialized and provided to the way allocation algorithm */
void Memories::break_exact_tables_into_ways() {
    exact_match_ways.clear();
    for (auto *ta : exact_tables) {
        auto match_ixbar = dynamic_cast<const IXBar::Use *>(ta->match_ixbar);
        BUG_CHECK(match_ixbar, "No match ixbar allocated?");
        for (auto u_id : ta->allocation_units()) {
            (*ta->memuse)[u_id].ways.clear();
            (*ta->memuse)[u_id].row.clear();
            int index = 0;
            for (auto &way : match_ixbar->way_use) {
                SRAM_group *wa = new SRAM_group(ta, ta->layout_option->way_sizes[index],
                                                ta->layout_option->way.width, index, way.source,
                                                SRAM_group::EXACT);
                wa->logical_table = u_id.logical_table;

                exact_match_ways.push_back(wa);
                (*ta->memuse)[u_id].ways.emplace_back(ta->layout_option->way_sizes[index],
                                                      way.select_mask);
                index++;
            }
        }
        BUG_CHECK(match_ixbar->way_use.size() == ta->layout_option->way_sizes.size(),
                  "Mismatch of memory ways and ixbar ways");
    }

    std::sort(exact_match_ways.begin(), exact_match_ways.end(),
              [=](const SRAM_group *a, const SRAM_group *b) {
                  int t;
                  if ((t = a->width - b->width) != 0) return t > 0;
                  if ((t = (a->left_to_place()) - (b->left_to_place())) != 0) return t > 0;
                  return a->logical_table < b->logical_table;
              });
}

/* Selects the best way to begin placing on the row, based on what was previously placed
   within this row.
*/
Memories::SRAM_group *Memories::find_best_candidate(SRAM_group *placed_wa, int row, int &loc) {
    if (exact_match_ways.empty()) return nullptr;
    auto bus = sram_search_bus[row];

    loc = 0;
    for (auto emw : exact_match_ways) {
        auto group_bus = emw->build_search_bus(placed_wa->width - 1);
        if ((!bus[0].free() && bus[0] == group_bus) || (!bus[1].free() && bus[1] == group_bus))
            return emw;
        loc++;
    }

    if (!bus[0].free() && !bus[1].free()) {
        return nullptr;
    }

    // FIXME: Perhaps do a best fit algorithm here
    loc = 0;
    return exact_match_ways[0];
}

/* Fill out the remainder of the row with other ways! */
bool Memories::fill_out_row(SRAM_group *placed_way, int row, unsigned column_mask) {
    int loc = 0;
    // FIXME: Need to adjust to the proper mask provided by earlier function
    while (bitcount(column_mask & ~sram_inuse[row]) > 0) {
        SRAM_group *way = find_best_candidate(placed_way, row, loc);
        if (way == nullptr) return true;

        match_selection match_select;
        if (!determine_match_rows_and_cols(way, row, column_mask, match_select, false))
            return false;
        fill_out_match_alloc(way, match_select, false);

        if (way->all_placed()) exact_match_ways.erase(exact_match_ways.begin() + loc);
    }
    return true;
}

bool Memories::search_bus_available(int search_row, search_bus_info &sbi) {
    for (auto bus : sram_search_bus[search_row]) {
        if (bus.free() || bus == sbi) return true;
    }
    return false;
}

bool Memories::result_bus_available(int search_row, result_bus_info &mbi) {
    for (auto bus : sram_result_bus[search_row]) {
        if (bus.free() || bus == mbi) return true;
    }
    return false;
}

/* Returns the search bus that we are selecting on this row */
int Memories::select_search_bus(SRAM_group *group, int width_sect, int row) {
    auto group_search_bus = group->build_search_bus(width_sect);
    int index = 0;
    for (auto bus : sram_search_bus[row]) {
        if (bus == group_search_bus) return index;
        index++;
    }

    index = 0;
    for (auto bus : sram_search_bus[row]) {
        if (bus.free()) return index;
        index++;
    }
    return -1;
}

int Memories::select_result_bus(SRAM_group *group, int width_sect, int row) {
    auto group_result_bus = group->build_result_bus(width_sect);
    if (!group_result_bus.init) return -1;
    int index = 0;
    for (auto bus : sram_result_bus[row]) {
        if (bus == group_result_bus) return index;
        index++;
    }
    index = 0;
    for (auto bus : sram_result_bus[row]) {
        if (bus.free()) return index;
        index++;
    }
    return -2;
}

/* Picks an empty/most open row, and begins to fill it in within a way */
bool Memories::find_best_row_and_fill_out(unsigned column_mask) {
    SRAM_group *way = exact_match_ways[0];
    safe_vector<std::pair<int, int>> available_rams =
        available_SRAMs_per_row(column_mask, way, way->width - 1);
    // No memories left to place anything
    if (available_rams.size() == 0) {
        failure_reason = "no rams left in exact_match.find_best_row"_cs;
        return false;
    }

    int row = available_rams[0].first;
    match_selection match_select;
    if (available_rams[0].second == 0) {
        failure_reason = "empty row in exact_match.find_best_row"_cs;
        return false;
    }
    if (!determine_match_rows_and_cols(way, row, column_mask, match_select, false)) return false;
    fill_out_match_alloc(way, match_select, false);

    if (way->all_placed()) exact_match_ways.erase(exact_match_ways.begin());

    if (bitcount(~sram_inuse[row] & column_mask) == 0)
        return true;
    else
        return fill_out_row(way, row, column_mask);
}

/* Determining if the side from which we want to give extra columns to the exact match
   allocation, potentially cutting room from either tind tables or twoport tables */
bool Memories::cut_from_left_side(const mem_info &mi, int left_given_columns,
                                  int right_given_columns) {
    if (right_given_columns > mi.columns(mi.right_side_RAMs()) &&
        left_given_columns <= mi.columns(mi.left_side_RAMs())) {
        return false;
    } else if (right_given_columns <= mi.columns(mi.right_side_RAMs()) &&
               left_given_columns > mi.columns(mi.left_side_RAMs())) {
        return true;
    } else if (mi.columns(mi.left_side_RAMs()) < left_given_columns) {
        return true;
    } else if (mi.columns(mi.right_side_RAMs()) < right_given_columns) {
        return false;
    } else if (left_given_columns > 0) {
        return true;
    } else if (left_given_columns == 0) {
        return false;
    } else {
        BUG("We have a problem in calculate column balance");
    }
}

/* Calculates the number of columns and the distribution of columns on the left and
   right side of the SRAM array in order to place all exact match tables */
void Memories::calculate_column_balance(const mem_info &mi, unsigned &row,
                                        bool &column_balance_init) {
    int min_columns_required = (mi.match_RAMs + SRAM_COLUMNS - 1) / SRAM_COLUMNS;
    int left_given_columns = 0;
    int right_given_columns = 0;

    if (!column_balance_init) {
        column_balance_init = true;
        left_given_columns = mi.left_side_RAMs();
        right_given_columns = mi.right_side_RAMs();

        bool add_to_right = true;
        while (mi.columns(mi.non_SRAM_RAMs()) > left_given_columns + right_given_columns) {
            if (add_to_right) {
                right_given_columns++;
                add_to_right = false;
            } else {
                left_given_columns++;
                add_to_right = true;
            }
        }

        while (min_columns_required > SRAM_COLUMNS - (left_given_columns + right_given_columns)) {
            if (cut_from_left_side(mi, left_given_columns, right_given_columns)) {
                left_given_columns--;
            } else {
                right_given_columns--;
            }
        }
    } else {
        left_given_columns = bitcount(~row & 0xf);
        right_given_columns = bitcount(~row & 0x3f0);
        if (cut_from_left_side(mi, left_given_columns, right_given_columns))
            left_given_columns--;
        else
            right_given_columns--;
    }

    unsigned mask = 0;
    for (int i = 0; i < LEFT_SIDE_COLUMNS - left_given_columns; i++) {
        mask |= (1 << i);
    }
    for (int i = 0; i < SRAM_COLUMNS - LEFT_SIDE_COLUMNS - right_given_columns; i++) {
        mask |= (1 << (i + LEFT_SIDE_COLUMNS));
    }
    row = mask;
}

/* Allocates all of the ways */
bool Memories::allocate_all_exact(unsigned column_mask) {
    allocation_count = 0;
    Log::TempIndent indent;
    LOG3("Allocating all exact tables" << indent);  // FIXME -- suppress log if there are none
    break_exact_tables_into_ways();
    while (exact_match_ways.size() > 0) {
        if (find_best_row_and_fill_out(column_mask) == false) {
            LOG4("failed to find best row");
            return false;
        }
    }
    compress_ways(false);
    // Determine stashes
    // With all exact match tables allocated, we determine where to place
    // stashes.
    // 2 stash entries are assigned per hash group (function id). If entry spans
    // multiple ram rows, a stash unit in each row will be associated with a
    // part of the wide stash entry
    if (Device::currentDevice() == Device::TOFINO) {
        // Currently stashes are only used for atomic modify (of entries) within
        // the RAM. Since JBAY HW has a way to deal with this (by introducing a
        // bubble) stashes are not required. We disable it for now.
        ordered_map<table_alloc *, int> table_alloc_ram_widths;
        for (auto *ta : exact_tables) {
            for (auto u_id : ta->allocation_units()) {
                auto &alloc = (*ta->memuse)[u_id];
                for (auto mem_way : alloc.ways) {
                    int ram_width = mem_way.rams.size() / mem_way.size;
                    if (ram_width > table_alloc_ram_widths[ta])
                        table_alloc_ram_widths[ta] = ram_width;
                }
            }
        }
        // Flip map to create a sorted multimap of ram widths (greater first) to table allocs
        std::multimap<int, table_alloc *, std::greater<int>> ram_width_table_allocs;
        for (auto &tr : table_alloc_ram_widths) {
            ram_width_table_allocs.insert(std::pair<int, table_alloc *>(tr.second, tr.first));
        }
        LOG7("Sorted table alloc for stash allocation");
        for (auto &ta : ram_width_table_allocs) {
            LOG7("\tRam width: " << ta.first << ", exact table: " << ta.second->table->name);
        }
        // Iterate over the sorted multimap, table allocs with wider matches are
        // allocated stashes first since they have to be aligned to same stash
        // column. However for single RAM matches there is no such restriction
        // and they can go in any column.
        for (auto &tr : ram_width_table_allocs) {
            auto *ta = tr.second;
            auto match_ixbar = dynamic_cast<const IXBar::Use *>(ta->match_ixbar);
            BUG_CHECK(match_ixbar, "match_ixbar wrong type");
            for (auto u_id : ta->allocation_units()) {
                LOG5("Exact match table " << u_id.build_name());
                auto &alloc = (*ta->memuse)[u_id];
                int wayno = 0;
                auto ixbar_way = match_ixbar->way_use.begin();
                // Store stash rows per hash group which need stash allocation
                std::map<int, std::vector<int>> stash_map;
                for (auto mem_way : alloc.ways) {
                    BUG_CHECK(ixbar_way != match_ixbar->way_use.end(), "No ixbar_way found!");
                    LOG5("\tway group : " << ixbar_way->source << " - way: " << wayno
                                          << " - size : " << mem_way.size
                                          << " - mem way: " << mem_way);
                    if (stash_map.count(ixbar_way->source) == 0) {
                        auto ram_width = mem_way.rams.size() / mem_way.size;
                        LOG6("\tRam Width: " << ram_width
                                             << ", mem way rams size : " << mem_way.rams.size()
                                             << " mem way size : " << mem_way.size);
                        // Determine stash unit, all units should be aligned in
                        // the same stash column for wide matches
                        auto stash_unit = -1;
                        for (int i = 0; i < STASH_UNITS; i++) {
                            if (stash_unit == -1) {
                                LOG7("\t\tAssigning for stash unit " << i);
                                unsigned width = 0;
                                // For wide RAMS, the stash units must be
                                // aligned in the same column on all rows of the
                                // wide match.
                                if (ram_width > 1) {
                                    for (auto ram : mem_way.rams) {
                                        LOG7("\t\t\tRam : " << ram.first << " stash unit: "
                                                            << stash_use[ram.first][i]
                                                            << " width: " << width
                                                            << ", ram_width: " << ram_width);
                                        if (width >= ram_width) break;
                                        if (!stash_use[ram.first][i].isNullOrEmpty()) {
                                            stash_unit = -1;
                                            break;
                                        }
                                        stash_unit = i;
                                        LOG7("\t\t\tAssigned stash unit " << stash_unit);
                                        width++;
                                    }
                                    // For single RAM width allocate any slot
                                    // available on any row the RAMs are present
                                } else {
                                    for (auto ram : mem_way.rams) {
                                        LOG7("\t\t\tRam : " << ram.first << " stash unit: "
                                                            << stash_use[ram.first][i]
                                                            << ", ram_width: " << ram_width);
                                        if (!stash_use[ram.first][i].isNullOrEmpty()) {
                                            continue;
                                        }
                                        stash_unit = i;
                                        LOG7("\t\t\tAssigned stash unit " << stash_unit);
                                        break;
                                    }
                                }
                            }
                            // Set the determined stash unit on table
                            if (stash_unit >= 0) {
                                unsigned width = 0;
                                for (auto ram : mem_way.rams) {
                                    if (width >= ram_width) break;
                                    stash_map[ixbar_way->source].push_back(ram.first);
                                    stash_use[ram.first][stash_unit] = u_id.build_name();
                                    LOG4("\t\tRams : " << "[" << ram.first << ", "
                                                       << (ram.second + 2) << "]"
                                                       << " - stash unit : " << stash_unit);
                                    for (auto &r : alloc.row) {
                                        if (r.row == ram.first) {
                                            r.stash_unit = stash_unit;
                                            r.stash_col = ram.second + 2;
                                            break;
                                        }
                                    }
                                    width++;
                                }
                                break;
                            }
                        }
                        LOG7("\tStash Usage: \n" << stash_use);
                        BUG_CHECK(stash_unit >= 0, "No stash unit found for table %1%",
                                  u_id.build_name());
                    }
                    wayno++;
                    ixbar_way++;
                }
            }
        }
    }

    for (auto *ta : exact_tables) {
        for (auto u_id : ta->allocation_units()) {
            LOG4("Exact match table " << u_id.build_name());
            auto alloc = (*ta->memuse)[u_id];
            for (auto row : alloc.row) {
                LOG4("Row is " << row.row << " and bus is " << row.bus);
                LOG4("Col is " << row.col);
            }
            int wayno = 0;
            for (auto way : alloc.ways) {
                LOG4("Rams for way " << wayno);
                for (auto ram : way.rams) {
                    LOG4(ram.first << ", " << ram.second);
                }
                wayno++;
            }
        }
    }
    return true;
}

void Memories::compress_row(Use &alloc) {
    std::stable_sort(alloc.row.begin(), alloc.row.end(),
                     [=](const Memories::Use::Row a, const Memories::Use::Row b) {
                         int t;
                         if ((t = a.row - b.row) != 0) return t < 0;
                         if ((t = a.bus - b.bus) != 0) return t < 0;
                         return a.alloc < b.alloc;
                     });
    for (size_t i = 0; i + 1 < alloc.row.size(); i++) {
        if (alloc.row[i].row == alloc.row[i + 1].row && alloc.row[i].bus == alloc.row[i + 1].bus) {
            BUG_CHECK(alloc.row[i].word == alloc.row[i + 1].word,
                      "SRAMs that share a row "
                      "and bus must share the same word");
            alloc.row[i].col.insert(alloc.row[i].col.end(), alloc.row[i + 1].col.begin(),
                                    alloc.row[i + 1].col.end());
            alloc.row.erase(alloc.row.begin() + i + 1);
            i--;
        }
    }

    std::stable_sort(alloc.row.begin(), alloc.row.end(),
                     [=](const Memories::Use::Row &a, const Memories::Use::Row &b) {
                         int t;
                         if ((t = a.alloc - b.alloc) != 0) return t < 0;
                         if ((t = a.word - b.word) != 0) return t < 0;
                         return a.row < b.row;
                     });
}

/** Adjust the Use structures so that there is is only one Use::Row object per row and bus.
 *  Otherwise, the algorithm will complain of a bus collision on a row for the same table
 */
void Memories::compress_ways(bool atcam) {
    if (atcam) {
        for (auto *ta : atcam_tables) {
            for (auto u_id : ta->allocation_units()) {
                auto &alloc = (*ta->memuse)[u_id];
                compress_row(alloc);
            }
        }
    } else {
        for (auto *ta : exact_tables) {
            for (auto u_id : ta->allocation_units()) {
                auto &alloc = (*ta->memuse)[u_id];
                compress_row(alloc);
            }
        }
    }
}

/** Creates the separate SRAM_group objects for the atcam partitions.  A partition is created per
 *  logical table per way in that logical table.  Let me demonstrate the math in the following
 *  example:
 *
 *  Say table t has a size of 122880 entries.  The partition index will be 12 bits in size, and
 *  the table key can fit two entries per RAM, and is only one RAM wide.  How many RAMs are
 *  required, and how should the algorithm split these partitions.
 *
 *  A 12 bit partition index will require 4 independent RAMs to lookup simultaneously.  This
 *  is because selecting a RAM line requires 10 bits, as the RAMs have 2^10 = 1024 entries.  The
 *  2 bits provides 2^2 = 4 degrees of freedom to select one of the RAMs.
 *
 *  The smaller table with this setup will be the following amount of entries:
 *      1024 RAM rows * 2 entries per RAM * 1 RAM per wide match * 4 degrees of freedom
 *
 *  This is equal to 8096 entries.  Furthermore, as the table grows, it grows by 4 RAMs, as
 *  a RAM is required by each degree of freedom.
 *
 *  The total number of RAMs will be:
 *     122880 entries / (1024 RAM rows * 2 entries per RAM) = 60 RAMs.
 *  However, the number of simultaneous RAMs looked up will be:
 *      60 RAMs / 4 degrees of freedom = 15 RAMs
 *  This means that the simultaneous entries looked up will be:
 *      15 RAMs * 2 entries per RAM = 30 simultaneous entries.
 *
 *  Now, due to the constraints of Tofino, if used in an algorithmic TCAM table, in order to
 *  cascade the priority, RAMs that are simultaneously looked up have to be within the same
 *  SRAM row.  Furthermore, one can only have a maximum of 6 RAMs cascading priority.  This is
 *  described in section 6.4.3.1.1 Exact Match Prioritization in the uArch.  In the algorithm
 *  we limit the maximum cascading to 5 RAMs.  The reason for this discrepancy is that the
 *  RAM rows are only 10 RAMs, and the algorithm tries to balance putting partitions on the right
 *  and left side of the RAM array.  6 cascading priorities comes out of the RAM row initially
 *  being 12 RAMs.
 *
 *  Thus with our example, we require the following number of rows:
 *      15 RAMs / 5 maximum rams per row = 3 total rows.
 *
 *  In order to get the prioritization to cross rows, the original ATCAM must be split into
 *  separate logical tables.  The number of logical tables required is the number of independent
 *  simultaneous look up rows, in our example 3 logical tables.  These logical tables are chained
 *  in a hit/miss chain.  On a hit a logical table will point to the original next table of the
 *  algorithmic TCAM table.  On a miss, the logical table will go to the next logical table in the
 *  chain.  Through this structure, earlier logical tables have a higher priority.
 */
void Memories::break_atcams_into_partitions() {
    atcam_partitions.clear();

    for (auto *ta : atcam_tables) {
        int search_bus_per_lt = (ta->table->layout.partition_count + SRAM_DEPTH - 1) / SRAM_DEPTH;
        for (auto u_id : ta->allocation_units()) {
            int lt = u_id.logical_table;
            (*ta->memuse)[u_id].row.clear();
            (*ta->memuse)[u_id].ways.clear();
            for (int j = 0; j < search_bus_per_lt; j++) {
                // Order is RAM depth, RAM width, number (i.e. which search bus), hash function
                auto part = new SRAM_group(ta, ta->layout_option->partition_sizes[lt],
                                           ta->layout_option->way.width, j, 0, SRAM_group::ATCAM);
                part->logical_table = lt;
                atcam_partitions.push_back(part);
            }
            auto match_ixbar = dynamic_cast<const IXBar::Use *>(ta->match_ixbar);
            BUG_CHECK(match_ixbar, "No match ixbar allocated?");
            BUG_CHECK(match_ixbar->way_use.size() == 1,
                      "Somehow multiple ixbar ways "
                      "calculated for an ATCAM table");
            auto way = match_ixbar->way_use[0];
            for (int j = 0; j < ta->layout_option->partition_sizes[lt]; j++) {
                (*ta->memuse)[u_id].ways.emplace_back(search_bus_per_lt, way.select_mask);
            }
        }
    }
    std::sort(atcam_partitions.begin(), atcam_partitions.end(),
              [=](const SRAM_group *a, const SRAM_group *b) {
                  int t;
                  if ((t = a->width - b->width) != 0) return t > 0;
                  if ((t = (a->left_to_place()) - (b->left_to_place())) != 0) return t > 0;
                  return a->logical_table < b->logical_table;
              });
}

/** Given an initial row to place a way or partition on, determine which search/match bus,
 *  and which RAMs in that row to assign to the partition.  If the match is wide, then one
 *  must find rows with columns.  Due to the constraint that wide matches must be in the same
 *  row in order for the hit signals to be able to traverse wide matches, the columns in each
 *  row must be the same.
 */
bool Memories::determine_match_rows_and_cols(SRAM_group *group, int row, unsigned column_mask,
                                             match_selection &match_select, bool atcam) {
    // Pick a bus
    int search_bus = select_search_bus(group, group->width - 1, row);
    BUG_CHECK(search_bus >= 0, "Search somehow found an impossible bus");
    int result_bus = select_result_bus(group, group->width - 1, row);
    BUG_CHECK(result_bus >= -1, "Somehow found an impossible result bus");
    match_select.rows.push_back(row);
    match_select.search_buses[row] = search_bus;
    match_select.result_buses[row] = result_bus;
    int cols = 0;
    std::set<int> determined_rows;
    determined_rows.emplace(row);
    match_select.widths[row] = group->width - 1;

    // Pick available columns
    for (int i = 0; i < SRAM_COLUMNS && cols < group->left_to_place(); i++) {
        if (((1 << i) & column_mask) == 0) continue;
        if (sram_use[row][i]) continue;
        match_select.column_mask |= (1 << i);
        match_select.cols.push_back(i);
        cols++;
    }

    // If the match is wide, pick rows that have these columns available
    for (int i = group->width - 2; i >= 0; i--) {
        safe_vector<int> matching_rows = available_match_SRAMs_per_row(
            match_select.column_mask, column_mask, determined_rows, group, i);
        if (matching_rows.size() == 0) {
            failure_reason = "empty row for wide match in determine_match_rows_and_cols"_cs;
            return false;
        }
        int wide_row = matching_rows[0];
        match_select.rows.push_back(wide_row);
        search_bus = select_search_bus(group, i, wide_row);
        BUG_CHECK(search_bus >= 0, "Search somehow found an impossible bus");
        result_bus = select_result_bus(group, i, wide_row);
        BUG_CHECK(result_bus >= -1, "Search somehow found an impossible result bus");
        match_select.search_buses[wide_row] = search_bus;
        match_select.result_buses[wide_row] = result_bus;
        match_select.widths[wide_row] = i;
        determined_rows.emplace(wide_row);
    }

    // A logical table partition must be fully placed
    if (atcam &&
        bitcount(match_select.column_mask) != static_cast<size_t>(group->left_to_place())) {
        failure_reason = "atcam fail in determine_match_rows_and_cols"_cs;
        return false;
    }

    std::sort(match_select.rows.begin(), match_select.rows.end());
    std::reverse(match_select.rows.begin(), match_select.rows.end());
    std::sort(match_select.cols.begin(), match_select.cols.end());
    // In order to maintain prioritization on the right side of the array
    if (atcam && column_mask == partition_mask(RIGHT))
        std::reverse(match_select.cols.begin(), match_select.cols.end());

    return true;
}

/** Given a list of rows and columns, save this in the Memories::Use object for each table,
 *  as well as store this information within the Memories object structure
 */
void Memories::fill_out_match_alloc(SRAM_group *group, match_selection &match_select, bool atcam) {
    auto unique_id = group->build_unique_id();
    auto &alloc = (*group->ta->memuse)[unique_id];
    auto name = unique_id.build_name();

    // Save row and column information
    for (auto row : match_select.rows) {
        auto bus = match_select.search_buses.at(row);
        auto result_bus = match_select.result_buses.at(row);
        int width = match_select.widths.at(row);
        alloc.row.emplace_back(row, bus, width, allocation_count);
        alloc.row.back().result_bus = result_bus;
        auto &back_row = alloc.row.back();
        for (auto col : match_select.cols) {
            sram_use[row][col] = name;
            back_row.col.push_back(col);
        }

        auto group_search_bus = group->build_search_bus(width);
        auto group_result_bus = group->build_result_bus(width);
        sram_inuse[row] |= match_select.column_mask;
        if (!sram_search_bus[row][bus].free()) {
            BUG_CHECK(sram_search_bus[row][bus] == group_search_bus,
                      "Search bus initialization mismatch");
        } else {
            sram_search_bus[row][bus] = group_search_bus;
            sram_print_search_bus[row][bus] = group_search_bus.name;
            LOG7("Setting sram search bus on row " << row << " and bus " << bus << " for "
                                                   << sram_print_search_bus[row][bus]);
        }

        if (result_bus >= 0) {
            if (!sram_result_bus[row][result_bus].free()) {
                BUG_CHECK(sram_result_bus[row][result_bus] == group_result_bus,
                          "Result bus initializaton mismatch");
            } else {
                sram_result_bus[row][result_bus] = group_result_bus;
                sram_print_result_bus[row][result_bus] = group_result_bus.name;
                LOG7("Setting sram result bus on row " << row << " and result bus " << result_bus
                                                       << " for "
                                                       << sram_print_result_bus[row][result_bus]);
            }
        }
    }

    group->placed += bitcount(match_select.column_mask);
    // Store information on ways. Only one RAM in each way will be searched per
    // packet. The RAM is chosen by the select bits in the upper 12 bits of the
    // hash matrix
    if (atcam) {
        alloc.type = Use::ATCAM;
        int number = 0;
        for (auto col : match_select.cols) {
            safe_vector<std::pair<int, int>> ram_pairs(match_select.rows.size());
            for (auto row : match_select.rows) {
                ram_pairs[match_select.widths.at(row)] = std::make_pair(row, col);
            }
            std::reverse(ram_pairs.begin(), ram_pairs.end());
            alloc.ways[number].rams.insert(alloc.ways[number].rams.end(), ram_pairs.begin(),
                                           ram_pairs.end());
            number++;
        }
    } else {
        for (auto col : match_select.cols) {
            safe_vector<std::pair<int, int>> ram_pairs(match_select.rows.size());
            for (auto row : match_select.rows) {
                ram_pairs[match_select.widths.at(row)] = std::make_pair(row, col);
            }
            std::reverse(ram_pairs.begin(), ram_pairs.end());
            alloc.ways[group->number].rams.insert(alloc.ways[group->number].rams.end(),
                                                  ram_pairs.begin(), ram_pairs.end());
        }
    }
    allocation_count++;
}

/** Given a partition, find a set of rows, buses, and columns for that partition.  An ATCAM
 *  partition must be fully placed, as the prioritization can only pass down through a row
 */
bool Memories::find_best_partition_for_atcam(unsigned partition_mask) {
    auto part = atcam_partitions[0];

    safe_vector<std::pair<int, int>> available_rams =
        available_SRAMs_per_row(partition_mask, part, part->width - 1);
    if (available_rams.size() == 0) {
        return false;
    }
    int row = available_rams[0].first;
    if (available_rams[0].second == 0) {
        return false;
    }

    match_selection match_select;
    if (!determine_match_rows_and_cols(part, row, partition_mask, match_select, true)) {
        return false;
    }
    fill_out_match_alloc(part, match_select, true);

    BUG_CHECK(part->all_placed(), "The partition was not fully placed within the partition");
    atcam_partitions.erase(atcam_partitions.begin());
    return fill_out_partition(row, partition_mask);
}

/** Given that a partition has already been placed within the row and partition side, find
 *  another partition that can be placed within that same space
 */
Memories::SRAM_group *Memories::best_partition_candidate(int row, unsigned column_mask, int &loc) {
    loc = -1;
    auto search_bus = sram_search_bus[row];
    auto result_bus = sram_result_bus[row];
    int available_rams = bitcount(column_mask & ~sram_inuse[row]);
    for (auto part : atcam_partitions) {
        loc++;
        auto group_search_bus = part->build_search_bus(part->width - 1);
        auto group_result_bus = part->build_result_bus(part->width - 1);
        if (group_search_bus != search_bus[0] && group_search_bus != search_bus[1]) continue;
        if (group_result_bus != result_bus[0] && group_result_bus != result_bus[1]) continue;
        if (part->left_to_place() > available_rams) continue;
        return part;
    }
    return nullptr;
}

/** Given that a partition has been placed on this row and side, try to fill in the remainder of
 *  the space with potentially another partition.  If the partition depth is 2 for instance, one
 *  can fit partitions on the same search and result bus, as long as it's in the same logical
 *  table.
 */
bool Memories::fill_out_partition(int row, unsigned partition_mask) {
    int loc = 0;
    while ((partition_mask & ~sram_inuse[row]) > 0) {
        auto part = best_partition_candidate(row, partition_mask, loc);
        if (part == nullptr) return true;

        match_selection match_select;
        if (!determine_match_rows_and_cols(part, row, partition_mask, match_select, true))
            return false;

        fill_out_match_alloc(part, match_select, true);
        BUG_CHECK(part->all_placed(), "A partition has to be entirely placed in a row");
        atcam_partitions.erase(atcam_partitions.begin() + loc);
    }
    return true;
}

/** Pick a side to allocate the next partition.  Based on how left and right RAMs are available,
 *  given what is required to be on either side and what has been previously allocated.  Tries to
 *  maintain a level of balance between the sides in order to allocate tind and swbox tables
 *  later.
 */
unsigned Memories::best_partition_side(mem_info &mi) {
    int RAMs_placed[RAM_SIDES] = {0, 0};
    int partitions_allocated[RAM_SIDES] = {0, 0};

    for (int i = 0; i < SRAM_ROWS; i++) {
        for (int side = 0; side < RAM_SIDES; side++) {
            int RAMs = bitcount(side_mask(static_cast<RAM_side_t>(side)) & sram_inuse[i]);
            RAMs_placed[side] += RAMs;
            if (RAMs > 0) partitions_allocated[side]++;
        }
    }

    int left_side_needed = RAMs_placed[LEFT] + mi.left_side_RAMs();
    int right_side_needed = RAMs_placed[RIGHT] + mi.right_side_RAMs();

    int left_RAMs_avail = LEFT_SIDE_RAMS - left_side_needed;
    int right_RAMs_avail = RIGHT_SIDE_RAMS - right_side_needed;

    RAM_side_t preferred_side;
    // If no partitions available on a side, pick the other
    if (partitions_allocated[LEFT] == SRAM_ROWS)
        preferred_side = RIGHT;
    else if (partitions_allocated[RIGHT] == SRAM_ROWS)
        preferred_side = LEFT;
    // Pick the side that has more relative RAMs available
    else if (right_RAMs_avail / RIGHT_SIDE_COLUMNS < left_RAMs_avail / LEFT_SIDE_COLUMNS)
        preferred_side = LEFT;
    else
        preferred_side = RIGHT;
    return partition_mask(preferred_side);
}

bool Memories::allocate_all_atcam(mem_info &mi) {
    allocation_count = 0;
    break_atcams_into_partitions();
    if (!atcam_partitions.empty()) {
        Log::TempIndent indent;
        LOG3("Allocating all ATCAM partitions" << indent);
        while (!atcam_partitions.empty()) {
            auto partition_mask = best_partition_side(mi);
            bool found = false;
            for (int iteration = 0; iteration < RAM_SIDES; iteration++) {
                // If the partition does not fit on what is classified as the better side, due
                // to wide width requirements, to allocate on the worse side
                if (iteration == 1) partition_mask = side_mask(RAM_SIDES) & ~partition_mask;
                if (find_best_partition_for_atcam(partition_mask)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                failure_reason = "atcam partition failure"_cs;
                LOG3(failure_reason);
                return false;
            }
        }
    }

    compress_ways(true);
    return true;
}

/* Number of continuous TCAMs needed for table width */
int Memories::ternary_TCAMs_necessary(table_alloc *ta, int &midbyte) {
    midbyte = ta->table_format->split_midbyte;
    return ta->table_format->tcam_use.size();
}

/* Finds the stretch on the ternary array that can hold entries */
bool Memories::find_ternary_stretch(int TCAMs_necessary, int &row, int &col, int midbyte,
                                    bool &split_first) {
    for (int j = 0; j < TCAM_COLUMNS; j++) {
        int clear_cols = 0;
        split_first = false;

        for (int i = 0; i < TCAM_ROWS; i++) {
            if (tcam_use[i][j]) {
                clear_cols = 0;
                split_first = false;
                continue;
            }

            // These checks are enforcing that the midbyte is shared correctly between two
            // contiguous blocks.
            if ((TCAMs_necessary % 2) == 0) {
                if ((i % 2) == 1 && clear_cols == 0) continue;
            } else {
                if ((i % 2) == 1 && clear_cols == 0) {
                    if (midbyte >= 0 && tcam_midbyte_use[i / 2][j] >= 0 &&
                        midbyte != tcam_midbyte_use[i / 2][j]) {
                        continue;
                    }
                    split_first = true;
                }
            }

            clear_cols++;
            if (clear_cols == TCAMs_necessary) {
                col = j;
                row = i - clear_cols + 1;
                return true;
            }
        }
    }
    failure_reason = "find_ternary_stretch failed"_cs;
    return false;
}

/* Allocates all ternary entries within the stage */
bool Memories::allocate_all_ternary() {
    allocation_count = 0;
    std::sort(ternary_tables.begin(), ternary_tables.end(),
              [=](const table_alloc *a, table_alloc *b) {
                  int t;
                  if ((t = a->table->layout.match_bytes - b->table->layout.match_bytes) != 0)
                      return t > 0;
                  return a->total_entries() > b->total_entries();
              });
    Log::TempIndent indent;
    if (!ternary_tables.empty()) LOG3("Allocating all ternary tables" << indent);

    // FIXME: All of this needs to be changed on this to match up with xbar
    for (auto *ta : ternary_tables) {
        int lt_entry = 0;
        for (auto u_id : ta->allocation_units()) {
            int midbyte = -1;
            int TCAMs_necessary = ternary_TCAMs_necessary(ta, midbyte);

            // FIXME: If the table is just a default action table, essentially a hack
            if (TCAMs_necessary == 0) continue;
            int row = 0;
            int col = 0;
            Memories::Use &alloc = (*ta->memuse)[u_id];
            for (int i = 0; i < ta->calc_entries_per_uid[lt_entry] / 512; i++) {
                bool split_midbyte = false;
                if (!find_ternary_stretch(TCAMs_necessary, row, col, midbyte, split_midbyte)) {
                    LOG4("failed find_ternary_stretch for " << ta->table->name);
                    return false;
                }
                int word = 0;
                bool increment = true;
                if (split_midbyte) {
                    word = TCAMs_necessary - 1;
                    increment = false;
                }
                for (int i = row; i < row + TCAMs_necessary; i++) {
                    tcam_use[i][col] = u_id.build_name();
                    auto tcam = ta->table_format->tcam_use[word];
                    if (tcam_midbyte_use[i / 2][col] >= 0 && tcam.byte_group >= 0)
                        BUG_CHECK(tcam_midbyte_use[i / 2][col] == tcam.byte_group,
                                  "Two contiguous "
                                  "TCAMs cannot correct share midbyte");
                    else if (tcam_midbyte_use[i / 2][col] < 0)
                        tcam_midbyte_use[i / 2][col] = tcam.byte_group;
                    alloc.row.emplace_back(i, col, word, allocation_count);
                    alloc.row.back().col.push_back(col);
                    word = increment ? word + 1 : word - 1;
                }
                allocation_count++;
            }
            lt_entry++;
        }
    }

    for (auto *ta : ternary_tables) {
        for (auto u_id : ta->allocation_units()) {
            auto &alloc = (*ta->memuse)[u_id];
            std::stable_sort(alloc.row.begin(), alloc.row.end(),
                             [=](const Memories::Use::Row &a, const Memories::Use::Row &b) {
                                 int t;
                                 if ((t = a.alloc - b.alloc) != 0) return t < 0;
                                 if ((t = a.word - b.word) != 0) return t < 0;
                                 return a.row < b.row;
                             });
        }
    }

    for (auto *ta : ternary_tables) {
        for (auto u_id : ta->allocation_units()) {
            auto &alloc = (*ta->memuse)[u_id];
            LOG4("Allocation of " << u_id.build_name());
            for (auto row : alloc.row) {
                LOG4("Row is " << row.row << " and bus is " << row.bus);
                LOG4("Col is " << row.col);
            }
        }
    }

    return true;
}

/* Breaks up the tables requiring tinds into groups that can be allocated within the
   SRAM array */
void Memories::find_tind_groups() {
    tind_groups.clear();
    for (auto *ta : tind_tables) {
        int lt_entry = 0;
        for (auto u_id : ta->allocation_units(nullptr, false, UniqueAttachedId::TIND_PP)) {
            // Cannot use the TernaryIndirectPerWord function call, as the overhead allocated
            // may differ than the estimated overhead in the layout_option
            BUG_CHECK(ta->table_format->has_overhead(),
                      "Allocating a ternary indirect table "
                      "%s with no overhead",
                      u_id.build_name());
            bitvec overhead = ta->table_format->overhead();
            int tind_size = 1 << ceil_log2(overhead.max().index() + 1);
            tind_size = std::max(tind_size, 8);
            int per_word = TableFormat::SINGLE_RAM_BITS / tind_size;
            int depth =
                mems_needed(ta->calc_entries_per_uid[lt_entry], SRAM_DEPTH, per_word, false);
            tind_groups.push_back(new SRAM_group(ta, depth, 0, SRAM_group::TIND));
            tind_groups.back()->logical_table = u_id.logical_table;
            tind_groups.back()->ppt = UniqueAttachedId::TIND_PP;
        }
    }
    std::sort(tind_groups.begin(), tind_groups.end(),
              [=](const SRAM_group *a, const SRAM_group *b) {
                  int t;
                  if ((t = (a->left_to_place()) - (b->left_to_place())) != 0) return t > 0;
                  if ((t = a->logical_table - b->logical_table) != 0) return t < 0;
                  return a->logical_table < b->logical_table;
              });
}

/* Finds the best row in which to put the tind table */
int Memories::find_best_tind_row(SRAM_group *tg, int &bus) {
    int open_space = 0;
    unsigned left_mask = 0xf;
    safe_vector<int> available_rows;
    auto name = tg->build_unique_id().build_name();
    for (int i = 0; i < SRAM_ROWS; i++) {
        auto tbus = tind_bus[i];
        if (!tbus[0] || tbus[0] == name || !tbus[1] || tbus[1] == name) {
            available_rows.push_back(i);
            open_space += bitcount(~sram_inuse[i] & left_mask);
        }
    }

    if (open_space == 0) return -1;

    std::sort(available_rows.begin(), available_rows.end(), [=](const int a, const int b) {
        int t;
        if ((t = bitcount(~sram_inuse[a] & left_mask) - bitcount(~sram_inuse[b] & left_mask)) != 0)
            return t > 0;

        auto tbus0 = tind_bus[a];
        auto tbus1 = tind_bus[b];
        if ((tbus0[0] == name || tbus0[1] == name) && !(tbus1[0] == name || tbus1[1] == name))
            return true;
        if ((tbus1[0] == name || tbus1[1] == name) && !(tbus0[0] == name || tbus0[1] == name))
            return false;
        return a < b;
    });

    int best_row = available_rows[0];
    if (!tind_bus[best_row][0] || tind_bus[best_row][0] == name)
        bus = 0;
    else
        bus = 1;

    return best_row;
}

/* Compresses the tind groups on the same row in the use, into one row */
void Memories::compress_tind_groups() {
    for (auto *ta : tind_tables) {
        for (auto u_id : ta->allocation_units(nullptr, false, UniqueAttachedId::TIND_PP)) {
            auto &alloc = (*ta->memuse)[u_id];
            std::sort(alloc.row.begin(), alloc.row.end(),
                      [=](const Memories::Use::Row a, const Memories::Use::Row b) {
                          int t;
                          if ((t = a.row - b.row) != 0) return t < 0;
                          if ((t = a.bus - b.bus) != 0) return t < 0;
                          return false;
                      });
            for (size_t i = 0; i < alloc.row.size() - 1; i++) {
                if (alloc.row[i].row == alloc.row[i + 1].row &&
                    alloc.row[i].bus == alloc.row[i + 1].bus) {
                    alloc.row[i].col.insert(alloc.row[i].col.end(), alloc.row[i + 1].col.begin(),
                                            alloc.row[i + 1].col.end());
                    alloc.row.erase(alloc.row.begin() + i + 1);
                    i--;
                }
            }
        }
    }
}

/* Allocates all of the ternary indirect tables into the first column if they fit.
   FIXME: This is obviously a punt and needs to be adjusted. */
bool Memories::allocate_all_tind() {
    find_tind_groups();
    Log::TempIndent indent;
    if (!tind_groups.empty()) LOG3("Allocating all ternary indirect tables" << indent);
    while (!tind_groups.empty()) {
        auto *tg = tind_groups[0];
        int best_bus = 0;
        int best_row = find_best_tind_row(tg, best_bus);
        if (best_row == -1) {
            failure_reason = "no tind row available"_cs;
            LOG4("no tind row available for " << tg->ta->table->name);
            return false;
        }
        for (int i = 0; i < LEFT_SIDE_COLUMNS; i++) {
            if (~sram_inuse[best_row] & (1 << i)) {
                auto unique_id = tg->build_unique_id();
                cstring name = unique_id.build_name();
                sram_inuse[best_row] |= (1 << i);
                sram_use[best_row][i] = name;
                tg->placed++;
                if (tg->all_placed()) {
                    tind_groups.erase(tind_groups.begin());
                }
                tind_bus[best_row][best_bus] = name;

                auto &alloc = (*tg->ta->memuse)[unique_id];
                alloc.row.emplace_back(best_row, best_bus);
                alloc.row.back().result_bus = best_bus;
                alloc.row.back().col.push_back(i);
                break;
            }
        }
    }
    compress_tind_groups();
    log_allocation(&tind_tables, UniqueAttachedId::TIND_PP);
    return true;
}

/* Calculates the necessary size and requirements for any and all indirect actions and selectors.
   Selectors must be done before indirect actions */
void Memories::swbox_bus_selectors_indirects() {
    for (auto *ta : selector_tables) {
        for (auto back_at : ta->table->attached) {
            auto at = back_at->attached;
            // FIXME: need to adjust if the action selector is larger than 2 RAMs, based
            // on the pragmas provided to the compiler
            const IR::MAU::Selector *as = nullptr;
            if ((as = at->to<IR::MAU::Selector>()) == nullptr) continue;

            BUG_CHECK(ta->allocation_units(as).size() == 1,
                      "Cannot have multiple selector "
                      "objects");
            for (auto u_id : ta->allocation_units(as)) {
                int RAM_lines = SelectorRAMLinesPerEntry(as) * as->num_pools;
                int depth = mems_needed(RAM_lines, SRAM_DEPTH, 1, true);
                auto selector_group = new SRAM_group(ta, depth, 0, SRAM_group::SELECTOR);
                selector_group->attached = as;
                selector_group->logical_table = u_id.logical_table;
                synth_bus_users.insert(selector_group);
            }
        }
    }

    for (auto *ta : indirect_action_tables) {
        LOG5("For indirect action table: " << ta);
        for (auto back_at : ta->table->attached) {
            LOG5("For backend attached table : " << back_at);
            auto at = back_at->attached;
            const IR::MAU::ActionData *ad = nullptr;
            if ((ad = at->to<IR::MAU::ActionData>()) == nullptr) continue;
            BUG_CHECK(ta->allocation_units(ad).size() == 1,
                      "Cannot have multiple action profile "
                      "objects");
            for (auto u_id : ta->allocation_units(ad)) {
                LOG5("For allocation unit: " << u_id);
                int width = 1;
                int per_row = ActionDataPerWord(&ta->layout_option->layout, &width);
                int depth = mems_needed(ad->size, SRAM_DEPTH, per_row, false);
                int vpn_increment = ActionDataVPNIncrement(&ta->layout_option->layout);
                int vpn_offset = ActionDataVPNStartPosition(&ta->layout_option->layout);
                SRAM_group *selector = nullptr;

                for (auto grp : synth_bus_users) {
                    if (grp->ta == ta && grp->type == SRAM_group::SELECTOR) {
                        selector = grp;
                        break;
                    }
                }
                for (int i = 0; i < width; i++) {
                    auto action_group = new SRAM_group(ta, depth, i, SRAM_group::ACTION);
                    action_group->logical_table = u_id.logical_table;
                    action_group->attached = ad;
                    action_group->vpn_increment = vpn_increment;
                    action_group->vpn_offset = vpn_offset;
                    if (selector != nullptr) {
                        action_group->sel.sel_group = selector;
                        selector->sel.action_groups.insert(action_group);
                    }
                    LOG5("Adding action bus user: " << action_group << " for width " << width);
                    action_bus_users.insert(action_group);
                }
            }
        }
    }
}

/** Sets up the SRAM_groups for stateful ALUs and the dleft hash tables.  For the dleft hash
 *  tables, the number of SRAM groups created are provided from the number of logical tables
 *  provided by the layout option.
 */
void Memories::swbox_bus_stateful_alus() {
    for (auto *ta : stateful_tables) {
        const IR::MAU::StatefulAlu *salu = nullptr;
        for (auto back_at : ta->table->attached) {
            auto at = back_at->attached;
            if ((salu = at->to<IR::MAU::StatefulAlu>()) == nullptr) continue;
            if (salu->selector)
                // use the selector's memory
                continue;
            int lt_entry = 0;
            for (auto u_id : ta->allocation_units(salu)) {
                int per_row = RegisterPerWord(salu);
                int entries = salu->direct ? ta->calc_entries_per_uid[lt_entry]
                                           : ta->attached_entries.at(salu).entries;
                if (entries == 0) continue;
                int depth = mems_needed(entries, SRAM_DEPTH, per_row, true);
                if (ta->table->for_dleft())
                    depth = ta->layout_option->dleft_hash_sizes[lt_entry] + 1;
                auto reg_group = new SRAM_group(ta, depth, 0, SRAM_group::REGISTER);
                reg_group->attached = salu;
                reg_group->requires_ab = salu->alu_output();
                reg_group->logical_table = u_id.logical_table;
                synth_bus_users.insert(reg_group);
                lt_entry++;
            }
        }
    }
}

/* Calculates the necessary size and requirements for any meter and counter tables within
   the stage */
void Memories::swbox_bus_meters_counters() {
    for (auto *ta : stats_tables) {
        for (auto back_at : ta->table->attached) {
            auto at = back_at->attached;
            const IR::MAU::Counter *stats = nullptr;
            if ((stats = at->to<IR::MAU::Counter>()) == nullptr) continue;
            int lt_entry = 0;
            for (auto u_id : ta->allocation_units(stats)) {
                int vpn_spare = 0;
                int per_row = CounterPerWord(stats);
                int entries = stats->direct ? ta->calc_entries_per_uid[lt_entry]
                                            : ta->attached_entries.at(stats).entries;
                if (entries == 0) continue;
                int depth = mems_needed(entries, SRAM_DEPTH, per_row, true);

                // JBay has no way to support a single stat SRAM group larger than 3 rows.
                // Splitting it in 2 SRAM Group using one ALU each can provide up to 6 rows
                // of indirect counter to match Tofino capability.
                if ((Device::isMemoryCoreSplit()) && (depth > MAX_STATS_RAM_PER_ALU) &&
                    !stats->direct) {
                    auto *stats_group =
                        new SRAM_group(ta, MAX_STATS_RAM_PER_ALU, 0, SRAM_group::STATS);
                    stats_group->attached = stats;
                    stats_group->logical_table = u_id.logical_table;
                    stats_group->vpn_offset = depth - MAX_STATS_RAM_PER_ALU;

                    // The same spare VPN value would be used for both spare bank.
                    vpn_spare = stats_group->vpn_spare = depth - 1;
                    synth_bus_users.insert(stats_group);
                    depth -= ((MAX_STATS_RAM_PER_ALU)-1);
                }
                auto *stats_group = new SRAM_group(ta, depth, 0, SRAM_group::STATS);
                stats_group->attached = stats;
                stats_group->logical_table = u_id.logical_table;
                if (vpn_spare) stats_group->vpn_spare = vpn_spare;

                synth_bus_users.insert(stats_group);
                lt_entry++;
            }
        }
    }

    for (auto *ta : meter_tables) {
        const IR::MAU::Meter *meter = nullptr;
        for (auto back_at : ta->table->attached) {
            auto at = back_at->attached;
            if ((meter = at->to<IR::MAU::Meter>()) == nullptr) continue;
            int lt_entry = 0;
            for (auto u_id : ta->allocation_units(meter)) {
                int vpn_spare = 0;
                int entries = meter->direct ? ta->calc_entries_per_uid[lt_entry]
                                            : ta->attached_entries.at(meter).entries;
                if (entries == 0) continue;
                int depth = mems_needed(entries, SRAM_DEPTH, 1, true);
                int max_half_ram_depth = MAX_METERS_RAM_PER_ALU;
                if (meter->color_output())
                    max_half_ram_depth = MAX_METERS_RAM_PER_ALU - MAX_METERS_COLOR_MAPRAM_PER_ALU;
                // JBay has no way to support a single meter SRAM group larger than 4 rows.
                // Splitting it in 2 SRAM Group using one ALU each can provide up to 8 rows
                // of indirect meter to match Tofino capability. The Map RAMs is the gating factor
                // if the meter have to output color.
                if ((Device::isMemoryCoreSplit()) && (depth > max_half_ram_depth) &&
                    !meter->direct) {
                    // Align the color mapram VPNs with the meter VPNs. To do that, we have to
                    // break the table at a multiple of 4. This make sure the same color mapram is
                    // not part of both half.
                    int top_half_ram_depth = depth;
                    while (top_half_ram_depth > max_half_ram_depth) top_half_ram_depth -= 4;

                    auto *meter_group =
                        new SRAM_group(ta, top_half_ram_depth, 0, SRAM_group::METER);
                    meter_group->attached = meter;
                    meter_group->logical_table = u_id.logical_table;
                    meter_group->vpn_offset = depth - top_half_ram_depth;

                    if (meter->color_output()) {
                        meter_group->cm.needed =
                            mems_needed((top_half_ram_depth - 1) * SRAM_DEPTH, SRAM_DEPTH,
                                        COLOR_MAPRAM_PER_ROW, false);
                        if (meter->mapram_possible(IR::MAU::ColorMapramAddress::IDLETIME))
                            meter_group->cm.cma = IR::MAU::ColorMapramAddress::IDLETIME;
                        else if (meter->mapram_possible(IR::MAU::ColorMapramAddress::STATS))
                            meter_group->cm.cma = IR::MAU::ColorMapramAddress::STATS;
                        else
                            BUG("The color mapram address scheme does not make sense");
                    } else {
                        meter_group->requires_ab = true;
                    }

                    // The same spare VPN value would be used for both spare bank.
                    vpn_spare = meter_group->vpn_spare = depth - 1;
                    synth_bus_users.insert(meter_group);
                    depth -= (top_half_ram_depth - 1);
                    entries -= ((top_half_ram_depth - 1) * SRAM_DEPTH);
                }

                auto *meter_group = new SRAM_group(ta, depth, 0, SRAM_group::METER);
                meter_group->attached = meter;
                meter_group->logical_table = u_id.logical_table;
                if (meter->color_output()) {
                    meter_group->cm.needed =
                        mems_needed(entries, SRAM_DEPTH, COLOR_MAPRAM_PER_ROW, false);
                    if (meter->mapram_possible(IR::MAU::ColorMapramAddress::IDLETIME))
                        meter_group->cm.cma = IR::MAU::ColorMapramAddress::IDLETIME;
                    else if (meter->mapram_possible(IR::MAU::ColorMapramAddress::STATS))
                        meter_group->cm.cma = IR::MAU::ColorMapramAddress::STATS;
                    else
                        BUG("The color mapram address scheme does not make sense");
                } else {
                    meter_group->requires_ab = true;
                }
                if (vpn_spare) meter_group->vpn_spare = vpn_spare;

                synth_bus_users.insert(meter_group);
            }
        }
    }
}

/* Breaks up all tables requiring an action to be parsed into SRAM_group, a structure
   designed for adding to SRAM array  */
void Memories::find_swbox_bus_users() {
    Log::TempIndent indent;
    LOG5("Find swbox bus users" << indent);
    action_bus_users.clear();
    synth_bus_users.clear();
    must_place_in_half.clear();

    for (auto *ta : action_tables) {
        LOG5("For action table: " << *ta);
        int width = 1;
        int per_row = ActionDataPerWord(&ta->layout_option->layout, &width);
        int vpn_increment = ActionDataVPNIncrement(&ta->layout_option->layout);
        int vpn_offset = ActionDataVPNStartPosition(&ta->layout_option->layout);
        int lt_entry = 0;
        for (auto u_id : ta->allocation_units(nullptr, false, UniqueAttachedId::ADATA_PP)) {
            LOG5("For action data allocation unit: " << u_id);
            int entries = ta->calc_entries_per_uid[lt_entry];
            int depth = mems_needed(entries, SRAM_DEPTH, per_row, false);
            for (int i = 0; i < width; i++) {
                LOG5("For width: " << i);
                auto *act_group = new SRAM_group(ta, depth, i, SRAM_group::ACTION);
                act_group->logical_table = u_id.logical_table;
                act_group->direct = true;
                act_group->vpn_increment = vpn_increment;
                act_group->vpn_offset = vpn_offset;
                act_group->ppt = UniqueAttachedId::ADATA_PP;
                action_bus_users.insert(act_group);
                LOG5("Adding action bus user : " << act_group);
            }
        }
    }
    swbox_bus_selectors_indirects();
    swbox_bus_meters_counters();
    swbox_bus_stateful_alus();
}

/**
 * These are the RAMs available for synth2port tables on a particular logical row.  Due to
 * color mapram constraints, currently occasionally not all RAMs on a row are available
 * to synth2port RAMs, as they themselves require map RAMs, and thus cannot be allocated
 * for color map RAM space.
 *
 * A color map RAM is read and written at different times, and requires two addresses, one
 * at processing time, and one at EOP.  The EOP information is from the meter ALU, and requires
 * the same overflowing constraints as the meter RAMs for addresses.  Thus the current setup
 * is to reserve the overflow position for a meter until both its color maprams and RAMs
 * are fully placed.
 *
 * The other address at processing time can either come from the statistics bus or the idletime
 * bus.  It is already known which bus is used at this point, but if this statistics bus
 * is used, then the stats bus cannot be used for anything else, and the RAMs cannot be
 * dedicated to synth2port
 */
void Memories::determine_synth_RAMs(int &RAMs_available, int row,
                                    const SRAM_group *curr_oflow) const {
    int unallocated_RAMs = bitcount(~sram_inuse[row] & side_mask(RIGHT));

    if (!(curr_oflow && curr_oflow->type == SRAM_group::METER &&
          curr_oflow->cm.left_to_place() > 0)) {
        RAMs_available = unallocated_RAMs;
        return;
    }
    // RAMs that have Match RAMs on them already.  Map RAMs not used for Exact Match
    int match_RAMs_allocated = bitcount(sram_inuse[row] & side_mask(RIGHT));

    int curr_oflow_synth_RAMs = std::min(unallocated_RAMs, curr_oflow->left_to_place());
    int extra_synth_RAMs = 0;

    // If other synth2port tables can be placed, as the curr_oflow table is going to be placed,
    // the color maprams must be placed, so that the address from the meter alu can overflow
    // to the color mapram.  By reserving the color mapram, you cannot reserve the corresponding
    // RAM to a synth2port table, as that RAM would require a corresponding map RAM as well.
    int possible_for_synth_RAMs = unallocated_RAMs - curr_oflow_synth_RAMs;
    if (possible_for_synth_RAMs > 0) {
        if (curr_oflow->cm.cma == IR::MAU::ColorMapramAddress::STATS) {
            // If the color mapram requires a stats address, then the mapram must appear on a stats
            // homerow.  Furthermore, all color maprams must be on the same row in order to not
            // use the overflow address, which is required for the meter address on color mapram
            // write.  (Perhaps requiring multiple stats ram homerows?).  Also if the table could
            // fit in a single row, it'd be fine as well.
            //
            // In order to prevent new synth2port candidates from starting on meter rows, if
            // they cannot finish in this row, this is always set to 0.  However, some RAMs
            // could in theory still be available, if the allocation was able to start and
            // finish in that particular row.  The best_synth_candidates function would
            // need to change, however, in order for this to work.  Frankly it's so rare
            // to address a meter by hash, that it's not worth it to do at this point
            extra_synth_RAMs = 0;
        } else if (curr_oflow->cm.cma == IR::MAU::ColorMapramAddress::IDLETIME) {
            // Anything that is impossible for synth2port RAMs is because a dedicated map RAM
            // must be on that particular RAM.  This RAM can be used for action data or for
            // matching
            int impossible_for_synth_RAMs =
                std::max(curr_oflow->cm.left_to_place() - match_RAMs_allocated, 0);
            extra_synth_RAMs = std::max(possible_for_synth_RAMs - impossible_for_synth_RAMs, 0);
        }
    }
    RAMs_available = curr_oflow_synth_RAMs + extra_synth_RAMs;
    LOG5("\tRAMs available for synth2port " << RAMs_available);
}

/**
 * Any leftover RAMs not used by synth2port tables are now available for action data.
 */
void Memories::determine_action_RAMs(int &RAMs_available, int row, RAM_side_t side,
                                     const safe_vector<LogicalRowUser> &lrus) const {
    int synth_RAMs_used = 0;
    for (auto &lru : lrus) {
        synth_RAMs_used += lru.RAM_mask.popcount();
    }
    int unallocated_RAMs = bitcount(~sram_inuse[row] & side_mask(side));
    RAMs_available = std::max(unallocated_RAMs - synth_RAMs_used, 0);
    LOG5("\tRAMs available for action data " << RAMs_available);
}

/**
 * In order to begin placing a synth2port table, there has to be a data pathway to the ALU
 * for that table.  The current check only allows meters/selectors/stateful ALUs to begin
 * on an odd row, and statistics tables to begin on an even row.  Perhaps, when the analysis
 * can verify that the overflow bus has not yet been used, this can be loosened
 */
bool Memories::alu_pathway_available(SRAM_group *synth_table, int row,
                                     const SRAM_group *curr_oflow) const {
    BUG_CHECK(synth_table->is_synth_type(), "Alu pathway only required for a synth2port table");
    if ((row % 2) == 0) {
        if (synth_table->type != SRAM_group::STATS) return false;
        // If the meter is to use statistics bus for the color map RAMs
        if (curr_oflow && curr_oflow->type == SRAM_group::METER &&
            curr_oflow->cm.left_to_place() > 0 && curr_oflow->cm.require_stats())
            return false;
    } else {
        if (synth_table->type == SRAM_group::STATS) return false;
    }
    return true;
}

int Memories::phys_to_log_row(int physical_row, RAM_side_t side) const {
    return physical_row * RAM_SIDES + static_cast<int>(side);
}

int Memories::log_to_phys_row(int logical_row, RAM_side_t *side) const {
    if (side) *side = static_cast<RAM_side_t>(logical_row % 2);
    return logical_row / 2;
}

/**
 * In Tofino specifically, due to a timing issue, the maximum number of rows a RAM can be
 * from its home row is 6 physical rows.  Thus if the home row is on physical row 7, the lowest
 * row a RAM can be on is physical row 2.
 *
 * For synth2port tables, this is extremely restrictive.  However, for action data tables,
 * a new home row can be started, as only one action data RAM will activate at most.  As long
 * as the home rows have the exact same action data bus mux setup, this pattern is fine
 */
int Memories::lowest_row_to_overflow(const SRAM_group *candidate, int logical_row) const {
    int lowest_phys_row = 0;
    if (!Device::isMemoryCoreSplit()) {
        int phys_row = log_to_phys_row(logical_row);
        if (candidate->recent_home_row >= 0) phys_row = log_to_phys_row(candidate->recent_home_row);
        lowest_phys_row = std::max(phys_row - MAX_DATA_SWBOX_ROWS, 0);
    } else {
        int phys_row = log_to_phys_row(logical_row);
        lowest_phys_row = (phys_row / MATCH_CENTRAL_ROW) * MATCH_CENTRAL_ROW;
    }
    return candidate->is_synth_type() ? phys_to_log_row(lowest_phys_row, RIGHT)
                                      : phys_to_log_row(lowest_phys_row, LEFT);
}

int Memories::open_rams_between_rows(int highest_logical_row, int lowest_logical_row,
                                     bitvec sides) const {
    int rv = 0;
    for (int r = highest_logical_row; r >= lowest_logical_row; r--) {
        RAM_side_t side = RAM_SIDES;
        int phys_row = log_to_phys_row(r, &side);
        if (!sides.getbit(static_cast<int>(side))) continue;
        rv += bitcount(side_mask(side) & ~sram_inuse[phys_row]);
    }
    return rv;
}

int Memories::open_maprams_between_rows(int highest_phys_row, int lowest_phys_row) const {
    int rv = 0;
    for (int r = highest_phys_row; r >= lowest_phys_row; r--) {
        rv += bitcount(MAPRAM_MASK & ~mapram_inuse[r]);
    }
    return rv;
}

/**
 * This verifies that there are enough open RAMs between the current row, and the lowest
 * physical row due to the maximum number of rows possible for overflow to begin placing this
 * table.
 */
bool Memories::overflow_possible(const SRAM_group *candidate, const SRAM_group *curr_oflow, int row,
                                 RAM_side_t side) const {
    if (!candidate->is_synth_type()) return true;
    int logical_row = phys_to_log_row(row, side);
    int lowest_logical_row = lowest_row_to_overflow(candidate, logical_row);
    if (candidate->type == SRAM_group::ACTION) lowest_logical_row = 0;
    bitvec sides;
    sides.setbit(RIGHT);
    int available_rams = open_rams_between_rows(logical_row, lowest_logical_row, sides);
    if (curr_oflow && curr_oflow->is_synth_type()) {
        available_rams -= curr_oflow->left_to_place();
        // Need to guarantee that there is enough color maprams for meters, as this will
        // take some potential RAM space for other RAMs.
        if (curr_oflow->type == SRAM_group::METER) {
            available_rams -= curr_oflow->cm.left_to_place();
        }
    }
    if (available_rams < candidate->left_to_place()) return false;
    return true;
}

/**
 * The purpose of this function is to guarantee by starting to place this table, by placing
 * this table in full, the tables that are currently overflowing will not break their
 * overflow constraint of the maximum number of possible rows.
 */
bool Memories::break_other_overflow(const SRAM_group *candidate, const SRAM_group *curr_oflow,
                                    int row, RAM_side_t side) const {
    if (!(curr_oflow && curr_oflow->is_synth_type())) return true;
    if (!candidate->is_synth_type()) return true;
    int logical_row = phys_to_log_row(row, side);
    int lowest_logical_row = lowest_row_to_overflow(curr_oflow, logical_row);
    bitvec sides;
    sides.setbit(RIGHT);
    int available_rams = open_rams_between_rows(logical_row, lowest_logical_row, sides);
    int total_RAMs_required = candidate->left_to_place() + curr_oflow->left_to_place();
    if (available_rams < total_RAMs_required) return false;
    return true;
}

/**
 * The only major difference in the memory algorithm in Tofino vs. JBay is that one
 * cannot overflow any information over match central, specifically:
 *     1. Data through the HV switchbox
 *     2. Color mapram data and from the meter ALU
 *     3. Selector Addresses to their associated Action Data RAMs
 *
 * The algorithm thus needs to prevent a synth2port table from being placed within a half
 * if it fully cannot be placed within that half.  For action rows, it is less important,
 * as the action row can start a brand new homerow.  However, a synth2port cannot begin
 * on a new ALU.
 *
 * Currently in the driver a synth2port table is only allocated to one stateful ALU, and
 * cannot be moved around.  Furthermore, in the direct case, one can only use movereg across
 * a single logical table, which means to move entries, the driver would have to support
 * a level above movereg.
 *
 * This also has to guarantee that all action data tables associated with a selector are
 * in the same physical half as the selector.  Again, possibly this can be loosened up
 * as selectors are never direct, and thus don't require movereg, but the driver can
 * currently interpret only one selector per stage.
 */
bool Memories::can_be_placed_in_half(const SRAM_group *candidate, int row, RAM_side_t side,
                                     const SRAM_group *synth, int avail_RAMs_on_row) const {
    if (!Device::isMemoryCoreSplit()) return true;

    if (must_place_in_half.find(candidate) != must_place_in_half.end()) return true;
    int synth_RAMs = 0;
    int map_RAMs = 0;
    int action_RAMs = 0;

    ///> Note that though the synth2port table has been selected to be placed, it has not yet
    ///> been placed.  Thus it will not yet be part of the must_place_in_half set at this point.
    if (synth && synth->type == SRAM_group::SELECTOR) {
        if (candidate->sel.sel_group == synth) {
            BUG_CHECK(candidate->type == SRAM_group::ACTION,
                      "Error coordinating selector "
                      "and action profile in memories");
            return true;
        }
        action_RAMs += synth->sel.action_left_to_place();
    }

    ///> Gather all of the constraints for the RAMs that are already locked into this match
    ///> central half
    for (auto must_place : must_place_in_half) {
        if (must_place->is_synth_type()) {
            synth_RAMs += must_place->left_to_place();
            map_RAMs += must_place->left_to_place() + must_place->cm.left_to_place();
        } else {
            action_RAMs += must_place->left_to_place();
        }
    }

    int lowest_phys_row = (row / MATCH_CENTRAL_ROW) * MATCH_CENTRAL_ROW;
    bitvec right_side(1U << RIGHT);
    bitvec all_sides(LEFT, RAM_SIDES);
    int avail_synth_RAMs = side == RIGHT ? avail_RAMs_on_row : 0;
    int avail_RAMs = avail_RAMs_on_row;

    ///> The RAMs available on the current RAM line are known, so we add this sum to
    ///> all available RAMs on the line below
    avail_synth_RAMs += open_rams_between_rows(phys_to_log_row(row, side) - 1,
                                               phys_to_log_row(lowest_phys_row, RIGHT), right_side);
    avail_RAMs += open_rams_between_rows(phys_to_log_row(row, side) - 1,
                                         phys_to_log_row(lowest_phys_row, LEFT), all_sides);

    ///> The map RAMs are completely free
    int avail_map_RAMs = open_maprams_between_rows(row, lowest_phys_row);

    int candidate_synth_RAMs = 0;
    int candidate_map_RAMs = 0;
    int candidate_action_RAMs = 0;

    if (candidate->is_synth_type()) {
        candidate_synth_RAMs += candidate->left_to_place();
        candidate_map_RAMs += candidate->left_to_place() + candidate->cm.left_to_place();
        if (candidate->type == SRAM_group::SELECTOR) {
            for (auto ag : candidate->sel.action_groups) {
                candidate_action_RAMs += ag->left_to_place();
            }
        }
    } else {
        candidate_action_RAMs += candidate->left_to_place();
    }

    ///> For standard overflow constraints
    if (candidate_synth_RAMs + synth_RAMs > avail_synth_RAMs) return false;
    ///> For color overflow constraints
    if (candidate_map_RAMs + map_RAMs > avail_map_RAMs) return false;

    ///> For selector overflow constraints
    if (candidate_synth_RAMs + synth_RAMs + candidate_action_RAMs + action_RAMs > avail_RAMs) {
        if (candidate->is_synth_type()) return false;
        ///> Action data RAMs don't have overflow issues, but in order to make sure
        ///> that all selection based action RAMs are prioritized, this check will not start
        ///> too large action data tables.
        else if (action_RAMs > 0)
            return false;
    }
    return true;
}

/**
 * This function verifies that by placing this candidate, the selector switchbox constraints
 * are not broken.  The selector switch box is described in uArch section 6.2.10.5.5
 * The Hash Word Selection Block, summarized below.
 *
 * Basically, the offset calculated by the selector has to be added to the action data RAM.  This
 * is done by sending an up to 7 bit address from the selector ALU to the associated action data
 * RAMs through a switchbox.  The constraint is that the logical row of the selector ALU must
 * be >= the logical row of any action data RAM.  Furthermore, there is only one pathway through
 * the selector switch box, so it is impossible for two selectors to be overflowing their address.
 *
 * This function verifies that this constraint is not violated.  It is the most annoying part of
 * this algorithm, as it is the only instance where synth2port tables and action data tables
 * cannot be thought of as separate entities, but as having intertwined constraints.
 *
 * FIXME: This function is also overly conservative.  Say the overflowing selectors action data
 * tables were able to finish on this logical (or even physical row).  Then a new selector could
 * be started.  However, because synth2port tables are selected before action data tables, this
 * information is not known ahead of time, and would be some extra work to both lock in an
 * action data strategy early, and then indicate that when selecting the action data candidates.
 */
bool Memories::satisfy_sel_swbox_constraints(const SRAM_group *candidate,
                                             const SRAM_group *sel_oflow, SRAM_group *synth) const {
    if (candidate->type != SRAM_group::SELECTOR && candidate->type != SRAM_group::ACTION)
        return true;

    ///> Prevent any selector from being placed until the previous selector is fully placed
    if (candidate->type == SRAM_group::SELECTOR) {
        if (sel_oflow != nullptr) return false;
        BUG_CHECK(!candidate->sel.action_any_placed(),
                  "A selector already has action data "
                  "tables placed, breaking selector switchbox constraints");
        return true;
    }

    if (candidate->type == SRAM_group::ACTION) {
        if (!(candidate->sel.sel_group == nullptr || candidate->sel.sel_group == sel_oflow ||
              candidate->sel.sel_group == synth))
            return false;
        return true;
    }
    BUG("Unreachable in selector swbox constraints");
    return false;
}

/**
 * Determine best fits, i.e. candidate that requires the most number of RAMs that can
 * still fit on the row
 */
void Memories::determine_fit_on_logical_row(SRAM_group **fit_on_logical_row, SRAM_group *candidate,
                                            int RAMs_avail) const {
    if (RAMs_avail - candidate->left_to_place() > 0) {
        if (*fit_on_logical_row) {
            if (candidate->left_to_place() > (**fit_on_logical_row).left_to_place())
                *fit_on_logical_row = candidate;
        } else {
            *fit_on_logical_row = candidate;
        }
    }
}

void Memories::determine_max_req(SRAM_group **max_req, SRAM_group *candidate) const {
    // FIXME: Due to limitations in the calculations in the can_be_placed_in_half, the
    // assumptions are that the selector action data will override the overflow, however
    // in the placement, the overflow always comes before the next ride.  Therefore, by
    // guaranteeing that the largest candidate linked to a selector is preferred, the
    // missing overflow over match central can be allocated properly
    //
    // Really, what needs to be opened is the reallocation of a selector in both halves,
    // if the action data table is too large
    if (Device::isMemoryCoreSplit()) {
        if (*max_req && candidate->type == SRAM_group::ACTION) {
            if ((**max_req).sel.sel_linked() != candidate->sel.sel_linked()) {
                *max_req = candidate->sel.sel_linked() ? candidate : *max_req;
                return;
            }
        }
    }

    if (*max_req) {
        ///> Selectors also include their action data
        if ((**max_req).total_left_to_place() < candidate->total_left_to_place())
            *max_req = candidate;
    } else {
        *max_req = candidate;
    }
}

/**
 * Determine the best-fit and the maximum requirement candidate for the SYNTH bus.
 */
void Memories::candidates_for_synth_row(SRAM_group **fit_on_logical_row, SRAM_group **max_req,
                                        int row, const SRAM_group *curr_oflow,
                                        const SRAM_group *sel_oflow, int RAMs_avail) const {
    for (auto synth_table : synth_bus_users) {
        if (curr_oflow == synth_table) continue;
        LOG5("\tSynth candidate " << synth_table);
        BUG_CHECK(!synth_table->any_placed(),
                  "Cannot start a synth2port table and not have "
                  "it as non-overflowing");
        if (!alu_pathway_available(synth_table, row, curr_oflow)) continue;
        LOG6("\t    Alu pathway available");
        if (!overflow_possible(synth_table, curr_oflow, row, RIGHT)) continue;
        LOG6("\t    Overflow is possible");
        if (!satisfy_sel_swbox_constraints(synth_table, sel_oflow, nullptr)) continue;
        LOG6("\t    Satisfies swbox constraints");
        if (!can_be_placed_in_half(synth_table, row, RIGHT, nullptr, RAMs_avail)) continue;

        LOG5("\tCan be max requirement");
        determine_max_req(max_req, synth_table);

        ///> FIXME: Because these calculations are based on RAMs only, the meter color
        ///> maprams cannot yet be determined if they will also not require overflow
        ///> needs.  Therefore, meters cannot yet be best fit
        if (synth_table->type == SRAM_group::METER && synth_table->cm.left_to_place() > 0) continue;
        if (!break_other_overflow(synth_table, curr_oflow, row, RIGHT)) continue;
        LOG5("\tCan be the best fit");
        determine_fit_on_logical_row(fit_on_logical_row, synth_table, RAMs_avail);
    }
}

void Memories::candidates_for_action_row(SRAM_group **fit_on_logical_row, SRAM_group **max_req,
                                         int row, RAM_side_t side, const SRAM_group *curr_oflow,
                                         const SRAM_group *sel_oflow, int RAMs_avail,
                                         SRAM_group *synth) const {
    for (auto action_table : action_bus_users) {
        if (curr_oflow == action_table) continue;
        LOG5("\t  Action candidate " << action_table);
        BUG_CHECK(!action_table->all_placed(),
                  "Cannot be a candidate for action table if fully "
                  "placed");
        if (synth && synth->needs_ab()) continue;
        if (!overflow_possible(action_table, curr_oflow, row, side)) continue;
        LOG6("\t\tOverflow possible");
        if (!satisfy_sel_swbox_constraints(action_table, sel_oflow, synth)) continue;
        LOG6("\t\tSwitchbox selector satisfied");
        if (!can_be_placed_in_half(action_table, row, side, synth, RAMs_avail)) continue;
        LOG6("\t\tCan be placed in half");
        LOG5("\t  Can be max requirement");
        determine_max_req(max_req, action_table);

        if (!break_other_overflow(action_table, curr_oflow, row, side)) continue;
        LOG5("\t  Can fit on logical row");
        determine_fit_on_logical_row(fit_on_logical_row, action_table, RAMs_avail);
    }
}

/**
 * Determines the user of the SYNTH bus, based on overflow restrictions and best-fit
 * properties.
 */
void Memories::determine_synth_logical_row_users(SRAM_group *fit_on_logical_row,
                                                 SRAM_group *max_req, SRAM_group *curr_oflow,
                                                 safe_vector<LogicalRowUser> &lrus,
                                                 int RAMs_avail) const {
    bool synth_bus_used = false;
    // If a synth2port is overflowing, and we can finish the table on a single row while using
    // all possible RAMs, then we place the single row table
    if (curr_oflow && curr_oflow->is_synth_type()) {
        if (fit_on_logical_row &&
            curr_oflow->left_to_place() + fit_on_logical_row->left_to_place() >= RAMs_avail) {
            lrus.emplace_back(fit_on_logical_row, SYNTH);
            synth_bus_used = true;
        }
    }

    // Next we will try to finish the overflow table
    if (curr_oflow && curr_oflow->is_synth_type()) lrus.emplace_back(curr_oflow, OFLOW);

    // Last choice will be for the largest requirements on synth2port
    if (max_req && !synth_bus_used) {
        lrus.emplace_back(max_req, SYNTH);
        synth_bus_used = true;
    }
}

/**
 * For JBay only, the math of action data tables placed with selectors requires those to be
 * favorited much earlier than any other group.  When the memory algorithm is refactored to
 * handle the core split better, potentially this preference could be updated
 *
 * If an action data table is currently the overflow into the row, then without this check
 * the algorithm would prefer it.  This will prefer the selector link, but will still possibly
 * place the overflow if the action RAMs with the selector do not take up the entirety of the
 * row.
 */
bool Memories::action_candidate_prefer_sel(SRAM_group *max_req, SRAM_group *synth,
                                           SRAM_group *curr_oflow, SRAM_group *sel_oflow,
                                           safe_vector<LogicalRowUser> &lrus) const {
    if (!Device::isMemoryCoreSplit()) return false;
    // If a selector is placed, and there is more room on the RAM line, prefer the action data
    // RAM
    if (synth && synth->type == SRAM_group::SELECTOR && curr_oflow &&
        curr_oflow->type == SRAM_group::ACTION) {
        if (max_req) {
            BUG_CHECK(max_req->sel.is_sel_corr_group(synth),
                      "Action RAMs in Tofino2 must be "
                      "preferred with its selector RAMs");
            lrus.emplace_back(max_req, ACTION);
            return true;
        }
    }

    // If a selector is already placed and an action RAM not connected to the selector
    // is overflowing (i.e. a selector wasthe only thing that could be placed, so overflow was
    // never cleared)
    if (sel_oflow && curr_oflow && curr_oflow->type == SRAM_group::ACTION &&
        !curr_oflow->sel.is_sel_corr_group(sel_oflow)) {
        if (max_req) {
            BUG_CHECK(max_req->sel.is_sel_corr_group(sel_oflow),
                      "Action RAMs in Tofino2 must be "
                      "preferred with its selector RAMs");
            lrus.emplace_back(max_req, ACTION);
            return true;
        }
    }
    return false;
}

void Memories::determine_action_logical_row_users(SRAM_group *fit_on_logical_row,
                                                  SRAM_group *max_req, SRAM_group *synth,
                                                  SRAM_group *curr_oflow, SRAM_group *sel_oflow,
                                                  safe_vector<LogicalRowUser> &lrus,
                                                  int RAMs_avail) const {
    bool action_bus_used = action_candidate_prefer_sel(max_req, synth, curr_oflow, sel_oflow, lrus);
    // If a action table is overflowing and we can finish a table on a single row while using
    // all possible RAMs, then we place the single row table
    if (curr_oflow && !curr_oflow->is_synth_type()) {
        if (fit_on_logical_row && !action_bus_used &&
            curr_oflow->left_to_place() + fit_on_logical_row->left_to_place() >= RAMs_avail) {
            lrus.emplace_back(fit_on_logical_row, ACTION);
            action_bus_used = true;
        }
    }

    // Next we try to finish the overflow table
    if (curr_oflow && !curr_oflow->is_synth_type()) lrus.emplace_back(curr_oflow, OFLOW);

    // Last choice will be for the largest requirements on synth2port
    if (max_req && !action_bus_used) {
        lrus.emplace_back(max_req, ACTION);
        action_bus_used = true;
    }
}

/**
 * This determines which RAMs (and corresponding map RAMs for addressing synth2port tables)
 * each candidate will be using
 */
void Memories::determine_RAM_masks(safe_vector<LogicalRowUser> &lrus, int row, RAM_side_t side,
                                   int RAMs_available, bool is_synth_type) const {
    bitvec RAM_in_use;
    bitvec map_RAM_in_use;
    int allocated_RAMs = 0;

    for (auto &lru : lrus) {
        RAM_in_use |= lru.RAM_mask;
        map_RAM_in_use |= lru.map_RAM_mask;
    }

    RAM_in_use |= bitvec(sram_inuse[row] & side_mask(side));
    int start_ram = side == LEFT ? 0 : LEFT_SIDE_COLUMNS;
    int end_ram = side == LEFT ? LEFT_SIDE_COLUMNS : SRAM_COLUMNS;

    for (auto &lru : lrus) {
        auto candidate = lru.group;
        ///> This function is called twice, once for synth and once for action data.  However,
        ///> the synth2port tables are already in the vector
        if (candidate->is_synth_type() != is_synth_type) {
            BUG_CHECK(lru.set(), "Determine RAM usage cannot be called this way");
            continue;
        }

        int allocated_RAMs_per_lru = 0;
        for (int ram = start_ram; ram < end_ram; ram++) {
            ///> Don't place more RAMs than are available
            if (allocated_RAMs == RAMs_available) break;
            ///> Don't place more RAMs than required per candidate
            if (allocated_RAMs_per_lru == candidate->left_to_place()) break;
            if (!RAM_in_use.getbit(ram)) {
                RAM_in_use.setbit(ram);
                lru.RAM_mask.setbit(ram);
                allocated_RAMs++;
                allocated_RAMs_per_lru++;
                if (candidate->is_synth_type()) {
                    map_RAM_in_use.setbit(ram - start_ram);
                    lru.map_RAM_mask.setbit(ram - start_ram);
                }
            }
        }
    }
}

/**
 * Color maprams are separate maprams that have to be reserved with a meter.  Color is Tofino
 * is stored as a 2bit field, and a mapram can store up to 4 colors per row.  The color maprams
 * are used to store information as the packet comes in to the MAU stage, as well as when the
 * packet leaves the switch.  The total constraints for how color maprams must be laid out are
 * detailed in a few sections.  First is 6.2.8.4.9 Map RAM Addressing.  The switchbox constraints
 * are detailed in sections 6.2.11.1.7 Meter Color Map RAM Read Switchbox and 6.2.11.1.8 Meter
 * Color Map RAM Write Switchbox.  Finally the constraints are summarized in 6.2.13.4 Meter
 * Color Map RAMs.
 *
 * Essentially, reads and writes happen at different times within a meter color mapram, and thus
 * are accessed differently.  First is on read, the meter uses either the home row meter address
 * or an overflow address bus to lookup the correct location.  This means that nothing else
 * on that row can use that overflow bus, unless it is the meter that corresponds to that
 * bus.
 *
 * On a write to the color mapram, the address used is either an idletime or stats address bus,
 * In general, it is preferable to use an idletime bus, as they're less constrained.  However,
 * occasionally you must use a stats address bus, and thus that stats address bus cannot be
 * accessing any other table.
 *
 * Also on a write, the meter provides the new color to a color mapram through a color mapram
 * switchbox.  Similar to the selector or data switchbox, only one color can pass between rows.
 * Thus, two meters cannot require to go through the same color mapram switchbox.
 *
 * You might wonder how the algorithm deals with all of these constraints.  The simple solution
 * is to just require both the RAMs the color maprams to both be placed before the next
 * synth2port table can be placed.  This is not too big of an issue, as both Action Data tables
 * and exact match tables do not require maprams, and thus can be placed in the same location
 * as the color mapram.  However, because of this constraint, the number of RAMs available
 * for synth2port tables and action data tables may be different.  Thus the need of the
 * function adjust_RAMs_available is specified.
 */
void Memories::one_color_map_RAM_mask(LogicalRowUser &lru, bitvec &map_RAM_in_use,
                                      bool &stats_bus_used, int row) const {
    auto candidate = lru.group;
    if (!(candidate->type == SRAM_group::METER && candidate->cm.left_to_place() > 0)) return;

    int available_map_RAMs = (bitvec(MAPRAM_MASK) - map_RAM_in_use).popcount();
    if (available_map_RAMs == 0) return;

    // Only can use one stats bus for the color map RAMs
    if (candidate->cm.cma == IR::MAU::ColorMapramAddress::STATS) {
        if (candidate->cm.left_to_place() > available_map_RAMs) return;
        if ((row % 2) == 1) return;
        if (stats_bus_used) return;
        stats_bus_used = true;
    }
    int allocated_map_RAMs = 0;

    for (int mr = 0; mr < MAPRAM_COLUMNS; mr++) {
        if (allocated_map_RAMs == candidate->cm.left_to_place()) break;
        if (!map_RAM_in_use.getbit(mr)) {
            map_RAM_in_use.setbit(mr);
            lru.map_RAM_mask.setbit(mr);
            allocated_map_RAMs++;
        }
    }
}

/**
 * This determines the color map RAMs each candidate uses.  Overflow map RAMs are prioritized
 * for the reason that currently no meter can be the fit_on_logical_row candidate, and thus
 * the synth2port table is always second.  Perhaps if this check is ever folded in, the
 * correct table can be prioritized.
 */
void Memories::determine_color_map_RAM_masks(safe_vector<LogicalRowUser> &lrus, int row) const {
    bitvec map_RAM_in_use;
    for (auto &lru : lrus) map_RAM_in_use |= lru.map_RAM_mask;

    map_RAM_in_use |= bitvec(mapram_inuse[row] & MAPRAM_MASK);
    bool stats_bus_used = false;

    bool overflow_finished = true;
    for (auto &lru : lrus) {
        if (lru.bus != OFLOW) continue;
        bool is_meter = lru.group->type == SRAM_group::METER;
        if (is_meter) one_color_map_RAM_mask(lru, map_RAM_in_use, stats_bus_used, row);
        // Cannot start placing unless both the table and curr oflow are both finished, i.e.
        // color maprams could be placed before any RAMs
        overflow_finished = false;
        if (lru.RAM_mask.popcount() == lru.group->left_to_place() &&
            (!is_meter || lru.color_map_RAM_mask().popcount() == lru.group->cm.left_to_place())) {
            overflow_finished = true;
        }
    }

    for (auto &lru : lrus) {
        if (lru.bus != SYNTH) continue;
        if (lru.group->type != SRAM_group::METER) continue;
        ///> If the overflow candidates is not finished yet, then no color mapram of the SYNTH
        ///> table can be placed, as that this will require the synth_oflow for both
        if (!overflow_finished) continue;
        ///> Don't place a color map RAM until a RAM has been placed.  FIXME Not actually
        ///> a constraint, but a corner case not yet handled correctly by the compiler or assembler
        if (lru.RAM_mask.empty()) continue;
        one_color_map_RAM_mask(lru, map_RAM_in_use, stats_bus_used, row);
    }
}

/**
 * Given the candidates selected, these function determine which RAMs and which map RAMs go
 * to the corresponding candidates.  The LogicalRowUser vector is ordered based on which
 * candidate to prioritize for RAMs.
 *
 * At the end of this function, the vector will only contain candidates that have actual
 * allocations, i.e. RAMs or color map RAMs.
 */
void Memories::determine_logical_row_masks(safe_vector<LogicalRowUser> &lrus, int row,
                                           RAM_side_t side, int RAMs_available,
                                           bool is_synth_type) const {
    std::set<switchbox_t> bus_users;
    ///> Only one user per switchbox_t
    for (auto &lru : lrus) {
        if (bus_users.count(lru.bus))
            BUG("Multiple group trying to use the same bus type %s on logical row %d", lru.bus,
                phys_to_log_row(row, side));
        bus_users.insert(lru.bus);
    }

    determine_RAM_masks(lrus, row, side, RAMs_available, is_synth_type);
    if (is_synth_type) {
        determine_color_map_RAM_masks(lrus, row);
    }

    bitvec RAM_mask(sram_inuse[row]);
    bitvec map_RAM_mask(mapram_inuse[row]);
    for (auto &lru : lrus) {
        if (!(RAM_mask & lru.RAM_mask).empty())
            BUG("Collision of RAMs on logical row %d", phys_to_log_row(row, side));
        if (!(map_RAM_mask & lru.map_RAM_mask).empty())
            BUG("Collision of map RAMs on physical row %d", row);
        RAM_mask |= lru.RAM_mask;
        map_RAM_mask |= lru.map_RAM_mask;
    }

    ///> Clear any non users
    auto it = lrus.begin();
    while (it != lrus.end()) {
        if (it->set())
            it++;
        else
            it = lrus.erase(it);
    }
}

/**
 * The purpose of this function is to determine which SRAM_groups use which RAMs and map RAMs.
 * The potential candidates are determined based on their needs and if they can fit into
 * the current overflow set up.
 *
 * The potential candidates for synth2port are determined first.  Then the RAMs and map RAMs
 * for these candidates are determined.  This is then followed by the action data candidates.
 * There are two major reasons for this:
 *
 *     1. The overflow constraints are much stricter on synth2port tables than action data
 *        tables, and their constraints are easier to handle first.
 *     2. Certain action data tables are not able to be placed until their corresponding
 *        selector table is placed, given the constraints behind the selector adr switchbox.
 */
void Memories::find_swbox_candidates(safe_vector<LogicalRowUser> &lrus, int row, RAM_side_t side,
                                     SRAM_group *curr_oflow, SRAM_group *sel_oflow) {
    if (action_bus_users.empty() && synth_bus_users.empty()) return;

    std::array<int, OFLOW> RAMs_avail_on_row = {{0, 0}};
    ///> These SRAM_groups can fit entirely on this row without overflowing, and can be placed
    ///> without breaking overflow constraints
    std::array<SRAM_group *, OFLOW> fit_on_logical_row = {{nullptr, nullptr}};
    ///> These SRAM_groups require the maximum number of resources and should be prioritized
    std::array<SRAM_group *, OFLOW> max_req = {{nullptr, nullptr}};

    ///> Determine the synth2port tables and requirements
    if (side == RIGHT) {
        determine_synth_RAMs(RAMs_avail_on_row[SYNTH], row, curr_oflow);
        candidates_for_synth_row(&fit_on_logical_row[SYNTH], &max_req[SYNTH], row, curr_oflow,
                                 sel_oflow, RAMs_avail_on_row[SYNTH]);
        determine_synth_logical_row_users(fit_on_logical_row[SYNTH], max_req[SYNTH], curr_oflow,
                                          lrus, RAMs_avail_on_row[SYNTH]);
        determine_logical_row_masks(lrus, row, side, RAMs_avail_on_row[SYNTH], true);
    }

    SRAM_group *synth = nullptr;
    for (auto &lru : lrus) {
        if (lru.bus != SYNTH) continue;
        synth = lru.group;
        break;
    }

    ///> Determine action data tables and requirements
    determine_action_RAMs(RAMs_avail_on_row[ACTION], row, side, lrus);
    candidates_for_action_row(&fit_on_logical_row[ACTION], &max_req[ACTION], row, side, curr_oflow,
                              sel_oflow, RAMs_avail_on_row[ACTION], synth);
    determine_action_logical_row_users(fit_on_logical_row[ACTION], max_req[ACTION], synth,
                                       curr_oflow, sel_oflow, lrus, RAMs_avail_on_row[ACTION]);
    determine_logical_row_masks(lrus, row, side, RAMs_avail_on_row[ACTION], false);

    for (auto &lru : lrus) {
        LOG4("    Candidate " << lru.group << " on bus " << lru.bus << " RAM mask : 0x"
                              << lru.RAM_mask << " map RAM mask 0x" << lru.map_RAM_mask);
    }
}

/**
 * This fills out the Memories::Use object, as well as the Memories objects on which
 * RAMs (and synth2port map RAMs) are being used, based on the allocation in that
 * particular row
 */
void Memories::fill_RAM_use(LogicalRowUser &lru, int row, RAM_side_t side) {
    if (lru.RAM_mask.empty()) return;
    auto candidate = lru.group;

    auto unique_id = candidate->build_unique_id();
    auto name = unique_id.build_name();
    auto &alloc = (*candidate->ta->memuse)[unique_id];
    if (lru.bus == OFLOW) {
        overflow_bus[row][side] = name;
        alloc.row.emplace_back(row);
        if (candidate->type == SRAM_group::ACTION) alloc.row.back().word = candidate->number;
    } else if (lru.bus == SYNTH) {
        BUG_CHECK(side == RIGHT, "Allocating Synth2Port table on left side of RAM array");
        twoport_bus[row] = name;
        candidate->recent_home_row = phys_to_log_row(row, side);
        alloc.home_row.emplace_back(phys_to_log_row(row, side), candidate->number);
        alloc.row.emplace_back(row);
    } else if (lru.bus == ACTION) {
        action_data_bus[row][side] = name;
        alloc.row.emplace_back(row, side, candidate->number);
        alloc.home_row.emplace_back(phys_to_log_row(row, side), candidate->number);
        candidate->recent_home_row = phys_to_log_row(row, side);
    }
    alloc.row.back().bus = lru.bus;

    ///> Fills out the RAM objects
    for (int k = 0; k < SRAM_COLUMNS; k++) {
        if (((1 << k) & side_mask(side)) == 0) continue;

        if (lru.RAM_mask.getbit(k)) {
            sram_use[row][k] = name;
            alloc.row.back().col.push_back(k);

            // Spare VPN is global across the logical table
            if (candidate->vpn_spare &&
                candidate->calculate_next_vpn() == candidate->depth + candidate->vpn_offset - 1)
                alloc.row.back().vpn.push_back(candidate->vpn_spare);
            else
                alloc.row.back().vpn.push_back(candidate->calculate_next_vpn());

            candidate->placed++;
            if (candidate->is_synth_type()) {
                BUG_CHECK(k >= LEFT_SIDE_COLUMNS,
                          "Trying to allocate non-existant left side maprams");
                mapram_use[row][k - LEFT_SIDE_COLUMNS] = name;
                mapram_inuse[row] |= 1 << (k - LEFT_SIDE_COLUMNS);
                alloc.row.back().mapcol.push_back(k - LEFT_SIDE_COLUMNS);
#if 0
                // FIXME: VPNs are provided to assembler now
                if (candidate->placed == candidate->depth) {
                    // This is the last RAM in a syn2port table, so it is the spare and should
                    // not have a VPN.  It turns out to not matter as we don't output the vpns
                    // for syn2port in the compiler, and let the assembler allocate them.
                    alloc.row.back().vpn.pop_back();
                }
#endif
            }
        }
    }

    sram_inuse[row] |= lru.RAM_mask.getrange(0, SRAM_COLUMNS);
    if (lru.bus == SYNTH) {
        if (candidate->type == SRAM_group::STATS)
            stats_alus[row / 2] = name;
        else
            meter_alus[row / 2] = name;
    }
}

/** Fills the Use object for meters specifically for color maprams. */
void Memories::fill_color_map_RAM_use(LogicalRowUser &lru, int row) {
    auto candidate = lru.group;
    if (candidate->type != SRAM_group::METER) return;
    if (lru.color_map_RAM_mask().empty()) return;
    BUG_CHECK(candidate->type == SRAM_group::METER && candidate->cm.left_to_place() > 0,
              "Color maprams assigned to a non-meter");

    auto unique_id = candidate->build_unique_id();
    auto &alloc = (*candidate->ta->memuse)[unique_id];
    auto name = unique_id.build_name();

    int bus = -1;
    if (candidate->cm.cma == IR::MAU::ColorMapramAddress::IDLETIME) {
        int half = row / (SRAM_ROWS / 2);
        // FIXME: This is the simple solution for color mapram.  There are cases when the
        // color mapram cannot use the idletime bus, i.e. when the information comes through
        // hash distribution or if the table requires meter and idletime
        for (int i = 0; i < NUM_IDLETIME_BUS; i++) {
            if (idletime_bus[half][i] == name) {
                bus = i;
                break;
            }
        }

        for (int i = 0; i < NUM_IDLETIME_BUS && bus < 0; i++) {
            if (!idletime_bus[half][i]) bus = i;
        }
        BUG_CHECK(bus >= 0,
                  "Cannot have a negative color mapram bus.  Should always have a free "
                  "choice at this point");
        idletime_bus[half][bus] = name;
    } else if (candidate->cm.cma == IR::MAU::ColorMapramAddress::STATS) {
        // On stats, bus 0 will for the stats homerow, which is the only possible bus
        bus = 0;
    } else {
        BUG("Color mapram address not appropriately set up");
    }

    int color_vpn;
    if (!candidate->cm.placed)
        color_vpn = candidate->vpn_offset / 4;
    else
        color_vpn = alloc.color_mapram.back().vpn.back() + 1;

    alloc.color_mapram.emplace_back(row, bus);
    alloc.cma = candidate->cm.cma;
    bitvec color_mapram_mask = lru.color_map_RAM_mask();

    for (int k = 0; k < MAPRAM_COLUMNS; k++) {
        if (color_mapram_mask.getbit(k)) {
            mapram_use[row][k] = name;
            mapram_inuse[row] |= 1 << k;
            alloc.color_mapram.back().col.push_back(k);
            alloc.color_mapram.back().vpn.push_back(color_vpn++);
        }
    }
    candidate->cm.placed += color_mapram_mask.popcount();
}

void Memories::remove_placed_group(SRAM_group *candidate, RAM_side_t side) {
    Log::TempIndent indent;
    LOG5("Remove placed groups for candidate " << *candidate << ", side: " << side);
    if (candidate->all_placed() && candidate->cm.all_placed()) {
        if (candidate->is_synth_type()) {
            BUG_CHECK(side == RIGHT, "Allocating Synth2Port table on left side of RAM array");
            auto synth_table_loc = synth_bus_users.find(candidate);
            BUG_CHECK(synth_table_loc != synth_bus_users.end(),
                      "Removing a synth2port table that isn't in the list of potential tables");
            synth_bus_users.erase(synth_table_loc);
        } else {
            auto action_table_loc = action_bus_users.find(candidate);
            BUG_CHECK(action_table_loc != action_bus_users.end(),
                      "Removing an action table that isn't in the list of potential tables");
            LOG5("Removing placed group: " << *action_table_loc);
            action_bus_users.erase(action_table_loc);
        }
    }
}

/**
 * Updates the must place in half object if the strategy has locked particular RAMs in.
 */
void Memories::update_must_place_in_half(const SRAM_group *candidate, switchbox_t bus) {
    if (bus != SYNTH) return;
    BUG_CHECK(must_place_in_half.find(candidate) == must_place_in_half.end(),
              "Trying to "
              "allocate a synth2port table to multiple home rows");
    BUG_CHECK(candidate->is_synth_type(), "Trying to place non-synth2port group as synth2port");
    must_place_in_half.insert(candidate);
    for (auto ag : candidate->sel.action_groups) must_place_in_half.insert(ag);
}

void Memories::fill_swbox_side(safe_vector<LogicalRowUser> &lrus, int row, RAM_side_t side) {
    for (auto &lru : lrus) {
        BUG_CHECK(lru.set(), "Trying to fill in a use object with nothing to allocate");
        fill_RAM_use(lru, row, side);
        if (side == RIGHT) fill_color_map_RAM_use(lru, row);
        remove_placed_group(lru.group, side);
        update_must_place_in_half(lru.group, lru.bus);
    }
}

/**
 * Each logical row has up to three buses in order to move data to the associated position.
 * These buses are described in the switchbox_t enum:
 *
 *     ACTION - Using this bus outputs any action data RAMs on this row to the action data bus
 *         muxes.  This is referred to as the home row of the action data table
 *     SYNTH - Using this bus takes the meter alu/stats alu on this row.  Because counters
 *         and meters are on opposite row, the SYNTH notation for this is fine.  This is
 *         also referred to as the home row of the synth2port table
 *     OFLOW - Any RAM using the overflow bus will be overflowing the data up to its homerow
 *         through the switchbox
 *
 * The purpose of this function is to determine which SRAM_groups uses which buses, and RAMs
 * in those rows, and then to allocate those RAMs to that positijon
 */
void Memories::swbox_logical_row(safe_vector<LogicalRowUser> &lrus, int row, RAM_side_t side,
                                 SRAM_group *curr_oflow, SRAM_group *sel_oflow) {
    lrus.clear();
    LOG4("    RAMs available on row " << bitcount(side_mask(side) & ~sram_inuse[row]));
    if (bitcount(side_mask(side) & ~sram_inuse[row]) == 0) return;
    find_swbox_candidates(lrus, row, side, curr_oflow, sel_oflow);

    if (lrus.empty()) return;
    fill_swbox_side(lrus, row, side);
}

/**
 * Calculates two possible overflow objects:
 *    curr_oflow - the group that is overflowing from the current logical row to the
 *        next logical row.  After this calculation, can only be action data, as synth2port
 *        only appear on half of the rows.
 *    synth_oflow - the synth2port table that will be overflowing when moving to the next
 *        physical row.  Will override the curr_oflow, if that object is action data
 *
 * All double pointers are set in this function
 */
void Memories::calculate_curr_oflow(safe_vector<LogicalRowUser> &lrus, SRAM_group **curr_oflow,
                                    SRAM_group **synth_oflow, RAM_side_t side) const {
    std::array<SRAM_group *, SWBOX_TYPES> candidates = {{nullptr, nullptr, nullptr}};

    for (auto &lru : lrus) {
        candidates[static_cast<int>(lru.bus)] = lru.group;
    }

    ///> The overflow candidate by definition must be the curr_oflow object
    if (candidates[OFLOW])
        BUG_CHECK(*curr_oflow != nullptr && *curr_oflow == candidates[OFLOW],
                  "Error in "
                  "calculating overflow");

    bool synth_oflow_needed = false;
    if (side == RIGHT) {
        if (candidates[SYNTH] &&
            !(candidates[SYNTH]->all_placed() && candidates[SYNTH]->cm.all_placed())) {
            *synth_oflow = candidates[SYNTH];
            synth_oflow_needed = true;
        }
        if (*curr_oflow && (*curr_oflow)->is_synth_type() &&
            !((*curr_oflow)->all_placed() && (*curr_oflow)->cm.all_placed())) {
            BUG_CHECK(!synth_oflow_needed, "Multiple synth2port require overflow");
            *synth_oflow = *curr_oflow;
            synth_oflow_needed = true;
        }

        if (!synth_oflow_needed) *synth_oflow = nullptr;
    }

    bool curr_oflow_needed = false;
    // Can potentially overflow from both the upper portion or right to left as well.  Right
    // now we favor upper than right to left, but if right to left is possible, then we use that
    if (*curr_oflow && !(*curr_oflow)->is_synth_type() && !(*curr_oflow)->all_placed()) {
        curr_oflow_needed = true;
        // Again, this will overflow curr_oflow if no RAMs were allocated in this logical row
    } else if (candidates[ACTION] && !candidates[ACTION]->all_placed()) {
        *curr_oflow = candidates[ACTION];
        curr_oflow_needed = true;
    }

    if (!curr_oflow_needed) *curr_oflow = nullptr;
}

/**
 * This function calculates the selector overflow, i.e. the link between selector and action
 * data headed through the selector_adr_switchbox
 */
void Memories::calculate_sel_oflow(safe_vector<LogicalRowUser> &lrus,
                                   SRAM_group **sel_oflow) const {
    bool sel_oflow_needed = false;
    SRAM_group *synth = nullptr;
    for (auto &lru : lrus) {
        if (lru.bus != SYNTH) continue;
        synth = lru.group;
        break;
    }

    if (*sel_oflow && !(*sel_oflow)->sel.action_all_placed()) sel_oflow_needed = true;

    if (synth && synth->type == SRAM_group::SELECTOR && !synth->sel.action_all_placed()) {
        BUG_CHECK(!sel_oflow_needed, "Collision on selector switch box");
        sel_oflow_needed = true;
        *sel_oflow = synth;
    }

    if (!sel_oflow_needed) *sel_oflow = nullptr;
}

void Memories::log_allocation(safe_vector<table_alloc *> *tas,
                              UniqueAttachedId::pre_placed_type_t ppt) {
    for (auto *ta : *tas) {
        for (auto u_id : ta->allocation_units(nullptr, false, ppt)) {
            auto alloc = (*ta->memuse)[u_id];
            LOG4("Allocation for " << u_id.build_name());
            for (auto row : alloc.row) {
                LOG4("(Row, Bus, [Columns]) : (" << row.row << ", " << row.bus << ", [" << row.col
                                                 << "])");
            }
        }
    }
}

void Memories::log_allocation(safe_vector<table_alloc *> *tas, UniqueAttachedId::type_t type) {
    for (auto *ta : *tas) {
        for (auto ba : ta->table->attached) {
            if (ba->attached->unique_id().type != type) continue;
            for (auto u_id : ta->allocation_units(ba->attached)) {
                auto alloc = (*ta->memuse)[u_id];
                LOG4("Allocation for " << u_id.build_name());
                for (auto row : alloc.row) {
                    LOG4("(Row, Bus, [Columns]) : (" << row.row << ", " << row.bus << ", ["
                                                     << row.col << "])");
                }
            }
        }
    }
}

/* Logging information for each individual action/twoport table information */
void Memories::action_bus_users_log() {
    log_allocation(&action_tables, UniqueAttachedId::ADATA_PP);
    log_allocation(&stats_tables, UniqueAttachedId::COUNTER);
    log_allocation(&meter_tables, UniqueAttachedId::METER);
    log_allocation(&stateful_tables, UniqueAttachedId::STATEFUL_ALU);
    log_allocation(&selector_tables, UniqueAttachedId::SELECTOR);
    log_allocation(&indirect_action_tables, UniqueAttachedId::ACTION_DATA);
}

/** The purpose of this section of code is to allocate all data HV (Horizontal/Vertical) switchbox
 *  users.  This particular switchbox is shown in section 6.2.4.4 (RAM Data Bus Horizontal/Vertical
 *  (HV) Switchbox.  The possible tables that can use this switch box are action data tables,
 *  as well as all synthetic twoport tables, (Stats, Meter, StatefulAlu, and Selector) tables.
 *
 *  The RAM array is 8 rows x 10 columns.  These columns are divided into 2 sides, a left and a
 *  right side.  The left side has 4 RAMs per row while the right side has 6 RAMs per row.  Each
 *  individual row and side has a bus from which action data can flow from  a RAM to the action
 *  data bus headed to the ALUs.
 *
 *  The right side of the RAM array is where all synth2port tables must go.  This is for a couple
 *  of reasons.  First, each synth2port RAM uses a corresponding map RAM, which is an 11 bit x
 *  1024 row RAM.  These are used to hold addressing information to perform simultaneous reads
 *  and writes.  There is a one-to-one correspondence with map RAMs and the RAM array table,
 *  but only on the right side of the RAM array.  Thus at most 6 * 8 = 48 possible locations
 *  for a synth2port table to go.
 *
 *  Also on the right side of the RAM array are ALUs to perform stateful operations.  Every odd
 *  row has a meter ALU, which can perform meter, selector, or stateful operations.  Every even
 *  row has a stats ALU, which can perform stats operations.  Each of these ALUs has a
 *  corrsponding meter/stats bus, depending on ALU on that particular row.
 *
 *  Lastly, every single row, both left and right, has an overflow bus.  This bus can be used
 *  on a row below the original home row of that attached table.  Using the switchbox, the data
 *  can then flow either horizontally or vertically to the home row location of that particular
 *  action data/synth2port table.  This is extremely useful if for instance the table cannot fit
 *  on only one row.
 *
 *  The switchbox itself has limitations.  Only one bus can vertically overflow between rows.
 *  Thus, the bus can either come from that row, or the previously overflowed bus.  Also,
 *  a left side bus can overflow to a right side home row, but not the other way.  A bus through
 *  the vertical switchbox bus can either go to the left or right side.  In the uArch, you might
 *  see the word overflow2.  That bus no longer exists.  Fake news!
 *
 *  This algorithm is to navigate all of these constraints.  The general algorithm follows the
 *  algorithm generally specified in section 6.2.4.1.  The algorithm occurs after exact match
 *  and ternary indirect have already taken place.  The algorithm selects row by row which
 *  candidates belong on that row, and then assign a corresponding number of RAMs and map RAMs
 *  to those candidates.
 *
 *  Two other things have separate overflow constraints, specifically selectors and color mapram.
 *  Selectors provide an address offset to an action table, and have a separate overflow switchbox
 *  in order to provide this address.  The overflow switchbox, similar to the data overflow,
 *  can only overflow one address at a time.  Also, this means that an action data table attached
 *  to a selector must be at or below the row the selector is in.  This is specified in uArch
 *  section 6.2.8.4.7 Selector RAM Addressing, and is handled by the sel_oflow structure.
 *
 *  The other overflow is color maprams.  Color maprams are attached to meters and save the color
 *  information, and have a separate overflow structure.  The details of how color maprams are
 *  currently allocated are summarized above the color_mapram_candidates section.
 *
 *  Also please note that there are two buses to consider, the buses to address the RAMs and
 *  the buses to move data to the correct place.  Unless specified directly, the constraints
 *  generally come directly from the data bus, as overflow for addresses is a little simpler.
 *  If the constraint is directly address bus related, this should be specified in the comments,
 *  (i.e. the selector oflow is a constraint related to address buses).
 */
bool Memories::allocate_all_swbox_users() {
    Log::TempIndent indent;
    LOG3("Allocating all swbox users" << indent);
    find_swbox_bus_users();
    safe_vector<LogicalRowUser> lrus;
    SRAM_group *curr_oflow = nullptr, *sel_oflow = nullptr, *synth_oflow = nullptr;

    for (int i = SRAM_ROWS - 1; i >= 0; i--) {
        synth_oflow = nullptr;
        for (int j = RAM_SIDES - 1; j >= 0; j--) {
            auto side = static_cast<RAM_side_t>(j);
            LOG4("    Allocating Logical Row " << phys_to_log_row(i, side));
            swbox_logical_row(lrus, i, side, curr_oflow, sel_oflow);
            calculate_curr_oflow(lrus, &curr_oflow, &synth_oflow, side);
            calculate_sel_oflow(lrus, &sel_oflow);
            if (curr_oflow)
                BUG_CHECK(!(curr_oflow->all_placed() && curr_oflow->cm.all_placed()),
                          "The "
                          "overflow candidate has already been placed");
        }

        // Always overflow the synth2port table between rows
        if (synth_oflow) {
            BUG_CHECK(!(synth_oflow->all_placed() && synth_oflow->cm.all_placed()),
                      "The "
                      "synth2port overflow is not correct");
            curr_oflow = synth_oflow;
        }

        if (i != SRAM_ROWS - 1 && curr_oflow)
            vert_overflow_bus[i] =
                std::make_pair(curr_oflow->build_unique_id().build_name(), curr_oflow->number);

        // Due to a timing constraint not yet even described in the uArch, the maximum number
        // of rows a particular table can overflow is 6.
        // FIXME: Need to cause this to fail specifically on twoport tables that can't use more
        // than 1 ALU
        if (curr_oflow) {
            LOG4("    curr_oflow recent home row " << curr_oflow->recent_home_row);
            BUG_CHECK(curr_oflow->recent_home_row >= 0, "Home row is not set on overflow");
            if (log_to_phys_row(curr_oflow->recent_home_row) - MAX_DATA_SWBOX_ROWS == i) {
                ///> Color map RAMs can overflow more than 6 rows.
                if (curr_oflow->is_synth_type()) {
                    if (!curr_oflow->all_placed())
                        BUG("A synth2port has overflowed over the maximum number of rows. "
                            "Should not have reached this point");
                } else {
                    LOG4("    We can clear out curr oflow of an action "
                         << curr_oflow->build_unique_id());
                    curr_oflow = nullptr;
                }
            }
        }

#ifdef HAVE_JBAY
        // JBay has no overflow bus between logical row 7 and 8
        if ((Device::isMemoryCoreSplit()) && i == MATCH_CENTRAL_ROW) {
            for (auto group : must_place_in_half) {
                if (!(group->all_placed() && group->cm.all_placed())) {
                    LOG4("    Group not finished " << group);
                    // FIXME -- we started placing some synth2port and/or selector table(s) in
                    // the upper half and didn't manage to fully place them.  This suggests a
                    // problem in Memories::can_be_placed_in_half where it either incorrectly
                    // computes the number of availble memories in the half, or doesn't
                    // consider interactions between multiple tables.
                    // As a stopgap, we just fail memory allocation instead of aborting
                    failure_reason = "syth2port failed over center on jbay"_cs;
                    LOG4(failure_reason);
                    return false;
                }
            }
            curr_oflow = nullptr;
            must_place_in_half.clear();
        }
#endif /* HAVE_JBAY */
    }

    if (!action_bus_users.empty() || !synth_bus_users.empty()) {
        int act_unused = 0;
        for (auto abu : action_bus_users) act_unused += abu->left_to_place();

        int sup_unused = 0;
        for (auto sbu : synth_bus_users) sup_unused += sbu->left_to_place();

        failure_reason = "allocate_all_swbox_users failed"_cs;
        LOG4(failure_reason);
        return false;
    }
    action_bus_users_log();
    return true;
}

Memories::table_alloc *Memories::find_corresponding_exact_match(cstring name) {
    for (auto *ta : exact_tables) {
        auto check_name = ta->build_unique_id().build_name();
        if (check_name == name) return ta;
    }
    return nullptr;
}

bool Memories::gw_search_bus_fit(table_alloc *ta, table_alloc *exact_ta, int row, int col) {
    // Proxy Hash Tables cannot share a search bus with a gateway, as the input
    // from the xbar cannot be merged onto the same bus.  See uArch 6.2.3. Exact Match Row
    // Vertical/Horizontal (VH) XBars
    if (exact_ta->layout_option->layout.proxy_hash) return false;
    auto search_bus = sram_search_bus[row][col];
    BUG_CHECK(exact_ta->table_format->ixbar_group_per_width.count(search_bus.hash_group) > 0,
              "Miscoordination of what hash groups are on the search bus vs. what hash "
              "groups are in the table format");

    // FIXME -- even if mulitple hash functions are needed for many ways, the search data
    // will always come from the first one, so that all ways will have the same match format
    // in the RAM (required by the driver).  If the driver ever changes, this will also change,
    // and we'll need a way to tell the assembler which search format to use for each way.
    // Currently an exact match table can only have one format in the .bfa
    const auto &ixbar_groups = exact_ta->table_format->ixbar_group_per_width.begin()->second;
    int ixbar_group = ixbar_groups[search_bus.width_section];

    if (ixbar_group != ta->match_ixbar->gateway_group()) return false;

    int lo = TableFormat::SINGLE_RAM_BYTES * search_bus.width_section;

    auto avail_bytes = exact_ta->table_format->avail_sb_bytes;
    auto match_ixbar = dynamic_cast<const IXBar::Use *>(ta->match_ixbar);
    BUG_CHECK(match_ixbar, "No match ixbar allocated?");
    if (avail_bytes.getslice(lo, match_ixbar->gw_search_bus_bytes).popcount() !=
        match_ixbar->gw_search_bus_bytes)
        return false;
    return true;
}

/** This algorithm looks for the first free available gateway.  The gateway may need a search
 *  bus as well, so the algorithm looks for that too.
 */
bool Memories::find_unit_gw(Memories::Use &alloc, cstring name, bool requires_search_bus) {
    for (int i = 0; i < SRAM_ROWS; i++) {
        for (int j = 0; j < GATEWAYS_PER_ROW; j++) {
            if (gateway_use[i][j]) continue;
            for (int k = 0; k < BUS_COUNT; k++) {
                if (requires_search_bus && sram_search_bus[i][k].name) continue;
                alloc.row.clear();
                alloc.row.emplace_back(i, k);
                alloc.gateway.unit = j;
                gateway_use[i][j] = name;
                if (requires_search_bus) {
                    sram_search_bus[i][k] = search_bus_info(name, 0, 0);
                    LOG7("Setting search bus for unit gw [" << i << "][" << k << "] to " << name);
                }
                return true;
            }
        }
    }
    return false;
}

/** Finds a search bus for the gateway.  The alogrithm first looks at all used search buses,
 *  to see if it can reuse a search bus on the table.  If it cannot find a search bus to
 *  share, then it finds the first free search bus to use
 */
bool Memories::find_search_bus_gw(table_alloc *ta, Memories::Use &alloc, cstring name) {
    auto match_ixbar = dynamic_cast<const IXBar::Use *>(ta->match_ixbar);
    BUG_CHECK(match_ixbar, "No match ixbar allocated?");
    for (int i = 0; i < SRAM_ROWS; i++) {
        for (int j = 0; j < GATEWAYS_PER_ROW; j++) {
            if (gateway_use[i][j]) continue;
            for (int k = 0; k < BUS_COUNT; k++) {
                auto search = sram_search_bus[i][k];
                if (search.free()) continue;
                table_alloc *exact_ta = find_corresponding_exact_match(search.name);
                if (exact_ta == nullptr) continue;
                // FIXME: currently we have to fold in the table format to this equation
                // in order to share a format
                if (match_ixbar->gw_search_bus) {
                    if (gw_bytes_reserved[i][k]) continue;
                    if (!gw_search_bus_fit(ta, exact_ta, i, k)) continue;
                }
                // Because multiple ways could have different hash groups, this check is no
                // longer valid
                if (match_ixbar->gw_hash_group) {
                    auto sbi = sram_search_bus[i][k];
                    if (sbi.hash_group != match_ixbar->bit_use[0].group) continue;
                    // FIXME: Currently all ways do not share the same hash_group
                    // if (match_ixbar->bit_use[0].group
                    //  != exact_match_ixbar->way_use[0].group)
                    // continue;
                }
                exact_ta->attached_gw_bytes += match_ixbar->gw_search_bus_bytes;
                gw_bytes_reserved[i][k] = true;
                alloc.row.clear();
                alloc.row.emplace_back(i, k);
                alloc.gateway.unit = j;
                gateway_use[i][j] = name;
                return true;
            }
        }
    }

    LOG7("No search bus found for reuse for gateway");
    return find_unit_gw(alloc, name, true);
}

/** Finding a result bus for the gateway.  Will save associated information, such as payload
 *  row, bus, and value, as well as link no match tables if necessary
 */
bool Memories::find_result_bus_gw(Memories::Use &alloc, uint64_t payload, cstring gw_name,
                                  table_alloc *table, int logical_table) {
    LOG5("Finding result bus for gateway " << gw_name << " on table " << table->table->name
                                           << IndentCtl::indent);
    Memories::Use *sram_use = nullptr;
    auto *result_bus = &sram_result_bus;
    auto *print_result_bus = &sram_print_result_bus;
    auto match_id = table->build_unique_id(nullptr, false, logical_table);
    bool ternary = false;
    if (table->memuse->count(match_id) != 0) {
        sram_use = &table->memuse->at(match_id);
        switch (sram_use->type) {
            case Use::TIND:
                print_result_bus = &tind_bus;
                result_bus = nullptr;
                ternary = true;
                break;
            case Use::EXACT:
            case Use::ATCAM:
                break;
            default:
                // no result bus in this memory type
                sram_use = nullptr;
                break;
        }
    }
    // FIXME: Check if below code is valid and gets executed in any regression
    // tests. Add a BUG_CHECK? Possible legacy code which can be removed
    if (!sram_use) {
        for (auto &mem : *table->memuse) {
            switch (mem.second.type) {
                case Use::TIND:
                    print_result_bus = &tind_bus;
                    result_bus = nullptr;
                    ternary = true;
                    BFN_FALLTHROUGH;
                case Use::EXACT:
                case Use::ATCAM:
                    match_id = mem.first;
                    sram_use = &mem.second;
                    break;
                default:
                    continue;
            }
            break;
        }
    }
    if (!sram_use) {
        if (table->memuse->count(match_id) == 0) {
            sram_use = &(*table->memuse)[match_id];
            sram_use->type = Use::EXACT;
        } else {
            // FIXME already have a memuse for the table, but no TIND or EXACT -- its probably
            // a TERNARY table with no TIND, so need to allocate a TIND bus.  For now we skip
            // this case, which will result in not outputting a bus spec, so the assembler
            // will allocate a bus that is otherwise not in use, if it can find one
            ternary = table->memuse->at(match_id).type == Use::TERNARY;
        }
    }
    // If the table has any direct attached synth2port tables, need to force the payload
    // match address to be invalid in case the PFE is defaulted.
    // FIXME -- this probably belongs somewhere else?
    for (auto at : table->table->attached) {
        if (at->attached->direct &&
            (at->attached->is<IR::MAU::Synth2Port>() || at->attached->is<IR::MAU::IdleTime>())) {
            LOG6("Forcing payload match address to 0x7ffff on attached table " << at);
            alloc.gateway.payload_match_address = 0x7ffff;
            break;
        }
    }
    ordered_set<int> rows;
    // search on rows already in use first -- FIXME for wide exact match should only be
    // searching on rows that already use a result bus first.
    if (sram_use)
        for (auto &r : sram_use->row) rows.insert(r.row);
    for (int i = 0; i < SRAM_ROWS; i++) rows.insert(i);
    for (int row : rows) {
        int bus = 0;
        if (sram_use) {
            for (auto &r : sram_use->row) {
                if (r.row == row && r.result_bus >= 0) {
                    bus = r.result_bus;
                    break;
                }
            }
        }
        for (; bus < BUS_COUNT; bus++) {
            if ((*print_result_bus)[row][bus] &&
                (*print_result_bus)[row][bus] != match_id.build_name()) {
                LOG7("Cannot assign on row " << row << " and bus " << bus
                                             << " as it is occupied by "
                                             << (*print_result_bus)[row][bus]);
                continue;
            }
            alloc.gateway.bus_type = ternary ? Use::TIND : Use::EXACT;
            LOG7("Payload use on row " << row << " and bus " << bus << " is "
                                       << payload_use[row][bus]);
            if (payload != 0ULL || alloc.gateway.payload_match_address >= 0) {
                if (auto pUse = payload_use[row][bus]) {
                    if (pUse != gw_name) continue;
                }
                alloc.gateway.payload_row = row;
                alloc.gateway.payload_unit = bus;
                alloc.gateway.payload_value = payload;
                payload_use[row][bus] = gw_name;
            }
            if (sram_use &&
                !std::any_of(sram_use->row.begin(), sram_use->row.end(), [row, bus](Use::Row &r) {
                    return r.row == row && r.result_bus == bus;
                })) {
                sram_use->row.emplace_back(row);
                sram_use->row.back().result_bus = bus;
                if (ternary) sram_use->row.back().bus = bus;
            }
            if (result_bus)
                (*result_bus)[row][bus] = result_bus_info(match_id.build_name(), 0, logical_table);
            (*print_result_bus)[row][bus] = match_id.build_name();
            LOG6("Result bus assigned on row " << row << " and bus " << bus << " for "
                                               << (*print_result_bus)[row][bus]);
            LOG5_UNINDENT;
            return true;
        }
    }
    LOG5("No result bus found" << IndentCtl::unindent);
    return false;
}

/**
 * Based on the information of what bits are to be pulled from the gateway payload to be
 * used as address/per flow enable/type lookups, this will determine the value to be
 * written into the 64 bit payload.
 *
 * Currently this is based off of the Tofino assumption that only one payload may possibly
 * exist.  This will need to be expanded for JBay, as multiple payloads based on the gateway
 * row are definitely possible, and will be required for multi-stage DLEFT hash tables.
 */
uint64_t Memories::determine_payload(table_alloc *ta) {
    TableResourceAlloc temp_alloc;
    temp_alloc.table_format = *ta->table_format;
    temp_alloc.instr_mem = *ta->instr_mem;
    temp_alloc.action_format = *ta->action_format;
    bitvec payload_bv;
    if (ta->payload_match_addr_only) {
        // FIXME -- determine_payload assumes the payload is coming from the
        // match table, which is not the case when we're just using a payload to
        // supply an invalid match address to avoid running a direct table
        // this needs cleaning up
    } else if (!ta->format_type.matchThisStage()) {
        // FIXME -- running a split attached table -- the address comes from hash_dist
        // if part of the action needs immed data (from match or tind), that would have to
        // come from payload -- not yet implemented
    } else {
        payload_bv = FindPayloadCandidates::determine_payload(ta->table, &temp_alloc,
                                                              &ta->layout_option->layout);
    }
    return payload_bv.getrange(0, 64);
}

/** Allocates all gateways with a payload, which is a conditional linked to a no match table.
 *  Thus it needs a payload, result bus, and search bus.
 */
bool Memories::allocate_all_payload_gw(bool alloc_search_bus) {
    LOG3("Allocate all payload gateways with alloc search bus set to "
         << (alloc_search_bus ? "true" : "false"));
    for (auto *ta : payload_gws) {
        auto match_ixbar = dynamic_cast<const IXBar::Use *>(ta->match_ixbar);
        BUG_CHECK(match_ixbar, "No match ixbar allocated?");
        safe_vector<UniqueId> u_ids;
        UniqueId unique_id;
        if (ta->table_link) {
            u_ids = ta->table_link->allocation_units(nullptr, true);
        } else {
            BUG_CHECK(ta->layout_option->layout.gateway_match,
                      "Payload requiring gateway must "
                      "have been originally a match table");
            u_ids = ta->allocation_units(nullptr, true);
        }

        for (auto u_id : u_ids) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Use::GATEWAY;
            alloc.used_by = ta->table->build_gateway_name();
            if (alloc_search_bus != match_ixbar->search_data()) continue;

            bool gw_found = false;
            if (alloc_search_bus)
                gw_found = find_search_bus_gw(ta, alloc, u_id.build_name());
            else
                gw_found = find_unit_gw(alloc, u_id.build_name(), false);

            table_alloc *payload_ta = ta->table_link ? ta->table_link : ta;
            uint64_t payload_value = determine_payload(payload_ta);
            bool result_bus_found = find_result_bus_gw(alloc, payload_value, u_id.build_name(),
                                                       // Change this in a little bit
                                                       payload_ta);
            if (!(gw_found && result_bus_found)) {
                if (!gw_found) LOG3("  failed to find gw for " << u_id);
                if (!result_bus_found) LOG3("  failed to find result_bus for " << u_id);
                failure_reason = "failed to place payload gw "_cs + u_id.build_name() +
                                 (alloc_search_bus ? " with search bus"_cs : " no search bus"_cs);
                return false;
            }
            BUG_CHECK(alloc.row.size() == 1, "Help me payload");
        }
    }
    return true;
}

/** Allocation of a conditional gateway that does not map to a no match hit path table.
 * Conditional gateways require a search bus and result bus.  If the table is linked to another
 * table, then the result bus will be the same as the result bus of the linked table.
 */
bool Memories::allocate_all_normal_gw(bool alloc_search_bus) {
    for (auto *ta : normal_gws) {
        bool need_search_bus = false;
        if (auto match_ixbar = dynamic_cast<const IXBar::Use *>(ta->match_ixbar)) {
            need_search_bus = match_ixbar->search_data();
        }
        safe_vector<UniqueId> u_ids = ta->table_link
                                          ? ta->table_link->allocation_units(nullptr, true)
                                          : ta->allocation_units(nullptr, true);
        std::string used_by = ta->table->build_gateway_name() + "";
        for (auto u_id : u_ids) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Use::GATEWAY;
            alloc.used_by = used_by;
            if (alloc_search_bus != need_search_bus) continue;
            bool gw_found = false;
            if (alloc_search_bus)
                gw_found = find_search_bus_gw(ta, alloc, u_id.build_name());
            else
                gw_found = find_unit_gw(alloc, u_id.build_name(), false);
            alloc.gateway.payload_value = 0ULL;
            alloc.gateway.payload_unit = -1;
            alloc.gateway.payload_row = -1;
            if (!gw_found) {
                LOG3("  failed to find gw for " << u_id);
                failure_reason = "failed to place normal gw "_cs + u_id.build_name() +
                                 (alloc_search_bus ? " with search bus"_cs : " no search bus"_cs);
                return false;
            }
            BUG_CHECK(alloc.row.size() == 1, "Help me normal");
        }
    }

    return true;
}

/** This function call allocates no match hit tables that don't require a payload.  This means
 *  only a result bus is needed
 */
bool Memories::allocate_all_no_match_gw() {
    for (auto *ta : no_match_gws) {
        for (auto u_id : ta->allocation_units(nullptr, true)) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Use::GATEWAY;
            alloc.used_by = ta->table->build_gateway_name();
            bool unit_found = find_unit_gw(alloc, u_id.build_name(), false);
            bool result_bus_found =
                find_result_bus_gw(alloc, 0ULL, u_id.build_name(), ta, u_id.logical_table);
            if (!(unit_found && result_bus_found)) {
                if (!unit_found) LOG3("  failed to find gw for " << u_id);
                if (!result_bus_found) LOG3("  failed to find result_bus for " << u_id);
                failure_reason = "failed to place no_match gw "_cs + u_id.build_name();
                return false;
            }
            BUG_CHECK(alloc.row.size() == 1, "Help me no match");
        }
    }
    return true;
}

/** The two allocation schemes allocate_all_gw and allocate_all_no_match_miss are responsible for
 *  the allocation of both all gateways and all no match tables.  Keyless tables can go through
 *  two pathways within match central, through the hit path or the miss path.
 *
 *  There are two types of standard no match tables.  Currently keyless tables that can have
 *  no action data, and only one action are allocated to go through the hit path, which is what
 *  glass does.  The tables that require action data or have multiple potential actions have
 *  to go through the miss path, as those values are configurable by the driver, and currently
 *  the driver can only rewrite the miss registers, even though these registers are completely
 *  configurable through the hit path.  Lastly, all keyless tables that require hash distribution
 *  require the hit path, as anything needed with hash distribution goes through the format
 *  and merge block.
 *
 *  A gateway is the hardware block for handling a conditional block.  There are 16 gateways per
 *  stage, two per rows.  Gateways are 4 row, 44 bit TCAMs, and can bring there comparison data
 *  in two ways.  The lower 32 bits are brought in through the lower 32 bits of an 128 bit
 *  search bus, while the upper 12 bits come from the upper 12 bits of a 52 bit hash.  The
 *  necessary gateway resources are known at this point, so it's a matter of reserving either
 *  a search bus, a hash bus or both.
 *
 *  Keyless table that go through the hit path must initialize a gateway to always hit, and thus
 *  must find a gateway.  However, if the no match table through the hit path has a gateway
 *  attached to it, this must use a payload in order for the gateway to be configured correctly.
 *  The details of why this is necessary are contained within the asm_output, which talks about
 *  the initialization of gateways.
 *
 *  Out of this comes three reservations of gateways.
 *  1. Gateways that require payloads, which are conditionals linked to no match hit path tables.
 *     In this case, a payload is required be resereved.
 *  2. Gateways that are conditionals.  These conditionals can be paired with a match table or
 *     exist by themselves.  (If the conditionals don't require search, then the search bus
 *     is not necessary)
 *  3. Gateways that are no match tables alone.  Due to the nature of the hit path, the gateway
 *     is an always true gateway that always hits, and does not need to search
 *
 *  This list goes from least to most complex, specifically:
 *  1.  Reserve gateway unit, search bus, result bus, payload
 *  2.  Reserve gateway unit, search bus
 *  3.  Reserve gateway unit, result bus, payload (if the gateway has no search data)
 *  4.  Reserve gateway unit, result bus
 *  3.  Reserve gateway unit (if the gateway has no search data)
 */
bool Memories::allocate_all_gw() {
    payload_gws.clear();
    normal_gws.clear();
    no_match_gws.clear();

    Log::TempIndent indent;
    if (!gw_tables.empty() && !no_match_hit_tables.empty())
        LOG3("Allocating all gateway tables" << indent);
    for (auto *ta_gw : gw_tables) {
        bool pushed_back = false;
        for (auto *ta_nm : no_match_hit_tables) {
            if (ta_gw->table_link == ta_nm) {
                payload_gws.push_back(ta_gw);
                pushed_back = true;
                break;
            }
        }
        if (pushed_back) continue;

        if (auto *mt = ta_gw->table_link ? ta_gw->table_link->table : nullptr) {
            for (auto at : mt->attached) {
                if (at->attached->direct && (at->attached->is<IR::MAU::Synth2Port>() ||
                                             at->attached->is<IR::MAU::IdleTime>())) {
                    LOG5("Adding payload gw for attached table : " << at);
                    /* direct attached synth2port / idletime tables will use the
                     * match address, so we need to make sure the gateway has an
                     * invalid match address in case it wants to completely
                     * override the linked match table
                     * FIXME -- the direct attached could in theory use a PFE in
                     * the match overhead, in which case this would not be
                     * necessary.  Hard to know when we could/should do that --
                     * in the match table layout perhaps? */
                    payload_gws.push_back(ta_gw);
                    ta_gw->payload_match_addr_only = true;
                    ta_gw->table_link->payload_match_addr_only = true;
                    pushed_back = true;
                    break;
                }
            }
        }
        if (pushed_back) continue;

        normal_gws.push_back(ta_gw);
    }

    for (auto *ta_nm : no_match_hit_tables) {
        bool linked = false;
        for (auto &ta_gw : gw_tables) {
            if (ta_gw->table_link == ta_nm) {
                linked = true;
                break;
            }
        }

        if (ta_nm->layout_option->layout.gateway_match) {
            payload_gws.push_back(ta_nm);
            linked = true;
        }

        if (!linked) {
            no_match_gws.push_back(ta_nm);
        }
    }

    int search_bus_free = 0;
    for (int i = 0; i < SRAM_ROWS; i++) {
        for (int j = 0; j < 2; j++) {
            if (sram_search_bus[i][j].free()) search_bus_free++;
        }
    }

    if (!allocate_all_payload_gw(true)) return false;
    if (!allocate_all_normal_gw(true)) return false;
    if (!allocate_all_payload_gw(false)) return false;
    if (!allocate_all_no_match_gw()) return false;
    if (!allocate_all_normal_gw(false)) return false;
    return true;
}

/** This allocates all tables that currently take the miss path information.  The miss path
 *  is how action data information can be configured by runtime.  This would be necessary
 *  if multiple actions are needed/action data is changeable.  Just reserved a result bus
 *  for the time being.
 */
bool Memories::allocate_all_no_match_miss() {
    Log::TempIndent indent;
    if (!no_match_miss_tables.empty()) LOG3("Allocate all no match miss" << indent);
    // FIXME: Currently the assembler supports exact match to make calls to immediate here,
    // so this is essentially what I'm doing.  More discussion is needed with the driver
    // team in order to determine if this is correct, or if this has to go through ternary and
    // tind tables
    size_t no_match_tables_allocated = 0;
    for (auto *ta : no_match_miss_tables) {
        for (auto u_id : ta->allocation_units(nullptr, false, UniqueAttachedId::TIND_PP)) {
            auto &alloc = (*ta->memuse)[u_id];
            alloc.type = Use::TIND;
            alloc.used_by = ta->table->externalName();
            bool found = false;
            for (int i = 0; i < SRAM_ROWS; i++) {
                for (int j = 0; j < BUS_COUNT && j < PAYLOAD_COUNT; j++) {
                    if (payload_use[i][j]) continue;
                    if (tind_bus[i][j]) continue;
                    alloc.row.emplace_back(i, j);
                    alloc.row.back().result_bus = j;
                    tind_bus[i][j] = u_id.build_name();
                    payload_use[i][j] = u_id.build_name();
                    found = true;
                    break;
                }
                if (found) break;
            }

            if (!found) {
                failure_reason = "failed to place no match miss "_cs + u_id.build_name();
                LOG4(failure_reason);
                return false;
            } else {
                no_match_tables_allocated++;
            }
        }
    }
    return true;
}

/**
 * The purpose of this function is to allocate a ternary indirect bus only for a TCAM table
 * that requires a single ternary indirect bus, but no ternary indirect table.  This
 * appears as an "indirect_bus" node in the assembly.
 *
 * This is slightly different than no-match-miss tables, as no-match-miss tables output
 * a TernaryIndirect with no RAMs, but a format, (though this format is never actually used)
 * as for any Assembler Calls, the key in the format are necessary.  However at this point
 * a TernaryIndirect Table with no format does not assemble.  These in theory are the
 * same table result, but have to be output in different ways in order to pass the assembler
 */
bool Memories::allocate_all_tind_result_bus_tables() {
    Log::TempIndent indent;
    if (!tind_result_bus_tables.empty()) LOG3("Allocate all tind result bus tables" << indent);
    for (auto *ta : tind_result_bus_tables) {
        for (auto u_id : ta->allocation_units()) {
            auto &alloc = (*ta->memuse).at(u_id);
            BUG_CHECK(alloc.type == Use::TERNARY, "Tind result bus not on a ternary table?");
            bool found = false;
            for (int i = 0; i < SRAM_ROWS; i++) {
                for (int j = 0; j < BUS_COUNT && j < PAYLOAD_COUNT; j++) {
                    if (payload_use[i][j]) continue;
                    if (tind_bus[i][j]) continue;
                    // Add a tind result bus node
                    alloc.tind_result_bus = i * BUS_COUNT + j;
                    tind_bus[i][j] = u_id.build_name();
                    payload_use[i][j] = u_id.build_name();
                    found = true;
                    break;
                }
                if (found) break;
            }

            if (!found) {
                failure_reason = "failed to place tind result bus "_cs + u_id.build_name();
                LOG4(failure_reason);
                return false;
            }
        }
    }
    return true;
}

bool Memories::find_mem_and_bus_for_idletime(
    std::vector<std::pair<int, std::vector<int>>> &mem_locs, int &bus, int total_mem_required,
    bool top_half) {
    // find mapram locs
    int total_requested = 0;

    int mem_start_row = (top_half) ? SRAM_ROWS / 2 : 0;
    int mem_end_row = (top_half) ? SRAM_ROWS : SRAM_ROWS / 2;

    mem_locs.clear();

    for (int i = mem_start_row; i < mem_end_row; i++) {
        if (total_requested == total_mem_required) break;

        std::vector<int> cols;
        for (int j = 0; j < MAPRAM_COLUMNS; j++) {
            if (!mapram_use[i][j]) {
                cols.push_back(j);

                total_requested++;
                if (total_requested == total_mem_required) break;
            }
        }
        mem_locs.emplace_back(i, cols);
    }

    const char *which_half = top_half ? "top" : "bottom";

    // find a bus
    bool found_bus = false;
    for (int i = 0; i < NUM_IDLETIME_BUS; i++) {
        if (!idletime_bus[(unsigned)top_half][i]) {
            bus = i;
            found_bus = true;
            break;
        }
    }

    if (!found_bus) {
        LOG3("Ran out of idletime bus in " << which_half << "half");
        return false;
    }

    return true;
}

bool Memories::allocate_idletime_in_top_or_bottom_half(SRAM_group *idletime_group,
                                                       bool in_top_half) {
    auto unique_id = idletime_group->build_unique_id();

    int total_required = idletime_group->left_to_place();

    std::vector<std::pair<int, std::vector<int>>> mem_locs;
    int bus = -1;

    // find mem and bus in top and bottom half of mapram
    bool resource_available =
        find_mem_and_bus_for_idletime(mem_locs, bus, total_required, in_top_half);

    if (!resource_available) return false;

    // update memuse and bus use
    auto &alloc = (*idletime_group->ta->memuse)[unique_id];

    for (auto &loc : mem_locs) {
        Memories::Use::Row row(loc.first, bus);
        for (auto col : loc.second) {
            mapram_use[loc.first][col] = unique_id.build_name();
            mapram_inuse[loc.first] |= (1 << col);
            row.col.push_back(col);  // TODO use col as bfas expects "column" for idletime
            idletime_group->placed++;
        }
        alloc.row.push_back(row);
    }

    idletime_bus[(unsigned)in_top_half][bus] = unique_id.build_name();
    return idletime_group->all_placed();
}

bool Memories::allocate_idletime(SRAM_group *idletime_group) {
    // Pick the most available section of the map RAM array to try to fit in an idletime
    // resource allocation
    int top_half_maprams = open_maprams_between_rows(SRAM_ROWS - 1, MATCH_CENTRAL_ROW);
    int bottom_half_maprams = open_maprams_between_rows(MATCH_CENTRAL_ROW - 1, 0);

    bool in_top_half = top_half_maprams >= bottom_half_maprams;
    if (allocate_idletime_in_top_or_bottom_half(idletime_group, in_top_half)) return true;
    if (allocate_idletime_in_top_or_bottom_half(idletime_group, !in_top_half)) return true;
    failure_reason = "allocate_idletime failed"_cs;
    return false;
}

bool Memories::allocate_all_idletime() {
    idletime_groups.clear();
    Log::TempIndent indent;
    if (!idletime_tables.empty()) LOG3("Allocating all idletime tables" << indent);
    for (auto *ta : idletime_tables) {
        for (auto back_at : ta->table->attached) {
            auto at = back_at->attached;
            const IR::MAU::IdleTime *id = nullptr;
            if ((id = at->to<IR::MAU::IdleTime>()) == nullptr) continue;
            int lt_entry = 0;
            for (auto u_id : ta->allocation_units(id)) {
                int per_row = IdleTimePerWord(id);
                int depth =
                    mems_needed(ta->calc_entries_per_uid[lt_entry], SRAM_DEPTH, per_row, false);
                auto idletime_group = new SRAM_group(ta, depth, 1, SRAM_group::IDLETIME);
                idletime_group->attached = id;
                idletime_group->logical_table = u_id.logical_table;
                idletime_groups.push_back(idletime_group);
                lt_entry++;
            }
        }
    }

    std::stable_sort(idletime_groups.begin(), idletime_groups.end(),
                     [](const SRAM_group *a, const SRAM_group *b) {
                         return a->left_to_place() > b->left_to_place();
                     });

    for (auto *idletime_group : idletime_groups) {
        if (!allocate_idletime(idletime_group)) {
            LOG4("failed to allocation idletime group " << idletime_group->ta->table->name);
            return false;
        }
    }

    return true;
}

void Memories::visitUse(const Use &alloc, std::function<void(cstring &, update_type_t)> fn) {
    BFN::Alloc2Dbase<cstring> *use = 0, *mapuse = 0, *bus = 0, *result_bus = 0, *gw_use = 0;
    unsigned *inuse = 0, *map_inuse = 0;
    update_type_t bus_type = NONE;
    switch (alloc.type) {
        case Use::EXACT:
        case Use::ATCAM:
            use = &sram_use;
            inuse = sram_inuse;
            bus = &sram_print_search_bus;
            bus_type = UPDATE_SEARCH_BUS;
            result_bus = &sram_print_result_bus;
            break;
        case Use::TERNARY:
            use = &tcam_use;
            break;
        case Use::GATEWAY:
            gw_use = &gateway_use;
            bus = &sram_print_search_bus;
            bus_type = UPDATE_SEARCH_BUS;
            BUG_CHECK(alloc.row.size() == 1, "multiple rows for gateway");
            break;
        case Use::TIND:
            use = &sram_use;
            inuse = sram_inuse;
            bus = &tind_bus;
            bus_type = UPDATE_TIND_BUS;
            break;
        case Use::COUNTER:
        case Use::METER:
        case Use::STATEFUL:
        case Use::SELECTOR:
            use = &sram_use;
            inuse = sram_inuse;
            mapuse = &mapram_use;
            map_inuse = mapram_inuse;
            break;
        case Use::ACTIONDATA:
            use = &sram_use;
            inuse = sram_inuse;
            break;
        case Use::IDLETIME:
            use = &mapram_use;
            inuse = mapram_inuse;
            break;
        default:
            BUG("Unhandled memory use type %d in visit", alloc.type);
    }
    for (auto &r : alloc.row) {
        if (bus && r.bus != -1) {
            fn((*bus)[r.row][r.bus], bus_type);
        }
        if (result_bus && r.result_bus != -1) {
            fn((*result_bus)[r.row][r.result_bus], UPDATE_RESULT_BUS);
        }
        if (alloc.is_twoport()) fn(twoport_bus[r.row], UPDATE_STATEFUL_BUS);
        if (alloc.type == Use::ACTIONDATA) {
            BUG_CHECK(r.bus == ACTION || r.bus == OFLOW, "invalid bus %d for actiondata", r.bus);
            int side = r.col.front() >= 6;
            if (r.bus == ACTION)
                fn(action_data_bus[r.row][side], UPDATE_ACTION_BUS);
            else
                fn(overflow_bus[r.row][side], UPDATE_ACTION_BUS);
        }
        if (use) {
            for (auto col : r.col) {
                fn((*use)[r.row][col], UPDATE_RAM);
                if (inuse) {
                    if ((*use)[r.row][col])
                        inuse[r.row] |= 1 << col;
                    else
                        inuse[r.row] &= ~(1 << col);
                }
            }
        }
        if (mapuse) {
            for (auto col : r.mapcol) {
                fn((*mapuse)[r.row][col], UPDATE_MAPRAM);
                if ((*mapuse)[r.row][col])
                    map_inuse[r.row] |= 1 << col;
                else
                    map_inuse[r.row] &= ~(1 << col);
            }
        }
        if (gw_use) {
            fn((*gw_use)[r.row][alloc.gateway.unit], UPDATE_GATEWAY);
            if (alloc.gateway.payload_row >= 0)
                fn(payload_use[alloc.gateway.payload_row][alloc.gateway.payload_unit],
                   UPDATE_PAYLOAD);
            if (r.result_bus >= 0) {
                if (alloc.gateway.bus_type == Use::EXACT)
                    fn(sram_print_result_bus[alloc.gateway.payload_row][r.result_bus],
                       UPDATE_RESULT_BUS);
                else if (alloc.gateway.bus_type == Use::TERNARY)
                    fn(tind_bus[alloc.gateway.payload_row][r.result_bus], UPDATE_RESULT_BUS);
                else
                    BUG("invalid bud type %d for payload", alloc.gateway.bus_type);
            }
        }
    }
    if (mapuse) {
        for (auto &r : alloc.color_mapram) {
            for (auto col : r.col) {
                fn((*mapuse)[r.row][col], UPDATE_MAPRAM);
                if ((*mapuse)[r.row][col])
                    map_inuse[r.row] |= 1 << col;
                else
                    map_inuse[r.row] &= ~(1 << col);
            }
        }
    }
}

void Memories::update(cstring name, const Use &alloc) {
    visitUse(alloc, [name](cstring &use, update_type_t u_type) {
        if (use && use != name) {
            bool collision = true;
            // A table may have different search buses, but the same result bus.  Say a table
            // required two hash functions (i.e. 5 way table).  The table would require two
            // search / address buses, but only one result bus.  Thus the name repeat is fine
            // for result_buses
            if (u_type == UPDATE_RESULT_BUS && name == use) collision = false;

            if (collision) BUG_CHECK("conflicting memory use between %s and %s", use, name);
        }
        use = name;
    });
}

void Memories::update(const std::map<UniqueId, Use> &alloc) {
    for (auto &a : alloc) update(a.first.build_name(), a.second);
}

void Memories::remove(cstring name, const Use &alloc) {
    visitUse(alloc, [name](cstring &use, update_type_t) {
        if (use != name) BUG("Undo failure for %s", name);
        use = nullptr;
    });
}
void Memories::remove(const std::map<UniqueId, Use> &alloc) {
    for (auto &a : alloc) remove(a.first.build_name(), a.second);
}

std::ostream &operator<<(std::ostream &out, const Memories::search_bus_info &sbi) {
    out << "search bus " << sbi.name << " width: " << sbi.width_section
        << " hash_group: " << sbi.hash_group;
    return out;
}

std::ostream &operator<<(std::ostream &out, const Memories::result_bus_info &rbi) {
    out << "result bus " << rbi.name << " width: " << rbi.width_section
        << " hash_group: " << rbi.logical_table;
    return out;
}

/* MemoriesPrinter in .gdbinit should match this */
void Memories::printOn(std::ostream &out) const {
    const BFN::Alloc2Dbase<cstring> *arrays[] = {&tcam_use,
                                                 &sram_print_search_bus,
                                                 &sram_print_result_bus,
                                                 &tind_bus,
                                                 &action_data_bus,
                                                 &stash_use,
                                                 &sram_use,
                                                 &mapram_use,
                                                 &overflow_bus,
                                                 &gateway_use,
                                                 &payload_use};
    std::map<cstring, char> tables;
    for (auto arr : arrays)
        for (int r = 0; r < arr->rows(); r++)
            for (int c = 0; c < arr->cols(); c++)
                if (arr->at(r, c)) tables[arr->at(r, c)] = '?';
    char ch = 'A' - 1;
    for (auto &t : tables) {
        t.second = ++ch;
        if (ch == 'Z')
            ch = 'a' - 1;
        else if (ch == 'z')
            ch = '0' - 1;
    }
    out << "tc  sb  rb  tib ab  st  srams       mapram  ov  gw pay 2p" << Log::endl;
    for (int r = 0; r < TCAM_ROWS; r++) {
        for (auto arr : arrays) {
            for (int c = 0; c < arr->cols(); c++) {
                if (r >= arr->rows()) {
                    out << ' ';
                } else {
                    if (auto tbl = arr->at(r, c)) {
                        out << tables.at(tbl);
                    } else {
                        out << '.';
                    }
                }
            }
            out << "  ";
        }
        if (r < Memories::SRAM_ROWS) {
            if (auto tbl = twoport_bus[r]) {
                out << tables.at(tbl);
            } else {
                out << '.';
            }
        }
        out << Log::endl;
    }
    for (auto &tbl : tables) out << "   " << tbl.second << " " << tbl.first << Log::endl;
}

void dump(const Memories *mem) { mem->printOn(std::cout); }

std::ostream &operator<<(std::ostream &out, const Memories::table_alloc &ta) {
    out << "table_alloc[" << ta.table->match_table->externalName() << ": ";
    for (auto &u : *ta.memuse)
        out << "(" << u.first.build_name() << ", " << use_type_to_str[u.second.type] << ") ";
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const safe_vector<Memories::table_alloc *> &v) {
    const char *sep = "";
    for (auto *ta : v) {
        out << sep << ta->table->name;
        sep = ", ";
    }
    return out;
}

}  // end namespace Tofino
