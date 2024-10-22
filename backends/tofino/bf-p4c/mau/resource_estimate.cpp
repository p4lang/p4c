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

#include "resource_estimate.h"

#include "hash_mask_annotations.h"
#include "lib/bitops.h"
#include "lib/log.h"
#include "memories.h"
#include "table_placement.h"

constexpr int RangeEntries::MULTIRANGE_DISTRIBUTION_LIMIT;
constexpr int RangeEntries::RANGE_ENTRY_PERCENTAGE;
constexpr int StageUseEstimate::MAX_DLEFT_HASH_SIZE;

constexpr int StageUseEstimate::COMPILER_DEFAULT_SELECTOR_POOLS;
constexpr int StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE;
constexpr int StageUseEstimate::MAX_MOD;
constexpr int StageUseEstimate::MAX_POOL_RAMLINES;
constexpr int StageUseEstimate::MAX_MOD_SHIFT;

int CounterPerWord(const IR::MAU::Counter *ctr) {
    switch (ctr->type) {
        case IR::MAU::DataAggregation::PACKETS:
            if (ctr->min_width <= 32) return 4;
            if (ctr->min_width > 64)
                error("%s: Maximum width for counter %s is 64 bits", ctr->srcInfo, ctr->name);
            return 2;
        case IR::MAU::DataAggregation::BYTES:
            if (ctr->min_width <= 32) return 4;
            if (ctr->min_width > 64)
                error("%s: Maximum width for counter %s is 64 bits", ctr->srcInfo, ctr->name);
            return 2;
        case IR::MAU::DataAggregation::BOTH:
            // min_width refers to the byte counter at a given index.
            // 1 "packet_and_byte" entry gives 64-bit packet counter and 64-bit byte counter.
            // 2 "packet_and_byte" entries gives 28-bit packet counter and 36-bit byte counter.
            // Hardware also supports 3-entries, but compiler does not (harder to assign VPNs).
            // 3 "packet_and_byte" entries gives 17-bit packet counter and 25-bit byte counter.
            if (ctr->min_width <= 36) return 2;
            if (ctr->min_width > 64)
                error("%s: Maximum width for byte counter %s is 64 bits", ctr->srcInfo, ctr->name);
            return 1;
        default:
            error("%s: No counter type for %s", ctr->srcInfo, ctr->name);
            return 1;
    }
}

int CounterWidth(const IR::MAU::Counter *cntr) {
    switch (cntr->type) {
        case IR::MAU::DataAggregation::PACKETS:
            if (cntr->min_width <= 32) return 32;
            return 64;
        case IR::MAU::DataAggregation::BYTES:
            if (cntr->min_width <= 32) return 32;
            return 64;
        case IR::MAU::DataAggregation::BOTH:
            // The min_width attribute is with respect to the byte counter.
            if (cntr->min_width <= 36) return 36;
            return 64;
        default:
            error("%s: No counter type for %s", cntr->srcInfo, cntr->name);
            return 1;
    }
}

int RegisterPerWord(const IR::MAU::StatefulAlu *reg) {
    BUG_CHECK(reg->width > 0 && (reg->width & (reg->width - 1)) == 0,
              "register width not a power of two");
    return 128 / reg->width;
}

int ActionDataPerWord(const IR::MAU::Table::Layout *layout, int *width) {
    int size = 0;
    if (layout->action_data_bytes_in_table > 0)
        size = ceil_log2(layout->action_data_bytes_in_table);
    if (size > 4) {
        *width = (layout->action_data_bytes_in_table / 16);
        return 1;
    }
    return 16 >> size;
}

/** Information necessary for the calculation of VPN addressing on action data tables.  This
 *  comes from section 6.2.8.4.3 Action RAM Addressing.
 *
 *  Essentially action table addresses have different sizes that are programmed through the
 *  Huffman encoding bits.  The address itself has 5 bits pre-pended to the lower portion
 *  of the address to indicate the action data size.  However, for entries of 256, 512, or 1024
 *  bit width, 6, 7, and 8 bits respectively are required to indicate what the encoding is.
 *  These bits are contained in the upper part of the address, and thus need to be part of
 *  the address that comes from match overhead.
 *
 *  Furthermore the encoding scheme affects the VPN allocation of the action data tables, as
 *  the Huffman Encoding appears in the VPN space of the action data table.  The increment is
 *  the difference between VPN per action data RAM.  The offset is the address within the
 *  VPN space for that table type.
 *
 *  ___Action_Data_Bits___|___VPN_Increment___|___VPN_Offset___
 *           256          |         2         |        0
 *           512          |         4         |        1
 *          1024          |         8         |        3
 */
int ActionDataHuffmanVPNBits(const IR::MAU::Table::Layout *layout) {
    int size = 0;
    if (layout->action_data_bytes_in_table > 0)
        size = ceil_log2(layout->action_data_bytes_in_table);
    int huffman_not_req_max_size = 4;
    return std::max(size - huffman_not_req_max_size, 0);
}

int ActionDataVPNStartPosition(const IR::MAU::Table::Layout *layout) {
    int size = 0;
    if (layout->action_data_bytes_in_table > 0)
        size = ceil_log2(layout->action_data_bytes_in_table);
    switch (size) {
        case 6:
            return 1;
        case 7:
            return 3;
        default:
            return 0;
    }
}

int ActionDataVPNIncrement(const IR::MAU::Table::Layout *layout) {
    int width = 1;
    ActionDataPerWord(layout, &width);
    if (width == 1) return 1;
    return (1 << ceil_log2(width));
}

int LocalTernaryIndirectPerWord(const IR::MAU::Table::Layout *layout, const IR::MAU::Table *tbl) {
    int indir_size = ceil_log2(layout->overhead_bits);
    if (indir_size > 5)
        BUG("can't have more than 32 bits of overhead in local ternary table %s", tbl->name);
    return (64 >> indir_size);
}

int TernaryIndirectPerWord(const IR::MAU::Table::Layout *layout, const IR::MAU::Table *tbl) {
    int indir_size = ceil_log2(layout->overhead_bits);
    if (indir_size > 6)
        BUG("can't have more than 64 bits of overhead in ternary table %s", tbl->name);
    if (indir_size < 3) indir_size = 3;
    return (128 >> indir_size);
}

int IdleTimePerWord(const IR::MAU::IdleTime *idletime) {
    switch (idletime->precision) {
        case 1:
            return 8;
        case 2:
            return 4;
        case 3:
            return 2;
        case 6:
            return 1;
        default:
            BUG("%s: Invalid idletime precision %s", idletime->precision);
    }
}

/**
 * Refer to the comments above the allocate_selector_length function in table_format.cpp
 */
int SelectorRAMLinesPerEntry(const IR::MAU::Selector *sel) {
    return (sel->max_pool_size + StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE - 1) /
           StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE;
}

/**
 * Here is the simple python code to generate the sel_len_mod and sel_len_shift
 * for all cases as agreed upon with driver team (Steve Licking and Pawel Kaminski).
 * The corresponding c++ implementation is used in compiler.
 *
  #!/bin/python
  import math

  ram = [1, 2, 3, 5, 9, 17, 31, 32, 34, 62, 64, 68,
         124, 128, 136, 248, 256, 272, 496, 512, 544, 992]

  for r in ram:
      sel_shift = 0
      while (r >> sel_shift) > 31:
          sel_shift = sel_shift + 1
      hash=0x123456789
      word_idx_1=(hash & 0x3ff) % (r >> sel_shift)
      word_idx_2=((word_idx_1 << sel_shift) | (hash >> 10)) & ((1 << sel_shift) - 1)
      sel_len_mod = math.ceil(math.log2((r>>sel_shift) + 1))
      if (r == 1):
          sel_len_mod = 0
      sel_len_shift = math.ceil(math.log2(sel_shift + 1)) if sel_shift != 0 else 0
      print("ram_word=", r, "\tsel_shift=", hex(sel_shift), "\tsel_len_shift=", hex(sel_len_shift),
            "\tsel_len_mod=", hex(sel_len_mod))

   Running the above script generates the following table for sel_len_shift and sel_len_mod for
   all edges cases, pasted here for future references. In particular, the second column is
   documented in uarch 6.2.8.4.7 figure 6.38 and 6.39. The third column is needed by driver
   as the length of sel_shift bits to reserve enough space to program all possible values for
   column 2.

   ram_word= 1     sel_shift= 0x0  sel_len_shift= 0x0      sel_len_mod= 0x0
   ram_word= 2     sel_shift= 0x0  sel_len_shift= 0x0      sel_len_mod= 0x2
   ram_word= 3     sel_shift= 0x0  sel_len_shift= 0x0      sel_len_mod= 0x2
   ram_word= 5     sel_shift= 0x0  sel_len_shift= 0x0      sel_len_mod= 0x3
   ram_word= 9     sel_shift= 0x0  sel_len_shift= 0x0      sel_len_mod= 0x4
   ram_word= 17    sel_shift= 0x0  sel_len_shift= 0x0      sel_len_mod= 0x5
   ram_word= 31    sel_shift= 0x0  sel_len_shift= 0x0      sel_len_mod= 0x5
   ram_word= 32    sel_shift= 0x1  sel_len_shift= 0x1      sel_len_mod= 0x5
   ram_word= 34    sel_shift= 0x1  sel_len_shift= 0x1      sel_len_mod= 0x5
   ram_word= 62    sel_shift= 0x1  sel_len_shift= 0x1      sel_len_mod= 0x5
   ram_word= 64    sel_shift= 0x2  sel_len_shift= 0x2      sel_len_mod= 0x5
   ram_word= 68    sel_shift= 0x2  sel_len_shift= 0x2      sel_len_mod= 0x5
   ram_word= 124   sel_shift= 0x2  sel_len_shift= 0x2      sel_len_mod= 0x5
   ram_word= 128   sel_shift= 0x3  sel_len_shift= 0x2      sel_len_mod= 0x5
   ram_word= 136   sel_shift= 0x3  sel_len_shift= 0x2      sel_len_mod= 0x5
   ram_word= 248   sel_shift= 0x3  sel_len_shift= 0x2      sel_len_mod= 0x5
   ram_word= 256   sel_shift= 0x4  sel_len_shift= 0x3      sel_len_mod= 0x5
   ram_word= 272   sel_shift= 0x4  sel_len_shift= 0x3      sel_len_mod= 0x5
   ram_word= 496   sel_shift= 0x4  sel_len_shift= 0x3      sel_len_mod= 0x5
   ram_word= 512   sel_shift= 0x5  sel_len_shift= 0x3      sel_len_mod= 0x5
   ram_word= 544   sel_shift= 0x5  sel_len_shift= 0x3      sel_len_mod= 0x5
   ram_word= 992   sel_shift= 0x5  sel_len_shift= 0x3      sel_len_mod= 0x5
 */

int SelectorModBits(const IR::MAU::Selector *sel) {
    int RAM_lines_necessary = SelectorRAMLinesPerEntry(sel);
    if (RAM_lines_necessary <= StageUseEstimate::MAX_MOD) {
        auto sel_num_words = ceil_log2((RAM_lines_necessary >> SelectorShiftBits(sel)) + 1);
        if (RAM_lines_necessary == 1) sel_num_words = 0;
        return sel_num_words;
    }
    return ceil_log2(StageUseEstimate::MAX_MOD);
}

int SelectorShiftBits(const IR::MAU::Selector *sel) {
    int RAM_lines_necessary = SelectorRAMLinesPerEntry(sel);
    if (RAM_lines_necessary > StageUseEstimate::MAX_POOL_RAMLINES) {
        error(
            "%s: The max pools size %d of selector %s requires %d RAM lines, more than maximum "
            "%d RAM lines possibly on Barefoot hardware",
            sel->srcInfo, sel->max_pool_size, sel->name, RAM_lines_necessary,
            StageUseEstimate::MAX_POOL_RAMLINES);
    }

    int sel_shift = 0;
    while ((RAM_lines_necessary >> sel_shift) > StageUseEstimate::MAX_MOD) {
        sel_shift++;
    }
    return sel_shift;
}

int SelectorHashModBits(const IR::MAU::Selector *sel) {
    int RAM_lines_necessary = SelectorRAMLinesPerEntry(sel);
    if (RAM_lines_necessary == 1) {
        return 0;
    }
    return SelectorShiftBits(sel) + StageUseEstimate::MOD_INPUT_BITS;
}

/* see comments above */
int SelectorLengthShiftBits(const IR::MAU::Selector *sel) {
    int sel_shift = SelectorShiftBits(sel);
    return sel_shift == 0 ? 0 : ceil_log2(sel_shift + 1);
}

int SelectorLengthBits(const IR::MAU::Selector *sel) {
    return SelectorModBits(sel) + SelectorLengthShiftBits(sel);
}

static int ways_pragma(const IR::MAU::Table *tbl, int min, int max) {
    auto annot = tbl->match_table->getAnnotations();
    if (auto s = annot->getSingle("ways"_cs)) {
        ERROR_CHECK(s->expr.size() >= 1,
                    "%s: The ways pragma on table %s does not have a "
                    "value",
                    tbl->srcInfo, tbl->name);
        auto pragma_val = s->expr.at(0)->to<IR::Constant>();
        if (pragma_val == nullptr) {
            error("%s: The ways pragma value on table %s is not a constant", tbl->srcInfo,
                  tbl->name);
            return -1;
        }
        int rv = pragma_val->asInt();
        if (rv < min || rv > max) {
            warning(
                "%s: The ways pragma value on table %s is not between %d and %d, and "
                "will be ignored",
                tbl->srcInfo, tbl->name, min, max);
            return -1;
        }
        return rv;
    }
    return -1;
}

static int simul_lookups_pragma(const IR::MAU::Table *tbl, int min, int max) {
    auto annot = tbl->match_table->getAnnotations();
    if (auto s = annot->getSingle("simul_lookups"_cs)) {
        ERROR_CHECK(s->expr.size() >= 1,
                    "%s: The simul_lookups pragma on table %s does not "
                    "have a value",
                    tbl->srcInfo, tbl->name);
        auto pragma_val = s->expr.at(0)->to<IR::Constant>();
        if (pragma_val == nullptr) {
            error("%s: The simul_lookups pragma value on table %s is not a constant", tbl->srcInfo,
                  tbl->name);
            return -1;
        }
        int rv = pragma_val->asInt();
        if (rv < min || rv > max) {
            warning(
                "%s: The simul_lookups pragma value on table %s is not between %d and %d, "
                "and will be ignored",
                tbl->srcInfo, tbl->name, min, max);
            return -1;
        }
        return rv;
    }
    return -1;
}

// for logging messages -- which resource ran out
cstring StageUseEstimate::ran_out() const {
    if (logical_ids > StageUse::MAX_LOGICAL_IDS) return "logical_ids"_cs;
    if (srams > StageUse::MAX_SRAMS) return "srams"_cs;
    if (tcams > StageUse::MAX_TCAMS) return "tcams"_cs;
    if (local_tinds > MAX_LOCAL_TINDS) return "local_tinds"_cs;
    if (maprams > StageUse::MAX_MAPRAMS) return "maprams"_cs;
    // For exact and ternary ixbar allocation, tables can share ixbar bytes.
    // However stage use estimate simply adds the ixbar bytes on the tables and
    // does not account for any sharing (see StageUseEstimate operator+=).
    // Therefore, we can have a valid allocation where stage estimate for ixbar
    // byte usage exceeds max allowed bytes but is still valid because of shared
    // bytes.
    // We remove the check here to ensure we dont fail for these scenarios. The
    // TablePlacement::try_alloc_all (where this code is called from) will fail
    // while picking layout options in case the xbar byte usage does exceed
    // (with/without sharing), hence the failing case is considered and no
    // invalid allocation occurs.
    // TBD: The estimate can be made more robust to account for shared
    // bytes and calculate the correct ixbar bytes and re-enable this check.
    // if (exact_ixbar_bytes > StageUse::MAX_IXBAR_BYTES) return "exact ixbar";
    // if (ternary_ixbar_groups > StageUse::MAX_TERNARY_GROUPS) return "ternary ixbar";
    if (meter_alus > MAX_METER_ALUS) return "meter_alus"_cs;
    if (stats_alus > MAX_STATS_ALUS) return "stats_alus"_cs;
    return cstring();
}

/**
 * There are now two support pragmas, ways and simul_lookups.  For an SRAM based table that uses
 * cuckoo hashing, multiple RAMs are looked up simultaneously, each accessed by a different
 * hash function.  The number of ways is the number of simultaneous lookup.  Each way corresponds
 * to a single hash function provided by the 52 bit hash bus.
 *
 * The difference in the meaning is the following:
 *
 *      ways - Each hash function must be entirely independent, i.e. cannot use the same
 *          hash bits
 *      simul_lookups - simultaneous lookups can use the same hash bits, an optimization
 *          supported only in Brig.  Really, one can think of this as making an
 *          individual way deeper
 *
 * simul_lookups is only supported internally at this point, and is necessary to make progress
 * on power.p4
 */
bool StageUseEstimate::ways_provided(const IR::MAU::Table *tbl, LayoutOption *lo,
                                     int &calculated_depth) {
    int ways = ways_pragma(tbl, MIN_WAYS, MAX_WAYS);
    int simul_lookups = simul_lookups_pragma(tbl, MIN_WAYS, MAX_WAYS + 2);

    if (ways == -1 && simul_lookups == -1) return false;

    bool independent_hash = ways != -1;
    int way_total = ways != -1 ? ways : simul_lookups;

    if (way_total == 1 && can_be_identity_hash(tbl, lo, calculated_depth)) return true;

    int depth_needed = std::min(calculated_depth, int(StageUse::MAX_SRAMS));
    int depth = 0;

    for (int i = 0; i < way_total; i++) {
        if (depth_needed <= 0) depth_needed = 1;
        int initial_way_size = (depth_needed + (way_total - i) - 1) / (way_total - i);
        int log2_way_size = 1 << (ceil_log2(initial_way_size));
        depth_needed -= log2_way_size;
        depth += log2_way_size;
        lo->way_sizes.push_back(log2_way_size);
    }

    int avail_hash_bits = Tofino::IXBar::get_hash_single_bits();
    if (independent_hash) {
        int select_upper_bits_required = 0;
        int index = 0;
        for (auto way_size : lo->way_sizes) {
            if (index == Device::ixbarSpec().hashIndexGroups()) {
                lo->select_bus_split = index;
                break;
            }
            int select_bits = floor_log2(way_size);
            if (select_bits + select_upper_bits_required > avail_hash_bits) {
                lo->select_bus_split = index;
                break;
            }
            select_upper_bits_required += floor_log2(way_size);
            index++;
        }
    } else {
        int select_upper_bits_required = 0;
        int way_index = 0;
        for (auto way_size : lo->way_sizes) {
            if (way_index == Device::ixbarSpec().hashIndexGroups()) break;
            select_upper_bits_required += floor_log2(way_size);
            if (select_upper_bits_required > avail_hash_bits) {
                lo->select_bus_split = way_index;
                break;
            }
            way_index++;
        }
    }

    calculated_depth = depth;
    return true;
}

/**
 * An optimization for an exact match table.
 *
 * If a key is under a certain number of bits, instead of using a random hash of that key
 * to find the position, an identity can be used instead.  This makes sense for keys
 * 10 bits or less, as an identity hash would just source to an individual RAM line.
 *
 * If one was to have, for example, a 12 bit key, this support is possible.  If 4 entries
 * fit per RAM line, then by using an identity hash, each entry can fit within a single RAM.
 *
 * The driver, however, is limited in its current support.  The driver uses a reserved entry as
 * the miss entry.  This miss entry will be used if the table ever misses.  If the miss-entry
 * has action data requirements or potentially stateful requirements, then those entries must be
 * stored.
 *
 * The driver always programs the miss entry at the highest address, meaning that if an identity
 * address is used, if the table requires a miss-entry, then the all 1 field will collide with
 * the miss-entry.
 *
 * This could be fixed by a dynamic miss-entry.  If the miss-entry could move to
 * any open miss-entry, then all of these tables could support this identity hash.  If the table
 * was to ever fill all entries, then by definition, the table could never miss.
 *
 * The current limitations are if a direct resource is required.  This will reserve the all 0
 * miss-entry, no matter what.
 *
 * In the future, when this is supported, a table with a direct resource can still use this
 * identity optimization if and only if the miss-entry never uses that resource, which is
 * a more complex check, but not hard to add
 *
 * UPDATE:
 * Based on driver fixes, driver checks if the EXM table
 * requires a table location to be reserved for the default (miss) entry.
 * Originally the check was simply whether or not the table used direct
 * resources (action, idle, counter, meter, stful). Now, the check is whether
 * any of the possible default actions use the direct resources.
 *
 * With this change, compiler check will mimic driver checks and use identity
 * hash for cases where default actions do not have an attached resource.
 */
bool StageUseEstimate::can_be_identity_hash(const IR::MAU::Table *tbl, LayoutOption *lo,
                                            int &calculated_depth) {
    // Check if idletime is used on table
    if (tbl->uses_idletime()) return false;

    // Check if default action(s) have a direct resource
    // See comments in UPDATE above
    bool default_action_has_attached_resource = false;
    for (auto act : Values(tbl->actions)) {
        if (act->default_allowed || act->is_constant_action) {
            if (!act->stateful_calls.empty()) {
                default_action_has_attached_resource = true;
                break;
            }
            // Table does not have any resources node populated at this stage,
            // we use the action format referred to by the action_format_index
            auto &format = action_formats[lo->action_format_index];
            if (format.if_action_has_action_data_table(act->name)) {
                default_action_has_attached_resource = true;
                break;
            }
        }
        if (act->is_constant_action) break;
    }

    if (default_action_has_attached_resource) return false;

    int extra_identity_bits =
        lo->layout.ixbar_width_bits - hash_bits_masked - ceil_log2(lo->layout.get_sram_depth());

    // Generally the limit becomes around 18 bits: 4 way pack plus 64 bits of entries
    if (extra_identity_bits <= 8) {
        int entries_needed = (1 << extra_identity_bits);
        int RAMs_needed = (entries_needed + lo->way.match_groups - 1) / lo->way.match_groups;
        RAMs_needed = 1 << ceil_log2(RAMs_needed);
        // Minimum way number is generally 4

        if (RAMs_needed <= calculated_depth) {
            lo->way_sizes = {RAMs_needed};
            lo->identity = true;
            calculated_depth = RAMs_needed;
            LOG3("\t Table " << tbl->name << " can have an identity hash");
            return true;
        }
    }
    return false;
}

/** This calculates the number of simultaneous lookups within an exact match table, using the
 *  cuckoo hashing.  The RAM selection is done through using particular bits on the 52 bit
 *  hash bus.  The lower 40 bits are broken into 4 10 bit sections for RAM line selection,
 *  and the upper 12 bits are used to do a RAM select.
 *
 *  In order to fit as at least 90% of entries without having to move other match entries,
 *  generally 4 ways are required for complete independent lookup.  Thus, if the entries
 *  requested for the table is smaller than a particular number, the algorithm will still
 *  bump the number of entries up in order to maintain this number of independent ways.
 *
 *  Let me provide the following example.  Say that the number of entries for a particular
 *  table requires 4 independent ways of size 8.  The hash bus would be allocated as
 *  the following:
 *    - Each of the 4 independent ways would each have a separate 10 bits of RAM row select,
 *      totalling 40 bits
 *    - To select a distinct RAM out of the 8 ways, each way would require 3 bits of RAM
 *      select, totalling 12 bits.
 *  This totals to 52 bits, which fortunately is the size of the number of hash select bits
 *
 *  An optimization that I take advantage of is the fact that I can repeat using of select bits.
 *  For example, say the number of entries required 40 RAMs.  One could in theory break this
 *  up into 5 independent ways of 8 RAMs.  However, this would not fit onto the 52 bits, as
 *  50 bits of RAM row select + 15 bits of RAM select, way larger than the 52 bits on a hash
 *  select bus.
 *
 *  However, the compiler will optimize so that way 1 and way 5 will actually share the 10
 *  bits of RAM row select, and the 3 upper bits of RAM select.  This means that ways 1 and 5
 *  are not independent, instead they are the exact same.  However, this is not an issue for
 *  our constraint, as we still have 4 independent hash lookups.
 *
 *  This cannot be used indefinitely however.  For example, say we needed 64 RAMs, with 4
 *  ways of 16 RAMs.  Even though we can fit all RAM row selection in the lower 40 bits, this
 *  would require 16 bits of RAM select.  In this case, we cannot repeat the use select bits
 *  as this would not provide at least 4 independent hash lookups, which is the standard
 *  required by the driver.
 *
 *  In the case just described, we would actually require 2 separate RAM select buses, and thus
 *  two separate search buses.  The fortunate thing is that the maximum number of RAMs is 80
 *  per MAU stage, so even the input xbar requirements are high, the RAM array requirements
 *  are high as well.
 */
void StageUseEstimate::calculate_way_sizes(const IR::MAU::Table *tbl, LayoutOption *lo,
                                           int &calculated_depth) {
    Log::TempIndent indent;
    LOG5("Calculating way sizes for table : " << tbl->name << ", initial calculated_depth: "
                                              << calculated_depth << indent);
    if (ways_provided(tbl, lo, calculated_depth)) return;

    // This indicates that we are using identity function.
    if ((lo->layout.ixbar_width_bits - hash_bits_masked) < ceil_log2(lo->layout.get_sram_depth()) &&
        !lo->layout.proxy_hash) {
        if (calculated_depth == 1) {
            lo->way_sizes = {1};
            lo->identity = true;
            return;
        }
    }

    if (calculated_depth <= 3) {
        if (lo->way.match_groups > 1 && calculated_depth <= 2)
            calculated_depth = 2;
        else
            calculated_depth = 3;
    }

    if (calculated_depth < 4) {
        // FIXME -- current must-pass testcases have been tuned assuming a minimum of
        // 4 hash ways -- using less (even though it makes tables smaller and should make
        // things easier to fit) results in the testcases not fitting.
        calculated_depth = 4;
    }

    if (can_be_identity_hash(tbl, lo, calculated_depth)) return;

    if (calculated_depth < 8) {
        switch (calculated_depth) {
            case 1:
                BUG("Illegal calculated depth for resources %1%", calculated_depth);
            case 2:
                lo->way_sizes = {1, 1};
                break;
            case 3:
                lo->way_sizes = {1, 1, 1};
                break;
            case 4:
                lo->way_sizes = {1, 1, 1, 1};
                break;
            case 5:
                lo->way_sizes = {2, 1, 1, 1};
                break;
            case 6:
                lo->way_sizes = {2, 2, 1, 1};
                break;
            case 7:
                lo->way_sizes = {2, 2, 2, 1};
                break;
        }
        // Anything larger than 8 for depth.
    } else {
        int test_depth = calculated_depth > 64 ? 64 : calculated_depth;
        int max_group_size = (1 << floor_log2(test_depth)) / 4;
        int depth =
            calculated_depth > Memories::TOTAL_SRAMS ? Memories::TOTAL_SRAMS : calculated_depth;
        int select_bits_added = 0;
        int ways_added = 0;
        int select_ways = 0;
        int avail_hash_bits = Tofino::IXBar::get_hash_single_bits();
        while (depth > 0) {
            int max_group_size_bits_reqd = ceil_log2(max_group_size);
            int hash_single_bits_left = avail_hash_bits - (select_bits_added % avail_hash_bits);
            // std::cout << " Max Group Size : " << max_group_size
            //           << "          Depth : " << depth
            //           << " Avail Hash Bits: " << avail_hash_bits
            //           << " Select Bits Add: " << select_bits_added
            //           << " Hash Bits Left : " << hash_single_bits_left
            //           << " Sel Bus Split  : " << lo->select_bus_split << std::endl;
            if ((max_group_size <= depth) && (max_group_size_bits_reqd <= hash_single_bits_left)) {
                lo->way_sizes.push_back(max_group_size);
                depth -= max_group_size;
                select_bits_added += ceil_log2(max_group_size);
                // New hash function if independent ways is < 4
                if ((select_bits_added > avail_hash_bits) && (ways_added < 4) &&
                    (lo->select_bus_split < 0)) {
                    lo->select_bus_split = ways_added;
                    select_ways = 0;
                }
                ways_added++;
                select_ways++;
            } else {
                max_group_size /= 2;
            }

            // Recalculate group size on reaching hash bit boundary based on
            // remaining depth. This spreads the rams across ways more evenly
            if (depth > 0 && select_ways >= 4) {
                select_ways = 0;
                max_group_size =
                    depth >= lo->way_sizes[0] ? lo->way_sizes[0] : (1 << floor_log2(depth));
            }
        }
        LOG5(" Determined Way Sizes : " << lo->way_sizes);
    }
}

/* Convert all possible layout options to the correct way sizes */
void StageUseEstimate::options_to_ways(const IR::MAU::Table *tbl, int entries) {
    Log::TempIndent indent;
    LOG5("Calculating options to ways for " << layout_options.size() << " options in table "
                                            << tbl->name << indent);
    for (auto &lo : layout_options) {
        LOG6("Picking layout option : " << lo);
        if (lo.layout.hash_action || lo.way.match_groups == 0 || lo.layout.gateway_match) {
            lo.entries = entries;
            lo.srams = 0;
            continue;
        }

        int per_row = lo.way.match_groups;
        int total_depth = (entries + per_row * lo.layout.get_sram_depth() - 1) /
                          (per_row * lo.layout.get_sram_depth());
        int calculated_depth = total_depth;
        calculate_way_sizes(tbl, &lo, calculated_depth);
        lo.entries = calculated_depth * lo.way.match_groups * lo.layout.get_sram_depth();
        if (lo.layout.is_lamb)
            lo.lambs = calculated_depth * lo.way.width;
        else
            lo.srams = calculated_depth * lo.way.width;
        lo.maprams = 0;
        LOG6("  Updated layout option : " << lo);
    }
}

/* Calculate the correct size of ternary tables.  Obviously with ternary tables, no way
   size calculation is necessary, as ternary has no ways */
void StageUseEstimate::options_to_ternary_entries(const IR::MAU::Table *tbl, int entries) {
    for (auto &lo : layout_options) {
        int depth = (entries + 511u) / 512u;
        int bytes = tbl->layout.match_bytes;
        int width = 0;
        while (bytes > 11) {
            bytes -= 11;
            width += 2;
        }
        if (bytes > 6)
            width += 2;
        else if (bytes || !width)
            width++;
        lo.tcams = depth * width;
        lo.srams = 0;
        lo.entries = depth * 512;
    }
}

/** Calculates an estimate for the total number of logical tables, given the number of RAMs
 *  dedicated to an ATCAM table.  The goal is, calculate the minimum logical tables that I
 *  need, and then balance the size of those logical tables.
 */
void StageUseEstimate::calculate_partition_sizes(const IR::MAU::Table *tbl, LayoutOption *lo,
                                                 int initial_ram_depth) {
    lo->partition_sizes.clear();
    int logical_tables_needed = (initial_ram_depth + Memories::MAX_PARTITION_RAMS_PER_ROW - 1) /
                                Memories::MAX_PARTITION_RAMS_PER_ROW;
    // This is my own metric.  Basically, if the partition size is large enough, the cost of
    // having the maximum number of logical tables is not that important, as each logical table
    // requires at least 5 RAMs, the current cost of a maximum partition
    if (tbl->layout.partition_count > 4096) logical_tables_needed = initial_ram_depth;

    for (int i = 0; i < logical_tables_needed; i++) {
        int logical_table_depth = initial_ram_depth / logical_tables_needed;
        if (i < (initial_ram_depth % logical_tables_needed)) logical_table_depth++;
        lo->partition_sizes.push_back(logical_table_depth);
    }
}

/** Calculating the total number of entries for each layout option for an atcam table.
 *  The number of RAMs for the whole table is the following calculation:
 *      ways_per_partition: ceil_log2(select_bits of the atcam_partition_index)
 *      partition_entries: total (logical) simultaneous lookups in the table
 *      ram_depth: number of RAMs to hold all partitions, if the match was one ram wide
 */
void StageUseEstimate::options_to_atcam_entries(const IR::MAU::Table *tbl, int entries) {
    int partition_entries =
        (entries + tbl->layout.partition_count - 1) / tbl->layout.partition_count;
    LOG1("\tpartition_entries : " << partition_entries);
    for (auto &lo : layout_options) {
        int per_row = lo.way.match_groups;
        int ram_depth = (partition_entries + per_row - 1) / per_row;
        calculate_partition_sizes(tbl, &lo, ram_depth);
        int ways_per_partition = (tbl->layout.partition_count + 1024 - 1) / 1024;
        LOG1("\tper_row " << per_row << " ram_depth" << ram_depth << " ways_per_partition "
                          << ways_per_partition << " width " << lo.way.width);
        lo.way_sizes.push_back(ways_per_partition);
        lo.srams = ram_depth * lo.way.width * ways_per_partition;
        lo.entries =
            ram_depth * ways_per_partition * lo.way.match_groups * tbl->layout.words_per_sram();
    }
}

/** Currently a very simple way to split up the dleft hash tables into a reasonable number
 *  of ALUs with a particular size.  Eventually, the hash mod can potentially be used in order
 *  to calculate a RAM size exactly, according to Mike Ferrera, so that the addresses
 *  don't have to be a power of two.
 */
void StageUseEstimate::options_to_dleft_entries(const IR::MAU::Table *tbl,
                                                const attached_entries_t &attached_entries) {
    const IR::MAU::StatefulAlu *salu = nullptr;
    for (auto back_at : tbl->attached) {
        salu = back_at->attached->to<IR::MAU::StatefulAlu>();
        if (salu != nullptr) break;
    }
    BUG_CHECK(salu, "No salu found associated with the attached table, %1%", tbl->name);

    if (!attached_entries.count(salu)) return;
    auto entries = attached_entries.at(salu).entries;
    if (entries == 0) return;

    /* Figure out how many dleft ways can/should be used for the specified number
     * of entries -- currently ways must be the same size and must be a power of two size.
     * Ways must fit within one half of an MAU stage (max 23 rams/maprams + 1 spare bank) */
    int per_row = RegisterPerWord(salu);
    int total_stateful_rams =
        (entries + per_row * Memories::SRAM_DEPTH - 1) / (per_row * Memories::SRAM_DEPTH);
    int available_alus = MAX_METER_ALUS - meter_alus;
    int needed_alus = ways_pragma(tbl, 1, MAX_METER_ALUS);
    if (needed_alus < 0) needed_alus = std::min(total_stateful_rams, available_alus);
    int per_alu = (total_stateful_rams + needed_alus - 1) / needed_alus;
    int max_rams_per_way = 1 << floor_log2(MAX_DLEFT_HASH_SIZE);
    // if there are more than two ways, then two will have to share one half
    if (needed_alus > 2) max_rams_per_way /= 2;

    for (auto &lo : layout_options) {
        // FIXME: The hash distribution units are all identical for each, so the sizes for
        // each of the stateful for the same, as the mask is per hash distribution unit.
        // Just currently a limit of the input xbar algorithm which will have to be opened up
        int dleft_entries = 0;
        for (int i = 0; i < needed_alus; i++) {
            int hash_size = std::min(1 << ceil_log2(per_alu), max_rams_per_way);
            lo.dleft_hash_sizes.push_back(hash_size);
            dleft_entries += hash_size * per_row * Memories::SRAM_DEPTH;
        }
        LOG3("Dleft rams per_alu: " << per_alu << " needed_alus: " << needed_alus
                                    << " entries: " << dleft_entries);
    }
}

/* Calculate the number of rams required for attached tables, given the number of entries
   provided from the table placment */
void StageUseEstimate::calculate_attached_rams(const IR::MAU::Table *tbl,
                                               const attached_entries_t &att_entries,
                                               LayoutOption *lo) {
    Log::TempIndent indent;
    LOG5("Calculating RAMs for layout: " << lo << indent);
    for (auto back_at : tbl->attached) {
        auto at = back_at->attached;
        int per_word = 0;
        int width = 1;
        int attached_entries = lo->entries;
        if (!at->direct) {
            if (!att_entries.count(at) || (attached_entries = att_entries.at(at).entries) == 0)
                continue;
        } else {
            // Direct attached table already filled entirely. No need to compute space required.
            if (attached_entries == 0) continue;
        }
        bool need_srams = true;
        bool need_maprams = false;
        if (auto *ctr = at->to<IR::MAU::Counter>()) {
            per_word = CounterPerWord(ctr);
            need_maprams = true;
        } else if (at->is<IR::MAU::Meter>()) {
            per_word = 1;
            need_maprams = true;
        } else if (auto *reg = at->to<IR::MAU::StatefulAlu>()) {
            per_word = RegisterPerWord(reg);
            need_maprams = true;
        } else if (auto *ad = at->to<IR::MAU::ActionData>()) {
            // FIXME: in theory, the table should not have an action data table,
            // as that is decided after the table layout is picked
            if (ad->direct) BUG("Direct Action Data table exists before table placement occurs");
            width = 1;
            per_word = ActionDataPerWord(&lo->layout, &width);
        } else if (at->is<IR::MAU::Selector>()) {
            // TODO
        } else if (at->is<IR::MAU::TernaryIndirect>()) {
            BUG("Ternary Indirect Data table exists before table placement occurs");
        } else if (auto *idle = at->to<IR::MAU::IdleTime>()) {
            need_srams = false;
            need_maprams = true;
            per_word = IdleTimePerWord(idle);
        } else {
            BUG("unknown attached table type %s", at->kind());
        }
        if (per_word > 0) {
            if (attached_entries <= 0)
                BUG("%s: no size in indirect %s %s", at->srcInfo, at->kind(), at->name);
            int entries_per_sram = 1024 * per_word;
            int units = (attached_entries + entries_per_sram - 1) / entries_per_sram;
            if (need_maprams) {
                if (need_srams) units++;  // spare bank
                lo->maprams += units;
            }
            if (need_srams) {
                lo->srams += units * width;
                LOG6("Update srams: " << lo->srams << ", units: " << units << ", width: " << width);
            }
        }
    }
    // Before table placment, tables do not have attached Ternary Indirect or
    // Action Data Tables
    if (lo->layout.direct_ad_required()) {
        int width = 1;
        int per_word = ActionDataPerWord(&lo->layout, &width);
        int attached_entries = lo->entries;
        int entries_per_sram = lo->layout.get_sram_depth() * per_word;
        int units = (attached_entries + entries_per_sram - 1) / entries_per_sram;
        lo->srams += units * width;
        LOG5("Action Data Reqd: per_word: "
             << per_word << ", attached_entries: " << attached_entries
             << ", entries_per_sram: " << entries_per_sram << ", units: " << units);
    }
    if (lo->layout.ternary_indirect_required()) {
        if (lo->layout.is_local_tind) {
            int per_word = LocalTernaryIndirectPerWord(&lo->layout, tbl);
            int attached_entries = lo->entries;
            int entries_per_sram = Memories::LOCAL_TIND_DEPTH * per_word;
            int units = (attached_entries + entries_per_sram - 1) / entries_per_sram;
            lo->local_tinds += units;
            LOG5("Local Ternary Indirect Reqd: per_word: "
                 << per_word << ", attached_entries: " << attached_entries
                 << ", entries_per_sram: " << entries_per_sram << ", units: " << units);
        } else {
            int per_word = TernaryIndirectPerWord(&lo->layout, tbl);
            int attached_entries = lo->entries;
            int entries_per_sram = lo->layout.get_sram_depth() * per_word;
            int units = (attached_entries + entries_per_sram - 1) / entries_per_sram;
            lo->srams += units;
            LOG5("Ternary Indirect Reqd: per_word: "
                 << per_word << ", attached_entries: " << attached_entries
                 << ", entries_per_sram: " << entries_per_sram << ", units: " << units);
        }
    }
    LOG5("Updated layout with attached rams: " << lo);
}

/* Calculate the number of attached rams for every single potential layout option */
void StageUseEstimate::options_to_rams(const IR::MAU::Table *tbl,
                                       const attached_entries_t &attached_entries) {
    for (auto &lo : layout_options) {
        calculate_attached_rams(tbl, attached_entries, &lo);
    }
}

/* Sorting algorithm to determine the best option given that it is before trying to
   fit the layout options to a certian number of resources, instead just using all entries */
void StageUseEstimate::select_best_option(const IR::MAU::Table *tbl) {
    bool small_table_allocation = true;
    int table_size = 0;
    if (auto k = tbl->match_table->getConstantProperty("size"_cs)) table_size = k->asInt();
    for (auto lo : layout_options) {
        if (lo.entries < table_size) {
            small_table_allocation = false;
            break;
        }
    }

    std::sort(layout_options.begin(), layout_options.end(),
              [=](const LayoutOption &a, const LayoutOption &b) {
                  int t;
                  // For comparisons between lamb vs non lamb layouts prefer lambs
                  if (a.layout.is_lamb && !b.layout.is_lamb) return true;
                  if (a.layout.is_lamb && b.layout.is_lamb) {
                      if ((t = a.lambs - b.lambs) != 0) return t < 0;
                  } else if ((t = a.srams - b.srams) != 0)
                      return t < 0;
                  // Added to keep obfuscated-nat-mpls for compiling.  In theory the match
                  // groups/width should not be used for hash action tables
                  if (a.layout.hash_action && !b.layout.hash_action) return true;
                  if (!a.layout.hash_action && b.layout.hash_action) return false;
                  if ((t = a.way.width - b.way.width) != 0) return t < 0;
                  if (a.entries == b.entries) {
                      if ((t = a.way.match_groups - b.way.match_groups) != 0) return t < 0;
                  }
                  if (!a.layout.direct_ad_required() && b.layout.direct_ad_required()) return true;
                  if (a.layout.direct_ad_required() && !b.layout.direct_ad_required()) return false;
                  if ((t = a.layout.action_data_bytes_in_table -
                           b.layout.action_data_bytes_in_table) != 0)
                      return t < 0;
                  return a.entries > b.entries;
              });

    LOG3("table " << tbl->name << " requiring " << table_size << " entries.");
    if (small_table_allocation)
        LOG3("small table allocation with " << layout_options.size() << " layout options");
    else
        LOG3("large table allocation with " << layout_options.size() << " layout options");
    LOG3(layout_options);

    preferred_index = 0;
}

/* A sorting algorithm for the best ternary option, without having to fit the table
   to a particular number of resources */
void StageUseEstimate::select_best_option_ternary() {
    std::sort(layout_options.begin(), layout_options.end(),
              [=](const LayoutOption &a, const LayoutOption &b) {
                  int t;
                  if ((t = a.srams - b.srams) != 0) return t < 0;
                  if ((t = a.local_tinds - b.local_tinds) != 0) return t < 0;
                  if (!a.layout.ternary_indirect_required()) return true;
                  if (!b.layout.ternary_indirect_required()) return false;
                  if (!a.layout.direct_ad_required()) return true;
                  if (!b.layout.direct_ad_required()) return false;
                  return false;
              });

    for (auto &lo : layout_options) {
        LOG3("entries " << lo.entries << " srams " << lo.srams << " tcams " << lo.tcams
                        << " local ternary indirect " << lo.local_tinds << " action data "
                        << lo.layout.action_data_bytes_in_table << " immediate "
                        << lo.layout.immediate_bits << " ternary indirect "
                        << lo.layout.ternary_indirect_required());
    }

    preferred_index = 0;
}

/* Set the correct parameters of the preferred index */
void StageUseEstimate::fill_estimate_from_option(int &entries) {
    logical_ids = preferred()->logical_tables();
    tcams = preferred()->tcams;
    local_tinds = preferred()->local_tinds;
    srams = preferred()->srams;
    maprams = preferred()->maprams;
    entries = preferred()->entries;
}

void StageUseEstimate::determine_initial_layout_option(const IR::MAU::Table *tbl, int &entries,
                                                       attached_entries_t &attached_entries) {
    LOG3("Determining initial layout options on table : " << tbl->name << ", entries: " << entries
                                                          << ", att entries: " << attached_entries);
    if (tbl->for_dleft()) {
        BUG_CHECK(layout_options.size() == 1,
                  "Should only be one layout option for dleft "
                  "hash tables");
        options_to_dleft_entries(tbl, attached_entries);
    }
    if (layout_options.size() == 1 && layout_options[0].layout.no_match_data() && !entries) {
        preferred_index = 0;
    } else if (tbl->layout.atcam) {
        options_to_atcam_entries(tbl, entries);
        options_to_rams(tbl, attached_entries);
        select_best_option(tbl);
        fill_estimate_from_option(entries);
    } else if (tbl->layout.ternary) {  // ternary
        options_to_ternary_entries(tbl, entries);
        options_to_rams(tbl, attached_entries);
        select_best_option_ternary();
        fill_estimate_from_option(entries);
    } else if (!tbl->conditional_gateway_only()) {  // exact_match
        /* assuming all ways have the same format and width (only differ in depth) */
        options_to_ways(tbl, entries);
        options_to_rams(tbl, attached_entries);
        select_best_option(tbl);
        fill_estimate_from_option(entries);
    } else {  // gw
        entries = 0;
    }
}

/* Constructor to estimate the number of srams, tcams, and maprams a table will require*/
StageUseEstimate::StageUseEstimate(const IR::MAU::Table *tbl, int &entries,
                                   attached_entries_t &attached_entries, LayoutChoices *lc,
                                   bool prev_placed, bool gateway_attached, bool disable_split,
                                   PhvInfo &phv) {
    HashMaskAnnotations hash_mask_annotations(tbl, phv);
    hash_bits_masked = hash_mask_annotations.hash_bits_masked();
    // Because the table is const, the layout options must be copied into the Object
    layout_options.clear();
    int initial_entries = entries;
    format_type.initialize(tbl, entries, prev_placed, attached_entries);
    if (!tbl->created_during_tp) {
        if (disable_split && format_type.anyAttachedLaterStage())
            LOG3("Pruning split layout with format type:" << format_type);
        else
            layout_options = lc->get_layout_options(tbl, format_type);
        if (layout_options.empty()) {
            // no layouts available?  Fall back to NORMAL for now.  TablePlacement will flag
            // an error if it needs to split this table.
            format_type = ActionData::FormatType_t::default_for_table(tbl);
            layout_options = lc->get_layout_options(tbl, format_type);
        }
        BUG_CHECK(tbl->conditional_gateway_only() || !layout_options.empty(),
                  "No %s layout options for %s", format_type, tbl);
        action_formats = lc->get_action_formats(tbl, format_type);
        meter_format = lc->get_attached_formats(tbl, format_type);
    }
    exact_ixbar_bytes = tbl->layout.ixbar_bytes;
    // A hash action table currently cannot be split across stages if it does an action lookup,
    // thus if the table has been previously been placed and has multiple entries (is not a
    // no_match table), the current stage table cannot be hash action
    if (prev_placed && entries > 1) {
        auto it = layout_options.begin();
        while (it != layout_options.end()) {
            if (it->layout.hash_action)
                it = layout_options.erase(it);
            else
                it++;
        }
    }

    if (gateway_attached) {
        auto it = layout_options.begin();
        while (it != layout_options.end()) {
            if (it->layout.gateway_match)
                it = layout_options.erase(it);
            else
                it++;
        }
    }
    BUG_CHECK(tbl->conditional_gateway_only() || !layout_options.empty(), "No layout for %s", tbl);
    determine_initial_layout_option(tbl, entries, attached_entries);

    // Remove empty layout that does not fit on the remaining resources.
    if (initial_entries > 0 && layout_options.size() != 1) remove_invalid_option();

    // FIXME: This is a quick hack to handle tables with only a default action

    // FIXME if the initial_entries was 0, we can't set it non-zero as that will cause
    // table placement to loop, never being able to finish the table.  So we reset it
    // it unconditionally.  This will likely cause an error about being unable to split
    // a table (generally due to not having a SPLIT_ATTACHED format, but thats better than
    // a non-informational crash,
    if (initial_entries == 0) entries = 0;

    LOG2("StageUseEstimate(" << tbl->name << ", " << entries << ", " << format_type << ") "
                             << layout_options.size() << " layouts");
}

bool StageUseEstimate::adjust_choices(const IR::MAU::Table *tbl, int &entries,
                                      attached_entries_t &attached_entries) {
    LOG3("Adjusting choices on table : " << tbl->name << ", entries: " << entries
                                         << ", att entries: " << attached_entries);
    if (tbl->is_a_gateway_table_only() || tbl->layout.no_match_data()) return false;

    if (tbl->layout.ternary) {
        preferred_index++;
        if (preferred_index == layout_options.size()) {
            LOG2("    adjust_choices: no more ternary layouts");
            return false;
        }
        LOG2("    adjust_choices: try ternary layout " << preferred_index);
        return true;
    }

    // A hash action table cannot be split, without adding a PHV.  Thus it is just removed
    if (layout_options[preferred_index].layout.hash_action) {
        layout_options.erase(layout_options.begin() + preferred_index);
        if (layout_options.size() == 0) {
            LOG2("    adjust_choices: no more layouts after removing hash_action");
            return false;
        }
    } else if (layout_options[preferred_index].previously_widened) {
        layout_options.erase(layout_options.begin() + preferred_index);
        LOG2("    adjust_choices: previously_widened.  Erasing");
    } else {
        IR::MAU::Table::Way &way = layout_options[preferred_index].way;
        way.width++;
        // While adjusting the layout we might end up creating one that is suboptimal. E.g.
        // 4 entries packed in 1 SRAMs can be adjusted to 4 entries packed in 2 SRAMs. This layout
        // is no longer a good one since a 2 entry in 1 SRAM is more flexible.
        if ((way.width % way.match_groups == 0) && way.match_groups != 1) {
            layout_options.erase(layout_options.begin() + preferred_index);
            LOG2("    adjust_choices: producing layout that can easily be split.  Erasing");
        } else {
            layout_options[preferred_index].previously_widened = true;
            LOG2("    adjust_choices: widen the exact match way");
        }
    }

    if (layout_options.size() == 0) {
        LOG2("    adjust_choices: no more layout choices as all have been widened");
        return false;
    }

    for (auto &lo : layout_options) {
        lo.clear_mems();
    }
    determine_initial_layout_option(tbl, entries, attached_entries);
    return true;
}

int StageUseEstimate::stages_required() const {
    return std::max({(srams + StageUse::MAX_SRAMS - 1) / StageUse::MAX_SRAMS,
                     (tcams + StageUse::MAX_TCAMS - 1) / StageUse::MAX_TCAMS,
                     (local_tinds + MAX_LOCAL_TINDS - 1) / MAX_LOCAL_TINDS,
                     (maprams + StageUse::MAX_MAPRAMS - 1) / StageUse::MAX_MAPRAMS});
}

/* Given a number of available srams within a stage, calculate the maximum size
   different layout options can be while still using up to the number of srams */
bool StageUseEstimate::calculate_for_leftover_srams(const IR::MAU::Table *tbl, int &srams_left,
                                                    int &entries,
                                                    attached_entries_t &attached_entries) {
    if (entries > 1) {
        // Can't split hash_action tables that do any actual lookup (but can duplicate
        // no-match-hit tables, which are hash_action with 1 entry)
        if (std::all_of(layout_options.begin(), layout_options.end(),
                        [](LayoutOption &lo) { return lo.layout.hash_action; }))
            return false;
        // Remove the hash action layout option(s)
        layout_options.erase(std::remove_if(layout_options.begin(), layout_options.end(),
                                            [](LayoutOption &lo) { return lo.layout.hash_action; }),
                             layout_options.end());
    }

    for (auto &lo : layout_options) {
        lo.clear_mems();
        known_srams_needed(tbl, attached_entries, &lo);
        unknown_srams_needed(tbl, &lo, srams_left);
    }
    srams_left_best_option(srams_left);
    fill_estimate_from_option(entries);
    return true;
}

/* Given a number of available tcams/srams within a stage, calculate the maximum size of different
   layout options that can with the available resources */
void StageUseEstimate::calculate_for_leftover_tcams(const IR::MAU::Table *tbl, int tcams_left,
                                                    int srams_left, int &entries,
                                                    attached_entries_t &attached_entries) {
    for (auto &lo : layout_options) {
        lo.clear_mems();
        known_srams_needed(tbl, attached_entries, &lo);
        unknown_tcams_needed(tbl, &lo, tcams_left, srams_left);
    }
    tcams_left_best_option();
    fill_estimate_from_option(entries);
}

void StageUseEstimate::calculate_for_leftover_atcams(const IR::MAU::Table *tbl, int srams_left,
                                                     int &entries,
                                                     attached_entries_t &attached_entries) {
    for (auto &lo : layout_options) {
        lo.clear_mems();
        known_srams_needed(tbl, attached_entries, &lo);
        unknown_atcams_needed(tbl, &lo, srams_left);
    }
    srams_left_best_option(srams_left);
    fill_estimate_from_option(entries);
}

// Shrink the preferred layout and reduce the SRAM consumption by at least one SRAM. Remove the
// layout option if the result is invalid. Finally sort the remaining layout option based on
// the number of entries that each layout can fit.
void StageUseEstimate::shrink_preferred_srams_lo(const IR::MAU::Table *tbl, int &entries,
                                                 attached_entries_t &attached_entries) {
    if (layout_options.empty()) return;
    LayoutOption *lo = &layout_options[preferred_index];
    int prev_srams = lo->srams;
    for (int new_srams = prev_srams - 1; lo->srams >= prev_srams && new_srams > 0; new_srams--) {
        lo->clear_mems();
        known_srams_needed(tbl, attached_entries, lo);
        unknown_srams_needed(tbl, lo, new_srams);
    }
    if (lo->srams >= prev_srams || lo->srams == 0) {
        layout_options.erase(layout_options.begin() + preferred_index);
        if (layout_options.size() == 0) return;
    }
    max_entries_best_option();
    fill_estimate_from_option(entries);
    return;
}

// Shrink the preferred layout and reduce the TCAM consumption by at least one TCAM. Remove the
// layout option if the result is invalid. Finally sort the remaining layout option based on
// the number of entries that each layout can fit.
void StageUseEstimate::shrink_preferred_tcams_lo(const IR::MAU::Table *tbl, int &entries,
                                                 attached_entries_t &attached_entries) {
    if (layout_options.empty()) return;
    LayoutOption *lo = &layout_options[preferred_index];
    int prev_tcams = lo->tcams;
    int prev_srams = lo->srams;
    for (int new_tcams = prev_tcams - 1; lo->tcams >= prev_tcams && new_tcams > 0; new_tcams--) {
        lo->clear_mems();
        known_srams_needed(tbl, attached_entries, lo);
        unknown_tcams_needed(tbl, lo, new_tcams, prev_srams);
    }
    if (lo->tcams >= prev_tcams || lo->tcams == 0) {
        layout_options.erase(layout_options.begin() + preferred_index);
        if (layout_options.size() == 0) return;
    }
    tcams_left_best_option();
    fill_estimate_from_option(entries);
    return;
}

// Shrink the preferred layout and reduce the SRAM consumption by at least one SRAM. Remove the
// layout option if the result is invalid. Finally sort the remaining layout option based on
// the number of entries that each layout can fit.
void StageUseEstimate::shrink_preferred_atcams_lo(const IR::MAU::Table *tbl, int &entries,
                                                  attached_entries_t &attached_entries) {
    if (layout_options.empty()) return;
    LayoutOption *lo = &layout_options[preferred_index];
    int prev_srams = lo->srams;
    for (int new_srams = prev_srams - 1; lo->srams >= prev_srams && new_srams > 0; new_srams--) {
        lo->clear_mems();
        known_srams_needed(tbl, attached_entries, lo);
        unknown_atcams_needed(tbl, lo, new_srams);
    }
    if (lo->srams >= prev_srams || lo->srams == 0) {
        layout_options.erase(layout_options.begin() + preferred_index);
        if (layout_options.size() == 0) return;
    }
    max_entries_best_option();
    fill_estimate_from_option(entries);
    return;
}

/* Calculates the number of resources needed by the attached tables that are independent
   of the size of table, such as indirect counters, action profiles, etc.*/
void StageUseEstimate::known_srams_needed(const IR::MAU::Table *tbl,
                                          const attached_entries_t &att_entries, LayoutOption *lo) {
    for (auto back_at : tbl->attached) {
        auto at = back_at->attached;
        if (at->direct || !att_entries.count(at)) continue;
        int attached_entries = att_entries.at(at).entries;
        if (attached_entries == 0) continue;
        int per_word = 0;
        int width = 1;
        bool need_maprams = false;
        if (auto *ctr = at->to<IR::MAU::Counter>()) {
            per_word = CounterPerWord(ctr);
            need_maprams = true;
        } else if (at->is<IR::MAU::Meter>()) {
            per_word = 1;
            need_maprams = true;
        } else if (auto *reg = at->to<IR::MAU::StatefulAlu>()) {
            per_word = RegisterPerWord(reg);
            need_maprams = true;
        } else if (at->is<IR::MAU::ActionData>()) {
            // Because this is called before and after table placement
            per_word = ActionDataPerWord(&lo->layout, &width);
        } else if (at->is<IR::MAU::Selector>()) {
            // TODO
        } else if (at->is<IR::MAU::TernaryIndirect>()) {
            // Again, because this is called before and after table placement
            continue;
        } else if (at->is<IR::MAU::IdleTime>()) {
            continue;
        } else {
            BUG("Unrecognized table type");
        }
        if (per_word > 0) {
            if (attached_entries <= 0)
                BUG("%s: no size in indirect %s %s", at->srcInfo, at->kind(), at->name);
            int entries_per_sram = 1024 * per_word;
            int units = (attached_entries + entries_per_sram - 1) / entries_per_sram;
            lo->srams += units * width;
            if (need_maprams) lo->maprams += units;
        }
    }
}

/* Calculate the RAM_counter for each attached table.  This contains the entries per_row,
   the width, and the need of maprams */
void StageUseEstimate::calculate_per_row_vector(safe_vector<RAM_counter> &per_word_and_width,
                                                const IR::MAU::Table *tbl, LayoutOption *lo) {
    for (auto back_at : tbl->attached) {
        auto at = back_at->attached;
        if (!at->direct) continue;
        int per_word = 0;
        int width = 1;
        bool need_srams = true;
        bool need_maprams = false;
        if (auto *ctr = at->to<IR::MAU::Counter>()) {
            per_word = CounterPerWord(ctr);
            need_maprams = true;
            ;
        } else if (at->is<IR::MAU::Meter>()) {
            per_word = 1;
            need_maprams = true;
        } else if (auto *reg = at->to<IR::MAU::StatefulAlu>()) {
            per_word = RegisterPerWord(reg);
            need_maprams = true;
        } else if (auto ad = at->to<IR::MAU::ActionData>()) {
            BUG_CHECK(!ad->direct, "Cannot have an action data table before table placement");
            continue;
        } else if (at->is<IR::MAU::Selector>()) {
            continue;
        } else if (auto *idle = at->to<IR::MAU::IdleTime>()) {
            per_word = IdleTimePerWord(idle);
            need_srams = false;
            need_maprams = true;
        } else {
            BUG("Unrecognized table type");
        }
        per_word_and_width.emplace_back(per_word, width, need_srams, need_maprams);
    }
    if (lo->layout.direct_ad_required()) {
        int width = 1;
        int per_word = ActionDataPerWord(&lo->layout, &width);
        per_word_and_width.emplace_back(per_word, width, true, false);
    }

    if (lo->layout.ternary_indirect_required()) {
        int width = 1;
        int per_word = TernaryIndirectPerWord(&lo->layout, tbl);
        per_word_and_width.emplace_back(per_word, width, true, false);
    }
}

/* Estimate the number of srams on a layout option, gradually growing the srams array
   size and then calculating the corresponding necessary attached rams that are related
   to the number of entries from the srams */
void StageUseEstimate::unknown_srams_needed(const IR::MAU::Table *tbl, LayoutOption *lo,
                                            int srams_left) {
    safe_vector<RAM_counter> per_word_and_width;
    calculate_per_row_vector(per_word_and_width, tbl, lo);

    // Shrink the available srams by the number of known srams needed
    int available_srams = srams_left - lo->srams;
    int used_srams = 0;
    int used_maprams = 0;
    int adding_entries = 0;
    int depth = 0;
    int words_per_sram = tbl->layout.words_per_sram();

    while (lo->way.match_groups > 0) {
        int attempted_depth = depth + 1;
        int sram_count = 0;
        int mapram_count = 0;
        int attempted_entries = lo->way.match_groups * words_per_sram * attempted_depth;
        sram_count += attempted_entries / (lo->way.match_groups * words_per_sram) * lo->way.width;
        for (auto rc : per_word_and_width) {
            int entries_per_sram = words_per_sram * rc.per_word;
            int units = (attempted_entries + entries_per_sram - 1) / entries_per_sram;
            if (rc.need_srams) sram_count += units * rc.width;
            if (rc.need_maprams) mapram_count += units;
        }

        if (sram_count > available_srams) break;
        depth = attempted_depth;
        adding_entries = attempted_entries;
        used_srams = sram_count;
        used_maprams = mapram_count;
    }

    int depth_test = depth;

    if (adding_entries > 0) calculate_way_sizes(tbl, lo, depth_test);

    if (depth_test != depth) {
        int attempted_entries = lo->way.match_groups * words_per_sram * depth_test;
        int sram_count =
            attempted_entries / (lo->way.match_groups * words_per_sram) * lo->way.width;
        int mapram_count = 0;
        for (auto rc : per_word_and_width) {
            int entries_per_sram = words_per_sram * rc.per_word;
            int units = (attempted_entries + entries_per_sram - 1) / entries_per_sram;
            if (rc.need_srams) sram_count += units * rc.width;
            if (rc.need_maprams) mapram_count += units;
        }
        used_srams = sram_count;
        adding_entries = attempted_entries;
        used_maprams = mapram_count;
    }
    lo->srams += used_srams;
    lo->maprams += used_maprams;
    lo->entries = adding_entries;
}

/** Given a number of srams, calculate the size of the possible atcam table, given the
 *  layout option.  It is different than normal SRAMs, because the algorithm has to grow all
 *  ways simultaneously.
 */
void StageUseEstimate::unknown_atcams_needed(const IR::MAU::Table *tbl, LayoutOption *lo,
                                             int srams_left) {
    safe_vector<RAM_counter> per_word_and_width;
    calculate_per_row_vector(per_word_and_width, tbl, lo);

    int available_srams = srams_left - lo->srams;
    int used_srams = 0;
    int used_maprams = 0;
    int adding_entries = 0;
    int depth = 0;
    int ways_per_partition = (tbl->layout.partition_count + 1023) / 1024;

    lo->way_sizes.push_back(ways_per_partition);

    while (true) {
        int attempted_depth = depth + 1;
        int sram_count = 0;
        int mapram_count = 0;
        int atcam_srams = attempted_depth * ways_per_partition;
        int words_per_sram = tbl->layout.words_per_sram();
        int attempted_entries = atcam_srams * lo->way.match_groups * words_per_sram;
        sram_count += atcam_srams * lo->way.width;

        for (auto rc : per_word_and_width) {
            int entries_per_sram = words_per_sram * rc.per_word;
            int units = (attempted_entries + entries_per_sram - 1) / entries_per_sram;
            if (rc.need_srams) sram_count += units * rc.width;
            if (rc.need_maprams) mapram_count += units;
        }
        if (sram_count > available_srams) break;
        depth = attempted_depth;
        adding_entries = attempted_entries;
        used_srams = sram_count;
        used_maprams = mapram_count;
    }

    calculate_partition_sizes(tbl, lo, depth);

    lo->srams += used_srams;
    lo->maprams += used_maprams;
    lo->entries = adding_entries;
}

/* Sorting the layout options in terms of best fit for the given number of resources left
 */
void StageUseEstimate::srams_left_best_option(int srams_left) {
    std::sort(layout_options.begin(), layout_options.end(),
              [=](const LayoutOption &a, const LayoutOption &b) {
                  int t;
                  if (a.srams > srams_left && b.srams <= srams_left) return false;
                  if (b.srams > srams_left && a.srams <= srams_left) return true;
                  if ((t = a.entries - b.entries) != 0) return t > 0;
                  if ((t = a.way.width - b.way.width) != 0) return t < 0;
                  if ((t = a.way.match_groups - b.way.match_groups) != 0) return t < 0;
                  if (!a.layout.direct_ad_required() && b.layout.direct_ad_required()) return true;
                  if (a.layout.direct_ad_required() && !b.layout.direct_ad_required()) return false;
                  return a.layout.action_data_bytes_in_table < b.layout.action_data_bytes_in_table;
              });
    for (auto &lo : layout_options) {
        LOG3("layout option width " << lo.way.width << " match groups " << lo.way.match_groups
                                    << " entries " << lo.entries << " srams " << lo.srams
                                    << " action data " << lo.layout.action_data_bytes_in_table
                                    << " immediate " << lo.layout.immediate_bits);
        LOG3("Layout option way sizes " << lo.way_sizes);
    }
    preferred_index = 0;
}

void StageUseEstimate::max_entries_best_option() {
    std::sort(layout_options.begin(), layout_options.end(),
              [=](const LayoutOption &a, const LayoutOption &b) {
                  int t;
                  if ((t = a.entries - b.entries) != 0) return t > 0;
                  if ((t = a.srams - b.srams) != 0) return t < 0;
                  if ((t = a.way.width - b.way.width) != 0) return t < 0;
                  if ((t = a.way.match_groups - b.way.match_groups) != 0) return t < 0;
                  if (!a.layout.direct_ad_required() && b.layout.direct_ad_required()) return true;
                  if (a.layout.direct_ad_required() && !b.layout.direct_ad_required()) return false;
                  return a.layout.action_data_bytes_in_table < b.layout.action_data_bytes_in_table;
              });
    for (auto &lo : layout_options) {
        LOG3("layout option width " << lo.way.width << " match groups " << lo.way.match_groups
                                    << " entries " << lo.entries << " srams " << lo.srams
                                    << " action data " << lo.layout.action_data_bytes_in_table
                                    << " immediate " << lo.layout.immediate_bits);
        LOG3("Layout option way sizes " << lo.way_sizes);
    }
    preferred_index = 0;
}

/* Calculate the relative size of the different ternary layout options by slowly increasing
   the number of entries until the available tcams or srams are full */
void StageUseEstimate::unknown_tcams_needed(const IR::MAU::Table *tbl, LayoutOption *lo,
                                            int tcams_left, int srams_left) {
    safe_vector<RAM_counter> per_word_and_width;
    calculate_per_row_vector(per_word_and_width, tbl, lo);

    int available_srams = srams_left - lo->srams;
    int available_tcams = tcams_left;
    int adding_entries = 0;
    int used_srams = 0;
    int used_maprams = 0;
    int used_tcams = 0;
    int depth = 0;

    while (true) {
        int attempted_depth = depth + 1;
        int sram_count = 0;
        int mapram_count = 0;
        int tcam_count = 0;
        int attempted_entries = attempted_depth * 512;
        int width = (tbl->layout.match_width_bits + 47) / 44;  // +4 bits for v/v, round up
        tcam_count += attempted_depth * width;

        for (auto rc : per_word_and_width) {
            int entries_per_sram = 1024 * rc.per_word;
            int units = (attempted_entries + entries_per_sram - 1) / entries_per_sram;
            if (rc.need_srams) sram_count += units * rc.width;
            if (rc.need_maprams) mapram_count += units;
        }

        if (sram_count > available_srams || tcam_count > available_tcams) break;
        depth = attempted_depth;
        adding_entries = attempted_entries;
        used_srams = sram_count;
        used_maprams = mapram_count;
        used_tcams = tcam_count;
    }
    lo->srams += used_srams;
    lo->maprams += used_maprams;
    lo->tcams += used_tcams;
    lo->entries = adding_entries;
}

/* Pick the best ternary option given that the ternary options are selecting from the
   available resources provided */
void StageUseEstimate::tcams_left_best_option() {
    std::sort(layout_options.begin(), layout_options.end(),
              [=](const LayoutOption &a, const LayoutOption &b) {
                  int t;
                  if ((t = a.entries - b.entries) != 0) return t > 0;
                  if ((t = a.srams - b.srams) != 0) return t < 0;
                  if (!a.layout.ternary_indirect_required()) return true;
                  if (!b.layout.ternary_indirect_required()) return false;
                  if (!a.layout.direct_ad_required()) return true;
                  if (!b.layout.direct_ad_required()) return false;
                  return a.entries < b.entries;
              });
    preferred_index = 0;

    for (auto &lo : layout_options) {
        LOG3("entries " << lo.entries << " srams " << lo.srams << " tcams " << lo.tcams
                        << " action data " << lo.layout.action_data_bytes_in_table << " immediate "
                        << lo.layout.immediate_bits << " ternary indirect "
                        << lo.layout.ternary_indirect_required());
    }
}

/** This pass calculates the number of lines an individual range match will take.  In order
 *  to perform a range match in Tofino, multiple lines of a TCAM may be necessary.  This is
 *  described in section 6.3.12 DirtCAM Example.  Essentially a larger range is broken into
 *  several smaller ranges that the hardware is capable of matching, which occur in factors
 *  of 16, as ranges are matched on a nibble by nibble basis, and 2^4 = 16.
 *
 *  The maximum number of rows that could be required by a range match is 2 * (nibbles involved
 *  in range match) - 1.  Let's look at a 12 bit range example 2 <= x <= 600
 *
 *  _____small_range_____|__nib_11_8__|__nib_7_4__|__nib_3_0__
 *      2 <= x <= 15     |      0     |     0     |  [2:15]
 *     16 <= x <= 255    |      0     |   [1:15]  |    X
 *    256 <= x <= 511    |      1     |     X     |    X
 *    512 <= x <= 591    |      2     |   [1:4]   |    X
 *    592 <= x <= 600    |      2     |     5     |  [1:8]
 *
 *  Calculations for these smaller ranges are done based on the factors within these
 *  individual ranges for nibbles.  The 4b_encoding provides a way to match a nibble to any
 *  combination of numbers between 0-15.  However, note that not all range need all rows.
 *
 *  The hardware to provide matching across TCAM rows is called the Multirange Distributor,
 *  described in section 6.3.9.2 of the uArch.  The distribution can combine the match signals,
 *  of nearby rows, in a 3 step process.  The first step can combine a signal in either the
 *  higher or lower row, the seocnd step can combine a row 2 rows away, and the third can combine
 *  a signal from 4 rows away, for a maximum distribution limit of 8 rows.  Thus the total rows
 *  is the max of the previous formula and 8.
 */
bool RangeEntries::preorder(const IR::MAU::TableKey *ixbar_read) {
    if (ixbar_read->match_type.name != "range") return false;

    le_bitrange bits = {0, 0};
    auto field = phv.field(ixbar_read->expr, &bits);

    auto tbl = findContext<IR::MAU::Table>();

    int range_nibbles = 0;
    PHV::FieldUse use(PHV::FieldUse::READ);
    field->foreach_byte(bits, tbl, &use, [&](const PHV::AllocSlice &sl) {
        if ((sl.container_slice().lo % 8) < 4) range_nibbles++;
        if ((sl.container_slice().hi % 8) > 3) range_nibbles++;
    });

    // FIXME: This is a limitation from Glass where glass requires all range matches to be on an
    // individual TCAM.  After looking at the hardware requirements and talking to the driver
    // team, I'm not sure if this constraint actually exists.  Will have to be verified by
    // packet testing.  Currently the input xbar algorithm ignores this constraint
    ERROR_CHECK(range_nibbles <= Tofino::IXBar::TERNARY_BYTES_PER_GROUP,
                "%s: Currently in p4c, the table %s cannot perform a range match on key %s "
                "as the key does not fit in under 5 PHV nibbles",
                ixbar_read->srcInfo, tbl->name, field->name);
    int range_lines_needed = 2 * range_nibbles - 1;
    range_lines_needed = std::min(MULTIRANGE_DISTRIBUTION_LIMIT, range_lines_needed);
    max_entry_TCAM_lines = std::max(range_lines_needed, max_entry_TCAM_lines);
    return false;
}

/** Because a range entry requires a lot of extra TCAM rows, and either not every range requires
 *  the maximum number of rows, or no range match at all, the programmer can provide a pragma
 *  to limit the number of rows.
 */
void RangeEntries::postorder(const IR::MAU::Table *tbl) {
    auto annot = tbl->match_table->getAnnotations();
    int range_entries = -1;
    if (auto s = annot->getSingle("entries_with_ranges"_cs)) {
        const IR::Constant *pragma_val = nullptr;
        if (s->expr.size() == 0) {
            error("%s: entries_with_ranges pragma on table %s has no value", s->srcInfo, tbl->name);
        } else {
            pragma_val = s->expr.at(0)->to<IR::Constant>();
            ERROR_CHECK(pragma_val != nullptr,
                        "%s: the value for the entries_with_ranges "
                        "pragma on table %s is not a constant",
                        s->srcInfo, tbl->name);
        }
        if (pragma_val) {
            int pragma_range_entries = pragma_val->asInt();
            WARN_CHECK(pragma_range_entries > 0,
                       "%s: The value for pragma entries_with_ranges "
                       "on table %s is %d, which is invalid and will be ignored",
                       s->srcInfo, tbl->name, pragma_range_entries);
            WARN_CHECK(pragma_range_entries <= table_entries,
                       "%s: The value for pragma "
                       "entries_with_ranges on table %s is %d, which is greater than the "
                       "entries provided for the table %d, and thus will be shrunk to that size",
                       s->srcInfo, tbl->name, pragma_range_entries, table_entries);
            if (pragma_range_entries > 0)
                range_entries = std::min(pragma_range_entries, table_entries);
        }
    }
    if (range_entries < 0) {
        range_entries = (table_entries * RANGE_ENTRY_PERCENTAGE) / 100;
    }
    total_TCAM_lines = range_entries * max_entry_TCAM_lines + (table_entries - range_entries);
}

std::ostream &operator<<(std::ostream &out, const StageUseEstimate &sue) {
    out << "{ id=" << sue.logical_ids << " ram=" << sue.srams << " tcam=" << sue.tcams
        << " mram=" << sue.maprams << " eixb=" << sue.exact_ixbar_bytes
        << " tixb=" << sue.ternary_ixbar_groups << " malu=" << sue.meter_alus
        << " salu=" << sue.stats_alus << " local_tinds=" << sue.local_tinds << " }";
    return out;
}
