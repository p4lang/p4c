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

#include "bf-p4c/mau/table_layout.h"

#include <math.h>

#include <set>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/mau/input_xbar.h"
#include "bf-p4c/mau/memories.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/mau/table_format.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/bitops.h"
#include "lib/log.h"

class DoTableLayout : public MauModifier, Backtrack {
    // In order to know how many field sections can be packed together into the same byte
    struct MatchByteKey {
        cstring name;
        int lo;
        int ixbar_multiplier;
        int match_multiplier;

        MatchByteKey(cstring n, int l, int im, int mm)
            : name(n), lo(l), ixbar_multiplier(im), match_multiplier(mm) {}

        bool operator<(const MatchByteKey &mbk) const {
            if (name != mbk.name) return name < mbk.name;
            if (lo != mbk.lo) return lo < mbk.lo;
            if (ixbar_multiplier < mbk.ixbar_multiplier)
                return ixbar_multiplier < mbk.ixbar_multiplier;
            if (match_multiplier < mbk.match_multiplier)
                return match_multiplier < mbk.match_multiplier;
            return false;
        }
    };

    PhvInfo &phv;
    LayoutChoices &lc;
    SplitAttachedInfo &att_info;
    bool alloc_done = false;
    int get_hit_actions(const IR::MAU::Table *tbl);
    profile_t init_apply(const IR::Node *root) override;
    bool backtrack(trigger &trig) override;
    bool preorder(IR::MAU::Table *tbl) override;
    bool preorder(IR::MAU::Action *act) override;
    bool preorder(IR::MAU::TableKey *read) override;
    bool preorder(IR::MAU::Selector *sel) override;
    friend class LayoutChoices;

    static bool can_be_hash_action(const IR::MAU::Table *tbl, std::string &reason);
    void check_atcam_parameters(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl,
                                const bool partition_found, const cstring &index_name);
    void check_for_alpm(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl,
                        cstring &partition_index);
    void check_for_proxy_hash(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl);
    bool check_for_versioning(const IR::MAU::Table *tbl);
    void determine_byte_impacts(const IR::MAU::Table *tbl, IR::MAU::Table::Layout &layout,
                                std::map<MatchByteKey, safe_vector<le_bitrange>> &byte_impacts,
                                bool &partition_found, cstring partition_index);
    void setup_action_layout(IR::MAU::Table *tbl);
    void setup_gateway_layout(IR::MAU::Table::Layout &, const IR::MAU::Table *);
    void setup_instr_and_next(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl);
    void setup_match_layout(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl);

 public:
    explicit DoTableLayout(PhvInfo &p, LayoutChoices &l, SplitAttachedInfo &sai)
        : phv(p), lc(l), att_info(sai) {}
};

Visitor::profile_t DoTableLayout::init_apply(const IR::Node *root) {
    alloc_done = phv.alloc_done();
    return MauModifier::init_apply(root);
}

LayoutChoices *LayoutChoices::create(PhvInfo &p, const ReductionOrInfo &ri, SplitAttachedInfo &a) {
    return new LayoutChoices(p, ri, a);
}

void LayoutChoices::add_payload_gw_layout(const IR::MAU::Table *tbl,
                                          const LayoutOption &base_option) {
    LayoutOption lo;
    lo.layout.gateway = lo.layout.gateway_match = true;
    lo.layout.hash_action = lo.layout.ternary = false;
    lo.layout.action_data_bytes = base_option.layout.action_data_bytes;
    lo.layout.action_data_bytes_in_table = base_option.layout.action_data_bytes_in_table;
    lo.layout.meter_addr = base_option.layout.meter_addr;
    lo.layout.stats_addr = base_option.layout.stats_addr;
    lo.layout.action_addr = base_option.layout.action_addr;
    lo.layout.immediate_bits = base_option.layout.immediate_bits;
    lo.action_format_index = base_option.action_format_index;
    lo.way.match_groups = tbl->entries_list->entries.size();
    BUG_CHECK(tbl->entries_list->entries.size(), "entry size of a table should not be 0");
    lo.way.width = 1;
    lo.layout.overhead_bits = base_option.layout.overhead_bits * lo.way.match_groups;
    auto ft = ActionData::FormatType_t::default_for_table(tbl);
    cache_layout_options[std::make_pair(tbl->name, ft)].push_back(lo);
    LOG5("Caching layout option: [" << tbl->name << ", " << ft << "] = " << lo);
}

/** Check to see if a table needs a meter format for a stage with the given format_type
 */
bool LayoutChoices::need_meter(const IR::MAU::Table *tbl,
                               ActionData::FormatType_t format_type) const {
    unsigned idx = 0;
    for (auto *ba : tbl->attached) {
        bool uses_meter = ba->attached->is<IR::MAU::MeterBus2Port>();
        if (ba->attached->direct) {
            if (uses_meter && format_type.matchThisStage()) return true;
        } else {
            if (uses_meter && format_type.attachedThisStage(idx)) return true;
        }
        if (format_type.track(ba->attached)) ++idx;
    }
    return false;
}

bool DoTableLayout::backtrack(trigger &trig) { return trig.is<IXBar::failure>() && !alloc_done; }

/** Algorithmic TCAM is a third type of table, in the same vein as ternary and exact.  An
 *  ATCAM table is a ternary table that Tofino can implement using exact hardware, specifically
 *  the SRAM array.
 *
 *  A ternary match requires two things that a normal exact match cannot.  A ternary match
 *  must be able to specify a don't care for any bit, and must have a priority system in order
 *  to rank the matches.  By using particular features of the SRAM array, the Tofino target
 *  can create these features.
 *
 *  The don't care bit is possible due by containing any ternary match data twice in the RAM
 *  line.  The match data is encoded to match as 0, 1, or don't care.  This is briefly
 *  described in section 6.4.2.2 Algorithmic TCAM section of the Tofino uArch, while the
 *  encoding scheme for ternary match in general is described in section 6.3.1 TCAM Data
 *  Representation.  The terms s0q1 and s1q0 describe these different encodings.
 *
 *  Priority is engineered through the placement of RAMs in a row.  If the RAMs are in the same
 *  row, and belong to the same ATCAM table, then if multiple tables match, as is possible
 *  within a single TCAM table, the one closer to the edge of the chip will have higher
 *  priority and will be the match.  If the algorithm cannot fit all of the entries within
 *  a single row, then the table will be split into multiple logical tables, and will use
 *  hit/miss predication of these logical tables in order to generate priority across tables.
 *  This is described by section 1.4.5 Algorithmic TCAM Overview in the uArch.
 *
 *  An ATCAM table requires a partition index.  This is an field in the key that must be
 *  an exact match.  This partition index is used as an identity hash to find the correct
 *  partition within the massive ATCAM table.
 *
 *  The user can also specify the number of partitions. In a standard TCAM, the size of the
 *  table specifies how many entries are looked up (logically) simultaneously.  In an ATCAM
 *  table, the number of entries that are looked up simultaneously is:
 *      table_size / number_of_partitions
 *
 *  If no number of partitions is specified, then the number of partitions is:
 *      2 ^ (partition index bits)
 */
void TableLayout::check_for_atcam(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl,
                                  cstring &partition_index, const PhvInfo &phv) {
    bool index_found = false;
    bool partitions_found = false;
    int partition_count = -1;

    for (auto key : tbl->match_key) {
        if (key->partition_index) {
            auto *partition_index_field = phv.field(key->expr);
            CHECK_NULL(partition_index_field);
            partition_index = partition_index_field->name;
            index_found = true;
        }
    }

    if (tbl->getAnnotation("atcam_number_partitions"_cs, partition_count)) {
        ERROR_CHECK(partition_count > 0, ErrorType::ERR_EXPECTED,
                    "the number of partitions specified for table %1% to be greater "
                    "than 0.",
                    tbl);
        partitions_found = true;
    }

    if (partitions_found) {
        // Check if atcam_partition_index pragma is applied
        auto pi = tbl->getExprAnnotation("atcam_partition_index"_cs);
        ERROR_CHECK(pi, ErrorType::WARN_MISSING,
                    "%1%: atcam parition index is not present for table %2%. "
                    "Ensure that an atcam_partition_index is added for a match key.",
                    tbl, tbl->externalName());

        // Check if atcam_partition_index name is consistent with the match key
        ERROR_CHECK(index_found, ErrorType::WARN_MISSING,
                    "%1%: Number of partitions are specified for table %2% but "
                    "the partition index %3% is not found consistent with the exact match key. "
                    "Ensure that the atcam_partition_index and exact match key name match, "
                    "or add a @name annotation to the match key expression consistent with "
                    "the partition index.",
                    tbl, tbl->externalName(), pi);
    }

#if 0
    if (index_found) {
        if (tbl->gress == INGRESS)
            partition_index = "ingress::" + partition_index;
        else
            partition_index = "egress::" + partition_index;
        ERROR_CHECK(phv.field(partition_index) != nullptr, ErrorType::ERR_NOT_FOUND,
                    "partition index %2% for table %1% in the PHV.",
                    tbl, partition_index);
    }
#endif

    layout.atcam = (index_found);
    if (partitions_found) layout.partition_count = partition_count;
}

void TableLayout::check_for_alpm(IR::MAU::Table::Layout &, const IR::MAU::Table *tbl,
                                 cstring &partition_index, const PhvInfo &phv) {
    for (auto ixbar_read : tbl->match_key) {
        if (ixbar_read->for_atcam_partition_index()) {
            if (auto var = ixbar_read->expr->to<IR::TempVar>()) {
                partition_index = var->name;
            } else {
                BUG("Unhandled type for alpm partition_index %1%", ixbar_read);
            }
        }
    }
    ERROR_CHECK(phv.field(partition_index) != nullptr, ErrorType::ERR_NOT_FOUND,
                "partition index %2% for table %1% in the PHV.", tbl, partition_index);
}

void TableLayout::check_for_ternary(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl) {
    auto annot = tbl->match_table->getAnnotations();
    if (auto s = annot->getSingle("ternary"_cs)) {
        if (s->expr.size() <= 0) {
            warning(BFN::ErrorType::WARN_PRAGMA_USE,
                    "Pragma ternary ignored for table %1% because value is undefined", tbl);
        } else {
            auto *pragma_val = s->expr.at(0)->to<IR::Constant>();
            ERROR_CHECK(pragma_val != nullptr, ErrorType::ERR_UNKNOWN,
                        "unknown ternary pragma %1% on table %2%.", s, tbl->externalName());
            if (pragma_val->asInt() == 1)
                layout.ternary = true;
            else
                warning(BFN::ErrorType::WARN_PRAGMA_USE,
                        "Pragma ternary ignored for table %1% because value is not 1", tbl);
        }
    } else {
        for (auto ixbar_read : tbl->match_key) {
            if (ixbar_read->match_type.name == "ternary" || ixbar_read->match_type.name == "lpm") {
                layout.ternary = true;
                break;
            }
        }
        for (auto ixbar_read : tbl->match_key) {
            if (ixbar_read->match_type == "range") {
                layout.ternary = true;
                layout.has_range = true;
                break;
            }
        }
    }
}

/**
 * Determines whether a table is set to a proxy hash table, based on the proxy_hash_width
 * function.
 */
void DoTableLayout::check_for_proxy_hash(IR::MAU::Table::Layout &layout,
                                         const IR::MAU::Table *tbl) {
    auto annot = tbl->match_table->getAnnotations();
    if (auto s = annot->getSingle("proxy_hash_width"_cs)) {
        if (s->expr.size() <= 0) {
            warning(BFN::ErrorType::WARN_PRAGMA_USE,
                    "Proxy hash pragma ignored for table %1% because value is undefined.", tbl);
        } else {
            auto *pragma_val = s->expr.at(0)->to<IR::Constant>();
            ERROR_CHECK(pragma_val != nullptr, ErrorType::ERR_INVALID,
                        "%1%: proxy hash pragma on table %2%. It is not a constant.", s,
                        tbl->externalName());
            ERROR_CHECK(pragma_val->asInt() > 0 &&
                            pragma_val->asInt() <= Device::ixbarSpec().hashMatrixSize(),
                        ErrorType::ERR_INVALID,
                        "proxy hash width %2% on table %1%. It does not fit on the "
                        "hash matrix.",
                        tbl, pragma_val->asInt());
            layout.proxy_hash = true;
            layout.proxy_hash_width = pragma_val->asInt();
        }
    }
}

bool DoTableLayout::check_for_versioning(const IR::MAU::Table *tbl) {
    if (!tbl->match_table) return false;
    bool rv = true;
    auto prop = tbl->match_table->properties->getProperty("requires_versioning"_cs);
    if (prop != nullptr) {
        if (auto ev = prop->value->to<IR::ExpressionValue>()) {
            auto bl = ev->expression->to<IR::BoolLiteral>();
            if (bl) {
                rv = bl->value;
            } else {
                error(
                    "%1% requires_versioning property on table %2% is only allowed to be "
                    "a boolean variable",
                    prop, tbl->name);
            }
        }
    }

    auto annot = tbl->match_table->getAnnotations();
    if (auto s = annot->getSingle("no_versioning"_cs)) {
        if (s->expr.size() > 0) {
            auto pragma_val = s->expr.at(0)->to<IR::Constant>();
            if (pragma_val && (pragma_val->asInt() == 1 || pragma_val->asInt() == 0)) {
                rv = pragma_val->asInt() == 0;
            } else {
                error(
                    "%1% the no_versioning pragma on table %2% is only allowed a 1 or 0 "
                    "option",
                    s, tbl->name);
            }
        }
    }
    return rv;
}

/** Because the input xbar allocates by container bytes, the estimate should also be
 *  based on container bytes rather than field bytes.  The point of this byte_impact
 *  maps is to build container bytes and run analysis on that.
 */
void DoTableLayout::determine_byte_impacts(
    const IR::MAU::Table *tbl, IR::MAU::Table::Layout &layout,
    std::map<MatchByteKey, safe_vector<le_bitrange>> &byte_impacts, bool &partition_found,
    cstring partition_index) {
    for (auto ixbar_read : tbl->match_key) {
        if (!ixbar_read->for_match()) continue;
        le_bitrange bits = {0, 0};
        auto *field = phv.field(ixbar_read->expr, &bits);
        int match_multiplier = layout.proxy_hash ? 0 : 1;
        int ixbar_multiplier = 1;

        int bytes = 0;
        if (field) {
            /* FIXME -- if a field is allocated to non-contiguous bits of a byte,
             * this will count that byte twice, when it is only needed once.  The
             * match layout in asm_output will likewise lay it out twice, so this
             * is consistent.  Should fix PHV alloc to not make such bad allocations */
            bool is_partition = false;
            if (layout.atcam) {
                auto *partition_index_field = phv.field(partition_index);
                if (partition_index_field && partition_index_field->name == field->name) {
                    match_multiplier = 0;
                    is_partition = true;
                    partition_found = true;
                    /* key should be exact type to support @atcam_partition_index annotation,
                     * or atcam_partition_index type */
                    ERROR_CHECK(ixbar_read->match_type.name == "atcam_partition_index" ||
                                    ixbar_read->match_type.name == "exact",
                                ErrorType::ERR_INVALID,
                                "partition index of algorithmic TCAM table %1%. Must"
                                " be an atcam_partition_index field or an exact field.",
                                tbl);
                } else if (ixbar_read->match_type.name == "ternary" ||
                           ixbar_read->match_type.name == "lpm") {
                    match_multiplier = 2;
                }
            }

            if (ixbar_read->match_type.name == "range") {
                ixbar_multiplier = 2;
                match_multiplier = 2;
            }

            // FIXME: This will currently not work before PHV allocation, because the
            // foreach_byte over alloc_slices only works if the alloc_slice has been allocated
            // If we move PHV allocation back to after Table Placement, this will need to
            // change
            PHV::FieldUse use(PHV::FieldUse::READ);
            field->foreach_byte(bits, tbl, &use, [&](const PHV::AllocSlice &sl) {
                cstring name = sl.container().toString();
                int lo = (sl.container_slice().lo / 8) * 8;
                MatchByteKey mbk(name, lo, ixbar_multiplier, match_multiplier);
                byte_impacts[mbk].push_back(sl.container_slice());
                bytes++;
            });

            if (bytes == 0) {
                // We hit this all the time for tempvars not yet allocated.  Should be ok as
                // we'll backtrack and redo TableLayout after allocating it later.
                LOG1("Field " << field->name
                              << " not allocated in PHV, byte_impacts will "
                                 "not be correct");
            }

            if (is_partition) {
                layout.partition_bits = bits.size();
                // If partition count is set and requires less bits than
                // partition index, set partition bits to lesser value. Rams are
                // determined based on this value and below check will ensure
                // select mask generated in ways is correct
                if (layout.partition_count > 0)
                    layout.partition_bits =
                        std::min(layout.partition_bits, ceil_log2(layout.partition_count));
            }
        } else {
            BUG("unexpected reads expression %s", ixbar_read->expr);
        }
    }
}

void DoTableLayout::check_atcam_parameters(IR::MAU::Table::Layout &layout,
                                           const IR::MAU::Table *tbl, const bool partition_found,
                                           const cstring &index_name) {
    ERROR_CHECK(partition_found, ErrorType::ERR_INVALID,
                "partition index %2%. Table %1% is specified to be an atcam, but partition "
                "index %2% is not found within the table key.",
                tbl, index_name);
    if (partition_found) {
        const int possible_partitions = 1 << layout.partition_bits;
        ERROR_CHECK(layout.partition_count <= possible_partitions, ErrorType::ERR_INSUFFICIENT,
                    "the number of partitions to be %3% due to the number of bits specified "
                    "in the partition. However table %1% has specified %2% partitions.",
                    tbl, layout.partition_count, possible_partitions);
        if (layout.partition_count == 0) {
            layout.partition_count = possible_partitions;
        }

        const auto index =
            std::find_if(tbl->match_key.begin(), tbl->match_key.end(),
                         [](const IR::MAU::TableKey *key) { return key->partition_index; });
        BUG_CHECK(index != tbl->match_key.end(),
                  "found partition but no key set as partition_index");

        // Maximum possible partitions ignoring partition_count set in table definition
        const int max_partitions = 1 << (*index)->expr->type->width_bits();

        // Check if specified number of partitions may lead to suboptimal resource allocation
        const int optimal_partitions = std::min(max_partitions, layout.get_sram_depth());
        WARN_CHECK(layout.partition_count >= optimal_partitions,
                   BFN::ErrorType::WARN_TABLE_PLACEMENT,
                   "specified number of partitions %2% for table %1% is less than the optimal "
                   "value of %3%. This may lead to suboptimal resource allocation.",
                   tbl, layout.partition_count, optimal_partitions);
    }
}

void DoTableLayout::setup_match_layout(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl) {
    if (tbl->layout.pre_classifier)
        layout.entries = tbl->layout.pre_classifer_number_entries;
    else if (auto k = tbl->match_table->getConstantProperty("size"_cs))
        layout.entries = k->asInt();
    else if (auto k = tbl->match_table->getConstantProperty("min_size"_cs))
        layout.entries = k->asInt();
    layout.match_width_bits = 0;
    if (tbl->match_key.empty()) return;
    layout.requires_versioning = check_for_versioning(tbl);

    cstring partition_index;
    if (layout.alpm) TableLayout::check_for_alpm(layout, tbl, partition_index, phv);
    if (!layout.atcam) TableLayout::check_for_atcam(layout, tbl, partition_index, phv);
    if (!layout.atcam) TableLayout::check_for_ternary(layout, tbl);
    check_for_proxy_hash(layout, tbl);

    if (!layout.alpm && !layout.atcam && !layout.ternary && !layout.proxy_hash) layout.exact = true;

    if (!layout.exact && tbl->match_table) {
        auto annot = tbl->match_table->getAnnotations();
        auto s = annot->getSingle("dynamic_table_key_masks"_cs);
        ERROR_CHECK(!s || s->expr.size() <= 0, ErrorType::ERR_INVALID,
                    "Table %1%. @dynamic_table_key_masks annotation only permissible with "
                    "exact matches",
                    tbl->externalName());
    }

    if (layout.proxy_hash) {
        ERROR_CHECK(!layout.atcam && !layout.ternary, ErrorType::ERR_INVALID,
                    "proxy hash table for table %1%. Cannot be ternary.", tbl);
    }

    if (!layout.requires_versioning && !layout.ternary)
        error(
            "%1%: Tables, such as %2% that do not require versioning must be allocated to "
            "the TCAM array",
            tbl, tbl->name);

    safe_vector<int> byte_sizes;
    bool partition_found = false;

    std::map<MatchByteKey, safe_vector<le_bitrange>> byte_impacts;
    determine_byte_impacts(tbl, layout, byte_impacts, partition_found, partition_index);

    // Code responsible for determining the ixbar_width_bits and match_width_bits
    for (auto &entry : byte_impacts) {
        safe_vector<bitvec> individual_bytes;
        auto &mbk = entry.first;

        for (auto range : entry.second) {
            bitvec entry_range(range.lo, range.size());
            auto it = individual_bytes.begin();
            while (it != individual_bytes.end()) {
                if ((*it & entry_range).empty()) break;
                it++;
            }
            if (it == individual_bytes.end())
                individual_bytes.push_back(entry_range);
            else
                *it |= entry_range;
        }

        for (auto &bv : individual_bytes) {
            layout.ixbar_bytes += mbk.ixbar_multiplier;
            layout.match_bytes += mbk.match_multiplier;
            layout.ixbar_width_bits += bv.popcount() * mbk.ixbar_multiplier;
            layout.match_width_bits += bv.popcount() * mbk.match_multiplier;
            byte_sizes.push_back(bv.popcount());
        }
    }
    LOG4("IXBar Bytes : " << layout.ixbar_bytes << " Match Bytes : " << layout.match_bytes
                          << " IXBar Width Bits : " << layout.ixbar_width_bits
                          << " Match Width Bits : " << layout.match_width_bits);

    if (layout.atcam) {
        check_atcam_parameters(layout, tbl, partition_found, partition_index);
    }

    if (!layout.ternary && !layout.atcam && !layout.proxy_hash) {
        int ghost_bits_left = layout.get_ram_ghost_bits();
        std::sort(byte_sizes.begin(), byte_sizes.end());
        for (auto byte_size : byte_sizes) {
            if (ghost_bits_left >= byte_size) {
                ghost_bits_left -= byte_size;
                layout.ghost_bytes++;
                layout.match_bytes--;
            }
        }
        layout.match_width_bits -= layout.get_ram_ghost_bits();
    } else if (layout.proxy_hash) {
        layout.match_width_bits = layout.proxy_hash_width;
    }
    LOG3("Setting up match layout : " << layout);
}

class GatewayLayout : public MauInspector {
    IR::MAU::Table::Layout &layout;
    std::set<cstring> added;
    bool preorder(const IR::Member *f) {
        cstring name = f->toString();
        if (!added.count(name)) {
            added.insert(name);
            layout.ixbar_bytes += (f->type->width_bits() + 7) / 8;
        }
        return false;
    }

 public:
    explicit GatewayLayout(IR::MAU::Table::Layout &l) : layout(l) {}
};

void DoTableLayout::setup_gateway_layout(IR::MAU::Table::Layout &layout,
                                         const IR::MAU::Table *tbl) {
    for (auto &gw : tbl->gateway_rows)
        if (gw.first) gw.first->apply(GatewayLayout(layout));
    // should count gw tcam width and depth to support gw splitting when needed
}

/** Initializes the list of action formats that are possible for the table, with different
 *  layouts in both action data tables as well as immediate
 */
void DoTableLayout::setup_action_layout(IR::MAU::Table *tbl) {
    tbl->layout.action_data_bytes = 0;
    IR::MAU::Table::ImmediateControl_t imm_ctrl = tbl->get_immediate_ctrl();

    if (imm_ctrl == IR::MAU::Table::FORCE_IMMEDIATE) {
        bool immediate_allowed = true;
        // Action Profiles cannot have any immediate data
        if (tbl->layout.action_addr.address_bits != 0) immediate_allowed = false;
        // chained salus need the immediate path for the address, so can't use it for data
        for (auto att : tbl->attached)
            if (auto salu = att->attached->to<IR::MAU::StatefulAlu>())
                if (salu->chain_vpn) immediate_allowed = false;

        if (!immediate_allowed) {
            fatal_error(
                "%1%: Cannot use force_immediate on table %2% when its action data "
                "table is indirectly addressed nor when the table needs to use the "
                "immediate path to write items like hash, meter color, or random "
                "number results.",
                tbl, tbl->externalName());
        }
    }
    auto af = lc.get_action_formats(tbl);
    if (af.size() > 0)
        tbl->layout.action_data_bytes = af[0].bytes_per_loc[ActionData::ACTION_DATA_TABLE] +
                                        af[0].bytes_per_loc[ActionData::IMMEDIATE];
}

void LayoutChoices::compute_action_formats(const IR::MAU::Table *tbl,
                                           ActionData::FormatType_t format_type) {
    LOG5("Computing action formats for table " << tbl->name);
    BUG_CHECK(format_type.valid(), "invalid format type in LayoutChoices::compute_action_formats");
    ActionData::Format af(phv, tbl, red_info, att_info);
    af.set_uses(&cache_action_formats[std::make_pair(tbl->name, format_type)]);

    IR::MAU::Table::ImmediateControl_t imm_ctrl = tbl->get_immediate_ctrl();
    af.allocate_format(imm_ctrl, format_type);
}

void LayoutChoices::setup_ternary_layout(const IR::MAU::Table *tbl,
                                         const IR::MAU::Table::Layout &layout_proto,
                                         ActionData::FormatType_t format_type,
                                         int action_data_bytes_in_table, int immediate_bits,
                                         int index) {
    Log::TempIndent indent;
    LOG4("Setting up ternary layout : "
         << layout_proto << ", format_type: " << format_type
         << ", action_data_bytes_in_table: " << action_data_bytes_in_table
         << ", immediate_bits: " << immediate_bits << ", index: " << index << indent);
    IR::MAU::Table::Layout layout = layout_proto;
    layout.action_data_bytes_in_table = action_data_bytes_in_table;
    layout.immediate_bits = immediate_bits;
    layout.overhead_bits += immediate_bits;
    LayoutOption lo(layout, index);
    cache_layout_options[std::make_pair(tbl->name, format_type)].push_back(lo);
    LOG5("Caching layout option: [" << tbl->name << ", " << format_type << "] = " << lo);
}

/* Setting up the potential layouts for ternary, either with or without immediate
   data if immediate is possible */
void LayoutChoices::setup_ternary_layout_options(const IR::MAU::Table *tbl,
                                                 const IR::MAU::Table::Layout &layout_proto,
                                                 ActionData::FormatType_t format_type) {
    Log::TempIndent indent;
    BUG_CHECK(format_type.valid(),
              "invalid format type in LayoutChoices::setup_ternary_layout_options");
    LOG2("Setup TCAM match layouts " << tbl->name << ":" << format_type << indent);
    int index = 0;
    for (auto &use : get_action_formats(tbl, format_type)) {
        setup_ternary_layout(tbl, layout_proto, format_type,
                             use.bytes_per_loc[ActionData::ACTION_DATA_TABLE], use.immediate_bits(),
                             index);
        index++;
    }
}

int LayoutChoices::get_pack_pragma_val(const IR::MAU::Table *tbl,
                                       const IR::MAU::Table::Layout &layout_proto) {
    auto MIN_PACK = Device::sramMinPackEntries();
    auto MAX_PACK = Device::sramMaxPackEntries();

    auto annot = tbl->match_table->getAnnotations();
    int pack_val = 0;
    if (auto s = annot->getSingle("pack"_cs)) {
        ERROR_CHECK(s->expr.size() > 0, ErrorType::ERR_INVALID,
                    "pack pragma. It has no value for table %1%.", tbl);
        auto pragma_val = s->expr.at(0)->to<IR::Constant>();
        ERROR_CHECK(pragma_val != nullptr, ErrorType::ERR_INVALID,
                    "pack pragma value for table %1%. Must be a constant.", tbl);
        if (pragma_val) {
            pack_val = pragma_val->asInt();
            if (pack_val < MIN_PACK || pack_val > MAX_PACK) {
                warning(ErrorType::WARN_INVALID,
                        "%1%: The provide pack pragma value for table %2% is %3%, when the "
                        "compiler only supports pack values between %4% and %5%",
                        tbl, tbl->externalName(), pack_val, MIN_PACK, MAX_PACK);
                pack_val = 0;
            }
        }
    }

    if (pack_val > 0 && layout_proto.sel_len_bits > 0 && pack_val != 1) {
        error(ErrorType::ERR_INVALID,
              "table %1%. It has a pack value of %2% provided, but also uses a wide selector, "
              "which requires a pack of 1.",
              tbl, pack_val);
        return 0;
    }

    if (pack_val > 0) {
        LOG2("Using packing pragma on table set to " << pack_val);
    }

    return pack_val;
}
/**
 * Responsible for the calculation of the potential layouts to try, and later adapt
 * if necessary in the try_place_table algorithm.
 *
 * Constraints generally come from the following:
 *    1. 128 bits maximally can be packed per RAM
 *    2. 16 individual bytes / (with some exceptions for the upper nibbles), can be
 *       matched in the algorithm.
 *    3. 64 bits of overhead are allowed maximally per RAM.  Overhead is any information
 *       that has to head to match central.
 *    4. At most 5 entries can be packed per RAM line.
 *
 * Lastly, the width <= 8, as that is the maximal width of the RAM array on which to
 * perform a wide match.
 */
void LayoutChoices::setup_exact_match(const IR::MAU::Table *tbl,
                                      const IR::MAU::Table::Layout &layout_proto,
                                      ActionData::FormatType_t format_type,
                                      int action_data_bytes_in_table, int immediate_bits,
                                      int index) {
    LOG5("Setting up exact match layouts for table "
         << tbl->name << ", format_type: " << format_type << " ADB: " << action_data_bytes_in_table
         << ", immediate_bits: " << immediate_bits << ", index: " << index);
    BUG_CHECK(format_type.valid(), "invalid format type in LayoutChoices::setup_exact_match");
    auto MIN_PACK = Device::sramMinPackEntries();
    auto MAX_PACK = Device::sramMaxPackEntries();
    auto MAX_ENTRIES_PER_ROW = Device::sramMaxPackEntriesPerRow();

    auto pack_val = get_pack_pragma_val(tbl, layout_proto);

    auto lo_key = std::make_pair(tbl->name, format_type);
    for (int entry_count = MIN_PACK; entry_count <= MAX_PACK; entry_count++) {
        LOG6("For entry count " << entry_count);
        if (pack_val > 0 && entry_count != pack_val) continue;
        if (entry_count != 1 && layout_proto.sel_len_bits > 0) continue;

        int single_overhead_bits = immediate_bits + layout_proto.overhead_bits;
        int single_entry_bits = single_overhead_bits;
        if (layout_proto.requires_versioning) single_entry_bits += TableFormat::VERSION_BITS;
        single_entry_bits += layout_proto.match_width_bits;

        int total_bits = entry_count * single_entry_bits;
        int total_bytes = entry_count * layout_proto.match_bytes;
        int total_overhead_bits = entry_count * single_overhead_bits;

        LOG6("\ttotal_bits: " << total_bits << ", total_bytes: " << total_bytes
                              << ", total_overhead_bits: " << total_overhead_bits);

        int bit_limit_width =
            (total_bits + TableFormat::SINGLE_RAM_BITS - 1) / TableFormat::SINGLE_RAM_BITS;
        int byte_limit_width =
            (total_bytes + TableFormat::SINGLE_RAM_BYTES - 1) / TableFormat::SINGLE_RAM_BYTES;
        int overhead_width =
            (total_overhead_bits + TableFormat::OVERHEAD_BITS - 1) / TableFormat::OVERHEAD_BITS;
        int pack_width = (entry_count + MAX_ENTRIES_PER_ROW - 1) / MAX_ENTRIES_PER_ROW;

        LOG6("\tbit_limit_width: " << bit_limit_width << ", byte_limit_width: " << byte_limit_width
                                   << ", overhead_width: " << overhead_width
                                   << ", pack_width: " << pack_width);

        // ATCAM tables can only have one payload bus, as the priority ranking happens on
        // a single bus
        if ((overhead_width > 1 || entry_count > MAX_ENTRIES_PER_ROW) && layout_proto.atcam) break;

        int width = std::max({bit_limit_width, byte_limit_width, overhead_width, pack_width});

        int mod_value;
        int min_value = 0;

        if (width > entry_count) {
            mod_value = width % entry_count;
            min_value = entry_count;
        } else {
            mod_value = entry_count % width;
            min_value = width;
        }

        LOG6("\twidth: " << width << ", mod_value: " << mod_value << ", min_value: " << min_value);

        // Skip potential doubling of layouts: i.e. if the layout is 2 entries per RAM row,
        // and 1 RAM wide, then there is no point to adding the double, 4 entries per RAM row,
        // and 2 RAM wide.  This is the same packing, and wider matches are more constrained
        if (mod_value == 0 && min_value != 1 && pack_val == 0) continue;

        if (width > Memories::SRAM_ROWS) break;

        LOG2("\tLayout Option: { pack : "
             << entry_count << ", width : " << width << ", action data table bytes : "
             << action_data_bytes_in_table << ", immediate bits : " << immediate_bits << " }");
        IR::MAU::Table::Layout layout_for_pack = layout_proto;
        IR::MAU::Table::Way way;
        layout_for_pack.action_data_bytes_in_table = action_data_bytes_in_table;
        layout_for_pack.immediate_bits = immediate_bits;
        layout_for_pack.overhead_bits += immediate_bits;
        layout_for_pack.action_data_bytes = action_data_bytes_in_table + (immediate_bits + 7) / 8;
        way.match_groups = entry_count;
        way.width = width;
        cache_layout_options[lo_key].emplace_back(layout_for_pack, way, index);
    }
    // FIXME sort layout options best (cheapest) to worst?
}

/* Setting up the potential layouts for exact match, with different numbers of entries per row,
   different ram widths, and immediate data on and off */
void LayoutChoices::setup_layout_options(const IR::MAU::Table *tbl,
                                         const IR::MAU::Table::Layout &layout_proto,
                                         ActionData::FormatType_t format_type) {
    BUG_CHECK(format_type.valid(), "invalid format type in LayoutChoices::setup_layout_options");
    LOG2("Determining SRAM match layouts " << tbl->name << ":" << format_type << IndentCtl::indent);

    bool hash_action_only = false;
    add_hash_action_option(tbl, layout_proto, format_type, hash_action_only);
    if (!hash_action_only && format_type.matchThisStage()) {
        int index = 0;
        for (auto &use : get_action_formats(tbl, format_type)) {
            setup_exact_match(tbl, layout_proto, format_type,
                              use.bytes_per_loc[ActionData::ACTION_DATA_TABLE],
                              use.immediate_bits(), index);
            index++;
        }
    }
    LOG2_UNINDENT;
}

/* FIXME: This function is for the setup of a table with no match data.  This is currently hacked
   together in order to pass many of the test cases.  This needs to have some standardization
   within the assembly so that all tables that do not require match can possibly work */
void LayoutChoices::setup_layout_option_no_match(const IR::MAU::Table *tbl,
                                                 const IR::MAU::Table::Layout &layout_proto,
                                                 ActionData::FormatType_t format_type) {
    BUG_CHECK(format_type.valid(),
              "invalid format type in LayoutChoices::setup_layout_option_no_match");
    LOG2("Determining no match table layouts " << tbl->name << ":" << format_type);
    GetActionRequirements ghdr;
    tbl->attached.apply(ghdr);
    for (auto v : Values(tbl->actions)) v->apply(ghdr);
    IR::MAU::Table::Layout layout = layout_proto;
    if (ghdr.is_hash_dist_needed() || ghdr.is_rng_needed()) {
        layout.hash_action = true;
    } else if (!format_type.matchThisStage()) {
        // post split gets index via hash_dist, so it needs to be hash_action
        layout.hash_action = true;
    }

    // No match tables are required to have only one layout option in a later pass, so the
    // algorithm picks the action format that has the most immediate.  This is the option
    // that is preferred generally, but not always, if somehow it couldn't fit on the action
    // data bus.  Action data bus allocation could properly be optimized a lot more before this
    // choice would have to be made
    auto uses = get_action_formats(tbl, format_type);
    if (uses.empty()) return;
    auto &use = uses.back();
    layout.immediate_bits = use.immediate_bits();
    layout.action_data_bytes_in_table = use.bytes_per_loc[ActionData::ACTION_DATA_TABLE];
    layout.overhead_bits += use.immediate_bits();
    LayoutOption lo(layout, uses.size() - 1);
    if (layout.hash_action) {
        lo.way.match_groups = 1;
        lo.way.width = 1;
    }
    cache_layout_options[std::make_pair(tbl->name, format_type)].push_back(lo);
    LOG5("Caching layout option: [" << tbl->name << ", " << format_type << "] = " << lo);
}

/**
 * When the stateful table has been split from the match table, the indirect pointer is not
 * necessary to the table format, and is instead contained within the action format
 */
void LayoutChoices::setup_indirect_ptrs(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl,
                                        ActionData::FormatType_t format_type) {
    BUG_CHECK(format_type.valid(), "invalid format type in LayoutChoices::setup_indirect_ptrs");
    ValidateAttachedOfSingleTable::TypeToAddressMap type_to_addr_map;
    ValidateAttachedOfSingleTable validate_attached(type_to_addr_map, tbl);
    tbl->attached.apply(validate_attached);
    layout.action_addr = type_to_addr_map[ValidateAttachedOfSingleTable::ACTIONDATA];
    layout.stats_addr = type_to_addr_map[ValidateAttachedOfSingleTable::STATS];
    layout.meter_addr = type_to_addr_map[ValidateAttachedOfSingleTable::METER];
    LOG5("add indirect ptr " << " action addr " << layout.action_addr.total_bits() << " stats_addr "
                             << layout.stats_addr.total_bits() << " meter_addr "
                             << layout.meter_addr.total_bits());
    layout.overhead_bits += layout.action_addr.total_bits() + layout.stats_addr.total_bits() +
                            layout.meter_addr.total_bits();
}

/**
 * A table can be a hash action table under the following condition:
 *     - The table has no overhead required
 *     - The number of entries is equivalent to 2^(key bits)
 *     - The table would initially be a standard exact match table
 *     - The table has no idletime table (no hash dist bus for idletime)
 *     - All side-effect tables are not addressed by an overhead field
 *
 * To note, direct addressed tables such as counters/meters/stateful tables can also have
 * their address replaced, similar to action data tables.
 */
bool DoTableLayout::can_be_hash_action(const IR::MAU::Table *tbl, std::string &reason) {
    if (tbl->layout.atcam || tbl->layout.no_match_data()) return false;

    int entries = 0;
    if (tbl->match_table) {
        if (auto k = tbl->match_table->getConstantProperty("size"_cs))
            entries = k->asInt();
        else if (auto k = tbl->match_table->getConstantProperty("min_size"_cs))
            entries = k->asInt();
    } else {
        return false;
    }

    /* this doesnt have to be a power of 2. This check is mostly
     * to make the driver happy.
     */
    if (entries != pow(2, tbl->layout.ixbar_width_bits)) {
        reason = "the size is not 2^(key bits)";
        return false;
    }
    if (tbl->layout.overhead_bits > 0) {
        reason = "the table requires match overhead";
        return false;
    }
    if (tbl->actions.size() > 1) {
        reason = "the table has multiple actions";
        return false;
    }

    for (auto ba : tbl->attached) {
        if (ba->attached->is<IR::MAU::IdleTime>()) {
            reason = "the table has idletime requirements";
            return false;
        }
    }

    // Making sure that there is a pathway for the color mapram address
    const IR::MAU::Meter *mtr = nullptr;
    for (auto ba : tbl->attached) {
        mtr = ba->attached->to<IR::MAU::Meter>();
        if (mtr && mtr->color_output() && !mtr->mapram_possible(IR::MAU::ColorMapramAddress::STATS))
            return false;
    }

    return true;
}

/**
 * Adds the hash action layout as a potential choice for a layout for a table, if that
 * layout is possible.
 */
void LayoutChoices::add_hash_action_option(const IR::MAU::Table *tbl,
                                           const IR::MAU::Table::Layout &layout_proto,
                                           ActionData::FormatType_t format_type,
                                           bool &hash_action_only) {
    BUG_CHECK(format_type.valid(), "invalid format type in LayoutChoices::add_hash_action_option");
    std::string hash_action_reason = "";
    bool possible = DoTableLayout::can_be_hash_action(tbl, hash_action_reason);
    hash_action_only = false;
    if (!tbl->match_table) return;

    auto annot = tbl->match_table->getAnnotations();
    if (auto s = annot->getSingle("use_hash_action"_cs)) {
        auto pragma_val = s->expr.at(0)->to<IR::Constant>()->asInt();
        ERROR_CHECK(pragma_val == 1 || pragma_val == 0, ErrorType::ERR_INVALID,
                    "%1%: value %3%. Please provide a 1 to enable the use of the hash action "
                    "path, or a 0 to disable the hash action path for for table %2%.",
                    s, tbl->externalName(), pragma_val);
        hash_action_only = (pragma_val == 1);
        LOG3("\tHash Action pragma value determined on " << tbl->externalName() << " as "
                                                         << pragma_val);
        if (pragma_val == 0) return;
    }

    if (hash_action_only) {
        ERROR_CHECK(possible, ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "Table %1% is required to be a hash action table, but cannot "
                    "be due to %2%.",
                    tbl, hash_action_reason);
    }

    if (!possible && format_type.matchThisStage()) return;

    if (possible) {
        GetActionRequirements gar;
        BUG_CHECK(tbl->actions.size() == 1, "Only one action allowed in hash action table");
        for (auto act : Values(tbl->actions)) act->apply(gar);
        if (gar.is_hash_dist_needed() || gar.is_rng_needed()) hash_action_only = true;
    }

    auto uses = get_action_formats(tbl, format_type);
    if (uses.empty()) return;
    auto &use = uses[0];
    BUG_CHECK(use.immediate_bits() == 0, "Cannot have overhead bits in a hash action table");
    IR::MAU::Table::Layout ha_layout = layout_proto;
    ha_layout.immediate_bits = 0;
    ha_layout.action_data_bytes_in_table = use.bytes_per_loc[ActionData::ACTION_DATA_TABLE];
    ha_layout.hash_action = true;
    LayoutOption lo(ha_layout, 0);
    lo.way.match_groups = 1;
    lo.way.width = 1;
    cache_layout_options[std::make_pair(tbl->name, format_type)].push_back(lo);
    LOG5("Caching layout option: [" << tbl->name << ", " << format_type << "] = " << lo);
}

namespace {
class SelLengthForLayout : public MauInspector {
    IR::MAU::Table::Layout &layout;

    bool preorder(const IR::MAU::StatefulAlu *) override { return false; }

    bool preorder(const IR::MAU::Selector *as) override {
        int sel_len = SelectorLengthBits(as);
        if (sel_len > 0) {
            layout.overhead_bits += sel_len;
            layout.sel_len_bits = sel_len;
        }
        return false;
    }

 public:
    SelLengthForLayout(IR::MAU::Table::Layout *l, const IR::MAU::Table *) : layout(*l) {}
};
}  // namespace

void DoTableLayout::setup_instr_and_next(IR::MAU::Table::Layout &layout,
                                         const IR::MAU::Table *tbl) {
    layout.total_actions = tbl->actions.size();
    int hit_actions = tbl->hit_actions();  // considers pragma max_actions
    if (hit_actions > 0) {
        if (hit_actions <= TableFormat::IMEM_MAP_TABLE_ENTRIES)
            layout.overhead_bits += ceil_log2(hit_actions);
        else
            layout.overhead_bits += TableFormat::FULL_IMEM_ADDRESS_BITS;
    }

    if (tbl->action_chain() && hit_actions > TableFormat::NEXT_MAP_TABLE_ENTRIES) {
        int next_tables = tbl->action_next_paths();
        if (!tbl->has_default_path()) next_tables++;
        if (next_tables <= TableFormat::NEXT_MAP_TABLE_ENTRIES) {
            layout.overhead_bits += ceil_log2(next_tables);
        } else {
            layout.overhead_bits += TableFormat::FULL_NEXT_TABLE_BITS;
        }
    }
}

bool DoTableLayout::preorder(IR::MAU::Table *tbl) {
    Log::TempIndent indent;
    LOG1("## layout table " << tbl->name << indent);
    tbl->layout.ixbar_bytes = tbl->layout.match_bytes = tbl->layout.match_width_bits =
        tbl->layout.action_data_bytes = tbl->layout.overhead_bits = 0;
    setup_instr_and_next(tbl->layout, tbl);
    if (tbl->match_table) setup_match_layout(tbl->layout, tbl);
    if ((tbl->layout.gateway = tbl->uses_gateway())) setup_gateway_layout(tbl->layout, tbl);
    SelLengthForLayout sel_length(&tbl->layout, tbl);
    tbl->attached.apply(sel_length);
    setup_action_layout(tbl);
    tbl->random_seed = tbl->get_random_seed();
    auto &layouts = lc.get_layout_options(tbl);
    bool not_hash_action = layouts.empty();
    for (auto &lo : layouts) {
        if (!lo.layout.hash_action) {
            not_hash_action = true;
            break;
        }
    }
    if (!not_hash_action) tbl->layout.hash_action = true;
    if (tbl->match_table) {
        auto annot = tbl->match_table->getAnnotations();
        if (auto s = annot->getSingle("dynamic_table_key_masks"_cs)) {
            ERROR_CHECK(s->expr.size() > 0, ErrorType::ERR_INVALID,
                        "dynamic_table_key_masks pragma. Has no value for table %1%.", tbl);
            auto pragma_val = s->expr.at(0)->to<IR::Constant>();
            ERROR_CHECK(pragma_val != nullptr, ErrorType::ERR_INVALID,
                        "pack pragma value for table %1%. Must be a constant.", tbl);
            if (pragma_val) {
                auto dkm = pragma_val->asInt();
                if (dkm == 1) {
                    tbl->dynamic_key_masks = true;
                } else {
                    warning(ErrorType::WARN_INVALID,
                            "%1%: The dynammic_table_key_masks pragma value for table %2% is %3%"
                            ", when the compiler only supports value of 1",
                            tbl, tbl->externalName(), dkm);
                }
            }
        }
    }
    if (!tbl->layout.gateway_match && !tbl->layout.no_match_data() && !tbl->layout.ternary) {
        int possible_pack_formats = layouts.size();
        ERROR_CHECK(possible_pack_formats > 0, ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "The table %1% cannot find a valid packing, and cannot be placed. "
                    "Possibly the match key is too wide given the hardware constraints",
                    tbl);
    }

    return true;
}

void LayoutChoices::compute_layout_options(const IR::MAU::Table *tbl,
                                           ActionData::FormatType_t format_type) {
    BUG_CHECK(format_type.valid(), "invalid format type in LayoutChoices::compute_layout_options");
    BUG_CHECK(cache_layout_options.count(std::make_pair(tbl->name, format_type)) == 0,
              "LayoutChoices::compute_layout_options cache already present");
    cache_layout_options[std::make_pair(tbl->name, format_type)];
    auto layout = tbl->layout;
    setup_indirect_ptrs(layout, tbl, format_type);
    if (tbl->layout.gateway_match) {
        /* do nothing */
    } else if (tbl->layout.no_match_data()) {
        setup_layout_option_no_match(tbl, layout, format_type);
    } else if (tbl->layout.ternary) {
        setup_ternary_layout_options(tbl, layout, format_type);
    } else {
        setup_layout_options(tbl, layout, format_type);
    }

    if (format_type.normal()) {
        // FIXME -- should be able to convert to payload gateway even when splitting?  Is it
        // useful to do so?  Should probably just avoid the splitting if a gateway payload is
        // possible?
        fpc.add_option(tbl, *this);
    }
}

bool DoTableLayout::preorder(IR::MAU::Action *act) {
    auto tbl = findContext<IR::MAU::Table>();
    CHECK_NULL(tbl);
    GetActionRequirements ghdr;
    act->apply(ghdr);
    if (!ghdr.is_hash_dist_needed() && !ghdr.is_rng_needed()) return false;

    bool all_options_hash_action = true;
    auto layout_options = lc.get_layout_options(tbl);
    for (auto lo : layout_options) {
        all_options_hash_action &= lo.layout.hash_action;
    }

    ERROR_CHECK(!act->init_default || tbl->layout.no_match_hit_path() || all_options_hash_action,
                ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "Cannot specify %1% as the default action, as it requires %2%", act,
                ghdr.is_hash_dist_needed() ? "the hash distribution unit." : "a random number.");
    /**
     * This check is to validate that a keyless table that has to go through the miss path,
     * because the driver has to potentially program the table, does not have any actions that
     * require hash.  These actions have to go through the hit path, and thus must go through
     * a mstch with no key table.
     */
    if (tbl->layout.no_match_rams()) {
        const char *error_reason = nullptr;
        const char *solution = nullptr;
        if (tbl->layout.total_actions > 1) {
            error_reason = "the table has multiple actions";
            solution = "declare only one action on the table, and mark it as the default action";
        } else if (tbl->layout.action_data_bytes > 0) {
            error_reason = "the table requires programming action data";
            solution = "remove all action data from the action";
        } else if (tbl->layout.no_match_miss_path()) {
            error_reason = "the table requires overhead";
            solution = "remove all match overhead";
        }
        if (error_reason) {
            error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                  "The table %2% with no key cannot have the action %1%.  This action requires "
                  "%5%, which can only be done through the hit pathway.  However, because %3%, "
                  "the driver may need to change at runtime, and the driver can only currently "
                  "program the miss pathway.  The solution may be to %4%.",
                  act, tbl->externalName(), error_reason, solution,
                  ghdr.is_hash_dist_needed() ? "hash" : "rng");
        }
    }

    if (act->miss_only()) {
        if (tbl->layout.no_match_rams()) {
            act->hit_allowed = true;
            act->default_allowed = false;
        } else {
            error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                  "The table %2% cannot have action %1% marked as defaultonly, as this action "
                  "can only run on hit because of its use of %3%",
                  act, tbl->externalName(), ghdr.is_hash_dist_needed() ? "hash" : "rng");
        }
    }

    ERROR_CHECK(!tbl->layout.no_match_miss_path(), ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "This table with no key cannot have the action %1% as an action here, "
                "because it requires %2%, which utilizes the hit path in Tofino, while the "
                "driver configures the miss path",
                act, ghdr.is_hash_dist_needed() ? "hash distribution" : "the rng unit");

    act->hit_path_imp_only = false;
    act->disallowed_reason = ghdr.is_hash_dist_needed() ? "uses_hash_dist"_cs : "uses_rng"_cs;
    return false;
}

bool DoTableLayout::preorder(IR::MAU::TableKey *read) {
    auto tbl = findContext<IR::MAU::Table>();
    CHECK_NULL(tbl);
    cstring partition_index;
    if (tbl->layout.alpm) {
        auto hdr_instance_name = tbl->name + "__metadata";
        auto pidx_field_name = tbl->name + "_partition_index";
        partition_index = hdr_instance_name + "." + pidx_field_name;

        // Look up the PHV::Field objects to ensure that we compare
        // fully-qualified names.
        auto *readInfo = phv.field(read->expr);
        auto *indexInfo = phv.field(partition_index);
        if (readInfo && indexInfo && readInfo->name == indexInfo->name)
            read->partition_index = true;
    }
    return false;
}

bool DoTableLayout::preorder(IR::MAU::Selector *sel) {
    auto tbl = findContext<IR::MAU::Table>();
    CHECK_NULL(tbl);
    if (SelectorLengthBits(sel) <= 0) {
        return false;
    }

    IR::Vector<IR::Expression> components;
    int inputBits = 0;
    int hashModBits = SelectorHashModBits(sel);
    for (auto ixbar_read : tbl->match_key) {
        if (!ixbar_read->for_selection()) continue;
        components.push_back(ixbar_read->expr);
        inputBits += ixbar_read->expr->type->width_bits();
    }
    if (sel->algorithm.type == IR::MAU::HashFunction::IDENTITY && inputBits > hashModBits) {
        // We have more input bits than output bits and we're using an identity hash.
        // This would just result in using the initial/low_order bits from the selector
        // key, but that will reuse the same bits for both the word mod (here), and bit
        // choice in the selectorALU, which results in correlations and the selector
        // being less fair than it should be.  So instead we only use the tail of the
        // selector key
        // FIXME -- it would be better to coordinate this with the allocation of the
        // selectorALU hash in IXBar::allocSelector.  For some non-identity hashes it would
        // be good to use disjoint output bits as well, but how to tell which bits
        // have more/less entropy so as to get enough in all the hashes?
        while (inputBits - components.front()->type->width_bits() > hashModBits) {
            inputBits -= components.front()->type->width_bits();
            components.erase(components.begin());
        }
        if (inputBits > hashModBits) {
            components.front() = MakeSlice(components.front(), inputBits - hashModBits,
                                           components.front()->type->width_bits() - 1);
        }
    }

    IR::ListExpression *field_list = new IR::ListExpression(components);

    auto hge = new IR::MAU::HashGenExpression(
        sel->srcInfo, IR::Type::Bits::get(SelectorHashModBits(sel)), field_list, sel->algorithm);
    auto *hd = new IR::MAU::HashDist(sel->srcInfo, hge->type, hge);
    if (sel->hash_mod != nullptr) delete sel->hash_mod;
    sel->hash_mod = hd;
    return false;
}

bool ValidateTableSize::preorder(const IR::MAU::Table *tbl) {
    if (tbl->match_table == nullptr) return true;

    bool for_match = false;
    for (auto key : tbl->match_key) {
        if (key->for_match()) {
            for_match = true;
            break;
        }
    }

    if (!for_match) return true;
    if (auto k = tbl->match_table->getConstantProperty("size"_cs)) {
        ERROR_CHECK(k->asInt() > 0,
                    "%1%: The table %2% has a size provided that is not "
                    "positive integer : %3%",
                    tbl->srcInfo, tbl->externalName(), k);
    } else if (auto k = tbl->match_table->getConstantProperty("min_size"_cs)) {
        ERROR_CHECK(k->asInt() > 0,
                    "%1%: The table %2% has a min_size provided that is not "
                    "positive integer : %3%",
                    tbl->srcInfo, tbl->externalName(), k);
    }
    return true;
}

static double eqn(double current, int num_counters) {
    return (current - 1) + (log(num_counters) / (log(current / (current - 1))) + 2 * current);
}

static bool tol(double b, double maxVal, int num_counters) {
    auto tmp = maxVal - 5000.0;
    auto eqnval = eqn(b, num_counters);
    if ((tmp <= eqnval) && (eqnval < maxVal)) {
        return true;
    }
    return false;
}

void AssignCounterLRTValues::ComputeLRT::calculate_lrt_threshold_and_interval(
    const IR::MAU::Table *tbl, IR::MAU::Counter *cntr) {
    if (cntr->threshold != -1) return; /* calculated already? */
    auto annot = cntr->annotations;
    bool lrt_enabled = CounterWidth(cntr) < 64;
    if (auto s = annot->getSingle("lrt_enable"_cs)) {
        ERROR_CHECK(s->expr.size() >= 1, ErrorType::ERR_INVALID,
                    "lrt_enable pragma on counter %1%. Does not have a value.", cntr);
        auto pragma_val = s->expr.at(0)->to<IR::Constant>();
        if (pragma_val == nullptr) {
            error(ErrorType::ERR_INVALID, "lrt_enable value on counter %1%. It is not a constant.",
                  cntr);
            return;
        }
        auto real_val = pragma_val->asInt();
        if (real_val == 0) return;  // disabled
        if (real_val != 1) {
            error(ErrorType::ERR_INVALID, "lrt_enable value on counter %1%.", cntr);
            return;
        }
        lrt_enabled = true;
        if (cntr->min_width >= 64) {
            error(ErrorType::ERR_INVALID, "LR(t). Cannot be used on 64 bit counter %1%.", cntr);
            return;
        }
    }

    int lrt_scale = 1;
    if (lrt_enabled) {
        auto s = annot->getSingle("lrt_scale"_cs);
        if (s) {
            auto lrt_val = s->expr.at(0)->to<IR::Constant>();
            if (lrt_val == nullptr) {
                error(ErrorType::ERR_INVALID,
                      "lrt_scale value on counter %1%. It is not a constant.", cntr);
                return;
            }
            lrt_scale = lrt_val->asInt();
            if (lrt_scale <= 0) {
                error(ErrorType::ERR_INVALID, "lrt_scale value on counter %1%.", cntr);
                return;
            }
        }
    }

    if (!lrt_enabled) return;

    int rams = 0;
    UniqueId lookup = tbl->unique_id(cntr);
    if (self_.totalCounterRams.count(lookup)) {
        rams = self_.totalCounterRams.at(lookup);
    } else {  // when counter is shared by multiple tables
        for (auto &use : tbl->resources->memuse) {
            auto &mem = use.second;
            if (mem.type == Memories::Use::EXACT || mem.type == Memories::Use::ATCAM ||
                mem.type == Memories::Use::TERNARY) {
                auto it = mem.unattached_tables.find(lookup);
                if (it != mem.unattached_tables.end()) {
                    for (auto other : it->second) {
                        if (self_.totalCounterRams.count(other))
                            rams += self_.totalCounterRams.at(other);
                    }
                }
            }
        }
    }
    if (rams == 0) {
        error(ErrorType::ERR_NOT_FOUND,
              "Unable to find memory allocation for counter %1% that was "
              "accessed by %2%",
              cntr->name, tbl->externalName());
        return;
    }

    auto num_counters = (rams - 1) * 1024 * CounterPerWord(cntr);
    auto width = CounterWidth(cntr);
    if (width == 1) return;

    double maxVal = pow(2, width) - 1;
    double b_last = 0.0;
    double b_cur = 50000.0;
    int iterations = 0;

    LOG3("Calulating LR(t) on counter:  " << cntr->name);
    LOG3("num_counters: " << num_counters);
    LOG3("width: " << width);
    LOG3("maxVal: " << maxVal);

    while (eqn(b_cur, num_counters) < maxVal) {
        b_last = b_cur;
        b_cur *= 2.0;
        iterations++;
    }

    double step = floor(b_cur - b_last);
    std::set<double> saw;
    while ((!tol(b_cur, maxVal, num_counters)) && (b_cur > 1.0)) {
        if (step > 4.0) {
            step /= 2.0;
            step = floor(step);
        }
        iterations++;

        auto e = eqn(b_cur, num_counters);

        if (e > maxVal) {
            b_cur -= step;
        } else if (e < maxVal) {
            auto it = saw.find(e);
            if (*it == e)
                break;
            else
                b_cur += step;
        } else {
            b_cur -= 1.0;
            break;
        }
        saw.insert(e);
        if (iterations > 10000) break;
    }

    auto threshold = static_cast<unsigned long>(b_cur) >> 4;
    auto interval = static_cast<unsigned long>(b_cur);
    if (cntr->type != IR::MAU::DataAggregation::PACKETS)
        interval = static_cast<unsigned long>(b_cur) >> 8;

    // Tofino register is only 28 bits, so have to use the largest value
    // This only occurs for packet counters that are 32-bits.
    // Expanded to necessary 29 bits for Tofino2.
    if (Device::currentDevice() == Device::TOFINO) {
        if (interval >= (1 << 28)) interval = (1 << 28) - 256;  // Largest value, multiple of 256
    }

#define roundUp(n, b) ((((n) + (b) - 1) - (((n) - 1) % (b))))
    auto scaled_threshold = threshold / lrt_scale;
    /* threshold - must be a multiple of 16 */
    cntr->threshold = roundUp(scaled_threshold, 16);
    LOG3("threshold: " << cntr->threshold);
    auto scaled_interval = interval / lrt_scale;
    /* interval - must be a multiple of 256 */
    cntr->interval = roundUp(scaled_interval, 256);
    LOG3("interval: " << cntr->interval);
#undef roundUp
}

bool AssignCounterLRTValues::ComputeLRT::preorder(IR::MAU::Counter *cntr) {
    auto tbl = findContext<IR::MAU::Table>();
    CHECK_NULL(tbl);
    if (!tbl->is_placed()) return true;
    calculate_lrt_threshold_and_interval(tbl, cntr);
    return true;
}

bool AssignCounterLRTValues::FindCounterRams::preorder(const IR::MAU::Table *t) {
    if (!t->is_placed()) return true;
    for (auto &use : t->resources->memuse) {
        auto &mem = use.second;
        if (mem.type == Memories::Use::COUNTER) {
            int allRams = 0;
            for (auto &r : mem.row) {
                allRams += r.col.size();
            }
            self_.totalCounterRams.emplace(use.first, allRams);
        }
    }
    return true;
}

/**
 * @sa ActionData::RandomNumber::Overlaps
 *
 * Only allows a random extern to get once per action, as currently it is too difficult to
 * tell when a random number get is using the same bits or separate bits.  This is used
 * so sparingly that this shouldn't be a major worry
 */
void RandomExternUsedOncePerAction::postorder(const IR::MAU::RandomNumber *rn) {
    auto act = findContext<IR::MAU::Action>();
    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl && act, "Random Number %1% is not found in an action/table", rn);

    RandKey key = std::make_pair(tbl, act);
    auto &externs = rand_extern_per_action[key];
    if (externs.count(rn))
        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
              "%1%: Random number extern %2% is used "
              "more than one time in action %3%, table %4%, which is currently unsupported "
              "by p4c.  Please use a random extern only once per action",
              rn, rn->name, act->name, tbl->name);
    externs.insert(rn);
}

/**
 * An action profile requires action data to be saved somewhere in RAM space.  If the action
 * profile does not require action data, then the profile is pointless, and is currently not
 * allowed in Brig, as the driver needs some RAM space in order to figure out where to save
 * entries
 */
bool ValidateActionProfileFormat::preorder(const IR::MAU::ActionData *ad) {
    auto tbl = findContext<IR::MAU::Table>();
    CHECK_NULL(tbl);
    auto formats = lc.get_action_formats(tbl);
    BUG_CHECK(formats.size() == 1,
              "%s: Compiler generated multiple formats for action profile "
              "%s on table %s",
              ad->srcInfo, ad->name, tbl->externalName());
    if (formats.size() > 0)
        ERROR_CHECK(formats[0].bytes_per_loc[ActionData::ACTION_DATA_TABLE] > 0,
                    ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "Action "
                    "profile %1% on table %2% does not have any action data (either because no "
                    "action data has been provided, or the action data has been dead code "
                    "eliminated.  An action data free action profile is not supported.",
                    ad, tbl->externalName());
    return false;
}
/**
 * This constraint code is copied from handle_assign.cpp, where this is validated
 *
 * Due to the register rams.match.merge.mau_meter_alu_to_logical_map being an OXBar,
 * one can only assign a single logical table to a wide hash mod.  Thus, a selector
 * that requires a hash mod cannot be shared
 */
bool ProhibitAtcamWideSelectors::preorder(const IR::MAU::Selector *as) {
    auto tbl = findContext<IR::MAU::Table>();
    CHECK_NULL(tbl);
    if (as->max_pool_size > StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE && tbl->layout.atcam) {
        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
              "ATCAM table %2% cannot have selector %1%.  An ATCAM table may require "
              "multiple logical tables, and a selector that has a max pool size greater "
              "than %3%, (in this instance %4%), the selector can only belong to a single "
              "logical table.",
              as, tbl->externalName(), StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE,
              as->max_pool_size);
    }
    return false;
}

Visitor::profile_t TableLayout::init_apply(const IR::Node *node) {
    auto rv = PassManager::init_apply(node);
    lc.clear();
    return rv;
}

/**
 * A Meter Table that is not an LPF or WRED has a color mapram output for the meter color.
 * The Meter ALU and Meter RAMs are addressed by the meter_adr registers.
 *
 * However, the color mapram uses a different address.  This is because the address to access
 * the color mapram happens during PHV processing, while the update to the meter alu happens
 * at the end of the packet, and thus the addressing passed to both pieces of hardware have
 * to be different.
 *
 * As defined in uArch section 6.2.8.4.9 Map RAM Addressing, the Meter Color Map RAM can
 * either use the idletime_adr or the stats_adr.  The idletime
 */
bool MeterColorMapramAddress::FindBusUsers::preorder(const IR::MAU::IdleTime *) {
    visitAgain();
    auto *tbl = findContext<IR::MAU::Table>();
    self.occupied_buses[tbl].setbit(static_cast<int>(IR::MAU::ColorMapramAddress::IDLETIME));
    return false;
}

bool MeterColorMapramAddress::FindBusUsers::preorder(const IR::MAU::Counter *) {
    visitAgain();
    auto *tbl = findContext<IR::MAU::Table>();
    self.occupied_buses[tbl].setbit(static_cast<int>(IR::MAU::ColorMapramAddress::STATS));
    return false;
}

bool MeterColorMapramAddress::DetermineMeterReqs::preorder(const IR::MAU::Meter *mtr) {
    Log::TempIndent indent;
    LOG1("DetermineMeterReqs preorder meter :" << mtr->name << indent);
    if (!mtr->color_output()) return false;
    visitAgain();
    auto *tbl = findContext<IR::MAU::Table>();
    auto *ba = findContext<IR::MAU::BackendAttached>();
    bitvec possible;
    possible.setbit(static_cast<int>(IR::MAU::ColorMapramAddress::STATS));
    bool need_stats = false;

    // In uArch section 6.4.3.5.3 Hash Distribution, the hash distribution unit can create
    // stats and meter addresses, but not idletime address.  Thus if the meter is accessed
    // by hash distribution, then the color mapram must be addressed by stats
    if (ba && ba->addr_location != IR::MAU::AddrLocation::HASH) {
        possible.setbit(static_cast<int>(IR::MAU::ColorMapramAddress::IDLETIME));
        LOG3("Setting Idletime as not a Hash");
    }

    // Can't use idletime if the meter is shared as only one logical table can write
    // to an idletime bus
    // For an atcam table a stage can have multiple partition tables of the same table
    // which effectively makes the meter shared across these partition tables.
    if (!mtr->direct &&
        ((self.att_info.tables_from_attached(mtr).size() > 1) || tbl->layout.atcam)) {
        need_stats = true;
        possible.clrbit(static_cast<int>(IR::MAU::ColorMapramAddress::IDLETIME));
        LOG3("Clearing Idletime as not direct meter and is shared across tables");
    }

    auto pos = self.possible_addresses.find(mtr);
    if (pos != self.possible_addresses.end()) possible &= pos->second;

    auto occ_pos = self.occupied_buses.find(tbl);
    if (occ_pos != self.occupied_buses.end()) possible -= occ_pos->second;

    if (possible.empty()) {
        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
              "The meter '%1%' requires %2% stats address bus to return a color value, and no "
              "bus is available.",
              mtr, need_stats ? "a" : "either an idletime or");
    }
    self.possible_addresses[mtr] = possible;
    LOG2("Possible meter addresses: " << possible);
    return false;
}

bool MeterColorMapramAddress::SetMapramAddress::preorder(IR::MAU::Meter *mtr) {
    if (!mtr->color_output()) return false;
    auto orig_meter = getOriginal()->to<IR::MAU::Meter>();
    auto bv = self.possible_addresses.at(orig_meter);
    mtr->possible_mapram_address =
        bv.getrange(0, static_cast<int>(IR::MAU::ColorMapramAddress::MAPRAM_ADDR_TYPES));
    return false;
}

bool CheckPlacementPriorities::preorder(const IR::MAU::Table *tbl) {
    tbl->get_placement_priority_int();
    placement_priorities[tbl->externalName()] = tbl->get_placement_priority_string();
    return true;
}

/**
 * TODO: the placement_priorities could use a check for a cycle
 */
void CheckPlacementPriorities::end_apply() {
    if (run_once) return;

    for (auto &entry : placement_priorities) {
        for (auto pri : entry.second) {
            auto other_entry = placement_priorities.find(pri);
            if (other_entry == placement_priorities.end()) {
                warning(
                    "The placement_priority %1% on table %2% cannot find the associated "
                    "table",
                    entry.first, pri);
                continue;
            } else if (other_entry->second.count(entry.first)) {
                error(
                    "Tables %1% and %2% have both been prioritized over each other, which "
                    "is nonsensical.",
                    entry.first, pri);
            }
        }
    }
    run_once = true;
}

TableLayout::TableLayout(PhvInfo &p, LayoutChoices &l, SplitAttachedInfo &sia)
    : lc(l), att_info(sia) {
    addPasses({new ValidateTableSize, &att_info, new MeterColorMapramAddress(att_info),
               new RandomExternUsedOncePerAction, new DoTableLayout(p, lc, att_info),
               new ValidateActionProfileFormat(lc), new ProhibitAtcamWideSelectors,
               new CheckPlacementPriorities});
}

LayoutOption *LayoutOption::clone() const { return new LayoutOption(*this); }

std::ostream &operator<<(std::ostream &out, const LayoutOption &lo) {
    Log::TempIndent indent;
    out << const_cast<P4::IR::MAU::Table::Layout &>(lo.layout) << Log::endl;
    bool empty = true;
    if (lo.way.match_groups || lo.way.entries || lo.way.width || !lo.way_sizes.empty()) {
        empty = false;
        out << "way:{ g:" << lo.way.match_groups << " e:" << lo.way.entries
            << " w:" << lo.way.width;
        for (auto s : lo.way_sizes) out << " " << s;
        out << " }";
    }
    if (!lo.partition_sizes.empty()) {
        empty = false;
        out << " part:";
        const char *sep = "";
        for (auto s : lo.partition_sizes) {
            out << sep << s;
            sep = "/";
        }
    }
    if (!lo.dleft_hash_sizes.empty()) {
        empty = false;
        out << " dleft:";
        const char *sep = "";
        for (auto s : lo.dleft_hash_sizes) {
            out << sep << s;
            sep = "/";
        }
    }
    if (!empty) out << Log::endl;
    out << indent << "entries:" << lo.entries;
    if (lo.layout.is_lamb) out << " lambs: " << lo.lambs;
    if (lo.layout.is_direct) {
        out << " (direct - " << lo.layout.entries_per_set << "/" << lo.layout.sets_per_word;
        out << ")";
    }
    out << " srams:" << lo.srams;
    out << " local_tinds: " << lo.local_tinds;
    out << " maprams:" << lo.maprams << " tcams:" << lo.tcams;
    if (lo.select_bus_split >= 0) out << " sbs:" << lo.select_bus_split;
    if (lo.action_format_index >= 0) out << " afi:" << lo.action_format_index;
    if (lo.previously_widened) out << " W";
    if (lo.identity) out << " I";
    return out;
}

void dump(const LayoutOption &lo) { std::cout << lo << std::endl; }
void dump(const LayoutOption *lo) { std::cout << *lo << std::endl; }
