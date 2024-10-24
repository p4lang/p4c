/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "table_format.h"

#include "gateway.h"
#include "hash_mask_annotations.h"
#include "lib/dyn_vector.h"
#include "memories.h"
#include "tofino/input_xbar.h"

void ByteInfo::InterleaveInfo::dbprint(std::ostream &out) const {
    if (!interleaved) return;
    out << "overhead_start " << overhead_start << " match_byte_start " << match_byte_start
        << " byte cycle " << byte_cycle;
}

void ByteInfo::dbprint(std::ostream &out) const {
    out << "Byte " << byte << " bit_use 0x" << bit_use << " byte_location " << byte_location;
    if (!il_info.interleaved) return;
    out << " il_info : { " << il_info << " }";
}
/**
 * For ATCAM specifically, only one result bus is allowed per match.  This is due to the
 * priority ranking.  Each RAM can at most hold 5 entries, and those entries, numbered 0-4
 * are ranked in a priority.
 *
 * Because multiple entries could potentially match within an ATCAM table, (unlike exact or
 * proxy hash), the entry with the highest priority (i.e. 4 > 3) will win.
 *
 * Other rules apply to these priority structures.  Entries 0 and 1 are the only entries
 * allowed to be shared across multiple RAMs.  This is described in uArch section 6.4.3.1
 * Exact Match Physical Row Result Generation, and by the register match.merge.col.hitmap_ouptut.
 *
 * Thus in order to maintain sanity, the entries published in the table format are numbered
 * 0 .. highest entries <= 4.  The numbered entries are the priority ranking of these entries,
 * specifically only for ATCAM.
 *
 * When the ATCAM table has overhead, the result bus is easy to determine.  However, when
 * there is no overhead, the result bus is more difficult and must be wherever any entry
 * 2-4 resides.
 *
 * This matches bf-asm function
 */
bitvec TableFormat::Use::no_overhead_atcam_result_bus_words() const {
    bitvec rv;
    int rb_word = -1;
    std::vector<int> shared_across_boundaries;
    BUG_CHECK(!match_groups.empty(), "ATCAM table has no match data?");
    int groupNo = match_groups.size() - 1;
    for (auto it = match_groups.rbegin(); it != match_groups.rend(); it++) {
        bitvec match_mask = it->match_bit_mask();
        int min_word = match_mask.min().index() / SINGLE_RAM_BITS;
        int max_word = match_mask.max().index() / SINGLE_RAM_BITS;
        bool shared = false;
        if (rb_word < 0) rb_word = min_word;
        if (min_word != max_word)
            shared = true;
        else if (rb_word != min_word)
            shared = true;
        if (shared) {
            BUG_CHECK(groupNo <= 1,
                      "Entries other than 0 & 1 cannot be shared "
                      "across words in an ATCAM\n. Invalid shared entry : %1%",
                      groupNo);
            shared_across_boundaries.push_back(groupNo);
        }
        groupNo--;
    }
    std::ostringstream sab;
    std::copy(shared_across_boundaries.begin(), shared_across_boundaries.end() - 1,
              std::ostream_iterator<int>(sab, ","));
    BUG_CHECK(shared_across_boundaries.size() <= MAX_SHARED_GROUPS,
              "ATCAM table has illegitimate format\n. "
              "Entries shared across word boundary exceeds max allowed (%1%)\n"
              "Invalid Entries - %2% ",
              MAX_SHARED_GROUPS, sab.str());
    rv.setbit(rb_word);
    return rv;
}

/**
 * Result buses are required by SRAM based tables to get data from the RAM to match central.
 * This includes both overhead data, and vpn based data.  The result bus itself is an 83 bit
 * bus, 64 bits for bits 0-63 of the RAM line that matches, and 19 bits for a direct address
 * location.
 *
 * The rules of result bus requirements for each RAM word (an 128 bit section of RAM)
 *     1. If a word has overhead of any entry, then a result bus is required for that word
 *     2. If no entries have overhead, then any portion of each entries match must have
 *        a result bus.  If an entry is within a single RAM section, then this RAM requires
 *        a result bus.  If an entry is split across multiple RAMs, then one of those words
 *        requires a result bus
 *
 * This matches the considerations in bf-asm/sram_match/verify_format and
 * bf-asm/sram_match/no_overhead_determine_result_bus_words
 */
bitvec TableFormat::Use::result_bus_words() const {
    bitvec rv;
    bitvec overhead_mask;
    for (const auto &match_group : match_groups) {
        overhead_mask |= match_group.overhead_mask();
    }

    if (!overhead_mask.empty()) {
        for (int i = 0; i <= overhead_mask.max().index(); i += SINGLE_RAM_BITS) {
            int word = i / SINGLE_RAM_BITS;
            if (!overhead_mask.getslice(i, SINGLE_RAM_BITS).empty()) rv.setbit(word);
        }

        if (only_one_result_bus)
            BUG_CHECK(rv.popcount() == 1, "An ATCAM table can at most only have one result bus");
        return rv;
    }

    if (only_one_result_bus) {
        return no_overhead_atcam_result_bus_words();
    }

    for (auto match_group : match_groups) {
        bitvec match_mask = match_group.match_bit_mask();
        int min_word = match_mask.min().index() / SINGLE_RAM_BITS;
        int max_word = match_mask.max().index() / SINGLE_RAM_BITS;

        if (min_word != max_word) continue;
        rv.setbit(min_word);
    }

    // Try to reuse any result bus for a RAM that has been previously allocated
    for (auto match_group : match_groups) {
        bitvec match_mask = match_group.match_bit_mask();
        int min_word = match_mask.min().index() / SINGLE_RAM_BITS;
        int max_word = match_mask.max().index() / SINGLE_RAM_BITS;
        if (min_word == max_word) continue;
        bool group_has_result_bus = false;
        for (int word = min_word; word <= max_word; word++) {
            if (match_mask.getslice(word * SINGLE_RAM_BITS, SINGLE_RAM_BITS).empty()) continue;
            if (rv.getbit(word)) {
                group_has_result_bus = true;
                break;
            }
        }

        if (group_has_result_bus) continue;
        rv.setbit(min_word);
    }
    return rv;
}

/** The goal of this code is to determine how to initial divide up the match data and the
 *  overhead, and which RAM correspond to which input xbar group.
 *
 *  If you look at section 6.2.3 Exact Match Row Vertical/Horizontal (VH) Xbars, one can
 *  the inputs to an individual RAM line.  There are two search buses per line, which
 *  themselves are 128 bits wide, the same as an individual RAM row.  Each search bus can
 *  select from one of 8 crossbar groups that come from the input crossbar.  Thus, the number
 *  of input xbars groups needed is the number of search buses needed.  (This is not entirely
 *  true, as in ATCAM a single xbar bytes is actually in multiple places on the RAM.  This
 *  information is tracked through the search_bus field in each IXBar::Byte)
 *
 *  Thus the algorithm is divided into two types, skinny and wide.  Skinny means that only
 *  one search bus is required, while wide means multiple search buses are required.
 *
 *  The analyze option assigns both a number of overhead entries per RAM as well as
 *  a search bus assigned to each width.  Thus only bytes with that search_bus value can
 *  be found at that particular location.
 */
bool TableFormat::analyze_layout_option() {
    // FIXME: In total needs some information variable passed about ghosting
    LOG2("  Layout option { pack : " << layout_option.way.match_groups
                                     << ", width : " << layout_option.way.width
                                     << ", entries: " << layout_option.entries << " }");

    // If table has @dynamic_table_key_masks pragma, the driver expects all bits
    // to be available in the table pack format, so we disable ghosting
    if (!layout_option.layout.atcam && !tbl->dynamic_key_masks &&
        !layout_option.layout.proxy_hash && layout_option.entries > 0) {
        int min_way_size =
            *std::min_element(layout_option.way_sizes.begin(), layout_option.way_sizes.end());
        ghost_bits_count = layout_option.layout.get_ram_ghost_bits() + floor_log2(min_way_size);
    }

    use->only_one_result_bus = layout_option.layout.atcam;

    // Initialize all information
    overhead_groups_per_RAM.resize(layout_option.way.width, 0);
    full_match_groups_per_RAM.resize(layout_option.way.width, 0);
    shared_groups_per_RAM.resize(layout_option.way.width, 0);
    search_bus_per_width.resize(layout_option.way.width, 0);
    use->match_group_map.resize(layout_option.way.width);

    for (int i = 0; i < layout_option.way.match_groups; i++) {
        use->match_groups.emplace_back();
    }

    int per_RAM = layout_option.way.match_groups / layout_option.way.width;
    if (layout_option.layout.proxy_hash) {
        analyze_proxy_hash_option(per_RAM);
    } else {
        use->identity_hash = layout_option.identity;
        auto total_info = match_ixbar->bits_per_search_bus();

        for (auto gi : total_info[0].all_group_info) LOG4("   Group info " << gi);

        single_match = *(match_ixbar->match_hash()[0]);
        if (!is_match_entry_wide()) {
            bool rv = analyze_skinny_layout_option(per_RAM, total_info[0].all_group_info);
            if (!rv) return false;
        } else {
            bool rv = analyze_wide_layout_option(total_info[0].all_group_info);
            if (!rv) return false;
        }

        for (auto &hi : total_info) {
            safe_vector<int> ixbar_groups;
            std::sort(hi.all_group_info.begin(), hi.all_group_info.end(),
                      [=](const IXBar::Use::GroupInfo &a, IXBar::Use::GroupInfo &b) {
                          return a.search_bus < b.search_bus;
                      });

            for (auto sb : search_bus_per_width) {
                if (sb >= 0) {
                    auto it = std::find_if(
                        hi.all_group_info.begin(), hi.all_group_info.end(),
                        [&](const IXBar::Use::GroupInfo &a) { return a.search_bus == sb; });
                    if (it != hi.all_group_info.end())
                        ixbar_groups.push_back(it->ixbar_group);
                    else
                        ixbar_groups.push_back(-1);
                } else {
                    ixbar_groups.push_back(-1);
                }
            }
            use->ixbar_group_per_width[hi.hash_group] = ixbar_groups;
        }
    }

    for (size_t i = 0; i < overhead_groups_per_RAM.size(); i++) {
        bool result_bus_needed = overhead_groups_per_RAM[i] > 0;
        use->result_bus_needed.push_back(result_bus_needed);
    }
    LOG3("  Search buses per word " << search_bus_per_width);
    LOG3("  Overhead groups per word " << overhead_groups_per_RAM);

    // Unsure if more code will be required here in the future
    return true;
}

/**
 * Rather than the bytes coming from match data, the bytes have to be built from the
 * hash matrix.  Thus the bytes are built from the proxy_hash_key_use, on a byte by byte
 * basis.
 */
void TableFormat::analyze_proxy_hash_option(int per_RAM) {
    auto *tphi = dynamic_cast<const Tofino::IXBar::Use *>(proxy_hash_ixbar);
    if (!tphi) return;
    auto &ph = tphi->proxy_hash_key_use;
    BUG_CHECK(ph.allocated,
              "%s: Proxy Hash Table %s does not have an allocation for a proxy "
              "hash key",
              tbl->srcInfo, tbl->name);

    use->proxy_hash_group = ph.group;
    int hashMatrixSize = Device::ixbarSpec().hashMatrixSize();
    for (int i = 0; i < hashMatrixSize; i += 8) {
        auto bv = ph.hash_bits.getslice(i, 8);
        if (bv.empty()) continue;
        IXBar::Use::Byte b(PHV::Container(), i);
        b.bit_use = bv;
        b.proxy_hash = true;
        b.search_bus = 0;
        single_match.push_back(b);
    }

    int total = 0;
    for (int i = 0; i < layout_option.way.width; i++) {
        overhead_groups_per_RAM[i] = per_RAM;
        total += per_RAM;
    }

    int index = 0;
    while (total < layout_option.way.match_groups) {
        overhead_groups_per_RAM[index]++;
        index++;
        total++;
    }

    for (int i = 0; i < layout_option.way.width; i++) {
        search_bus_per_width[i] = 0;
    }
}

/* Specifically for the allocation of groups that only require one RAM.  If it requires
   multiple match groups, then balance these match groups and corresponding overhead */
bool TableFormat::analyze_skinny_layout_option(int per_RAM,
                                               safe_vector<IXBar::Use::GroupInfo> &sizes) {
    // This checks to see if the algorithm  can possibly ghost off any extra search buses
    // to leave at most one search bus.  If that's the case, then choose_ghost_bits will
    // prioritize those search buses
    skinny = true;
    if (match_ixbar->search_buses_single() > 1) {
        std::sort(sizes.begin(), sizes.end(),
                  [](const IXBar::Use::GroupInfo &a, const IXBar::Use::GroupInfo &b) {
                      int t;
                      if ((t = a.bits - b.bits) != 0) return a.bits < b.bits;
                      return a.search_bus < b.search_bus;
                  });

        int ghostable_search_buses = 0;
        int bits_seen = 0;
        for (auto group_info : sizes) {
            if (bits_seen + group_info.bits <= ghost_bits_count) {
                ghostable_search_buses++;
                fully_ghosted_search_buses.insert(group_info.search_bus);
                bits_seen += group_info.bits;
            } else {
                break;
            }
        }
        if (match_ixbar->search_buses_single() - ghostable_search_buses > 1) return false;
    }

    if (layout_option.layout.atcam) {
        // ATCAM tables can only have one priority branch
        overhead_groups_per_RAM[0] = layout_option.way.match_groups;
    } else {
        // Evenly assign overhead information per RAM.  In the case of single sized RAMs, one
        // can later share match groups.
        int total = 0;
        for (int i = 0; i < layout_option.way.width; i++) {
            overhead_groups_per_RAM[i] = per_RAM;
            total += per_RAM;
        }

        int index = 0;
        while (total < layout_option.way.match_groups) {
            overhead_groups_per_RAM[index]++;
            index++;
            total++;
        }
    }

    // Every single RAM is assigned the same search bus, as there is only one
    for (int i = 0; i < layout_option.way.width; i++) {
        search_bus_per_width[i] = sizes.back().search_bus;
    }

    for (auto group_info : sizes) {
        ghost_bit_buses.push_back(group_info.search_bus);
    }

    return true;
}

/* Specifically for the allocation of groups that require multiple RAMs.  Determine where
   the overhead has to be, and which RAMs contain the particular match groups */
bool TableFormat::analyze_wide_layout_option(safe_vector<IXBar::Use::GroupInfo> &sizes) {
    skinny = false;
    size_t RAM_per = (layout_option.way.width + layout_option.way.match_groups - 1) /
                     layout_option.way.match_groups;
    if (layout_option.way.width % layout_option.way.match_groups == 0 &&
        layout_option.way.match_groups != 1) {
        warning(
            "Format for table %s to be %d entries and %d width, when that allocation "
            "could easily be split.",
            tbl->name, layout_option.way.match_groups, layout_option.way.width);
    }
    if (size_t(match_ixbar->search_buses_single()) > RAM_per) {
        return false;  // FIXME: Again, can potentially be saved by ghosting off certain bits
    }

    // Whichever one has the least amount of bits will be the group in which the overhead
    // will be stored.  This is because we can ghost the most bits off in that section
    std::sort(sizes.begin(), sizes.end(),
              [=](const IXBar::Use::GroupInfo &a, const IXBar::Use::GroupInfo &b) {
                  int t;
                  if ((t = a.bits - b.bits) != 0) return a.bits < b.bits;
                  return a.search_bus < b.search_bus;
              });

    bool search_bus_on_overhead = true;
    // full_RAMs are the number of RAMs that are dedicated to a non overhead RAMs
    int full_RAMs = layout_option.way.match_groups * (sizes.size() - 1);
    // Overhead RAMs are the number of RAMs that need to be dedicated to holding the overhead
    int overhead_RAMs = layout_option.way.width - full_RAMs;
    if (overhead_RAMs > layout_option.way.match_groups) {
        overhead_RAMs = layout_option.way.match_groups;
    }

    if (layout_option.way.width - overhead_RAMs > full_RAMs) {
        search_bus_on_overhead = false;
    }

    if (overhead_RAMs * MAX_SHARED_GROUPS < layout_option.way.match_groups) return false;

    if (overhead_RAMs > 1 && layout_option.layout.atcam) return false;

    BUG_CHECK(overhead_RAMs <= layout_option.way.match_groups,
              "Allocation for %s has %d RAMs for "
              "overhead allocation, but it only requires %d.  Issue in width calculation.",
              tbl->name, overhead_RAMs, layout_option.way.match_groups);

    // Determine where the overhead groups go.  Evenly distribute the overhead.  At most, only
    // 2 sections of overhead can be within a wide match, as that is the maximum sharing
    for (auto group_info : sizes) {
        ghost_bit_buses.push_back(group_info.search_bus);
    }

    int total = 0;
    for (int i = 0; i < overhead_RAMs; i++) {
        overhead_groups_per_RAM[i] = 1;
        if (search_bus_on_overhead)
            search_bus_per_width[i] = sizes[0].search_bus;
        else
            search_bus_per_width[i] = -1;
        total++;
    }

    int index = 0;
    while (total < layout_option.way.match_groups) {
        BUG_CHECK(index < overhead_RAMs, "Overhead check should fail earlier");
        overhead_groups_per_RAM[index]++;
        index++;
        total++;
    }

    // Assign a search bus to the non overhead groups
    auto it = sizes.begin();
    if (search_bus_on_overhead) it++;

    int match_groups_set = 0;
    for (int i = overhead_RAMs; i < layout_option.way.width; i++) {
        if (it == sizes.end()) {
            match_groups_set = -1;
            LOG4("  Layout option wider than the number of ixbar groups");
        } else if (match_groups_set == layout_option.way.match_groups) {
            it++;
            match_groups_set = 0;
        }

        int search_bus = 0;
        if (it != sizes.end()) {
            search_bus = it->search_bus;
        } else {
            search_bus = -1;
            LOG4("   WARNING: Allocating an extra RAM with neither overhead nor match");
        }

        search_bus_per_width[i] = search_bus;
        match_groups_set++;
    }

    return true;
}

/** The algorithm find_format is to determine how to best pack the RAMs of match tables.  *  For any
 * table using SRAMs only (i.e. exact match/atcam), this means determining how the RAM line is
 * filled.  For tables using the TCAMs, (i.e. ternary), this is specifically for the ternary
 * indirect packing.
 *
 *  The RAM is packed with two classes of information, match data and overhead.  Match data
 *  is anything that is to be directly compared with packet data.  Overhead is everything else.
 *  Overhead consists of anything that could go to match central as well as version bits.
 *
 *  When an entry hits within a table, the lower 64 bits of the RAM (or in the case of a wide
 *  match, one of the RAMs), are sent to match central for further processing.  Thus any
 *  information that is needed by match central for later processing is considered overhead,
 *  and must fit within the lower 64 bits.
 *
 *  The exception to the previous paragraph is what we call version bits.  These are bits
 *  that are matched not as part of the packet, but as a way to ensure that an entry is valid,
 *  and that all data is atomically written into the RAM.  Version bits are then appended
 *  on before the match, and thus can be anywhere within the RAM line.
 *
 *  Data from the packet comes in through the input xbar.  The algorithm is as follows.
 *      1. Analyze the estimate and calculate initial set up information based on the input
 *         xbar allocation and the estimate.
 *      2. Allocate all overhead that is not version bits
 *      3. Allocate all match and version bits.
 *      4. Verify that the algorithm works.
 *
 *  The constraints for these individual pieces will be described above the function which
 *  are part of the algorithm.
 *
 *  FIXME: Noted weaknesses in the algorithm to address in the future:
 *     1. Ghost bits could be worked in with overhead, so that holes in overhead could
 *        be filled with match data.  Glass currently does not do this, so no support
 *        necessary yet.
 */
bool TableFormat::find_format(Use *u) {
    use = u;
    LOG1("Find format for table " << tbl->name);
    LOG2("  Layout option action { adt_bytes: " << layout_option.layout.action_data_bytes_in_table
                                                << ", immediate_bits: "
                                                << layout_option.layout.immediate_bits << " }");
    setup_pfes_and_types();
    if (layout_option.layout.ternary) {
        LOG3("Ternary table?");
        overhead_groups_per_RAM.push_back(1);
        use->match_groups.emplace_back();
        if (!allocate_all_ternary_match()) return false;
        if (!allocate_overhead()) return false;
        return true;
    }

    if (layout_option.layout.gateway_match) {
        BUG_CHECK(layout_option.way.match_groups > 0 &&
                      layout_option.way.match_groups <= Device::gatewaySpec().ExactShifts,
                  "Unsupported immediate profile on a gateway payload table");
        overhead_groups_per_RAM.push_back(layout_option.way.match_groups);
        LOG3("Gateway payload table");
        for (int i = 0; i < layout_option.way.match_groups; i++) use->match_groups.emplace_back();
        if (!allocate_overhead()) return false;
        if (Device::gatewaySpec().ExactShifts > 1) build_payload_map();
        return true;
    } else if (layout_option.layout.no_match_miss_path()) {
        overhead_groups_per_RAM.push_back(1);
        LOG3("No match miss");
        use->match_groups.emplace_back();
        if (!allocate_overhead()) return false;
        return true;
    } else if (layout_option.layout.no_match_hit_path()) {
        BUG_CHECK(layout_option.way.match_groups > 0 &&
                      layout_option.way.match_groups <= Device::gatewaySpec().ExactShifts,
                  "Unsupported immediate profile on a hash action table");
        overhead_groups_per_RAM.push_back(layout_option.way.match_groups);
        LOG3("No match hit");
        for (int i = 0; i < layout_option.way.match_groups; i++) use->match_groups.emplace_back();
        if (!allocate_overhead()) return false;
        return true;
    }

    if (!analyze_layout_option()) return false;
    if (!allocate_sram_match()) return false;
    LOG3("Match and Version");
    if (layout_option.layout.atcam) {
        if (!redistribute_entry_priority()) return false;
    }
    redistribute_next_table();

    LOG3("Build match group map");
    if (!build_match_group_map()) return false;
    verify();
    LOG2("SRAM Table format is successful");
    return true;
}

/** Allocate all overhead data that could head to match central.  This includes the following
 *  information, if needed:
 *    1. Next table, if the table has multiple next table choices and cannot be specified by
 *       the action alone
 *    2. Instruction selection, the bits to indicate which action is to be run.
 *    3. Indirect Pointers: addresses for indirect tables, such as counters, meters, action,
 *       etc.  These needs are specified by the program
 *    4. Immediate: Action data that is stored with the match rather than in a separate action
 *       data table.
 *
 *  The current algorithm just packs as close to the bottom as it can, and does not leave
 *  any holes to put match data in.  This could be optimized to pack match data.
 */
bool TableFormat::allocate_overhead(bool alloc_match) {
    BUG_CHECK(!alloc_match, "Allocating Match must be done independently");
    if (!allocate_next_table()) return false;
    LOG3("Next Table");
    if (tbl->action_chain() && bits_necessary(NEXT) == 0) {
        // if we use action chaining and share action/next bits, allocate action bits
        // first, so group 0 ends up being at bit 0, as required for next
        if (!allocate_all_instr_selection()) return false;
        LOG3("Instruction Selection");
        if (!allocate_selector_length()) return false;
        LOG3("Selector Length");
    } else {
        if (!allocate_selector_length()) return false;
        LOG3("Selector Length");
        if (!allocate_all_instr_selection()) return false;
        LOG3("Instruction Selection");
    }
    if (!allocate_all_indirect_ptrs()) return false;
    LOG3("Indirect Pointers");
    if (!allocate_all_immediate()) return false;
    LOG3("Immediate");
    return true;
}

bool TableFormat::allocate_sram_match() {
    if (allocate_overhead())
        if (allocate_match()) return true;
    clear_pre_allocation_state();
    return interleave_match_and_overhead();
}

/**
 * For each type, return the bits necessary for each overhead requirement.  One can then gather
 * all of the requirements for a single entries overhead.
 *
 * Returns a bitvec, as at some point, overhead could have holes in it.  Right now, all overhead
 * requirements are contiguous, but things like immediate could in theory could be not contiguous
 */
bitvec TableFormat::bitvec_necessary(type_t type) const {
    bitvec rv;
    if (type == NEXT) {
        if (!tbl->action_chain() || hit_actions() <= NEXT_MAP_TABLE_ENTRIES) return rv;
        int next_tables = tbl->action_next_paths();
        if (!tbl->has_default_path()) next_tables++;
        if (next_tables <= NEXT_MAP_TABLE_ENTRIES)
            rv.setrange(0, ceil_log2(next_tables));
        else
            rv.setrange(0, FULL_NEXT_TABLE_BITS);
    } else if (type == ACTION) {
        int instr_select = 0;
        if (hit_actions() == 0) {
            instr_select = 0;
        } else if (hit_actions() > 0 && hit_actions() <= Device::imemSpec().map_table_entries()) {
            instr_select = ceil_log2(hit_actions());
        } else {
            instr_select = Device::imemSpec().address_bits();
        }

        // For no match miss path specifically a format is required, as a ternary indirect is how
        // this pathway is setup.  Therefore, we guarantee at least one overhead bit, though the
        // ternary indirect RAMs don't actually exist
        if (instr_select == 0 && layout_option.layout.no_match_miss_path()) instr_select++;
        /* If actions cannot be fit inside a lookup table, the action instruction can be
           anywhere in the IMEM and will need entire imem bits. The assembler decides
           based on color scheme allocations. Assembler will flag an error if it fails
           to fit the action code in the given bits. */
        rv.setrange(0, instr_select);
    } else if (type == IMMEDIATE) {
        rv |= immediate_mask;
    } else if (type == COUNTER || type == COUNTER_PFE) {
        const IR::MAU::AttachedMemory *stats_addr_user = nullptr;
        for (auto ba : tbl->attached) {
            if (!ba->attached->is<IR::MAU::Counter>()) continue;
            stats_addr_user = ba->attached;
            break;
        }

        if (!layout_option.layout.stats_addr.shifter_enabled) return rv;
        if (stats_addr_user == nullptr) return rv;
        bool move_to_overhead = gw_linked && !stats_addr_user->direct;

        if (type == COUNTER) {
            if (!stats_addr_user->direct) {
                rv.setrange(0, layout_option.layout.stats_addr.address_bits);
            }
        } else if (type == COUNTER_PFE) {
            if (layout_option.layout.stats_addr.per_flow_enable || move_to_overhead)
                rv.setrange(0, 1);
        } else {
            BUG("Unreachable");
        }
    } else if (type == METER || type == METER_PFE || type == METER_TYPE) {
        const IR::MAU::AttachedMemory *meter_addr_user = nullptr;
        for (auto *ba : tbl->attached) {
            if (ba->attached->is<IR::MAU::StatefulAlu>() &&
                ba->use != IR::MAU::StatefulUse::NO_USE) {
                meter_addr_user = ba->attached;
            } else if (ba->attached->is<IR::MAU::Selector>() ||
                       ba->attached->is<IR::MAU::Meter>()) {
                meter_addr_user = ba->attached;
            }
        }
        if (!layout_option.layout.meter_addr.shifter_enabled) return rv;
        if (meter_addr_user == nullptr) return rv;
        bool move_to_overhead =
            gw_linked && !(meter_addr_user->direct || meter_addr_user->is<IR::MAU::Selector>());
        if (type == METER) {
            rv.setrange(0, layout_option.layout.meter_addr.address_bits);
        } else if (type == METER_PFE) {
            if (layout_option.layout.meter_addr.per_flow_enable || move_to_overhead) rv.setbit(1);
        } else if (type == METER_TYPE) {
            if (layout_option.layout.meter_addr.meter_type_bits > 0 || move_to_overhead)
                rv.setrange(0, 3);
        } else {
            BUG("Unreachable");
        }
    } else if (type == INDIRECT_ACTION) {
        // FIXME: unsure if the defaulting of the Huffman bits happens for indirect
        // action data addresses, as potentially it shouldn't be if the compiler
        // was to have different sized action data.  Right now the full bits are
        // reserved even if the size of the action profile does not warrant it
#if 0
         const IR::MAU::ActionData *ad = nullptr;
         for (auto back_at : tbl->attached) {
             ad = back_at->to<IR::MAU::ActionData>();
             if (ad != nullptr)
                 break;
         }
         BUG_CHECK(ad, "No action data table found with an associated action"
                   "address");
         // Extra Huffman encoding required in the address, see section 6.2.8.4.3
         if (ad->size > Memories::SRAM_DEPTH)
             total += ActionDataHuffmanVPNBits(&layout_option.layout);
#endif
        if (!layout_option.layout.action_addr.shifter_enabled) return rv;
        int ad_adr_bits = layout_option.layout.action_addr.address_bits;
        ad_adr_bits += ActionDataHuffmanVPNBits(&layout_option.layout);
        rv.setrange(0, ad_adr_bits);
    } else if (type == SEL_LEN_MOD || type == SEL_LEN_SHIFT) {
        const IR::MAU::Selector *sel = nullptr;
        for (auto ba : tbl->attached) {
            sel = ba->attached->to<IR::MAU::Selector>();
            if (sel != nullptr) break;
        }
        if (sel == nullptr) return rv;
        if (type == SEL_LEN_MOD)
            rv.setrange(0, SelectorModBits(sel));
        else if (type == SEL_LEN_SHIFT)
            rv.setrange(0, SelectorLengthShiftBits(sel));
        else
            BUG("Unreachable");
    } else if ((type == VALID) && requires_valid_bit()) {
        rv.setrange(0, 1);  // Single valid bit
    }
    return rv;
}

int TableFormat::bits_necessary(type_t type) const { return bitvec_necessary(type).popcount(); }

/**
 * Gather the number of bits for a non-overhead field, which is necessary to calculate the bits
 * for possibly interleaving
 */
int TableFormat::overhead_bits_necessary() const {
    int rv = 0;
    for (int i = NEXT; i < ENTRY_TYPES; i++) {
        if (i == VERS) continue;
        rv += bits_necessary(static_cast<type_t>(i));
    }
    return rv;
}

/**
 * Because a gateway requires the 0 position in the action instruction 8 entry matrix, one must
 * add an extra action to a table if linked with a gateway.
 */
int TableFormat::hit_actions() const {
    int extra_action_needed = gw_linked ? 1 : 0;
    return tbl->hit_actions() + extra_action_needed;
}

/**
 * Pretty standard, coordinated to the context JSON parameters:
 *     - type - The type of the field
 *     - lsb_mem_word_offset - the first bit position in the RAM word
 *     - bit_width - how many bits the field requires
 *     - entry - the match group to be in
 *     - RAM_word - the RAM in a wide match
 */
bool TableFormat::allocate_overhead_field(type_t type, int lsb_mem_word_offset, int bit_width,
                                          int entry, int RAM_word) {
    if (lsb_mem_word_offset + bit_width > OVERHEAD_BITS) return false;
    bitvec pack_req(RAM_word * SINGLE_RAM_BITS + lsb_mem_word_offset, bit_width);
    BUG_CHECK((total_use & pack_req).empty(), "Illegally packing data");
    use->match_groups[entry].mask[type] |= pack_req;
    total_use |= pack_req;
    return true;
}

bool TableFormat::allocate_overhead_entry(int entry, int RAM_word, int lsb_mem_word_offset) {
    int curr_lsb_mem_word_offset = lsb_mem_word_offset;
    for (int i = NEXT; i < ENTRY_TYPES; i++) {
        if (i == VERS) continue;
        type_t type = static_cast<type_t>(i);
        int bit_width = bits_necessary(type);
        if (bit_width == 0) continue;
        if (!allocate_overhead_field(type, curr_lsb_mem_word_offset, bit_width, entry, RAM_word))
            return false;
        curr_lsb_mem_word_offset += bit_width;
    }
    return true;
}

/* Bits for selecting the next table from an action chain table must be in the lower part
   of the overhead.  This is specifically to handle this corner case.  If this is run,
   then instruction selection does not necessarily need to be run.  This information has
   to be placed very specifically, according to section 6.4.1.3.2 Next Table Bits */

/** This algorithm is to allocate next table bits in the match overhead when necessary
 *  A table has multiple potential next tables for the following reasons:
 *     - A gateway has a true or false branch
 *     - The table has different behavior on hit and miss
 *     - Based on which actions ran, the table can choose different next pathways
 *
 *  The first two pathways do not require next table information stored in match overhead,
 *  as the next table is stored elsewhere.  Only when the next table is chained from different
 *  actions would the next table have to come from match overhead.
 *
 *  Next table bits, as 12 Stages * 16 = 196 next tables, may have to be up to
 *  ceil(log2(196)) = 8 bits.  On JBay, 20 stages actually leads to 9 bits, though only
 *  8 bits are able to be pulled in the next table extractor.  In order to save bits,
 *  match central has an 8 entry table called next_table_map_data per each logical table,
 *  described in section 6.4.3.3 Next Table Processing.  Thus if <= 8 next tables are not
 *  needed, one just needs to save the ceil_log2(next tables), as this map_data table can
 *  translate up to 3 bit address into an 8 bit next table.
 *
 *  The placement of next table bits are described in section 6.4.1.3.2 Next Table Bits of uArch.
 *  Up to 5 overhead entries exist for exact match.  The next tables bits must be within bits
 *  0 through (Entry # + 1) * 8 - 1, i.e. Entry 0 can be within bits 0-7, and Entry 4 can be
 *  within bits 0-39.  Thus when placing next tables within the entries, the algorithm always
 *  places next table first.
 *
 *  Brig takes advantage of an optimization.  Similar to next tables, instruction memory has
 *  a map_data table similar to next table.  If the number of instructions is under 8, then the
 *  same up to 3 bit pointer can be used for both the instruction selection and next table.  Only
 *  when this optimization cannot be used is this function called.
 */
bool TableFormat::allocate_next_table() {
    int next_table_bits = bits_necessary(NEXT);
    if (next_table_bits == 0) return true;

    int group = 0;
    for (size_t i = 0; i < overhead_groups_per_RAM.size(); i++) {
        for (int j = 0; j < overhead_groups_per_RAM[i]; j++) {
            size_t start = total_use.ffz(i * SINGLE_RAM_BITS);
            if (start + next_table_bits >= OVERHEAD_BITS + i * SINGLE_RAM_BITS) return false;
            bitvec ptr_mask(start, next_table_bits);
            use->match_groups[group].mask[NEXT] |= ptr_mask;
            total_use |= ptr_mask;
            group++;
        }
    }
    return true;
}

/**
 * Data-Plane selection works in the following way.
 *
 * Rather than a standard one-to-one mapping between match and action data, a single match entry
 * can have many possible action data entries, and must select one of these entries at runtime.
 * This we will refer to as a pool of action data entries.  Some entries in the pool may
 * be valid, while other may not be.
 *
 * Selection is the process of picking one of these possible action data entries.  The selector
 * holds a pool of single-bit entries, indicate whether that index of the pool is valid or invalid.
 * The input to a selector is a hash calculated from associated PHV fields and constants
 * and the value of this hash will indicate which of these valid entries to choose from.
 *
 * The action data pool has a base address at the beginning of the pool.  The valid index from the
 * selector pool, chosen by the hash, will be the offset into the pool of action data table entries.
 * This index from the selector is mathematically added to the base address in order to find the
 * location in the action data pool that the selector has chosen.
 *
 * Tofino selectors are broken into two sections, the selector ALU and the selector RAM.  The
 * RAM is a pool, with 120 members per RAM line.  The selector ALU takes in as input, a hash
 * from the input xbar as well as a selector RAM line, and will return the index of that RAM line.
 * 120 bits of pool require a 7-bit all 0 section of action data address in which the selector
 * address will OR into.  If the pool size is smaller, say 64 possible members, the selector
 * offset itself will only be 6-bits wide and thus require a 6-bit place to OR the offset into
 *
 * This works if the pool size is at most 120 entries.  If a pool size > 120 entries, then a
 * single RAM line will not suffice.  Thus, there is a second portion that requires a RAM line
 * select.  A second hash is used to determine an individual RAM line out of the possible
 * multiple RAM lines, and this selected RAM line is now considered the pool of 120 entries.
 *
 * The RAM line select works similar to action data address.  The selector pool has a base
 * address, and the calculated RAM line is ORed into another all 0-bit section of that
 * base address.  The number of 0s needed depends on the size of the pool.  This selector
 * RAM line offset also has to be ORed into the action data address as well.
 *
 * Because the pool size can vary by entry, a selector length can be extracted from the entry.
 * The length of the selector is then used as a max size of RAM lines coordinated as a single
 * pool, and a RAM line is chosen between 0 and the max size of the number of RAM lines.
 *
 * The relevant sections in the uArch are:
 *    - 6.2.8.4.7 - Selector RAM Addressing - The addresses that are generated for the selector
 *      and the action address, with the bits ORed in depending on the address
 *    - 6.2.10 - Selector Tables - Everything the compiler needs and then some on selectors.
 *      For wide selectors, sections 6.2.10.4 Large Selector Pools and 6.2.10.5.5 The Hash
 *      Word Selection Block are useful
 *   - 6.4.3.5.3 - Hash Distribution - Details the pathway for the hash mod calculation
 */

/**
 * The selector length is the parameter used to determine the max size of the RAM lines in the
 * pool.  From this max size, an individual RAM line is chosen.
 *
 * As described in section 6.2.10.5.5 The Hash Word Selection Block, the selector length field
 * is an 8 bit field broken into two parts:
 *
 *    sel_len[4:0] = number of selector words
 *    sel_len[7:5] = selector address shift
 *
 * The calculation is as follows.  The number of possible words ranges from 0-31, which is used
 * the base of a mod operation.  The return of this mod can be shifted up to 5 times, and the bits
 * to the right of the shift are filled in by hash bits.
 *
 * The selector pool is calculated as:
 *
 * RAM_line
 *     = ((hash_value1 % (number_of_selector_words)) << selector_address_shift) | hash_value2
 *
 * where hash_value1 and hash_value2 come from the hash distribution as two portions of a 15 bit
 * hash.  hash_value1 is from bits[9:0] as an input to the mod, and the hash_value2 comes
 * from bits[14:10].  The number of bits used depends of the address shift, i.e.
 *
 *     if (selector_address_shift > 0)
 *     hash_value2 = bits[selector_address_shift-1:0];
 *
 * Thus, if one was to calculate the max selector pool size, it would be the following:
 *     - the max dividend of the mod is 31, meaning the max value of the mod calculation
 *       would be 30
 *     - the max shift is 5, and the thus the max hash_value2 = 2^5 - 1 = 31;
 *     - thus the (max_mod << max_shift) | max hash_value2 = (30 << 5) | 31 = 991
 *     - The pools range from 0-991, meaning a max pool size of 992.
 *
 * The oddity of this comes from the mod operation not being able to return a 31.  This is
 * doubly strange, as a mod by 0 is an impossible operation, and is a return 0 by the hardware.
 *
 * This structure of this calculation also leads to holes in the possible pool sizes.  A pool
 * size of 32 cannot be done by mod value only, as the mod value can at max go to 30 bits.
 * Thus the way this is accomplished is that a mod value of 16 is provided and shifted up one
 * bit leading to the RAM_line_index[4:1] range from 0-15 and RAM_line_index[0:0] = 0-1.
 *
 * However, with this limitation, one cannot come up a way for these bits to have a maximum pool
 * size of 33, because the shifted bits on the bottom are always possible to be 0 or 1, and by
 * guaranteeing a maximum, this requires the lower bit to always be 0.  Thus, one needs
 * to move to the next largest pool.
 *
 * The address formats for the different sizes are described in section 6.2.8.4.7.  These
 * indicate where the hash mod bits and hash shift bits are ORed into the addresses, both
 * for the meter_adr for the selector, and the action_adr for the selectors action data table.
 * It is the drivers responsiblity to write the RAM line addresses with 0s to be ORed into,
 * in order for the addresses not to collide.
 *
 * The 5 bit mod value, if enabled by the hash_value, is always ORed into the address.  This
 * works only if the driver has left space in the selector address and action address of all
 * 0s.  The upper bits of the mod value may also always be known to be zero.  Thus, by using
 * ORs on 0s, the full 5-bit mod can always be input in.
 */

/**
 * The selector length is done through a single extractor through the standard shift mask
 * default pathway.  However, the context JSON requires two fields for the selector length in
 * the pack format, broken into selector_length_shift and selector_length.
 *
 * The choice was to keep the context JSON by having separate fields in the format even though
 * both are extracted by the same extractor.
 *
 * The selector length is at most up to 8 bits, and because the mod operation is timing critical,
 * the length must be at bit 15 or below.  The next table for entry 0 has to go between bits 0-7,
 * so if the table needs both next table and selector length, then the selector length must be
 * in bits 8-15.
 *
 * The selector length has only one extractor per overhead, which is different than all other
 * address types, which have an extractor per entry.  Thus, the entries per RAM line of a table
 * using a programmable selector length is at most 1.  (Possible corner case.  If all entries in
 * an ATCAM partition want to use the same selector length, because they're all in the same RAM
 * row, this constraint goes away).
 */
bool TableFormat::allocate_selector_length() {
    int sel_len_mod_bits = bits_necessary(SEL_LEN_MOD);
    int sel_len_shift_bits = bits_necessary(SEL_LEN_SHIFT);

    if (sel_len_mod_bits == 0 && sel_len_shift_bits == 0) return true;
    if (sel_len_shift_bits > 0)
        BUG_CHECK(sel_len_mod_bits == ceil_log2(StageUseEstimate::MAX_MOD),
                  "Errors in the selection mod calculation");
    int group = 0;
    for (size_t i = 0; i < overhead_groups_per_RAM.size(); i++) {
        for (int j = 0; j < overhead_groups_per_RAM[i]; j++) {
            size_t len_mod_start = total_use.ffz(i * SINGLE_RAM_BITS);
            if (len_mod_start + sel_len_mod_bits >= SELECTOR_LENGTH_MAX_BIT + i * SINGLE_RAM_BITS)
                return false;
            bitvec mask(len_mod_start, sel_len_mod_bits);
            use->match_groups[group].mask[SEL_LEN_MOD] |= mask;
            total_use |= mask;

            size_t shift_start = mask.max().index() + 1;
            if (shift_start + sel_len_shift_bits >= SELECTOR_LENGTH_MAX_BIT + i * SINGLE_RAM_BITS)
                return false;
            bitvec shift_mask(shift_start, sel_len_shift_bits);
            use->match_groups[group].mask[SEL_LEN_SHIFT] |= shift_mask;
            total_use |= shift_mask;
            group++;
        }
    }
    return true;
}

/* Finds space for an individual indirect pointer.  Bases it on the type.  Total is the
   number of bits needed */
bool TableFormat::allocate_indirect_ptr(int total, type_t type, int group, int RAM) {
    size_t start = total_use.ffz(RAM * SINGLE_RAM_BITS);
    if (start + total > size_t(OVERHEAD_BITS + RAM * SINGLE_RAM_BITS)) return false;
    bitvec ptr_mask;
    ptr_mask.setrange(start, total);
    use->match_groups[group].mask[type] |= ptr_mask;
    total_use |= ptr_mask;
    return true;
}

void TableFormat::setup_pfes_and_types() {
    IR::MAU::PfeLocation update_pfe_loc = layout_option.layout.no_match_hit_path()
                                              ? IR::MAU::PfeLocation::GATEWAY_PAYLOAD
                                              : IR::MAU::PfeLocation::OVERHEAD;
    IR::MAU::TypeLocation update_type_loc = layout_option.layout.no_match_hit_path()
                                                ? IR::MAU::TypeLocation::GATEWAY_PAYLOAD
                                                : IR::MAU::TypeLocation::OVERHEAD;
    if (bits_necessary(COUNTER_PFE) > 0) use->stats_pfe_loc = update_pfe_loc;
    if (bits_necessary(METER_PFE) > 0) use->meter_pfe_loc = update_pfe_loc;
    if (bits_necessary(METER_TYPE) > 0) use->meter_type_loc = update_type_loc;
}
/* Algorithm to find the space for any indirect pointers.  Allocated back to back, as they are
   easiest to pack.  No gaps are possible at all within the indirect pointers */
bool TableFormat::allocate_all_indirect_ptrs() {
    int group = 0;
    for (size_t i = 0; i < overhead_groups_per_RAM.size(); i++) {
        for (int j = 0; j < overhead_groups_per_RAM[i]; j++) {
            int stats_addr_bits_required = bits_necessary(COUNTER);
            if (stats_addr_bits_required != 0) {
                if (!allocate_indirect_ptr(stats_addr_bits_required, COUNTER, group, i))
                    return false;
            }

            int stats_pfe_bits_required = bits_necessary(COUNTER_PFE);
            if (stats_pfe_bits_required > 0) {
                if (!allocate_indirect_ptr(stats_pfe_bits_required, COUNTER_PFE, group, i))
                    return false;
            }

            int meter_addr_bits_required = bits_necessary(METER);
            if (meter_addr_bits_required > 0) {
                if (!allocate_indirect_ptr(meter_addr_bits_required, METER, group, i)) return false;
            }

            int meter_pfe_bits_required = bits_necessary(METER_PFE);
            if (meter_pfe_bits_required > 0) {
                if (!allocate_indirect_ptr(meter_pfe_bits_required, METER_PFE, group, i))
                    return false;
            }

            int meter_type_bits_required = bits_necessary(METER_TYPE);
            if (meter_type_bits_required > 0) {
                if (!allocate_indirect_ptr(meter_type_bits_required, METER_TYPE, group, i))
                    return false;
            }

            int ad_addr_bits_required = bits_necessary(INDIRECT_ACTION);
            if (ad_addr_bits_required > 0) {
                if (!allocate_indirect_ptr(ad_addr_bits_required, INDIRECT_ACTION, group, i))
                    return false;
            }
            group++;
        }
    }
    return true;
}

/* Algorithm to find space for the immediate data.  Immediate information may have gaps, i.e. a 9
   bit field has 7 free bits which can potentially be allocated into.  Thus the spaces
   are left open for space to be filled by either instr selection or match bytes */
bool TableFormat::allocate_all_immediate() {
    LOG5("Allocating all immediate bits: " << layout_option.layout.immediate_bits);
    if (layout_option.layout.immediate_bits == 0) return true;
    use->immed_mask = immediate_mask;

    // Allocate the immediate mask for each overhead section
    int group = 0;
    for (size_t i = 0; i < overhead_groups_per_RAM.size(); i++) {
        size_t end = i * SINGLE_RAM_BITS;
        for (int j = 0; j < overhead_groups_per_RAM[i]; j++) {
            size_t start = total_use.ffz(end);
            bitvec immediate_shift = immediate_mask << start;
            if (start + immediate_mask.popcount() >= OVERHEAD_BITS + i * SINGLE_RAM_BITS)
                return false;
            total_use |= immediate_shift;
            use->match_groups[group].mask[IMMEDIATE] |= immediate_shift;
            end = immediate_shift.max().index();
            group++;
        }
    }
    return true;
}

/** This algorithm allocates the pointer to the instruction memory per table entry.  Because
 *  match central has 8 entry map_data table, described in section 6.4.3.5.5 Map Indirection
 *  Table, if the table only has at most 8 hit actions, an up to 3 bit address needs to be
 *  saved, rather than a full instruction memory address.
 *
 *  The full address is 6 bits, 5 for instruction memory line and 1 for color.  The bit for
 *  gress does not have to be saved, as it is added by match central.  This is described within
 *  the uArch in section 6.1.10.3 Action Instruction Memory.
 *
 *  Also, due to the optimization described over the allocate_all_next_table algorithm, it may
 *  be crucial that these action instruction comes first, if next table is not already allocated
 */
bool TableFormat::allocate_all_instr_selection() {
    bitvec instr_mask = bitvec_necessary(ACTION);
    if (instr_mask.empty()) return true;
    int group = 0;
    for (size_t i = 0; i < overhead_groups_per_RAM.size(); i++) {
        size_t end = i * SINGLE_RAM_BITS;
        for (int j = 0; j < overhead_groups_per_RAM[i]; j++) {
            size_t start = total_use.ffz(end);
            bitvec instr_shift = instr_mask << start;
            total_use |= instr_shift;
            if (start >= OVERHEAD_BITS + i * SINGLE_RAM_BITS) return false;
            use->match_groups[group].mask[ACTION] |= instr_shift;
            end = instr_shift.max().index();
            group++;
        }
    }
    return true;
}

/**
 * This function determines whether the table will require multiple search buses to be
 * matched against.  Wide does not mean that the the RAM match is wide, rather that each
 * individual match entry requires multiple RAMs.
 *
 * This match_entry_wide leads to different strategies during allocation
 */
bool TableFormat::is_match_entry_wide() const {
    return !(match_ixbar->search_buses_single() == 1 || layout_option.way.width == 1);
}

/** Save information on a byte by byte basis so that fill out use can correctly be used.
 *  Note that each individual byte from PHV requires an individual byte in the match format,
 *  and cannot be reused by a separate entry.
 */
bool TableFormat::initialize_byte(int byte_offset, int width_sect, const ByteInfo &info,
                                  safe_vector<ByteInfo> &alloced, bitvec &byte_attempt,
                                  bitvec &bit_attempt) {
    Log::TempIndent indent;
    LOG5("Initialize Byte with byte_offset " << byte_offset << ", width_sect: " << width_sect
                                             << indent);
    int initial_offset = byte_offset + width_sect * SINGLE_RAM_BYTES;

    if (match_byte_use.getbit(initial_offset) || byte_attempt.getbit(initial_offset)) return false;
    // Only interleaved bytes can go to the interleaved positions
    if (!info.il_info.interleaved && interleaved_match_byte_use.getbit(initial_offset))
        return false;

    auto use_slice = total_use.getslice(initial_offset * 8, 8);
    use_slice |= bit_attempt.getslice(initial_offset * 8, 8);
    if (!info.il_info.interleaved) use_slice |= interleaved_bit_use.getslice(initial_offset * 8, 8);

    if (!(use_slice & info.bit_use).empty()) return false;

    byte_attempt.setbit(initial_offset);
    bit_attempt |= info.bit_use << (initial_offset * 8);
    alloced.push_back(info);
    alloced.back().byte_location = initial_offset;
    LOG6("Initialized byte " << alloced);
    LOG6("Initial offset: " << initial_offset << ", Bit attempt " << bit_attempt << " Byte attempt "
                            << byte_attempt << " Use slice " << use_slice);
    return true;
}

bool TableFormat::allocate_match_byte(const ByteInfo &info, safe_vector<ByteInfo> &alloced,
                                      int width_sect, bitvec &byte_attempt, bitvec &bit_attempt) {
    int lo = pa == SAVE_GW_SPACE ? GATEWAY_BYTES : 0;
    int hi = pa == SAVE_GW_SPACE ? VERSION_BYTES : SINGLE_RAM_BYTES;
    Log::TempIndent indent;
    LOG5("Allocate Match Byte [lo: " << lo << ", hi: " << hi << "]" << indent);

    for (int i = 0; i < SINGLE_RAM_BYTES; i++) {
        if (i < lo || i >= hi) continue;
        if (initialize_byte(i, width_sect, info, alloced, byte_attempt, bit_attempt)) return true;
    }

    for (int i = 0; i < SINGLE_RAM_BYTES; i++) {
        if (i >= lo && i < hi) continue;
        if (initialize_byte(i, width_sect, info, alloced, byte_attempt, bit_attempt)) return true;
    }
    return false;
}

/**
 * Find the correct position for the interleaved byte of a particular entry, and allocate that
 * byte to that position.
 */
bool TableFormat::allocate_interleaved_byte(const ByteInfo &info, safe_vector<ByteInfo> &alloced,
                                            int width_sect, int entry, bitvec &byte_attempt,
                                            bitvec &bit_attempt) {
    Log::TempIndent indent;
    LOG5("Allocate Interleaved Byte" << indent);
    BUG_CHECK(info.il_info.interleaved, "Illegally calling allocate_interleaved_byte");

    int first_oh_bit = use->match_groups[entry].overhead_mask().min().index();
    int first_match_bit =
        first_oh_bit + info.il_info.match_byte_start - info.il_info.overhead_start;
    BUG_CHECK(first_match_bit % 8 == 0, "Interleaving incorrectly done");
    BUG_CHECK(first_match_bit / SINGLE_RAM_BITS == width_sect,
              "Interleaving cannot find the "
              "correct match bit location");
    int byte_offset = (first_match_bit / 8) % SINGLE_RAM_BYTES;
    BUG_CHECK(initialize_byte(byte_offset, width_sect, info, alloced, byte_attempt, bit_attempt),
              "Always should be able to initialize a split byte");
    return true;
}

/** Pull out all bytes that coordinate to a particular search bus
 */
void TableFormat::find_bytes_to_allocate(int width_sect, safe_vector<ByteInfo> &unalloced) {
    Log::TempIndent indent;
    LOG5("Find bytes to allocate" << indent);
    int search_bus = search_bus_per_width[width_sect];
    for (const auto &info : match_bytes) {
        LOG6("Match Byte: " << info);
        if (info.byte.search_bus != search_bus) continue;
        unalloced.push_back(info);
    }

    std::sort(unalloced.begin(), unalloced.end(), [=](const ByteInfo &a, const ByteInfo &b) {
        int t;
        if ((t = a.bit_use.popcount() - b.bit_use.popcount()) != 0) return t > 0;
        return a.byte < b.byte;
    });
}

/** Version bits are used to indicate whether an entry is valid or not.  For instance, during
 *  the addition of an entry, packets may be running while the entry is not fully added.  Thus
 *  the version bits are used in order to keep track of atomic adds.  Version bits are placed
 *  on the search bus after the input xbar, as specified by section 6.2.3 Exact Match Row
 *  Vertical/Horizontal (VH) Bar, and go anywhere within the 128 bits at a four byte
 *  alignment.
 *
 *  The other trick to version bits is that the upper two bytes of each RAM have nibble by
 *  nibble checks rather than byte by byte checks.  This means that in the upper two nibble,
 *  the algorithm does not have to burn an entire byte for 4 bits, but can fully use a
 *  nibble instead.
 *
 *  The algorithm first tries to put the version with bytes already allocated for that
 *  group.  Then if the algorithm is not try to save space, then the allocation tries
 *  to find places that version could overlap with overhead.  Finally, if that does not
 *  succeed, then try to place in the upper two bytes at nibble alignment
 */
bool TableFormat::allocate_version(int width_sect, const safe_vector<ByteInfo> &alloced,
                                   bitvec &version_loc, bitvec &byte_attempt, bitvec &bit_attempt) {
    bitvec lo_vers(0, VERSION_BITS);
    bitvec hi_vers(VERSION_BITS, VERSION_BITS);
    // Try to place with a match byte already
    for (auto &info : alloced) {
        if (info.byte_location / SINGLE_RAM_BITS != width_sect) continue;
        auto use_slice = total_use.getslice(info.byte_location * 8, 8);
        use_slice |= bit_attempt.getslice(info.byte_location * 8, 8);
        use_slice |= interleaved_bit_use.getslice(info.byte_location * 8, 8);

        if (((info.bit_use | use_slice) & lo_vers).empty()) {
            version_loc = lo_vers << (8 * info.byte_location);
            bit_attempt |= version_loc;
            return true;
        } else if (((info.bit_use | use_slice) & hi_vers).empty()) {
            version_loc = hi_vers << (8 * info.byte_location);
            bit_attempt |= version_loc;
            return true;
        }
    }

    // Look for a corresponding place within the overhead

    for (int i = 0; i < OVERHEAD_BITS / 8; i++) {
        if (pa != PACK_TIGHT) break;
        int byte = width_sect * SINGLE_RAM_BYTES + i;
        if (byte_attempt.getbit(byte) || match_byte_use.getbit(byte) ||
            interleaved_match_byte_use.getbit(byte))
            continue;

        auto use_slice = total_use.getslice(byte * 8, 8);
        use_slice |= bit_attempt.getslice(byte * 8, 8);
        use_slice |= interleaved_bit_use.getslice(byte * 8, 8);

        if ((use_slice & lo_vers).empty() && !(use_slice & hi_vers).empty()) {
            version_loc = lo_vers << (8 * byte);
            bit_attempt |= version_loc;
            byte_attempt |= byte;
            return true;
        }
        if ((use_slice & hi_vers).empty() && !(use_slice & lo_vers).empty()) {
            version_loc = hi_vers << (8 * byte);
            bit_attempt |= version_loc;
            byte_attempt |= byte;
            return true;
        }
    }

    // Look in the upper two nibbles.
    for (int i = 0; i < VERSION_NIBBLES; i++) {
        int initial_bit_offset = (width_sect * SINGLE_RAM_BYTES + VERSION_BYTES) * 8;
        initial_bit_offset += i * VERSION_BITS;
        auto use_slice = total_use.getslice(initial_bit_offset, VERSION_BITS);
        use_slice |= bit_attempt.getslice(initial_bit_offset, VERSION_BITS);
        use_slice |= interleaved_bit_use.getslice(initial_bit_offset, VERSION_BITS);

        if ((use_slice).empty()) {
            version_loc = lo_vers << initial_bit_offset;
            bit_attempt |= version_loc;
            return true;
        }
    }

    // Pick any available byte
    for (int i = 0; i < SINGLE_RAM_BYTES; i++) {
        bitvec version_bits(0, VERSION_BITS);
        int initial_byte = (width_sect * SINGLE_RAM_BYTES) + i;
        if (match_byte_use.getbit(initial_byte) || byte_attempt.getbit(initial_byte) ||
            interleaved_match_byte_use.getbit(i))
            continue;
        auto use_slice = total_use.getslice(initial_byte * 8, 8);
        use_slice |= bit_attempt.getslice(initial_byte * 8, 8);
        use_slice |= interleaved_bit_use.getslice(initial_byte * 8, 8);

        if (!(use_slice & version_bits).empty()) continue;
        version_loc = version_bits << (initial_byte * 8);
        bit_attempt |= version_loc;
        byte_attempt.setbit(initial_byte);
        return true;
    }

    return false;
}

/// Adds the specified byte along with its mask to vector potential_ghost.
///
/// The mask originates from the @hash_mask() annotation specified with the
/// match key in the P4 code.  Bits that are masked off by @hash_mask() are
/// excluded from the list of ghost bits candidates.
///
/// When @hash_mask() is not specified, the mask is set to byte.bit_use,
/// allowing all bits to be candidates for ghost bits selection.
///
void TableFormat::get_potential_ghost_byte(
    const IXBar::Use::Byte byte, const std::map<cstring, bitvec> &hash_masks,
    safe_vector<std::pair<IXBar::Use::Byte, bitvec>> &potential_ghost) {
    bitvec hash_mask = byte.bit_use;
    if (!hash_masks.empty()) {
        for (auto &field_byte : byte.field_bytes) {
            if (hash_masks.count(field_byte.field)) {
                // Get the section of annotation that's in this slice.
                bitvec annot_slice =
                    hash_masks.at(field_byte.field)
                        .getslice(field_byte.lo, field_byte.hi - field_byte.lo + 1);

                // Shift annotation slice to align with container position.
                annot_slice <<= field_byte.cont_lo;

                // Update associated hash mask slice
                hash_mask.clrrange(field_byte.cont_lo, field_byte.hi - field_byte.lo + 1);
                hash_mask |= annot_slice;
            }
        }
    }
    potential_ghost.push_back({byte, hash_mask});
}

void TableFormat::classify_match_bits() {
    HashMaskAnnotations hash_mask_annotations(tbl, phv);

    if (layout_option.layout.atcam) {
        auto partition = match_ixbar->atcam_partition();
        for (auto byte : partition) {
            use->ghost_bits[byte] = byte.bit_use;
        }
    }

    // The pair below consists of the IXBar byte and its associated
    // mask extracted from the @hash_mask() annotation.  When the
    // annotation is not specified, all bits are considered by
    // setting the mask to byte.bit_use.
    safe_vector<std::pair<IXBar::Use::Byte, bitvec>> potential_ghost;

    for (const auto &byte : single_match)
        get_potential_ghost_byte(byte, hash_mask_annotations.hash_masks(), potential_ghost);

    if (LOGGING(5)) {
        LOG5("Potential ghost bytes:");
        for (auto &g : potential_ghost) LOG5("  byte: " << g.first << "  hash mask: " << g.second);
    }

    if (ghost_bits_count > 0) choose_ghost_bits(potential_ghost);

    std::set<int> search_buses;

    for (const auto &[byte, hash_mask] : potential_ghost) {
        search_buses.insert(byte.search_bus);
        match_bytes.emplace_back(byte, byte.bit_use);
    }

    for (auto sb : search_buses) {
        BUG_CHECK(std::count(search_bus_per_width.begin(), search_bus_per_width.end(), sb) > 0,
                  "Byte on search bus %d appears as a match byte when no search bus is "
                  "provided on match",
                  sb);
    }

    for (const auto &info : ghost_bytes) {
        LOG5("\t\tGhost " << info.byte << " bit_use: 0x" << info.bit_use);
        use->ghost_bits[info.byte] = info.bit_use;
    }

    if (LOGGING(5)) {
        for (const auto &info : match_bytes)
            LOG5("\t\tMatch " << info.byte << " bit_use: 0x" << info.bit_use);
    }
}

/** Ghost bits are bits that are used in the hash to find the location of the entry, but are not
 *  contained within the match.  It is an optimization to save space on match bits.
 *
 *  The number of bits one can ghost is the minimum number of bits used to select a RAM row
 *  and a RAM on the hash bus.  One automatically gets 10 bits, for the 10 bits of hash that
 *  determines the RAM row.  Extra bits can be ghosted by the log2size of the minimum way.
 *
 *  This algorithm chooses which bits to ghost.  If the match requires multiple search buses
 *  then the search bus which is going to have the overhead is preferred.  Match requirements
 *  that don't require the full byte are preferred over bytes that require the full 8 bits.
 *  That way, the algorithm can eliminate more match bytes.
 *
 *  For examples, say the match has the following, which were all in separate PHV containers:
 *     3 3 bit fields
 *     1 1 bit field
 *     4 8 bit fields
 *
 *  It would be optimal to ghost off the 3 3 bit fields, and the 1 bit fields, as it would remove
 *  4 total PHV bytes to match on.
 *
 *            Ghost bits selection now considers the mask specified with the @hash_mask
 *            annotation: bits that are masked off through the annotation are not selected
 *            to be part of ghost bits.
 */
void TableFormat::choose_ghost_bits(
    safe_vector<std::pair<IXBar::Use::Byte, bitvec>> &potential_ghost) {
    std::sort(potential_ghost.begin(), potential_ghost.end(),
              [=](const std::pair<IXBar::Use::Byte, bitvec> &a,
                  const std::pair<IXBar::Use::Byte, bitvec> &b) {
                  int t = 0;

                  IXBar::Use::Byte a_byte = a.first;
                  IXBar::Use::Byte b_byte = b.first;

                  auto a_loc =
                      std::find(ghost_bit_buses.begin(), ghost_bit_buses.end(), a_byte.search_bus);
                  auto b_loc =
                      std::find(ghost_bit_buses.begin(), ghost_bit_buses.end(), b_byte.search_bus);

                  BUG_CHECK(a_loc != ghost_bit_buses.end() && b_loc != ghost_bit_buses.end(),
                            "Search bus must be found within possible ghost bit candidates");

                  if (a_loc != b_loc) return a_loc < b_loc;

                  bitvec a_mask = a.second;
                  bitvec b_mask = b.second;
                  bitvec a_masked_bit_use = a_byte.bit_use & a_mask;
                  bitvec b_masked_bit_use = b_byte.bit_use & b_mask;
                  if ((t = a_masked_bit_use.popcount() - b_masked_bit_use.popcount()) != 0)
                      return t < 0;
                  return a < b;
              });

    int ghost_bits_allocated = 0;
    while (ghost_bits_allocated < ghost_bits_count) {
        int diff = ghost_bits_count - ghost_bits_allocated;
        auto it = potential_ghost.begin();
        if (it == potential_ghost.end()) break;
        bitvec masked_bit_use = it->first.bit_use & it->second;
        if (diff >= masked_bit_use.popcount()) {
            if (masked_bit_use.popcount()) ghost_bytes.emplace_back(it->first, masked_bit_use);
            ghost_bits_allocated += masked_bit_use.popcount();
            bitvec match_bits = it->first.bit_use - masked_bit_use;
            if (match_bits.popcount()) match_bytes.emplace_back(it->first, match_bits);
        } else {
            bitvec ghosted_bits;
            int start = masked_bit_use.ffs();
            int split_bit = -1;
            do {
                int end = masked_bit_use.ffz(start);
                if (end - start + ghosted_bits.popcount() < diff) {
                    ghosted_bits.setrange(start, end - start);
                } else {
                    split_bit = start + (diff - ghosted_bits.popcount()) - 1;
                    ghosted_bits.setrange(start, split_bit - start + 1);
                }
                start = masked_bit_use.ffs(end);
            } while (start >= 0);
            BUG_CHECK(split_bit >= 0,
                      "Could not correctly split a byte into a ghosted and "
                      "match section");
            bitvec match_bits = masked_bit_use - ghosted_bits;
            if (ghosted_bits.popcount()) ghost_bytes.emplace_back(it->first, ghosted_bits);
            ghost_bits_allocated += ghosted_bits.popcount();
            match_bytes.emplace_back(it->first, match_bits);
        }
        it = potential_ghost.erase(it);
    }
}

/** Determines which match group that the algorithm is attempting to allocate, given where
 *  overhead is as well as whether the match is skinny or wide
 */
int TableFormat::determine_group(int width_sect, int groups_allocated) {
    if (skinny) {
        int overhead_groups_seen = 0;
        for (int i = 0; i < layout_option.way.width; i++) {
            if (width_sect == i) {
                if (groups_allocated == overhead_groups_per_RAM[i]) return -1;
                return overhead_groups_seen + groups_allocated;
            } else {
                overhead_groups_seen += overhead_groups_per_RAM[i];
            }
        }
    }

    // Could in theory put version bits at this position, so don't skip if:
    // the search_bus_per_width[width_sect] == -1
    int search_bus_seen = 0;
    for (int i = 0; i < layout_option.way.width; i++) {
        if (width_sect == i) {
            if (overhead_groups_per_RAM[i] > 0) {
                if (groups_allocated == overhead_groups_per_RAM[i]) return -1;
                return search_bus_seen + groups_allocated;
            } else {
                if (groups_allocated > 0) return -1;
                return search_bus_seen;
            }
        }
        if (search_bus_per_width[width_sect] == search_bus_per_width[i]) {
            if (overhead_groups_per_RAM[i] > 0)
                search_bus_seen += overhead_groups_per_RAM[i];
            else
                search_bus_seen++;
        }
    }
    BUG("Should never reach this point in table format allocation");
    return -1;
}

/** This fills out the use object, as well as the global structures for keeping track of the
 *  format.  This does this for both match and version information.
 */
void TableFormat::fill_out_use(int group, const safe_vector<ByteInfo> &alloced,
                               bitvec &version_loc) {
    Log::TempIndent indent;
    LOG5("Filling out match byte and group use for group " << group << indent);
    auto &group_use = use->match_groups[group];
    for (const auto &info : alloced) {
        bitvec match_location = info.bit_use << (8 * info.byte_location);
        group_use.match[info.byte] = match_location;
        group_use.mask[MATCH] |= match_location;
        group_use.match_byte_mask.setbit(info.byte_location);
        match_byte_use.setbit(info.byte_location);
        total_use |= match_location;
    }

    if (!version_loc.empty()) {
        group_use.mask[VERS] |= version_loc;
        total_use |= version_loc;
        version_allocated.setbit(group);
        auto byte_offset = (version_loc.min().index() / 8);
        if ((byte_offset % SINGLE_RAM_BYTES) < VERSION_BYTES) match_byte_use.setbit(byte_offset);
    }

    LOG6("Total Use: " << total_use << ", group use: " << group_use
                       << ", match byte use: " << match_byte_use);
}

/** Given a number of overhead entries, this algorithm determines how many match groups
 *  can fully fit into that particular RAM.  It both allocates match and version, as both
 *  of those have to be placed in order for the entry to fit.
 *
 *  For wide matches, this ensures that the entirety of the search bus is placed, but not
 *  necessarily version, as version can be placed in any of the wide match sections.
 */
void TableFormat::allocate_full_fits(int width_sect, int group) {
    Log::TempIndent indent;
    LOG4("Allocating Full Fits on RAM word " << width_sect << " search bus "
                                             << search_bus_per_width[width_sect] << " for group "
                                             << group << indent);
    safe_vector<ByteInfo> allocation_needed;
    safe_vector<ByteInfo> alloced;
    find_bytes_to_allocate(width_sect, allocation_needed);
    bitvec byte_attempt;
    bitvec bit_attempt;

    int groups_allocated = 0;
    int allocate_single_group = (group >= 0);
    while (true) {
        alloced.clear();
        byte_attempt.clear();
        bit_attempt.clear();
        if (!allocate_single_group) group = determine_group(width_sect, groups_allocated);
        if (group == -1) break;
        LOG4("Attempting Entry " << group);
        for (const auto &info : allocation_needed) {
            if (info.il_info.interleaved) {
                if (!allocate_interleaved_byte(info, alloced, width_sect, group, byte_attempt,
                                               bit_attempt))
                    break;
            } else {
                if (!allocate_match_byte(info, alloced, width_sect, byte_attempt, bit_attempt))
                    break;
            }
        }

        if (allocation_needed.size() != alloced.size()) break;

        LOG6("Bytes used for match 0x"
             << byte_attempt.getslice(width_sect * SINGLE_RAM_BYTES, SINGLE_RAM_BYTES));

        bitvec version_loc;
        if (requires_versioning()) {
            if (is_match_entry_wide()) {
                if (!version_allocated[group])
                    allocate_version(width_sect, alloced, version_loc, byte_attempt, bit_attempt);
            } else {
                if (!allocate_version(width_sect, alloced, version_loc, byte_attempt,
                                      bit_attempt)) {
                    break;
                }
            }
        }

        if (!version_allocated[group] && !version_loc.empty()) {
            int version_byte = version_loc.min().index() / 8;
            LOG6("Version Loc : { Byte : " << (version_byte % SINGLE_RAM_BYTES)
                                           << " Bits in Byte : "
                                           << version_loc.getslice(version_byte * 8, 8) << "}");
        } else {
            LOG6("Version not allocated on RAM word " << width_sect);
        }

        LOG4("Entry " << group << " fully fits on RAM word");

        groups_allocated++;
        full_match_groups_per_RAM[width_sect]++;
        fill_out_use(group, alloced, version_loc);
        LOG4("Entry usage: " << total_use);

        if (allocate_single_group) break;
    }
}

/** Given all the information about a group that currently has not been placed, try to fit
 *  that group within a current section.  If the group has never been attempted before, (which
 *  happens when the overhead section is found), then try to fit the information in all
 *  previous RAMs as well.
 */
void TableFormat::allocate_share(int width_sect, int group, safe_vector<ByteInfo> &unalloced_group,
                                 safe_vector<ByteInfo> &alloced, bitvec &version_loc,
                                 bitvec &byte_attempt, bitvec &bit_attempt, bool overhead_section) {
    std::set<int> width_sections;
    int min_sect = width_sect;

    BUG_CHECK(shared_groups_per_RAM[width_sect] < MAX_SHARED_GROUPS,
              "Trying to share a group "
              "on a section that is already allocated");

    // Try all previous RAMs
    if (overhead_section) {
        min_sect = 0;
    }

    if (overhead_section) {
        auto it = unalloced_group.begin();
        while (it != unalloced_group.end()) {
            if (!it->il_info.interleaved) {
                it++;
                continue;
            }
            allocate_interleaved_byte(*it, alloced, width_sect, group, byte_attempt, bit_attempt);
            width_sections.emplace(width_sect);
            it = unalloced_group.erase(it);
        }
    }

    for (int current_sect = min_sect; current_sect <= width_sect; current_sect++) {
        if (shared_groups_per_RAM[width_sect] == MAX_SHARED_GROUPS) continue;
        LOG5("\t    Allocating shared entry " << group << " on RAM word " << width_sect);
        auto it = unalloced_group.begin();
        bitvec prev_attempt = byte_attempt;
        while (it != unalloced_group.end()) {
            if (allocate_match_byte(*it, alloced, current_sect, byte_attempt, bit_attempt)) {
                width_sections.emplace(current_sect);
                it = unalloced_group.erase(it);
            } else {
                it++;
            }
        }

        bitvec local_attempt = byte_attempt - prev_attempt;
        LOG6("\t\tBytes used for match 0x"
             << local_attempt.getslice(current_sect * SINGLE_RAM_BYTES, SINGLE_RAM_BYTES));
    }

    for (int current_sect = min_sect; current_sect <= width_sect; current_sect++) {
        if (shared_groups_per_RAM[current_sect] == MAX_SHARED_GROUPS) continue;
        if (!requires_versioning()) break;

        if (!version_loc.empty()) break;
        if (allocate_version(current_sect, alloced, version_loc, byte_attempt, bit_attempt)) {
            width_sections.emplace(current_sect);
            int version_byte = version_loc.min().index() / 8;
            LOG6("\t\tVersion Loc : { Byte : " << (version_byte % SINGLE_RAM_BYTES)
                                               << " Bits in Byte : "
                                               << version_loc.getslice(version_byte * 8, 8) << "}");
            break;
        }
    }

    // Note that if a group has overhead on that section, it must be able to access the
    // match result bus that has that particular information, as specified in 6.4.1.4 Match
    // Overhead Enable
    bool on_current_section = false;
    for (auto section : width_sections) {
        shared_groups_per_RAM[section]++;
        on_current_section |= (section == width_sect);
    }

    if (!on_current_section && overhead_section) {
        shared_groups_per_RAM[width_sect]++;
    }
}

/** Given that no entry can fully fit within a particular RAM line, this allocation fills in
 *  the gaps within those RAMs will currently unallocated groups.  The constraint that comes
 *  from section 6.4.3.1 Exact Match Physical Row Result Generation is that at most 2 groups
 *  can be shared within a particular RAM, as only two hit signals will be able to merge.
 *
 *  The other constraint from the hardware is described in section 6.4.1.4 Match Overhead
 *  Enable.  This constraint says that if the overhead is contained within a particular
 *  RAM, then that match group must be either in the RAM, or a shared spot has to be open
 *  to access this overhead.  Thus these constraints have to be maintained while placing
 *  these shared groups.
 */
bool TableFormat::allocate_shares() {
    safe_vector<ByteInfo> unalloced;

    std::map<int, safe_vector<ByteInfo>> unalloced_groups;
    std::map<int, safe_vector<ByteInfo>> allocated;
    std::map<int, bitvec> version_locs;
    bitvec byte_attempt;
    bitvec bit_attempt;

    find_bytes_to_allocate(0, unalloced);

    int overhead_groups_seen = 0;
    // Initialize unallocated groups.  Group number is determined by what overhead is within
    // that section
    for (int width_sect = 0; width_sect < layout_option.way.width; width_sect++) {
        int total_groups =
            overhead_groups_per_RAM[width_sect] - full_match_groups_per_RAM[width_sect];
        for (int i = 0; i < total_groups; i++) {
            int unallocated_group = overhead_groups_seen + full_match_groups_per_RAM[width_sect];
            unallocated_group += i;
            unalloced_groups[unallocated_group] = unalloced;
        }
        overhead_groups_seen += overhead_groups_per_RAM[width_sect];
    }

    // Groups currently partially allocated
    safe_vector<int> groups_begun;

    overhead_groups_seen = 0;
    for (int width_sect = 0; width_sect < layout_option.way.width; width_sect++) {
        LOG4("\t  Attempting cross RAM entries on RAM word " << width_sect);
        // Groups that aren't allocated that will need sharing
        int groups_to_start =
            overhead_groups_per_RAM[width_sect] - full_match_groups_per_RAM[width_sect];
        if (groups_to_start == 0 && groups_begun.size() == 0) continue;
        int shared_group_count = groups_to_start + groups_begun.size();
        // Cannot share the resources
        if (shared_group_count > MAX_SHARED_GROUPS) return false;

        // Try to complete all groups that have been previously placed, to try and get rid
        // of them as quickly as possible
        for (auto group : groups_begun) {
            if (shared_groups_per_RAM[width_sect] > MAX_SHARED_GROUPS) break;
            LOG4("\t  Attempting already split entry " << group);
            allocate_share(width_sect, group, unalloced_groups[group], allocated[group],
                           version_locs[group], byte_attempt, bit_attempt, false);
        }

        // Try to allocate new overhead groups
        for (int i = 0; i < groups_to_start; i++) {
            if (shared_groups_per_RAM[width_sect] > MAX_SHARED_GROUPS) break;
            int group = overhead_groups_seen + full_match_groups_per_RAM[width_sect] + i;
            LOG4("\t  Attempting newly split entry " << group);
            allocate_share(width_sect, group, unalloced_groups[group], allocated[group],
                           version_locs[group], byte_attempt, bit_attempt, true);
        }

        // Eliminate placed groups
        auto it = groups_begun.begin();
        while (it != groups_begun.end()) {
            if (unalloced_groups[*it].empty() && !version_locs[*it].empty())
                it = groups_begun.erase(it);
            else
                it++;
        }

        // Add new overhead groups that are not fully placed
        for (int i = 0; i < groups_to_start; i++) {
            int group = overhead_groups_seen + full_match_groups_per_RAM[width_sect];
            group += i;
            if (!unalloced_groups[group].empty() ||
                (requires_versioning() && version_locs[group].empty()))
                groups_begun.push_back(group);
        }
        overhead_groups_seen += overhead_groups_per_RAM[width_sect];
    }

    // If everything is not placed, then complain
    for (auto unalloced_group : unalloced_groups) {
        if (!unalloced_group.second.empty()) return false;
    }

    if (requires_versioning()) {
        for (auto version_loc : version_locs) {
            if (version_loc.second.empty()) return false;
        }
    }

    for (auto entry : allocated) {
        BUG_CHECK(entry.second.size() == unalloced.size(),
                  "During sharing of match "
                  "groups, allocation for group %d not filled out",
                  entry.first);
        BUG_CHECK(!requires_versioning() || !version_locs[entry.first].empty(),
                  "During sharing of match groups, allocation of version for group %d is "
                  "not filled out",
                  entry.first);
        fill_out_use(entry.first, entry.second, version_locs[entry.first]);
    }
    return true;
}

/** This is a further optimization on allocating shares.  Because version is placed relatively
 *  early, occasionally this leads to a packing issue as versions in separate RAMs could be
 *  combined into an early upper nibble, and save an extra necessary byte.  This actually
 *  will clear the placing of the full fit match a version from back, and try to actually
 *  share this section.
 *
 *  The algorithm will keep removing fully placed section until a fit is found, or the
 *  algorithm determines that with the extra sharing the matches still will not fit.
 */
bool TableFormat::attempt_allocate_shares() {
    LOG4("\t  Attempt Allocating Shares");
    // Try with all full fits
    if (allocate_shares()) return true;
    // Eliminate a full fit section from the back one at a time, clear it out, and repeat
    // allocation
    for (int width_sect = layout_option.way.width - 1; width_sect >= 0; width_sect--) {
        std::fill(shared_groups_per_RAM.begin(), shared_groups_per_RAM.end(), 0);
        int overhead_groups_seen = 0;
        if (full_match_groups_per_RAM[width_sect] == 0) continue;
        for (int j = 0; j < width_sect; j++) overhead_groups_seen += overhead_groups_per_RAM[j];
        int group = overhead_groups_seen + full_match_groups_per_RAM[width_sect] - 1;
        total_use -= use->match_groups[group].mask[MATCH];
        total_use -= use->match_groups[group].mask[VERS];
        match_byte_use -= use->match_groups[group].match_byte_mask;
        use->match_groups[group].clear_match();
        full_match_groups_per_RAM[width_sect]--;

        if (allocate_shares()) return true;
    }
    return false;
}

void TableFormat::clear_match_state() {
    for (int entry = 0; entry < layout_option.way.match_groups; entry++) {
        use->match_groups[entry].clear_match();
    }
    match_bytes.clear();
    ghost_bytes.clear();
    std::fill(full_match_groups_per_RAM.begin(), full_match_groups_per_RAM.end(), 0);
    std::fill(shared_groups_per_RAM.begin(), shared_groups_per_RAM.end(), 0);
    use->ghost_bits.clear();
    match_byte_use.clear();
    version_allocated.clear();
    interleaved_bit_use.clear();
    interleaved_match_byte_use.clear();
}

void TableFormat::clear_pre_allocation_state() {
    clear_match_state();
    total_use.clear();
    for (int entry = 0; entry < layout_option.way.match_groups; entry++) {
        use->match_groups[entry].clear();
    }
}

/** Gateway tables and exact match tables share search buses to perform lookups.  Thus
 *  in order to potentially share a search bus, the allocation algorithm is run up to
 *  two times.  The first run, the lower 4 bytes of the gateway are packed last.  However,
 *  occasionally this is suboptimal, as the combination of overhead with match bytes might
 *  provide extra room.  Thus if the first iteration doesn't fit, the bytes closest to the
 *  overhead are attempted first
 */
bool TableFormat::allocate_match() {
    pre_match_total_use = total_use;
    bool success = false;

    for (int i = 0; i < PACKING_ALGORITHMS; i++) {
        total_use = pre_match_total_use;
        for (int group = 0; group < layout_option.way.match_groups; group++) {
            use->match_groups[group].clear_match();
        }
        match_bytes.clear();
        ghost_bytes.clear();
        std::fill(full_match_groups_per_RAM.begin(), full_match_groups_per_RAM.end(), 0);
        version_allocated.clear();

        use->ghost_bits.clear();
        match_byte_use.clear();

        classify_match_bits();
        pa = static_cast<packing_algorithm_t>(i);
        LOG4("\tAllocate match with algorithm " << pa);
        success = allocate_match_with_algorithm();

        if (success) break;
    }
    return success;
}

/** This section allocates all of the match data.  Given that we have assigned search buses to
 *  each individual RAM as well as overhead, this algorithm fills in the gap with all of the
 *  match data.
 *
 *  The general constraints for match data are described in many sections, but mostly in the
 *  from 6.4.1 Exact Match Table Entry Description - 6.4.3 Match Merge.  Section 6.4.2
 *  Match Generation indicates that at most 5 entries can be contained per RAM line.  The total
 *  data used for all match can be specified at bit granularity, while each of the 5 entries
 *  can specify which bytes to match on of these bits.  Thus every byte must coordinate to at
 *  most one match, but any bits of those bytes can be ignored as well.
 *
 *  The other major constraint that the algorithm takes advantage of is specified in section
 *  6.4.3.1 Exact Match Physical Row Generation.  This is the hardware that allows matches
 *  to chain, for example wide matches.  However, the maximum number of hits allowed to chain
 *  is at most two.
 *
 *  (The previous paragraph does have the following exception, the upper 4 nibbles are not
 *   on a byte by byte granularity, but on a nibble by nibble granularity, to pack version
 *   nibbles)
 *
 *  The algorithm is the following:
 *    1. Choose ghost bits: bits that won't appear in the match format
 *    2. Allocate full fits: i.e. match groups that fit on the entirety of the individual RAM
 *    3. Allocate shares: share match groups RAMs
 */
bool TableFormat::allocate_match_with_algorithm() {
    for (int width_sect = 0; width_sect < layout_option.way.width; width_sect++) {
        allocate_full_fits(width_sect);
    }

    // Determine if everything is fully allocated
    safe_vector<int> search_bus_alloc(match_ixbar->search_buses_single(), 0);
    for (int width_sect = 0; width_sect < layout_option.way.width; width_sect++) {
        int search_bus = search_bus_per_width[width_sect];
        if (search_bus < 0) continue;
        search_bus_alloc[search_bus] += full_match_groups_per_RAM[width_sect];
    }

    bool split_match = false;

    for (size_t sb = 0; sb < search_bus_alloc.size(); sb++) {
        BUG_CHECK(search_bus_alloc[sb] <= layout_option.way.match_groups,
                  "Allocating of more "
                  "match groups than actually required");
        if (fully_ghosted_search_buses.count(sb) > 0) continue;
        split_match |= search_bus_alloc[sb] < layout_option.way.match_groups;
    }

    if (requires_versioning())
        split_match |= version_allocated.popcount() != layout_option.way.match_groups;

    if (split_match) {
        // Will not split up wide matches
        if (pa != PACK_TIGHT) return false;
        if (is_match_entry_wide()) {
            return false;
        } else {
            return attempt_allocate_shares();
        }
    }
    return true;
}

/** As specified in the uArch document in section 6.3.6 TCAM Search Data Format, a TCAM search
 *  data/match data can be encoded in 4 ways:
 *
 *  0 - tcam_normal
 *  1 - dirtcam_2b
 *  2 - dirtcam_4b_lo
 *  3 - dirtcam_4b_hi
 *
 *  TCAM values have two search words and two match words.  These two words give the ability to
 *  do all 0, 1, and don't care matches.
 *
 *  The encoding for TCAM normal is described in section 6.3.1 TCAM Data Representation
 *
 *  2b/4b encoding is a translation of all possible matches through a Karnaugh map.  The
 *  following table is a map of 2b encoding
 *
 *  __Desired_Match__|____Encoding____
 *         00        |  word0[0] = 1
 *         01        |  word0[1] = 1
 *         10        |  word1[0] = 1
 *         11        |  word1[1] = 1
 *
 *  Thus one can calculate all 0 1 and * patterns, i.e if the match was *0, word0 would be 01
 *  and word1 would be 01
 *
 *  4b encoding has a similar table:
 *
 *  __Desired_Match__|____Encoding____
 *       0000        |  word0[0] = 1
 *       ....        |  ............
 *       0111        |  word0[7] = 1
 *       1000        |  word1[0] = 1
 *       ....        |  ............
 *       1111        |  word1[7] = 1
 *
 *  One can surmise that ranges are now possible, as each individual number translates to a
 *  particular byte in either word0 or word1.  Notice also that it takes a full byte to match
 *  on the range of a nibble.  Thus the reason for 4b_lo and 4b_hi, as the encoding will match
 *  on the lower or higher nibble respectively.
 *
 *  The encodings for Barefoot are the following:
 *     - TCAM normal for version/valid matching
 *     - 2b encoding for standard match data, as this apparently saves power when compared to
 *       TCAM normal encoding
 *     - Each full byte of range match will need a 4b_lo and a 4b_hi
 */
void TableFormat::initialize_dirtcam_value(bitvec &dirtcam, const IXBar::Use::Byte &byte) {
    LOG5("    Initializing dirtcam value : " << std::hex << dirtcam);
    if (byte.is_spec(IXBar::RANGE_LO)) {
        dirtcam.setbit(byte.loc.byte * 2 + 1);  // 4b_lo encoding
    } else if (byte.is_spec(IXBar::RANGE_HI)) {
        dirtcam.setrange(byte.loc.byte * 2, 2);  // 4b_hi_encoding
    } else {
        dirtcam.setbit(byte.loc.byte * 2);  // 2b_encoding
    }
}

void TableFormat::Use::TCAM_use::set_group(int _group, bitvec _dirtcam) {
    group = _group;
    dirtcam = _dirtcam;
}

void TableFormat::Use::TCAM_use::set_midbyte(int _byte_group, int _byte_config) {
    byte_group = _byte_group;
    byte_config = _byte_config;

    if (byte_config == MID_BYTE_LO || byte_config == MID_BYTE_HI)
        dirtcam.setbit(Tofino::IXBar::TERNARY_BYTES_PER_GROUP * 2);
}

void TableFormat::ternary_midbyte(int midbyte, size_t &index, bool lo_midbyte) {
    Use::TCAM_use *tcam_p;
    Use::TCAM_use tcam;
    if (index < use->tcam_use.size())
        tcam_p = &(use->tcam_use[index]);
    else
        tcam_p = &tcam;

    int midbyte_type = lo_midbyte ? MID_BYTE_LO : MID_BYTE_HI;

    tcam_p->set_midbyte(midbyte, midbyte_type);

    if (tcam_p == &tcam) use->tcam_use.push_back(tcam);
    index++;
}

void TableFormat::ternary_version(size_t &index) {
    Use::TCAM_use *tcam_version_p;
    Use::TCAM_use tcam_version;
    if (index < use->tcam_use.size()) {
        tcam_version_p = &(use->tcam_use[index]);
    } else {
        tcam_version_p = &tcam_version;
    }
    tcam_version_p->set_midbyte(-1, MID_BYTE_VERS);
    if (tcam_version_p == &tcam_version) use->tcam_use.push_back(tcam_version);
    index++;
}

/**
 * Reservation of ternary match tables.  Allocate the group and midbyte config.
 *
 * There is some specific constraints on tables with multiple ranges, due to the multirange
 * distribution described in section 6.3.9.2 Multirange distributor:
 *     1. If a range key is allocated in multiple TCAMs, then no other range fields can
 *        appear in that TCAM
 *     2. If multiple TCAMs contain a single range match, then TCAMs between these
 *        two TCAM in hardware cannot contain any range matches.
 *
 * The reason for this is that the multirange distribution has to happen on an individual
 * key by key basis.  By breaking these constraints, you are breaking these restrictions on
 * multirange distribution.
 *
 * The compiler currently implements a tighter set of constraints:
 *     1. Each TCAM can at most have only one range field
 *     2. TCAMs with split range fields are contiguous.
 */
bool TableFormat::allocate_all_ternary_match() {
    bitvec used_groups;
    std::map<int, bitvec> dirtcam;
    bitvec used_midbytes;
    std::map<int, std::pair<bool, bool>> midbyte_lo_hi;
    std::map<int, int> range_indexes;

    for (auto &byte : match_ixbar->use) {
        LOG5("  Checking match ixbar use byte : " << byte);
        if (byte.loc.byte == Tofino::IXBar::TERNARY_BYTES_PER_GROUP) {
            // Reserves groups and the mid bytes
            used_midbytes.setbit(byte.loc.group / 2);
            std::pair<bool, bool> lo_hi = {false, false};
            if (byte.bit_use.min().index() <= 3) {
                lo_hi.first = true;
            }
            if (byte.bit_use.max().index() >= 4) {
                lo_hi.second = true;
            }
            midbyte_lo_hi[byte.loc.group / 2] = lo_hi;
        } else {
            used_groups.setbit(byte.loc.group);
            initialize_dirtcam_value(dirtcam[byte.loc.group], byte);
            if (byte.is_range()) {
                range_indexes[byte.loc.group] = byte.range_index;
            }
        }
    }

    // Sets up the the TCAM_use with the four fields output to the assembler
    for (auto group : used_groups) {
        Use::TCAM_use tcam;
        tcam.set_group(group, dirtcam.at(group));
        if (range_indexes.count(group)) {
            tcam.range_index = range_indexes.at(group);
        }
        use->tcam_use.push_back(tcam);
    }

    // In order to maintain that split ranges are continuous
    std::sort(use->tcam_use.begin(), use->tcam_use.end(),
              [](const Use::TCAM_use &a, const Use::TCAM_use &b) {
                  int t;
                  if ((t = a.range_index - b.range_index) != 0) return t > 0;
                  return a.group < b.group;
              });

    size_t index = 0;
    bitvec done_midbytes;
    // Because midbyte is shared between two TCAMs, make sure that contiguous TCAMs keep their
    // bytes together
    for (auto midbyte : used_midbytes) {
        if (!(midbyte_lo_hi[midbyte].first && midbyte_lo_hi[midbyte].second)) continue;
        for (int i = 0; i < 2; i++) {
            ternary_midbyte(midbyte, index, (i % 2) == 0);
        }
        done_midbytes.setbit(midbyte);
    }

    // Allocate all groups that have either a lo or hi section of midbyte but not both.
    // Potentially one can use this for version as well
    bool version_placed = requires_versioning() ? false : true;
    for (auto midbyte : used_midbytes) {
        if (done_midbytes.getbit(midbyte)) continue;
        bool lo = midbyte_lo_hi[midbyte].first;
        bool hi = midbyte_lo_hi[midbyte].second;
        BUG_CHECK((lo || hi) && !(lo && hi),
                  "Midbytes with both a lo and hi range should have "
                  "been handled");
        ternary_midbyte(midbyte, index, lo);
        if (!version_placed) {
            version_placed = true;
            ternary_version(index);
        } else {
            index++;
        }
    }
    if (!version_placed) ternary_version(index);

    // For corner cases, i.e. sharing midbytes, where extra gaps in the TCAMs have to be
    // added
    for (size_t i = 0; i < use->tcam_use.size(); i += 2) {
        if (use->tcam_use.size() - 1 == i) break;
        if (use->tcam_use[i].byte_group >= 0 && use->tcam_use[i + 1].byte_group >= 0 &&
            use->tcam_use[i].byte_group != use->tcam_use[i + 1].byte_group) {
            auto tcam = use->tcam_use[i];
            use->tcam_use.insert(use->tcam_use.begin() + i, tcam);
        }
    }

    if ((use->tcam_use.size() % 2) == 1) {
        use->split_midbyte = use->tcam_use.back().byte_group;
    }
    return true;
}

/**
 * As described by section 6.4.3.1 Exact Match Physical Row Result Generation, the 5 possible
 * entries per RAM line are ranked through a priority.  For exact match, the priority does not
 * matter, but for ATCAM, the lower the priority number, the higher the prioirity ranking.
 *
 * Only 2 entries can be in a different RAM than their match overhead, and those two entries
 * are the two highest priority entries.  The order in which the entries are in the match_groups
 * vector determine the ranking of these entries in the assembler.  Thus after the match bits
 * have been determined for each entry, the compiler reorders them so that the split entries
 * are the first entries in the vector.
 *
 * @sa comments on no_overhead_atcam_result_bus_words
 */

bool TableFormat::redistribute_entry_priority() {
    // This function tries to check if there exists a word such that all other
    // words are used by no more than 2 entries. If not this will return false as
    // an invalid entry format.
    // If found, the entries are then sorted such that 2 entries that use either words
    // have highest priority
    LOG3("Redistributing priority for table : " << tbl->name);
    // Populate no. of groups in each word
    dyn_vector<int> groupsPerWord;
    for (size_t idx = 0; idx < use->match_groups.size(); idx++) {
        auto entry = use->match_groups[idx];
        auto wordLo = entry.entry_min_word();
        auto wordHi = entry.entry_max_word();
        LOG3(" Entry " << idx << " word lo: " << wordLo << ", word hi: " << wordHi);
        // Note here we assume groups can only straddle adjacent words
        for (int w = wordLo; w <= wordHi; w++) groupsPerWord[w]++;
    }

    // Assign result bus word to the one having max no of groups
    auto result_bus_word = std::distance(
        groupsPerWord.begin(), std::max_element(groupsPerWord.begin(), groupsPerWord.end()));

    LOG5("  result_bus_word : " << result_bus_word);

    // Reorder groups by adding groups not on result bus word first.
    safe_vector<Use::match_group_use> reordered_match_groups;
    for (auto &g : use->match_groups) {
        if (g.entry_min_word() != result_bus_word || g.entry_max_word() != result_bus_word)
            reordered_match_groups.push_back(g);
    }

    // No. of groups not on the result bus cannot exceed max allowed (2)
    if (reordered_match_groups.size() > 2) return false;

    // Now add groups on the result bus
    for (auto &g : use->match_groups) {
        if (g.entry_min_word() == result_bus_word && g.entry_max_word() == result_bus_word)
            reordered_match_groups.push_back(g);
    }

    LOG3("Reordered priority for table : " << tbl->name);
    for (size_t idx = 0; idx < reordered_match_groups.size(); idx++) {
        auto entry = reordered_match_groups[idx];
        auto wordLo = entry.entry_min_word();
        auto wordHi = entry.entry_max_word();
        LOG3(" Entry " << idx << " word lo: " << wordLo << ", word hi: " << wordHi);
    }
    use->match_groups = reordered_match_groups;
    return true;
}

/**
 * This is implementing a constraint detailed in section 6.4.1.3.2 Next Table Bits / 6.4.3.1
 * Exact Match Physical Row Result Generation.  As described in that section, the next table
 * bits per each entry must be organized so that lower entries have lower lsbs in their next
 * table bits.
 *
 * On top of this constraint, any entry that is spread across multiple RAMs must be considered
 * entry 0 or entry 1.  When the next table is allocated, the algorithm has not yet determined
 * which entries are split across multiple RAMs.  Thus, this is to redistribute the next
 * table mapping after this information is known so that multi-ram entries have less significant
 * bit next tables.
 *
 * @sa comments over build_match_group_map for a full description.
 */
void TableFormat::redistribute_next_table() {
    int next_index;
    if (!tbl->action_chain())
        return;
    else if (hit_actions() <= NEXT_MAP_TABLE_ENTRIES)
        next_index = ACTION;
    else
        next_index = NEXT;

    safe_vector<safe_vector<size_t>> multi_ram_entries(layout_option.way.width);
    safe_vector<safe_vector<size_t>> single_ram_entries(layout_option.way.width);

    for (size_t idx = 0; idx < use->match_groups.size(); idx++) {
        auto &match_group = use->match_groups[idx];
        int overhead_position = match_group.overhead_mask().min().index() / SINGLE_RAM_BITS;
        bitvec entry_use = use->match_groups[idx].entry_info_bit_mask();
        if ((entry_use.min().index() / SINGLE_RAM_BITS) ==
            (entry_use.max().index() / SINGLE_RAM_BITS)) {
            single_ram_entries[overhead_position].push_back(idx);
        } else {
            multi_ram_entries[overhead_position].push_back(idx);
        }
    }

    for (int idx = 0; idx < layout_option.way.width; idx++) {
        safe_vector<bitvec> next_masks;

        for (auto entry : multi_ram_entries[idx]) {
            next_masks.push_back(use->match_groups[entry].mask[next_index]);
        }

        for (auto entry : single_ram_entries[idx]) {
            next_masks.push_back(use->match_groups[entry].mask[next_index]);
        }

        std::sort(next_masks.begin(), next_masks.end(), [](const bitvec &a, const bitvec &b) {
            return a.min().index() < b.min().index();
        });

        size_t next_mask_entry = 0;
        for (auto entry : multi_ram_entries[idx]) {
            use->match_groups[entry].mask[next_index] = next_masks[next_mask_entry];
            next_mask_entry++;
        }

        for (auto entry : single_ram_entries[idx]) {
            use->match_groups[entry].mask[next_index] = next_masks[next_mask_entry];
            next_mask_entry++;
        }
    }
}

/**
 * In each RAM word, one can have data for up to 5 separate table entries.  These entries,
 * which will described from hereonforth as hit-entries are numbered 0-4.  The number of entries
 * per wide RAM line will be referred to as table-entries.  The purpose of this map is to
 * coordinate table-entries to hit-entries.
 *
 * The hit-entries have two rules to follow:
 *
 * Rule #1: All of entries 2, 3, 4's match data and overhead data must reside in that RAM word
 * Entries 0, 1 can have their data spread across multiple RAM words, which is then combined
 * later in a hitmap_ixbar.  This is decribed in uArch section 6.4.3.1 Exact Match Physical Row
 * Result Generation.
 *
 * Rule #2: If the next table pointer is stored with the entry, then the next table pointer
 * has a particular range of bits where it can be extracted from.  This is described in
 * uArch section 6.4.3.1.2. Next Table Bits.  However, this description is not correct, according
 * to Mike Ferrera and the register: rams.array.row.ram.match_next_table_bitpos.  The following
 * table is the range of bits the next table pointer can live in:
 *
 *      Hit-Entry |  Overhead Range
 *          0     |    0-7, but must start at bit position 0
 *          1     |    1-15
 *          2     |    1-23 (uArch says 2-23, incorrect)
 *          3     |    1-31 (uArch says 3-31, incorrect)
 *          4     |    1-39 (uArch says 4-39, incorrect)
 *
 * This is the only example where the overhead has a minimum range based on the entry.
 * This could occasionally break the algorithm.  Currently, the allocation works so that overhead
 * is always placed before match at the lsb, and then RAM data is placed next.  Let's say I have
 * the following scenario:
 *
 * 3 table entries, 2 RAMs wide: Let's say the next table pointer for table-entries 0 and 1 are
 * in RAM word 0 and the next table pointe for table-entry 2 is in RAM word 1, all packed at the
 * lsb.  Now during the match allocation, let's say the match data for table-entries 0 and 1 are
 * in both RAM word 0 and 1, while table-entry 2 is fully in RAM word 1.  This allocation would
 * become problematic in RAM word 1.
 *
 * In RAM word 1, because table-entries 0 and 1 have some match data there, this takes up both
 * hit-entries in RAM word 0 and 1.  However, because the next table pointer of table-entry 2
 * has been allocated to the lsb of RAM word 1, starting at bit 0, this means that it cannot
 * be extracted as any entry of either hit-entry 2, 3, or 4.
 *
 * In order to not run into this scenario, the algorithm will have to potentially adjust overhead
 * position as the determination of what table-entries are spread across multiple RAMs.  Right
 * now, the algorithm will just fail in this and return false, invalidating a possible table
 * format if the algorithm could take this extra constraint into account.
 *
 ***********************************************************************************************
 *
 * The match_group_map is output as a vector of vectors in an assembly file:
 *     The 1st dimension of the vector is the RAM word
 *     The 2nd dimension of the vector is the hit-entry
 *     The values are the table entry.
 *
 * Let's say we have a 2 RAM wide 5 table entry match.  Let's say table the overhead for table
 * entries 0-2 is in RAM word 0 and table entries 3-4 is in RAM word 1.  The match data for table
 * entries 0-1 is completely in RAM word 0, the match data for table entry 3 is completely in RAM
 * word 1, and the match data for entries 2 and 4 are in both RAM word 0 and 1.
 *
 * The following would be a valid match_group_map in assembly:
 *
 * - [ [ 2, 4, 0, 1 ] , [ 4, 2, 3 ] ]
 *
 * This is because table entries 2 and 4 are shared across multiple RAM lines, and thus must be
 * hit-entries 0 and 1, while the remaining table entries, because they are stored on a single
 * RAM line, can be the higher hit-entries.
 */
bool TableFormat::build_match_group_map() {
    int next_index = -1;
    bool ntp_in_ram = true;
    if (!tbl->action_chain())
        ntp_in_ram = false;
    else if (hit_actions() <= NEXT_MAP_TABLE_ENTRIES)
        next_index = ACTION;
    else
        next_index = NEXT;

    if (ntp_in_ram) {
        // A next table pointer may not be needed even if it is an action chain, i.e. a single
        // hit action would mean only one next table is possible on hit
        ntp_in_ram &= bits_necessary(static_cast<type_t>(next_index)) > 0;
    }

    safe_vector<safe_vector<int>> entries_per_width(layout_option.way.width);
    std::set<int> wide_entries;

    int result_buses_seen = 0;
    for (int idx = 0; idx < layout_option.way.width; idx++) {
        for (int entry = result_buses_seen;
             entry < result_buses_seen + overhead_groups_per_RAM[idx]; entry++) {
            bitvec entry_overhead = use->match_groups[entry].overhead_mask();

            // If an entry does not have any match data or overhead but does use the result bus
            // of that RAM line, then that entry must be considered a hit-entry.  This is
            // especially important for ATCAM tables, where the entries must chain to the same
            // result bus in order for predication to work
            BUG_CHECK(entry_overhead.empty() ||
                          (entry_overhead.min().index() >= idx * SINGLE_RAM_BITS &&
                           entry_overhead.max().index() < idx * SINGLE_RAM_BITS + OVERHEAD_BITS),
                      "Illegal overhead entry");
            std::set<int> ram_sections;
            // Mark which entries are in which RAM sectionm and which entries are wide
            for (int i = 0; i < layout_option.way.width; i++) {
                if (use->match_groups[entry].overhead_in_RAM_word(i) ||
                    use->match_groups[entry].match_data_in_RAM_word(i))
                    ram_sections.insert(i);
            }
            for (auto sect : ram_sections) entries_per_width[sect].push_back(entry);
            if (ram_sections.size() > 1) wide_entries.insert(entry);
        }
        result_buses_seen += overhead_groups_per_RAM[idx];
    }

    result_buses_seen = 0;
    for (int idx = 0; idx < layout_option.way.width; idx++) {
        // If an entry has a next table starting at bit 0, then by definition it must be
        // hit-entry 0, even if it isn't a wide entry
        int zero_ntp_non_wide_entry = -1;
        if (ntp_in_ram) {
            for (int entry = result_buses_seen;
                 entry < result_buses_seen + overhead_groups_per_RAM[idx]; entry++) {
                if (wide_entries.count(entry)) continue;
                if (use->match_groups[entry].mask[next_index].min().index() ==
                    idx * SINGLE_RAM_BITS) {
                    zero_ntp_non_wide_entry = entry;
                    break;
                }
            }
        }

        // Entries spanning multiple RAM lines have higher priority
        std::sort(entries_per_width[idx].begin(), entries_per_width[idx].end(),
                  [&](const ssize_t &a, const ssize_t &b) {
                      int t;
                      if (a == zero_ntp_non_wide_entry && b != zero_ntp_non_wide_entry) return true;
                      if (b == zero_ntp_non_wide_entry && a != zero_ntp_non_wide_entry)
                          return false;

                      if ((t = wide_entries.count(a) - wide_entries.count(b)) != 0) return t > 0;
                      if (ntp_in_ram) {
                          // If you have two shared words, pick the shared word that is in the
                          // overhead
                          if (use->match_groups.at(a).overhead_in_RAM_word(idx) !=
                              use->match_groups.at(b).overhead_in_RAM_word(idx))
                              return use->match_groups.at(a).overhead_in_RAM_word(idx);
                          return use->match_groups.at(a).mask[next_index].min().index() <
                                 use->match_groups.at(b).mask[next_index].min().index();
                      }
                      return a < b;
                  });

        int access_to_nt_bitpos_0 = 0;
        for (auto entry : entries_per_width[idx]) {
            if (wide_entries.count(entry) > 0) access_to_nt_bitpos_0++;
        }

        if (zero_ntp_non_wide_entry >= 0) access_to_nt_bitpos_0++;

        // As described in the comments, the algorithm currently cannot safely guarantee this
        // Just returning false for now, so that the algorithm can run with a different pack
        if (access_to_nt_bitpos_0 > MAX_SHARED_GROUPS) return false;

        for (auto entry : entries_per_width[idx]) {
            if (ntp_in_ram && wide_entries.count(entry) == 0)
                BUG_CHECK(use->match_groups.at(entry).overhead_in_RAM_word(idx),
                          "Single entry "
                          "must have data in this RAM");
            // Assume that the ntp has been moved to the correct position by the
            // redistribute_next_table function
            if (ntp_in_ram && use->match_groups[entry].overhead_in_RAM_word(idx)) {
                int min_next_bit = use->match_groups[entry].mask[next_index].min().index();
                int max_next_bit = use->match_groups[entry].mask[next_index].max().index();
                int next_bit_lower_bound = use->match_group_map[idx].size() == 0 ? 0 : 1;
                next_bit_lower_bound += idx * SINGLE_RAM_BITS;
                int next_bit_upper_bound =
                    FULL_NEXT_TABLE_BITS * (use->match_group_map[idx].size() + 1) - 1;
                next_bit_upper_bound += idx * SINGLE_RAM_BITS;
                // Bounds described in the comments
                BUG_CHECK(
                    min_next_bit >= next_bit_lower_bound && max_next_bit <= next_bit_upper_bound,
                    "Next table pointers not saved correctly to entries");
            }
            use->match_group_map[idx].push_back(entry);
        }
        result_buses_seen += overhead_groups_per_RAM[idx];
    }
    return true;
}

/**
 * A gateway that is used as small match table, similar to a normal exact match
 * table, has different shiftcounts per entry.  Each table entry can be assigned
 * a RAM entry, which is what is done in build_match_group_map.
 *
 * For a gateway, the shifts are hard-coded to the gateway line that matches.
 * The four matching lines are for shifts, 3, 2, 1, and 0, while the miss entry
 * is shift 4.
 *
 * Each gateway_row in the IR corresponds to an entry, specifically each gateway row
 * corresponds to an individual match_group.  Every entry that is running a payload
 * will have a corresponding match group.
 *
 * This is to coordinate all the match_groups, which are entries running a payload action
 * with the actual RAM entry so that the exact_shiftcount registers are programmed
 * correctly.
 *
 * For instance, the miss entry may run the payload, and must always use shift 4.
 * The miss entry has null gateway row.
 */
bool TableFormat::build_payload_map() {
    std::vector<int> gateway_entry_to_table_entries = {3, 2, 1, 0, 4};
    auto gw_tbl = fpc.convert_to_gateway(tbl);
    BUG_CHECK(gw_tbl, "Building a payload map requires a gateway option");
    int index = -1;
    bool miss_seen = false;
    for (auto gw_row : gw_tbl->gateway_rows) {
        index++;
        if (gw_row.second.isNull()) continue;
        if (gw_row.first) {
            BUG_CHECK(!miss_seen,
                      "A miss entry on a table has a higher priority than "
                      "any other entry on table %1%",
                      tbl->externalName());
            use->payload_map[gateway_entry_to_table_entries.at(index)] = index;
        } else {
            miss_seen = true;
            use->payload_map[4] = index;
        }
    }
    return true;
}

int ByteInfo::hole_size(HoleType_t hole_type, int *hole_start_pos) const {
    if (bit_use.empty()) return 0;
    if (hole_type == LSB) {
        if (hole_start_pos) *hole_start_pos = 0;
        return bit_use.min().index();
    } else if (hole_type == MSB) {
        if (hole_start_pos) *hole_start_pos = bit_use.max().index() + 1;
        return 7 - bit_use.max().index();
    } else if (hole_type == MIDDLE) {
        int max_hole = 0;
        int lsb_hole_end = bit_use.ffs();
        int hole_start = bit_use.ffz(lsb_hole_end);
        int hole_end = bit_use.ffs(hole_start);
        while (hole_end >= 0) {
            if (max_hole < hole_end - hole_start) {
                if (hole_start_pos) *hole_start_pos = hole_start;
                max_hole = std::max(hole_end - hole_start, max_hole);
            }
            hole_start = bit_use.ffz(hole_end);
            hole_end = bit_use.ffs(hole_start);
        }
        return max_hole;
    }
    return -1;
}

/**
 * Return the better hole for minimizing byte requirements for packing
 *     Least number of bytes in the il_info.byte_cycle
 *     Least number
 */
bool ByteInfo::better_hole_type(int hole, int comp_hole, int overhead_bits) const {
    // Return the actual hole
    if (hole != 0 && comp_hole == 0) return true;
    if (hole == 0 && comp_hole != 0) return false;

    // The bits used by the match byte and the overhead
    int a_bits_used = overhead_bits + (8 - hole);
    int b_bits_used = overhead_bits + (8 - comp_hole);

    // The lower number of bytes used for the bytes
    int a_bytes_used = (a_bits_used + 7) / 8;
    int b_bytes_used = (b_bits_used + 7) / 8;

    if (a_bytes_used != b_bytes_used) return a_bytes_used < b_bytes_used;

    // Return the byte that uses the overlap best
    return a_bits_used > b_bits_used;
}

/**
 * Determine which byte is better for overhead, based on either of their LSB or MSB holes
 */
bool ByteInfo::is_better_for_overhead(const ByteInfo &bi, int overhead_bits) const {
    int a_middle_hole_size = hole_size(MIDDLE);
    int b_middle_hole_size = bi.hole_size(MIDDLE);

    if (a_middle_hole_size >= overhead_bits && b_middle_hole_size < overhead_bits) {
        return true;
    } else if (a_middle_hole_size < overhead_bits && b_middle_hole_size >= overhead_bits) {
        return false;
    } else if (a_middle_hole_size >= overhead_bits && b_middle_hole_size >= overhead_bits) {
        return a_middle_hole_size < b_middle_hole_size;
    }

    // definitely the best a**hole I know
    HoleType_t best_a_hole =
        better_hole_type(hole_size(LSB), hole_size(MSB), overhead_bits) ? LSB : MSB;

    HoleType_t best_b_hole =
        better_hole_type(bi.hole_size(LSB), bi.hole_size(MSB), overhead_bits) ? LSB : MSB;

    return better_hole_type(hole_size(best_a_hole), bi.hole_size(best_b_hole), overhead_bits);
}

/**
 * Set the information for allocating the interleaved byte.
 * \sa comments on ByteInfo for the definitions of these values
 */
void ByteInfo::set_interleave_info(int overhead_bits) {
    il_info.interleaved = true;
    int middle_hole_start = 0;
    int middle_hole_size = hole_size(MIDDLE, &middle_hole_start);
    // Overhead would start at the beginning of the largest hole
    if (middle_hole_size >= overhead_bits) {
        il_info.overhead_start = middle_hole_start;
        il_info.match_byte_start = 0;
        il_info.byte_cycle = 1;
        return;
    }

    int lsb_hole_start;
    int msb_hole_start = 0;
    HoleType_t best_hole = better_hole_type(hole_size(LSB, &lsb_hole_start),
                                            hole_size(MSB, &msb_hole_start), overhead_bits)
                               ? LSB
                               : MSB;

    // Hole for overhead is at the LSB of the byte
    if (best_hole == LSB) {
        // Overhead can go all the way to the first match bit
        int lsb_first_match_bit_of_byte = 8 - hole_size(LSB);
        il_info.byte_cycle = (lsb_first_match_bit_of_byte + overhead_bits + 7) / 8;
        il_info.match_byte_start = (il_info.byte_cycle - 1) * 8;
        il_info.overhead_start = il_info.match_byte_start + hole_size(LSB) - overhead_bits;
    } else {
        // Overhead starts at the first bit of the hole
        il_info.match_byte_start = 0;
        il_info.overhead_start = msb_hole_start;
        il_info.byte_cycle = (msb_hole_start + overhead_bits + 7) / 8;
    }
}

/**
 * The premise of this function is to potentially interleave overhead data with match data.
 * The original approach of the interleaving was to pack all overhead data closest to the least
 * significant bit, and then pack the match data above this data.  However, though this strategy
 * works fairly well, by expanding the search space, the actual allocation can be improved.
 * This generally can be taken advantage of especially when no bits are ghosted.
 *
 * Let's take the following example:
 *     - 2 entries, 1 RAM wide
 *     - Each entry has 4 bits of overhead
 *     - Each entry has 8 match bytes, 2 of which are matching on a single nibble, let's say the
 * lower
 *     - Each entry requires a nibble for version/validity
 * Could be thought of as an ATCAM entry with 28 bits of a ternary match key and some number of
 * actions
 *
 * If we were to pack the overhead all at the lsb, this would be definition take up an entire byte,
 * and thus because 16 match bytes are needed, this would never fit.  However, because the overhead
 * and a match byte can be combined into a single byte requirement, this now becomes possible if
 * match data and overhead are interleaved.
 *
 * Right now, overhead is still allocated as one step.  One could take this even further, and
 * break up overhead into its constituent pieces, but a single block of overhead is easiest to
 * consider.
 *
 * Also the current basic assumption is that the interleaved byte both doesn't have a hole at both
 * the msb and the lsb, but that in theory could still save bits if this corner case is found
 */
bool TableFormat::interleave_match_and_overhead() {
    if (tbl->action_chain() || bits_necessary(SEL_LEN_MOD) > 0 || bits_necessary(SEL_LEN_SHIFT) > 0)
        return false;

    int overhead_bits = overhead_bits_necessary();
    if (overhead_bits == 0) return false;
    clear_pre_allocation_state();
    classify_match_bits();

    // Only makes sense to interleave bytes that share RAMs with overhead
    int search_bus_with_overhead = -1;
    for (size_t i = 0; i < overhead_groups_per_RAM.size(); i++) {
        if (overhead_groups_per_RAM[i] == 0) continue;
        if (search_bus_with_overhead == -1) {
            search_bus_with_overhead = search_bus_per_width[i];
        } else if (search_bus_with_overhead != search_bus_per_width[i]) {
            return false;
        }
    }

    if (search_bus_with_overhead < 0) return false;

    // Determine the best byte to interleave with overhead
    ssize_t best_entry_index = -1;
    for (size_t i = 0; i < match_bytes.size(); i++) {
        if (match_bytes[i].byte.search_bus != search_bus_with_overhead) continue;
        if (best_entry_index < 0) {
            best_entry_index = i;
            continue;
        }

        if (match_bytes[i].is_better_for_overhead(match_bytes[best_entry_index], overhead_bits)) {
            best_entry_index = i;
        }
    }

    if (best_entry_index < 0) return false;

    // If the byte has no holes, then this byte is generally not useful
    ByteInfo *il_byte = &match_bytes[best_entry_index];
    if (il_byte->bit_use.popcount() == 8)
        return false;
    else if (il_byte->bit_use.min().index() == 0 && il_byte->bit_use.max().index() == 7 &&
             il_byte->hole_size(ByteInfo::MIDDLE) < overhead_bits)
        return false;

    il_byte->set_interleave_info(overhead_bits);
    LOG5("\t  Interleaved byte candidate " << il_byte);

    // Reserve space for the match bytes and allocate the overhead to the groups.  All match bytes
    // are actually reserved during allocate_match_with_algorithm due to internal structures
    // necessary to maintain in that function
    int entry = 0;
    for (size_t i = 0; i < overhead_groups_per_RAM.size(); i++) {
        int start_byte = 0;
        for (int j = 0; j < overhead_groups_per_RAM[i]; j++) {
            if (!allocate_overhead_entry(entry, i,
                                         start_byte * 8 + il_byte->il_info.overhead_start))
                return false;
            int match_bit_start = start_byte * 8 + il_byte->il_info.match_byte_start;
            match_bit_start += (i * SINGLE_RAM_BITS);
            BUG_CHECK(match_bit_start % 8 == 0, "Interleaved match byte is not correct");
            interleaved_bit_use |= (il_byte->bit_use << match_bit_start);
            interleaved_match_byte_use.setbit(match_bit_start / 8);
            BUG_CHECK((total_use & interleaved_bit_use).empty(),
                      "Interleaving allocation is "
                      "incorrect");
            start_byte += il_byte->il_info.byte_cycle;
            entry++;
        }
    }

    pa = PACK_TIGHT;
    return allocate_match_with_algorithm();
}

/* This is a verification pass that guarantees that we don't have overlap.  More constraints can
   be checked as well.  */
void TableFormat::verify() {
    bitvec verify_mask;
    bitvec on_search_bus_mask;

    for (int i = 0; i < layout_option.way.match_groups; i++) {
        for (int j = NEXT; j <= INDIRECT_ACTION; j++) {
            if (!use->match_groups[i].mask[j].empty()) {
                if ((verify_mask & use->match_groups[i].mask[j]).popcount() != 0) {
                    BUG("Overlap of multiple things in the format");
                    verify_mask |= use->match_groups[i].mask[j];
                }
                if (j == VERS) on_search_bus_mask |= use->match_groups[i].mask[j];
            } else if (j == VERS && requires_versioning()) {
                BUG("A group has been allocated without version bits");
            }
        }
    }

    for (int i = 0; i < layout_option.way.match_groups; i++) {
        for (auto byte_info : use->match_groups[i].match) {
            bitvec &byte_mask = byte_info.second;
            if ((verify_mask & byte_mask).popcount() != 0)
                BUG("Overlap of a match byte in the format");
            verify_mask |= byte_mask;
            on_search_bus_mask |= byte_mask;
        }
    }

    for (int i = 0; i < layout_option.way.width; i++) {
        for (int j = 0; j < GATEWAY_BYTES * 2; j++) {
            if (on_search_bus_mask.getrange(i * SINGLE_RAM_BITS + j * 8, 8)) continue;
            use->avail_sb_bytes.setbit(i * SINGLE_RAM_BYTES + j);
        }
    }
}

TableFormat *TableFormat::create(const LayoutOption &l, const IXBar::Use *mi, const IXBar::Use *phi,
                                 const IR::MAU::Table *t, const bitvec im, bool gl,
                                 FindPayloadCandidates &fpc, const PhvInfo &phv) {
    return new TableFormat(l, mi, phi, t, im, gl, fpc, phv);
}

std::ostream &operator<<(std::ostream &out, const TableFormat::Use::match_group_use &m) {
    out << "Match Group Use: [";
    out << " mask: " << *m.mask;
    out << ", match_byte_mask: " << m.match_byte_mask;
    out << ", allocated_bytes: " << m.allocated_bytes;
    out << " ]" << Log::endl;
    out << "\t(";
    for (auto &byte : m.match) {
        out << " " << byte.first << " : " << byte.second << " ";
    }
    out << " )";
    return out;
}
