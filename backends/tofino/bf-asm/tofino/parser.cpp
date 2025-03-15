/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <initializer_list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "backends/tofino/bf-asm/misc.h"
#include "backends/tofino/bf-asm/parser-tofino-jbay.h"
#include "backends/tofino/bf-asm/target.h"
#include "backends/tofino/bf-asm/top_level.h"

// ----------------------------------------------------------------------------
// Slots & Useful constants
// ----------------------------------------------------------------------------

// Following constants are used for detection of unused slot (e.g., initial value) and
// minimal/maximal indexes for extractor slots
static unsigned EXTRACT_SLOT_UNUSED = 511;
static unsigned EXTRACT_SLOT_CONSTANT_DIS = 0;
static unsigned EXTRACT_SLOT_CONSTANT_EN = 1;
static unsigned EXTRACT_SLOT_CONSTANT_ZERO = 0;
static unsigned PHV_MIN_INDEX = 0;
static unsigned PHV_MAX_INDEX = 224;
static unsigned TPHV_MIN_INDEX = 256;
static unsigned TPHV_MAX_INDEX = 368;

/* remapping structure for getting at the config bits for phv output
 * programming in a systematic way */
struct tofino_phv_output_map {
    int size; /* 8, 16, or 32 */
    ubits<9> *dst;
    ubits_base *src; /* 6 or 8 bits */
    ubits<1> *src_type, *offset_add, *offset_rot;
};
std::ostream &operator<<(std::ostream &of, const tofino_phv_output_map *om) {
    of << om->size << "bit, dst = " << std::to_string(om->dst->value)
       << ", src = " << std::to_string(om->src->value);
    if (om->src_type) of << ", src_type = " << std::to_string(om->src_type->value);
    if (om->offset_add) of << ", offset_add = " << std::to_string(om->offset_add->value);
    if (om->offset_rot) of << ", offset_rot = " << std::to_string(om->offset_rot->value);
    return of;
}
enum extractor_slots {
    /* enum for indexes in the tofino_phv_output_map */
    phv_32b_0,
    phv_32b_1,
    phv_32b_2,
    phv_32b_3,
    phv_16b_0,
    phv_16b_1,
    phv_16b_2,
    phv_16b_3,
    phv_8b_0,
    phv_8b_1,
    phv_8b_2,
    phv_8b_3,
    tofino_phv_output_map_size,
};

// PHV use slots: ordered list of slots to try
//
// For example, when trying to find a 32b slot:
//  1. First try the 4 x 32b extractors.
//  2. If that fails, try pairs of 16b extractors.
//  3. If that still fails, finally try the 8b extractors together.
//
// For checksums, allocate in the reverse order as we need to fill
// from the last container back due to a HW bug (see MODEL-210).
//
// FIXME: what does "shift" represent???
static struct phv_use_slots {
    int idx;
    unsigned usemask, shift, size;
} phv_32b_slots[] = {{phv_32b_0, 1U << phv_32b_0, 0, 32},  {phv_32b_1, 1U << phv_32b_1, 0, 32},
                     {phv_32b_2, 1U << phv_32b_2, 0, 32},  {phv_32b_3, 1U << phv_32b_3, 0, 32},
                     {phv_16b_0, 3U << phv_16b_0, 16, 16}, {phv_16b_2, 3U << phv_16b_2, 16, 16},
                     {phv_8b_0, 0xfU << phv_8b_0, 24, 8},  {0, 0, 0, 0}},
  phv_16b_slots[] = {{phv_16b_0, 1U << phv_16b_0, 0, 16},
                     {phv_16b_1, 1U << phv_16b_1, 0, 16},
                     {phv_16b_2, 1U << phv_16b_2, 0, 16},
                     {phv_16b_3, 1U << phv_16b_3, 0, 16},
                     {phv_8b_0, 3U << phv_8b_0, 8, 8},
                     {phv_8b_2, 3U << phv_8b_2, 8, 8},
                     {0, 0, 0, 0}},
  phv_8b_slots[] = {{phv_8b_0, 1U << phv_8b_0, 0, 8},
                    {phv_8b_1, 1U << phv_8b_1, 0, 8},
                    {phv_8b_2, 1U << phv_8b_2, 0, 8},
                    {phv_8b_3, 1U << phv_8b_3, 0, 8},
                    {0, 0, 0, 0}},
  phv_32b_csum_slots[] = {{phv_32b_3, 1U << phv_32b_3, 0, 32},
                          {phv_32b_2, 1U << phv_32b_2, 0, 32},
                          {phv_32b_1, 1U << phv_32b_1, 0, 32},
                          {phv_32b_0, 1U << phv_32b_0, 0, 32},
                          {phv_16b_2, 3U << phv_16b_2, 16, 16},
                          {phv_16b_0, 3U << phv_16b_0, 16, 16},
                          {phv_8b_0, 0xfU << phv_8b_0, 24, 8},
                          {0, 0, 0, 0}},
  phv_16b_csum_slots[] = {{phv_16b_3, 1U << phv_16b_3, 0, 16},
                          {phv_16b_2, 1U << phv_16b_2, 0, 16},
                          {phv_16b_1, 1U << phv_16b_1, 0, 16},
                          {phv_16b_0, 1U << phv_16b_0, 0, 16},
                          {phv_8b_2, 3U << phv_8b_2, 8, 8},
                          {phv_8b_0, 3U << phv_8b_0, 8, 8},
                          {0, 0, 0, 0}},
  phv_8b_csum_slots[] = {{phv_8b_3, 1U << phv_8b_3, 0, 8},
                         {phv_8b_2, 1U << phv_8b_2, 0, 8},
                         {phv_8b_1, 1U << phv_8b_1, 0, 8},
                         {phv_8b_0, 1U << phv_8b_0, 0, 8},
                         {0, 0, 0, 0}};

static phv_use_slots *get_phv_use_slots(int size) {
    phv_use_slots *usable_slots = nullptr;

    if (size == 32)
        usable_slots = phv_32b_slots;
    else if (size == 16)
        usable_slots = phv_16b_slots;
    else if (size == 8)
        usable_slots = phv_8b_slots;
    else
        BUG();

    return usable_slots;
}

static phv_use_slots *get_phv_csum_use_slots(int size) {
    phv_use_slots *usable_slots = nullptr;

    if (size == 32)
        usable_slots = phv_32b_csum_slots;
    else if (size == 16)
        usable_slots = phv_16b_csum_slots;
    else if (size == 8)
        usable_slots = phv_8b_csum_slots;
    else
        BUG();

    return usable_slots;
}

// ----------------------------------------------------------------------------
// Helping classes
// ----------------------------------------------------------------------------

/// Helping cache to remember values for a different parser objects
/// based on the type of extraction size. The storage is done into two
/// layers and user is free to specify the values of layer 1 and layer 2
/// types. The third type specifies the return value
template <typename T1, typename T2, typename T3>
class TwoLevelCache {
    std::map<T1, std::map<T2, T3>> m_cache;

 public:
    void insert(const T1 key1, T2 key2, T3 val) { m_cache[key1][key2] = val; }

    bool has(const T1 key1, T2 key2) const {
        if (!m_cache.count(key1)) return false;
        auto level1 = m_cache.at(key1);

        return level1.count(key2);
    }

    T3 get(const T1 key1, T2 key2) const { return m_cache.at(key1).at(key2); }
};

/**
 * @brief This class is used for internal tracking of Tofino output map
 * extractor allocation. It is beneficial during the debugging process of this
 * functionality because it can provid the answer to question:
 * "What is alloacted into this extractor slot?"
 */
class MatchSlotTracker {
    using SetMap =
        TwoLevelCache<const Parser::State::Match *, int, const Parser::State::Match::Set *>;
    using SaveMap =
        TwoLevelCache<const Parser::State::Match *, int, const Parser::State::Match::Save *>;
    using CsumMap = TwoLevelCache<const Parser::State::Match *, int, const Parser::Checksum *>;
    using PaddingMap = TwoLevelCache<const Parser::State::Match *, int, tofino_phv_output_map *>;

 public:
    // Helping caches for tracking of slot occupancy mapping
    SetMap setMap;
    SaveMap saveMap;
    CsumMap csumMap;
    PaddingMap padMap;

    /**
     * @brief Get the db slots object
     *
     * @param match Match line to dump
     * @param slot_idx Passed index of slot which needs to be dumped
     * @return std::string object with dumped data
     */
    std::string get_db_slots(const Parser::State::Match *match, const int slot_idx) const {
        std::stringstream ss;
        ss << "Mapping for state " << match->state->name;
        if (match->match) ss << ", match " << match->match;
        ss << ", slot " << slot_idx << ": ";

        if (setMap.has(match, slot_idx)) {
            auto set = setMap.get(match, slot_idx);
            ss << "set, " << set->where;
        } else if (saveMap.has(match, slot_idx)) {
            auto save = saveMap.get(match, slot_idx);
            ss << "save, " << save->where;
        } else if (csumMap.has(match, slot_idx)) {
            auto csum = csumMap.get(match, slot_idx);
            ss << "csum, " << csum->dest;
        } else if (padMap.has(match, slot_idx)) {
            ss << "fake extraction padding, " << padMap.get(match, slot_idx);
        } else {
            ss << "<unknown>";
        }

        return ss.str();
    }
};

static MatchSlotTracker matchSlotTracker;

// ----------------------------------------------------------------------------
//  Parser configuration dump
// ----------------------------------------------------------------------------

template <>
void Parser::Checksum::write_config(Target::Tofino::parser_regs &regs, Parser *parser) {
    if (unit == 0)
        write_tofino_row_config(regs.memory[gress].po_csum_ctrl_0_row[addr]);
    else if (unit == 1)
        write_tofino_row_config(regs.memory[gress].po_csum_ctrl_1_row[addr]);
    else
        error(lineno, "invalid unit for parser checksum");
}

template <>
void Parser::CounterInit::write_config(Target::Tofino::parser_regs &regs, gress_t gress, int idx) {
    auto &ctr_init_ram = regs.memory[gress].ml_ctr_init_ram[idx];
    ctr_init_ram.add = add;
    ctr_init_ram.mask = mask;
    ctr_init_ram.rotate = rot;
    ctr_init_ram.max = max;
    ctr_init_ram.src = src;
}

template <>
void Parser::RateLimit::write_config(::Tofino::regs_pipe &regs, gress_t gress) {
    if (gress == INGRESS) {
        auto &ctrl = regs.pmarb.parb_reg.parb_group.i_output_rate_ctrl;
        ctrl.ratectrl_inc = inc;
        ctrl.ratectrl_dec = dec;
        ctrl.ratectrl_max = max;
        ctrl.ratectrl_ena = 1;
    } else if (gress == EGRESS) {
        auto &ctrl = regs.pmarb.parb_reg.parb_group.e_output_rate_ctrl;
        ctrl.ratectrl_inc = inc;
        ctrl.ratectrl_dec = dec;
        ctrl.ratectrl_max = max;
        ctrl.ratectrl_ena = 1;
    }
}

template <>
void Parser::State::Match::write_lookup_config(Target::Tofino::parser_regs &regs, State *state,
                                               int row) const {
    auto &word0 = regs.memory[state->gress].ml_tcam_row_word0[row];
    auto &word1 = regs.memory[state->gress].ml_tcam_row_word1[row];
    match_t lookup = {0, 0};
    unsigned dont_care = 0;
    for (int i = 0; i < 4; i++) {
        lookup.word0 <<= 8;
        lookup.word1 <<= 8;
        dont_care <<= 8;
        if (state->key.data[i].bit >= 0) {
            lookup.word0 |= ((match.word0 >> state->key.data[i].bit) & 0xff);
            lookup.word1 |= ((match.word1 >> state->key.data[i].bit) & 0xff);
        } else {
            dont_care |= 0xff;
        }
    }
    lookup.word0 |= dont_care;
    lookup.word1 |= dont_care;
    word0.lookup_16 = (lookup.word0 >> 16) & 0xffff;
    word1.lookup_16 = (lookup.word1 >> 16) & 0xffff;
    word0.lookup_8[0] = (lookup.word0 >> 8) & 0xff;
    word1.lookup_8[0] = (lookup.word1 >> 8) & 0xff;
    word0.lookup_8[1] = lookup.word0 & 0xff;
    word1.lookup_8[1] = lookup.word1 & 0xff;
    word0.curr_state = state->stateno.word0;
    word1.curr_state = state->stateno.word1;
    if (state->key.ctr_zero >= 0) {
        word0.ctr_zero = (match.word0 >> state->key.ctr_zero) & 1;
        word1.ctr_zero = (match.word1 >> state->key.ctr_zero) & 1;
    } else {
        word0.ctr_zero = word1.ctr_zero = 1;
    }

    if (state->key.ctr_neg >= 0) {
        word0.ctr_neg = (match.word0 >> state->key.ctr_neg) & 1;
        word1.ctr_neg = (match.word1 >> state->key.ctr_neg) & 1;
    } else {
        word0.ctr_neg = word1.ctr_neg = 1;
    }

    word0.ver_0 = word1.ver_0 = 1;
    word0.ver_1 = word1.ver_1 = 1;
}

/* FIXME -- combine these next two methods into a single method on MatchKey */
/* FIXME -- factor Tofino/JBay variation better (most is common) */
template <>
int Parser::State::write_lookup_config(Target::Tofino::parser_regs &regs, Parser *pa, State *state,
                                       int row, const std::vector<State *> &prev) {
    LOG2("-- checking match from state " << name << " (" << stateno << ')');
    auto &ea_row = regs.memory[gress].ml_ea_row[row];
    int max_off = -1;
    for (int i = 0; i < 4; i++) {
        if (i == 1) continue;
        if (key.data[i].bit < 0) continue;
        bool set = true;
        for (State *p : prev) {
            if (p->key.data[i].bit >= 0) {
                set = false;
                if (p->key.data[i].byte != key.data[i].byte)
                    error(p->lineno,
                          "Incompatible match fields between states "
                          "%s and %s, triggered from state %s",
                          name.c_str(), p->name.c_str(), state->name.c_str());
            }
        }
        if (set && key.data[i].byte != MatchKey::USE_SAVED) {
            int off = key.data[i].byte + ea_row.shift_amt;
            if (off < 0 || off >= 32) {
                error(key.lineno,
                      "Match offset of %d in state %s out of range "
                      "for previous state %s",
                      key.data[i].byte, name.c_str(), state->name.c_str());
            } else if (i) {
                ea_row.lookup_offset_8[(i - 2)] = off;
                ea_row.ld_lookup_8[(i - 2)] = 1;
                max_off = std::max(max_off, off);
            } else {
                ea_row.lookup_offset_16 = off;
                ea_row.ld_lookup_16 = 1;
                max_off = std::max(max_off, off + 1);
            }
        }
    }
    return max_off;
}

template <>
int Parser::State::Match::write_load_config(Target::Tofino::parser_regs &regs, Parser *pa,
                                            State *state, int row) const {
    auto &ea_row = regs.memory[state->gress].ml_ea_row[row];
    int max_off = -1;
    for (int i = 0; i < 4; i++) {
        if (i == 1) continue;
        if (load.data[i].bit < 0) continue;
        if (load.data[i].byte != MatchKey::USE_SAVED) {
            int off = load.data[i].byte;
            if (off < 0 || off >= 32) {
                error(load.lineno, "Load offset of %d in state %s out of range", load.data[i].byte,
                      state->name.c_str());
            } else if (i) {
                ea_row.lookup_offset_8[(i - 2)] = off;
                ea_row.ld_lookup_8[(i - 2)] = 1;
                max_off = std::max(max_off, off);
            } else {
                ea_row.lookup_offset_16 = off;
                ea_row.ld_lookup_16 = 1;
                max_off = std::max(max_off, off + 1);
            }
        }
    }
    return max_off;
}

// Narrow-to-wide extraction alignment needs adjusting when
// 8b/16b checksum validations are written in the same cycle
bool adjust_phv_use_slot(phv_use_slots &slot, int size, int csum_8b, int csum_16b) {
    if ((size == 32 && slot.idx >= phv_16b_0) || (size == 16 && slot.idx >= phv_8b_0)) {
        if (slot.idx <= phv_16b_3) {
            slot.idx -= csum_16b;
            slot.usemask >>= csum_16b;
            return slot.idx >= phv_16b_0;
        } else {
            slot.idx -= csum_8b;
            slot.usemask >>= csum_8b;
            return slot.idx >= phv_8b_0;
        }
    }
    return true;
}

template <>
void Parser::Checksum::write_output_config(Target::Tofino::parser_regs &regs, Parser *pa,
                                           State::Match *ma, void *_map, unsigned &used) const {
    if (type != 0 || !dest) return;

    // checksum verification requires the last extractor to be a dummy (to work around a RTL bug)
    // see MODEL-210 for discussion.

    tofino_phv_output_map *map = reinterpret_cast<tofino_phv_output_map *>(_map);

    phv_use_slots *usable_slots = get_phv_csum_use_slots(dest->reg.size);

    auto &slot = usable_slots[0];

    auto id = dest->reg.parser_id();
    *map[slot.idx].dst = id;
    matchSlotTracker.csumMap.insert(ma, slot.idx, this);
    // The source address is checked for source extract errors whenever the dest
    // is not 511. To prevent errors when buf_req = 0 (corresponding to states with no extracts),
    // point the source to the version area of the source range which is always valid.
    *map[slot.idx].src = PARSER_SRC_MAX_IDX - (dest->reg.size / 8) + 1;
    used |= slot.usemask;

    pa->phv_allow_bitwise_or[id] = 1;
}

template <>
int Parser::State::Match::Save::write_output_config(Target::Tofino::parser_regs &regs, void *_map,
                                                    unsigned &used, int csum_8b,
                                                    int csum_16b) const {
    tofino_phv_output_map *map = reinterpret_cast<tofino_phv_output_map *>(_map);

    int slot_size = (hi - lo + 1) * 8;
    phv_use_slots *usable_slots = get_phv_use_slots(slot_size);

    for (int i = 0; usable_slots[i].usemask; i++) {
        auto slot = usable_slots[i];
        if (!adjust_phv_use_slot(slot, where->reg.size, csum_8b, csum_16b)) continue;
        if (used & slot.usemask) continue;
        if ((flags & ROTATE) && !map[slot.idx].offset_rot) continue;

        if ((where->reg.size == 32 && slot.idx >= phv_16b_0) ||
            (where->reg.size == 16 && slot.idx >= phv_8b_0)) {
            match->has_narrow_to_wide_extract = true;

            if (where->reg.size == 32 && slot.idx == phv_8b_0) {
                match->narrow_to_wide_32b_8.push_back(&where);
            } else if (where->reg.size == 32 && slot.idx >= phv_16b_0) {
                match->narrow_to_wide_32b_16.push_back(&where);
            } else {
                match->narrow_to_wide_16b_8.push_back(&where);
            }
        }

        // swizzle upper/lower pairs of extractors for 4x8->32
        // a 32b value using 8b extractors must use the extractors in this order: [2 3 0 1]
        bool swizzle_b1 = where->reg.size == 32 && slot.idx == phv_8b_0;

        int byte = lo;
        for (int i = slot.idx; slot.usemask & (1U << i); i++, byte += slot.size / 8U) {
            int x = i;
            if (swizzle_b1) x ^= 2;

            *map[x].dst = where->reg.parser_id();
            *map[x].src = byte;
            matchSlotTracker.saveMap.insert(match, x, this);
            if (flags & OFFSET) *map[x].offset_add = 1;
            if (flags & ROTATE) *map[x].offset_rot = 1;
        }
        used |= slot.usemask;
        return hi;
    }
    error(where.lineno, "Ran out of phv output extractor slots");
    return -1;
}

bool can_slot_extract_constant(int slot) {
    return slot != phv_16b_2 && slot != phv_16b_3 && slot != phv_32b_2 && slot != phv_32b_3;
}

/**
 * @brief Encode constant @p val for use with extractor slot @p slot.
 *
 * @param slot Valid value of enum extractor_slot
 * @param val Constant to encode
 * @return int The encoded constant, or -1 if given @p slot cannot extract a constant or is not a
 * valid value of enum extractor_slot
 */
static int encode_constant_for_slot(int slot, unsigned val) {
    if (!can_slot_extract_constant(slot)) return -1;
    if (val == 0) return val;
    switch (slot) {
        case phv_32b_0:
        case phv_32b_1:
            for (int i = 0; i < 32; i++) {
                if ((val & 1) && (0x7 & val) == val) return (i << 3) | val;
                val = ((val >> 1) | (val << 31)) & 0xffffffffU;
            }
            return -1;
        case phv_16b_0:
        case phv_16b_1:
            if ((val >> 16) && encode_constant_for_slot(slot, val >> 16) < 0) return -1;
            val &= 0xffff;
            for (int i = 0; i < 16; i++) {
                if ((val & 1) && (0xf & val) == val) return (i << 4) | val;
                val = ((val >> 1) | (val << 15)) & 0xffffU;
            }
            return -1;
        case phv_8b_0:
        case phv_8b_1:
        case phv_8b_2:
        case phv_8b_3:
            return val & 0xff;
        default:
            BUG();
            return -1;
    }
}

template <>
void Parser::State::Match::Set::write_output_config(Target::Tofino::parser_regs &regs, void *_map,
                                                    unsigned &used, int csum_8b,
                                                    int csum_16b) const {
    tofino_phv_output_map *map = reinterpret_cast<tofino_phv_output_map *>(_map);

    phv_use_slots *usable_slots = get_phv_use_slots(where->reg.size);

    for (int i = 0; usable_slots[i].usemask; i++) {
        auto slot = usable_slots[i];
        if (!adjust_phv_use_slot(slot, where->reg.size, csum_8b, csum_16b)) continue;
        if (used & slot.usemask) continue;
        if (!map[slot.idx].src_type) continue;
        if ((flags & ROTATE) && (!map[slot.idx].offset_rot || slot.shift)) continue;
        unsigned shift = 0;
        bool can_encode = true;
        for (int i = slot.idx; slot.usemask & (1U << i); i++) {
            if (encode_constant_for_slot(i, (what << where->lo) >> shift) < 0) {
                can_encode = false;
                break;
            }
            shift += slot.size;
        }
        if (!can_encode) continue;

        if ((where->reg.size == 32 && slot.idx >= phv_16b_0) ||
            (where->reg.size == 16 && slot.idx >= phv_8b_0)) {
            match->has_narrow_to_wide_extract = true;

            if (where->reg.size == 32 && slot.idx == phv_8b_0) {
                match->narrow_to_wide_32b_8.push_back(&where);
            } else if (where->reg.size == 32 && slot.idx >= phv_16b_0) {
                match->narrow_to_wide_32b_16.push_back(&where);
            } else {
                match->narrow_to_wide_16b_8.push_back(&where);
            }
        }

        // swizzle upper/lower pairs of extractors for 4x8->32
        // a 32b value using 8b extractors must use the extractors in this order: [2 3 0 1]
        bool swizzle_b1 = where->reg.size == 32 && slot.idx == phv_8b_0;

        // Go from most- to least-significant slice
        shift = where->reg.size - slot.size;
        for (int i = slot.idx; slot.usemask & (1U << i); i++) {
            int x = i;
            if (swizzle_b1) x ^= 2;

            *map[x].dst = where->reg.parser_id();
            *map[x].src_type = 1;
            auto v = encode_constant_for_slot(x, (what << where->lo) >> shift);
            *map[x].src = v;
            matchSlotTracker.setMap.insert(match, x, this);
            if (flags & OFFSET) *map[x].offset_add = 1;
            if (flags & ROTATE) *map[x].offset_rot = 1;
            shift -= slot.size;
        }
        used |= slot.usemask;
        return;
    }
    error(where.lineno, "Ran out of phv output extractor slots");
}

/** Tofino1-specific output map management
 * Tofino1 has separate 8- 16- and 32-bit extractors with various limitations on extracting
 * constants and capability of ganging extractors to extract larger PHVs or extrating adjacent
 * pairs of smaller PHVs.  They're also addressed via named registers rather than an array,
 * so we build an array of pointers into the reg object to simplify things.  The `used`
 * value ends up begin a simple 12-bit bitmap with 1 bit for each extractor.
 */

#define OUTPUT_MAP_INIT(MAP, ROW, SIZE, INDEX)                                         \
    MAP[phv_##SIZE##b_##INDEX].size = SIZE;                                            \
    MAP[phv_##SIZE##b_##INDEX].dst = &ROW.phv_##SIZE##b_dst_##INDEX;                   \
    MAP[phv_##SIZE##b_##INDEX].src = &ROW.phv_##SIZE##b_src_##INDEX;                   \
    MAP[phv_##SIZE##b_##INDEX].src_type = &ROW.phv_##SIZE##b_src_type_##INDEX;         \
    MAP[phv_##SIZE##b_##INDEX].offset_add = &ROW.phv_##SIZE##b_offset_add_dst_##INDEX; \
    MAP[phv_##SIZE##b_##INDEX].offset_rot = &ROW.phv_##SIZE##b_offset_rot_imm_##INDEX;
#define OUTPUT_MAP_INIT_PART(MAP, ROW, SIZE, INDEX)                                    \
    MAP[phv_##SIZE##b_##INDEX].size = SIZE;                                            \
    MAP[phv_##SIZE##b_##INDEX].dst = &ROW.phv_##SIZE##b_dst_##INDEX;                   \
    MAP[phv_##SIZE##b_##INDEX].src = &ROW.phv_##SIZE##b_src_##INDEX;                   \
    MAP[phv_##SIZE##b_##INDEX].src_type = 0;                                           \
    MAP[phv_##SIZE##b_##INDEX].offset_add = &ROW.phv_##SIZE##b_offset_add_dst_##INDEX; \
    MAP[phv_##SIZE##b_##INDEX].offset_rot = 0;

template <>
void *Parser::setup_phv_output_map(Target::Tofino::parser_regs &regs, gress_t gress, int row) {
    static tofino_phv_output_map map[tofino_phv_output_map_size];
    auto &action_row = regs.memory[gress].po_action_row[row];
    OUTPUT_MAP_INIT(map, action_row, 32, 0)
    OUTPUT_MAP_INIT(map, action_row, 32, 1)
    OUTPUT_MAP_INIT_PART(map, action_row, 32, 2)
    OUTPUT_MAP_INIT_PART(map, action_row, 32, 3)
    OUTPUT_MAP_INIT(map, action_row, 16, 0)
    OUTPUT_MAP_INIT(map, action_row, 16, 1)
    OUTPUT_MAP_INIT_PART(map, action_row, 16, 2)
    OUTPUT_MAP_INIT_PART(map, action_row, 16, 3)
    OUTPUT_MAP_INIT(map, action_row, 8, 0)
    OUTPUT_MAP_INIT(map, action_row, 8, 1)
    OUTPUT_MAP_INIT(map, action_row, 8, 2)
    OUTPUT_MAP_INIT(map, action_row, 8, 3)
    return map;
}

template <>
void Parser::mark_unused_output_map(Target::Tofino::parser_regs &regs, void *_map, unsigned used) {
    tofino_phv_output_map *map = reinterpret_cast<tofino_phv_output_map *>(_map);
    for (int i = 0; i < tofino_phv_output_map_size; i++)
        if (!(used & (1U << i))) *map[i].dst = 0x1ff;
}

template <>
void Parser::State::Match::HdrLenIncStop::write_config(
    Tofino::memories_all_parser_::_po_action_row &) const {
    BUG();  // no hdr_len_inc_stop on tofino; should not get here
}

template <>
void Parser::State::Match::Clot::write_config(Tofino::memories_all_parser_::_po_action_row &, int,
                                              bool) const {
    BUG();  // no CLOTs on tofino; should not get here
}

template <>
void Parser::State::Match::write_counter_config(
    Target::Tofino::parser_regs::_memory::_ml_ea_row &ea_row) const {
    ea_row.ctr_amt_idx = ctr_instr ? ctr_instr->addr : ctr_imm_amt;
    ea_row.ctr_ld_src = ctr_ld_src;
    ea_row.ctr_load = ctr_load;
}

template <class COMMON>
void init_common_regs(Parser *p, COMMON &regs, gress_t gress) {
    // TODO: fixed config copied from compiler -- needs to be controllable
    for (int i = 0; i < 4; i++) {
        if (p->start_state[i]) {
            regs.start_state.state[i] = p->start_state[i]->stateno.word1;
            regs.enable_.enable_[i] = 1;
        }
        regs.pri_start.pri[i] = p->priority[i];
        regs.pri_thresh.pri[i] = p->pri_thresh[i];
    }
    regs.mode = 4;
    regs.max_iter.max = 128;
    if (p->parser_error.lineno >= 0) {
        regs.err_phv_cfg.dst = p->parser_error->reg.parser_id();
        regs.err_phv_cfg.aram_mbe_en = 1;
        regs.err_phv_cfg.ctr_range_err_en = 1;
        regs.err_phv_cfg.dst_cont_err_en = 1;
        regs.err_phv_cfg.fcs_err_en = 1;
        regs.err_phv_cfg.multi_wr_err_en = 1;
        regs.err_phv_cfg.no_tcam_match_err_en = 1;
        regs.err_phv_cfg.partial_hdr_err_en = 1;
        regs.err_phv_cfg.phv_owner_err_en = 1;
        regs.err_phv_cfg.src_ext_err_en = 1;
        regs.err_phv_cfg.timeout_cycle_err_en = 1;
        regs.err_phv_cfg.timeout_iter_err_en = 1;
    }
}

enum class AnalysisType { BIT8, BIT16 };
using extractor_slots_list = std::initializer_list<extractor_slots>;
const extractor_slots_list phv_8bit_extractors = {phv_8b_0, phv_8b_1, phv_8b_2, phv_8b_3};
const extractor_slots_list phv_16bit_extractors = {phv_16b_0, phv_16b_1, phv_16b_2, phv_16b_3};
// Declare a helping type for the count cache class
using ExtractionCountCache = TwoLevelCache<const Parser::State::Match *, AnalysisType, int>;

/// Count the number of extractions for a given @p match.
/// The method takes the @p elems list which holds PHV indexes to check
/// (accepted lists are @p phv_8bit_extractors and @p phv_16_bit_extractors).
int count_number_of_extractions(Parser *parser, Target::Tofino::parser_regs &regs,
                                Parser::State::Match *match, const AnalysisType type) {
    int used = 0;
    int row = parser->match_to_row.at(match);
    auto map = reinterpret_cast<tofino_phv_output_map *>(
        parser->setup_phv_output_map(regs, parser->gress, row));

    auto elems = type == AnalysisType::BIT8 ? phv_8bit_extractors : phv_16bit_extractors;
    for (auto i : elems) {
        if (map[i].dst->value != EXTRACT_SLOT_UNUSED) {
            used++;
        }
    }

    return used;
}

/// Pad collector object which provides mapping from a narrow-to-wide match
/// to added padding
class PaddingInfoCollector {
 public:
    struct PadInfo {
        /// The number of added extractors to work correctly
        int m_count8;
        int m_count16;

        PadInfo() {
            m_count8 = 0;
            m_count16 = 0;
        }

        void add(const AnalysisType type, const int val) {
            if (type == AnalysisType::BIT8) {
                m_count8 += val;
            } else {
                m_count16 += val;
            }
        }
    };

    /// Information for one parser state where the padding
    // is being added
    struct PadState {
        /// Added padding information into parser states (successors or predecessors)
        std::map<Parser::State::Match *, PadInfo *> m_padding;

        void addPadInfo(Parser::State::Match *match, AnalysisType pad, int count) {
            if (count == 0) return;

            if (!m_padding.count(match)) {
                m_padding[match] = new PadInfo;
            }

            m_padding[match]->add(pad, count);
        }

        bool hasPadInfo() { return m_padding.size() != 0; }

        void print() {
            std::stringstream message;
            message << " Pad State Info : " << std::endl;
            for (auto &m : m_padding) {
                message << " \t State(match) : " << m.first->state->name << "(" << m.first->match
                        << ")" << " -> { m_count8 : " << m.second->m_count8
                        << ", m_count16 : " << m.second->m_count16 << " }" << std::endl;
            }
            LOG1(message.str());
        }
    };

    PadState *getPadState(Parser::State::Match *match) {
        if (!m_nrw_matches.count(match)) {
            m_nrw_matches[match] = new PadState;
        }

        return m_nrw_matches[match];
    }

    void printPadInfo() {
        for (auto s : m_nrw_matches) {
            auto nrw_match = s.first;
            auto info_collector = s.second;
            // Skip the info if we don't have any stored padding
            if (!info_collector->hasPadInfo()) {
                continue;
            }

            std::stringstream message;
            message << "State " << nrw_match->state->name;
            if (nrw_match->match == true) {
                message << ", match " << nrw_match->match;
            }

            message << " is using the narrow-to-wide extraction: " << std::endl;

            if (nrw_match->narrow_to_wide_32b_16.size() != 0) {
                message << "\t* 32 bit extractors are replaced by 2 x 16 bit extractors: ";
                for (auto ref : nrw_match->narrow_to_wide_32b_16) {
                    message << ref->name() << " ";
                }
                message << std::endl;
            }

            if (nrw_match->narrow_to_wide_32b_8.size() != 0) {
                message << "\t* 32 bit extractors are replaced by 4 x 8 bit extractors: ";
                for (auto ref : nrw_match->narrow_to_wide_32b_8) {
                    message << ref->name() << " ";
                }
                message << std::endl;
            }

            if (nrw_match->narrow_to_wide_16b_8.size() != 0) {
                message << "\t* 16 bit extractors are replaced by 2 x 8 bit extractors: ";
                for (auto ref : nrw_match->narrow_to_wide_16b_8) {
                    message << ref->name() << " ";
                }
                message << std::endl;
            }

            message
                << "The following extractions need to be added to parser states to work correctly:"
                << std::endl;

            for (auto pad : info_collector->m_padding) {
                auto match = pad.first;
                auto pad_info = pad.second;

                message << "\t* State " << match->state->name;
                if (match->match == true) {
                    message << ", match " << match->match;
                }

                message << " needs " << pad_info->m_count8 << " x 8 bit and " << pad_info->m_count16
                        << " x 16 bit extractions to be added" << std::endl;
            }

            LOG1("WARNING: " << message.str());
        }
    }

 private:
    /// This provides mapping between the narrow-to-wide (NRW) match and collected
    /// padding information
    std::map<Parser::State::Match *, PadState *> m_nrw_matches;
};

/// Size of internal parser FIFO
static const int parser_fifo_size = 32;

/// Compute the @p val / @p div and ceil it to the nearest upper value. The result
/// will be wrapped to the FIFO size in parser.
int ceil_and_wrap_to_fifo_size(int val, int div) {
    int fifo_items = val > parser_fifo_size ? parser_fifo_size : val;
    return (fifo_items + div - 1) / div;
}

int analyze_worst_extractor_path(Parser *parser, Target::Tofino::parser_regs &regs,
                                 Parser::State::Match *match, AnalysisType type,
                                 std::set<Parser::State *> &visited, ExtractionCountCache &cache) {
    if (visited.count(match->state)) {
        // We have found a node in a loop --> we will get via our predessors into the same state.
        // This means that we can take this loop many times and that we need to distribute the
        // maximal FIFO value to our parets (we are predecessors of our parents).
        //
        // IN SUCH CASE THE NUMBER OF EXTRACTIONS FOR ALL SUCCESSORS DOESN'T CATCH
        // THE REALITY. IT IS JUST FOR THE SIMULATION OF FULL PARSER FIFO BLOCKS.
        // REALITY IS COVERED WHEN THE GRAPH DOESN'T HAVE LOOPS
        return parser_fifo_size;
    }

    if (LOGGING(3)) {
        std::stringstream ss;
        ss << "Processing match " << match->state->name << ", gress = " << match->state->gress;
        if (match->match) {
            ss << ", match " << match->match;
        }
        LOG3(ss.str());
    }

    // Check the cache if we know the result
    if (cache.has(match, type)) {
        return cache.get(match, type);
    }

    // Mark node as visited and run the analysis
    visited.insert(match->state);
    int extractions = count_number_of_extractions(parser, regs, match, type);
    int pred_extractions = 0;
    for (auto pred : match->state->pred) {
        pred_extractions =
            std::max(pred_extractions,
                     analyze_worst_extractor_path(parser, regs, pred, type, visited, cache));
    }

    // Insert the result into the cache and unmark the node as visited
    visited.erase(match->state);
    int extraction_result = extractions + pred_extractions;
    cache.insert(match, type, extraction_result);

    return extraction_result;
}

/**
 * @brief Dump the occupancy of extraction slots in output map
 *
 * @param match Current match which is being processed
 * @param indexes Indexes to inspect
 * @param prefix Prefix to add before the print
 */
static void print_slot_occupancy(const Parser::State::Match *match,
                                 const std::initializer_list<extractor_slots> indexes,
                                 const std::string prefix = "") {
    // Print the prefix if not empty, iterate over checked indexes and
    // print slot occupancy information.
    std::stringstream ss;
    std::string sep;
    if (prefix != "") {
        ss << prefix << " : " << std::endl;
        sep = "\t* ";
    }

    for (auto idx : indexes) {
        ss << sep << matchSlotTracker.get_db_slots(match, idx) << std::endl;
    }

    auto output = ss.str();
    if (output.size() == 0) return;
    LOG5(output);
}

/**
 * @brief Set the @p pad_idx or @p from_idx based on the used extractor scenario
 *
 * @param pad_idx Input/output for padding index
 * @param from_idx Input/output for source index
 * @param used Number of used extractors
 * @param has_csum State is using the VERIFY checksum
 * @param match State match which is being processed
 * @param map Pointer on the output map configuration
 */
static void set_idx_for_16b_extractions(unsigned &pad_idx, unsigned &from_idx, const unsigned used,
                                        const bool has_csum, const Parser::State::Match *match,
                                        struct tofino_phv_output_map *map) {
    if (used == 1) {
        // One extractor is being used and the index is stored in from_idx. The allocation
        // strategy here is to keep data in tuples {0,1} and {2,3}.
        if (from_idx == phv_16b_0 || from_idx == phv_16b_2) {
            pad_idx = from_idx + 1;
        } else if (from_idx == phv_16b_1 || from_idx == phv_16b_3) {
            pad_idx = from_idx - 1;
        } else {
            // We should never reach this point
            error(match->lineno,
                  "Cannot identify index for 16bit extractor padding (1 extractor)!");
        }
    } else {
        // Three extractors are used and the unused extractor index is stored in
        // the pad_idx variable. We can keep indexes in tuples
        if (has_csum) {
            from_idx = phv_16b_3;
        } else if (pad_idx == phv_16b_0 || pad_idx == phv_16b_2) {
            from_idx = pad_idx + 1;
        } else if (pad_idx == phv_16b_1 || pad_idx == phv_16b_3) {
            from_idx = pad_idx - 1;
        } else {
            // We should never reach this point
            error(match->lineno,
                  "Cannot identify index for 16bit extractor padding (3 extractors)!");
        }
    }
}

/**
 * @brief Verify all invariants for the extractor padding configuration
 *
 * @param pad_idx Input/output for padding index
 * @param from_idx Input/output for source index
 * @param used Number of used extractors
 * @param has_csum State is using the VERIFY checksum
 * @param match State match which is being processed
 * @param map Pointer on the output map configuration
 */
static void check_16b_extractor_configuration(const unsigned pad_idx, const unsigned from_idx,
                                              const unsigned used, const bool has_csum,
                                              const struct tofino_phv_output_map *map) {
    // 1] Indexes are kept in tuples {0,1} and {2,3}. Checksum means that from_idx is
    // set to the last extractor.
    bool csum = has_csum && (from_idx == phv_16b_3);
    bool first_tuple = (from_idx == phv_16b_0 || from_idx == phv_16b_1) && (pad_idx <= phv_16b_1);
    bool second_tuple = (from_idx == phv_16b_2 || from_idx == phv_16b_3) && (pad_idx >= phv_16b_2);
    BUG_CHECK(has_csum || first_tuple || second_tuple,
              "Source and destination index are not configured correctly for 16bit 2n extractor "
              "padding!");

    // 2] All indexes are sourced from global version field which is tied to zeros.
    // The Checksum case means that we need to set the from index on the last 16b extractor
    BUG_CHECK(map[pad_idx].dst->value != EXTRACT_SLOT_UNUSED,
              "Invalid extractor destination for 16bit 2n padding!");
    if (has_csum) {
        BUG_CHECK(from_idx == phv_16b_3,
                  "Invalid from_idx for the 16bit 2n padding with checksum!");
    }

    // Check the slot configuration - sourcing from global field and no constant for {0,1}
    BUG_CHECK(
        *map[pad_idx].src >= PARSER_SRC_MAX_IDX - 3 && *map[pad_idx].src != EXTRACT_SLOT_UNUSED,
        "Field is not sourcing from the global version field!");
    if (pad_idx == phv_16b_0 || pad_idx == phv_16b_1) {
        BUG_CHECK(*map[pad_idx].src_type == 0,
                  "Invalid configuration of the source type for 16b 2n padding!");
    }
}

//
// uArch for Tofino Parser:
// * Parser Output section
// * Checksum section
// * Parse Merge section

/**
 * @brief Perform the 16b padding onto map and computed index.
 *
 * @param regs Register configuration instance
 * @param map Tofino output map pointer
 * @param pad_idx Index which needs to be padded
 * @param from_idx Index which is the source of the slot configuration
 */
static void do_16b_padding(Target::Tofino::parser_regs &regs, tofino_phv_output_map *map,
                           const unsigned pad_idx, const unsigned from_idx) {
    // Add fake extractors to reach 2n constraint, we need to copy destination and source from
    // the global version field which is tied to zeros in RTL.
    // We are keeping both indexes in tuples {0,1} or {2,3}.
    map[pad_idx].dst->rewrite();
    *map[pad_idx].dst = *map[from_idx].dst;
    *map[pad_idx].src = PARSER_SRC_MAX_IDX - 1;
    if (pad_idx < phv_16b_2) {
        // Even though extractors {0, 1} can extract from constants, we want to extract
        // from the tied-to-zero global version field for consistency across all 16b extractors
        *map[pad_idx].src_type = EXTRACT_SLOT_CONSTANT_DIS;
    }

    // Mark the dummy write dest as multi-write, we need to distinguish between PHV and TPHV
    if (*map[from_idx].dst >= PHV_MIN_INDEX && *map[from_idx].dst < PHV_MAX_INDEX) {
        regs.ingress.prsr_reg.no_multi_wr.nmw[*map[from_idx].dst].rewrite();
        regs.egress.prsr_reg.no_multi_wr.nmw[*map[from_idx].dst].rewrite();

        regs.ingress.prsr_reg.no_multi_wr.nmw[*map[from_idx].dst] = 0;
        regs.egress.prsr_reg.no_multi_wr.nmw[*map[from_idx].dst] = 0;
    } else if (*map[from_idx].dst >= TPHV_MIN_INDEX && *map[from_idx].dst < TPHV_MAX_INDEX) {
        auto tphv_idx = *map[from_idx].dst - TPHV_MIN_INDEX;
        regs.ingress.prsr_reg.no_multi_wr.t_nmw[tphv_idx].rewrite();
        regs.egress.prsr_reg.no_multi_wr.t_nmw[tphv_idx].rewrite();

        regs.ingress.prsr_reg.no_multi_wr.t_nmw[tphv_idx] = 0;
        regs.egress.prsr_reg.no_multi_wr.t_nmw[tphv_idx] = 0;
    }
}

/// Add the fake extractions to have 2n 16b extractions. The function returns
/// the number of added extractions.
static int pad_to_16b_extracts_to_2n(Parser *parser, Target::Tofino::parser_regs &regs,
                                     Parser::State::Match *match) {
    // Obtain the slot configuration for given row
    int row = parser->match_to_row.at(match);
    auto map = reinterpret_cast<tofino_phv_output_map *>(
        parser->setup_phv_output_map(regs, parser->gress, row));
    if (LOGGING(5)) {
        print_slot_occupancy(match, phv_16bit_extractors, "Before 16bit padding");
    }

    // Count number of used extractors - the number of extractions/constant set operations and
    // checksums should be the padded to 2n.
    unsigned used = 0;
    unsigned pad_idx = 0;
    unsigned from_idx = 0;
    bool from_idx_is_8b_16b = false;
    for (auto i : phv_16bit_extractors) {
        if (map[i].dst->value == EXTRACT_SLOT_UNUSED) {
            pad_idx = i;
        } else {
            used++;
            // Try to get an 8b or 16b index.
            // If we use a single 32b index to pad then we'll hit problems
            // because we need 2 x 16b to fill a 32b container.
            if (!from_idx_is_8b_16b) {
                from_idx = i;
                auto *reg = Phv::reg(map[i].dst->value);
                from_idx_is_8b_16b = reg->size == 8 || reg->size == 16;
            }
        }
    }

    // Check if csum equals VERIFY type and destination register size is 16
    bool has_csum = false;
    for (auto &c : match->csum) {
        if (c.type == 0 && c.dest && c.dest->reg.size == 16) {
            has_csum = true;
            break;
        }
    }

    // Identify indexes for source and destination slots for the padding
    if (used == 1 || used == 3) {
        set_idx_for_16b_extractions(pad_idx, from_idx, used, has_csum, match, map);
    } else {
        // Value is 0,2 or 4, we are good!
        if (LOGGING(5)) {
            LOG5("No 16bit padding is needed to add in " << match->state->name << " state.");
        }

        return 0;
    }

    // Add fake extractors to reach 2n constraint, we need to copy destination and source from
    // the global version field which is tied to zeros in RTL.
    // We are keeping both indexes in tuples {0,1} or {2,3}.
    do_16b_padding(regs, map, pad_idx, from_idx);
    matchSlotTracker.padMap.insert(match, pad_idx, &map[pad_idx]);
    check_16b_extractor_configuration(pad_idx, from_idx, used, has_csum, map);

    // Match can also have a value set which means we need to do the initialization for
    // other rows. The first row is being initialized, other rows needs the initialization
    // and padding configuration
    for (int vs_offset = 1; vs_offset < match->value_set_size; ++vs_offset) {
        int nrow = row + vs_offset;
        LOG5("Adding the padding for value_set "
             << match->value_set_name << " offset = " << vs_offset << " (row = " << nrow << ")");
        map = reinterpret_cast<tofino_phv_output_map *>(
            parser->setup_phv_output_map(regs, parser->gress, nrow));
        do_16b_padding(regs, map, pad_idx, from_idx);
    }

    if (LOGGING(5)) {
        print_slot_occupancy(match, phv_16bit_extractors, "After 16bit padding");
    }

    return 1;
}

/**
 * @brief Perform the 8b padding onto map and computed index.
 *
 * @param regs Register configuration instance
 * @param map Tofino output map pointer
 * @param from_idx Index which is the source of the slot configuration
 */
static void do_8b_padding(Target::Tofino::parser_regs &regs, tofino_phv_output_map *map,
                          const unsigned from_idx) {
    for (auto pad_idx : phv_8bit_extractors) {
        if (map[pad_idx].dst->value != EXTRACT_SLOT_UNUSED) continue;

        // Extraction slot is not used and we need to put padding there. The main idea
        // is to source from the zero constant
        map[pad_idx].dst->rewrite();
        *map[pad_idx].dst = *map[from_idx].dst;
        *map[pad_idx].src = EXTRACT_SLOT_CONSTANT_ZERO;
        *map[pad_idx].src_type = EXTRACT_SLOT_CONSTANT_EN;
    }

    // Mark the dummy write dest as multi-write, we need to distinguish between PHV and TPHV
    if (*map[from_idx].dst >= PHV_MIN_INDEX && *map[from_idx].dst < PHV_MAX_INDEX) {
        regs.ingress.prsr_reg.no_multi_wr.nmw[*map[from_idx].dst].rewrite();
        regs.egress.prsr_reg.no_multi_wr.nmw[*map[from_idx].dst].rewrite();

        regs.ingress.prsr_reg.no_multi_wr.nmw[*map[from_idx].dst] = 0;
        regs.egress.prsr_reg.no_multi_wr.nmw[*map[from_idx].dst] = 0;
    } else if (*map[from_idx].dst >= TPHV_MIN_INDEX && *map[from_idx].dst < TPHV_MAX_INDEX) {
        auto tphv_idx = *map[from_idx].dst - TPHV_MIN_INDEX;
        regs.ingress.prsr_reg.no_multi_wr.t_nmw[tphv_idx].rewrite();
        regs.egress.prsr_reg.no_multi_wr.t_nmw[tphv_idx].rewrite();

        regs.ingress.prsr_reg.no_multi_wr.t_nmw[tphv_idx] = 0;
        regs.egress.prsr_reg.no_multi_wr.t_nmw[tphv_idx] = 0;
    }
}

/// Add the fake extractions to have 4n 8b extractions. The function returns
/// the number of added extractions.
static int pad_to_8b_extracts_to_4n(Parser *parser, Target::Tofino::parser_regs &regs,
                                    Parser::State::Match *match) {
    // Obtain the slot configuration for given row
    int row = parser->match_to_row.at(match);
    auto map = reinterpret_cast<tofino_phv_output_map *>(
        parser->setup_phv_output_map(regs, parser->gress, row));
    if (LOGGING(5)) {
        print_slot_occupancy(match, phv_8bit_extractors, "Before 8bit padding");
    }

    // Count number of used extractors - the number of extraction/constant set operations and
    // checksums should be padded to 4n. The source of the added padding will be stored in
    // the from_idx variable.
    unsigned used = 0;
    unsigned from_idx = 0;
    bool from_idx_is_8b = false;
    for (auto i : phv_8bit_extractors) {
        if (map[i].dst->value == EXTRACT_SLOT_UNUSED) continue;
        // Update the used counter and remember the used slot
        used++;
        // Try to get an 8b index.
        // If we use a single 16b index to pad then we'll hit problems
        // because we need 2 x 8b to fill a 16b container.
        if (!from_idx_is_8b) {
            from_idx = i;
            from_idx_is_8b = Phv::reg(map[i].dst->value)->size == 8;
        }
    }

    if (used % 4 == 0) {
        if (LOGGING(5)) {
            LOG5("No 8bit padding is needed to add in " << match->state->name << " state.");
        }

        return 0;
    }

    // Add fake extractions to meet the 4n constraint and setup tracking
    do_8b_padding(regs, map, from_idx);
    for (auto pad_idx : phv_8bit_extractors) {
        matchSlotTracker.padMap.insert(match, pad_idx, &map[pad_idx]);
    }

    // Match can also have a value set which means we need to do the initialization for
    // other rows. The first row is being initialized, other rows needs the initialization
    // and padding configuration
    for (int vs_offset = 1; vs_offset < match->value_set_size; ++vs_offset) {
        int nrow = row + vs_offset;
        LOG5("Adding the padding for value_set "
             << match->value_set_name << " offset = " << vs_offset << " (row = " << nrow << ")");
        map = reinterpret_cast<tofino_phv_output_map *>(
            parser->setup_phv_output_map(regs, parser->gress, nrow));
        do_8b_padding(regs, map, from_idx);
    }

    if (LOGGING(5)) {
        print_slot_occupancy(match, phv_8bit_extractors, "After 8bit padding");
    }

    return 4 - (used % 4);
}

/// Add padding extracts to a parser state and its children.
///
/// @tparam use_8bit Apply to 8b extracts (true) or 16b extracts (false)
/// @param parser Parser containing the state being padded
/// @param regs
/// @param node_count Number of states to pad, including this state. States with zero extracts are
///        not counted in @p node_count.
/// @param visited
/// @param pstate
template <bool use_8bit>
void pad_nodes_extracts(Parser *parser, Target::Tofino::parser_regs &regs, int node_count,
                        Parser::State::Match *match, std::set<Parser::State *> &visited,
                        PaddingInfoCollector::PadState *pstate,
                        std::map<Parser::State::Match *, int> &cache) {
    if (node_count == 0 || !match) {
        LOG4("Node count or nullptr match was reached");
        return;
    }

    const std::string log_pad = use_8bit ? "8b" : "16b";
    if (visited.count(match->state)) {
        LOG4("State " << match->state << " was already visited in " << log_pad << " padding.");
        return;
    }

    visited.insert(match->state);
    LOG3("Padding " << log_pad << " extracts - state = " << match->state->name << ", "
                    << "remaining " << log_pad << " states to pad is " << node_count);

    pstate->print();
    // Memoization to minimize path visits
    // If state is visited before with the same or higher node count dont visit it
    // again. Cache holds a map from state to node count
    if (cache[match] >= node_count && node_count > 0) {
        LOG5(" Using cached state(match) : " << match->state->name << "(" << match->match
                                             << ") -> node count " << cache[match]);
        visited.erase(match->state);
        return;
    }
    cache[match] = node_count;
    LOG5(" Caching state(match) : " << match->state->name << "(" << match->match
                                    << ") -> node count " << cache[match]);

    // We need to be sure that we will not be passing data to 2x16bit busses. Therefore, we have
    // to pad the bus entirely for 16bit extractions. In addition, we need to see node_cout 16bit
    // extactions - due to possible FIFO stalls. If the node doesn't contain the required
    // extraction, we don't decrement the node_count value.
    int new_node_count = node_count;
    auto phv_type = use_8bit ? AnalysisType::BIT8 : AnalysisType::BIT16;
    if (count_number_of_extractions(parser, regs, match, phv_type)) {
        if (use_8bit) {
            int pad = pad_to_8b_extracts_to_4n(parser, regs, match);
            pstate->addPadInfo(match, AnalysisType::BIT8, pad);
        } else {
            int pad = pad_to_16b_extracts_to_2n(parser, regs, match);
            pstate->addPadInfo(match, AnalysisType::BIT16, pad);
        }

        new_node_count--;
    }
    if (LOGGING(5)) pstate->print();

    for (auto state : match->next) {
        for (auto next_match : state->match) {
            pad_nodes_extracts<use_8bit>(parser, regs, new_node_count, next_match, visited, pstate,
                                         cache);
        }
    }

    visited.erase(match->state);
}

// Helping aliases for padding functions
const auto pad_nodes_8b_extracts = pad_nodes_extracts<true>;
const auto pad_nodes_16b_extracts = pad_nodes_extracts<false>;

void handle_narrow_to_wide_constraint(Parser *parser, Target::Tofino::parser_regs &regs) {
    // 1] Apply narrow-to-wide constraints to all predecessors
    std::set<Parser::State::Match *> narrow_to_wide_matches;
    PaddingInfoCollector pad_collector;

    for (auto &kv : parser->match_to_row) {
        if (kv.first->has_narrow_to_wide_extract) narrow_to_wide_matches.insert(kv.first);
    }

    if (narrow_to_wide_matches.size() == 0) {
        LOG2("No narrow to wide matches has been detected.");
        return;
    }

    // Pad all predecessors
    std::set<Parser::State::Match *> all_preds;
    for (auto m : narrow_to_wide_matches) {
        auto states = m->get_all_preds();
        states.insert(m);
        auto pstate = pad_collector.getPadState(m);

        for (auto p : states) {
            if (all_preds.count(p)) continue;

            all_preds.insert(p);
            int pad = pad_to_16b_extracts_to_2n(parser, regs, p);
            pstate->addPadInfo(p, AnalysisType::BIT16, pad);
            pad = pad_to_8b_extracts_to_4n(parser, regs, p);
            pstate->addPadInfo(p, AnalysisType::BIT8, pad);
        }
    }

    // 2] Apply the narrow-to-wide constraints to a given number
    // of child nodes.
    ExtractionCountCache cache;
    for (auto m : narrow_to_wide_matches) {
        auto pstate = pad_collector.getPadState(m);
        std::set<Parser::State *> visited_states;
        int extracts_16b = analyze_worst_extractor_path(parser, regs, m, AnalysisType::BIT16,
                                                        visited_states, cache);
        int extracts_8b = analyze_worst_extractor_path(parser, regs, m, AnalysisType::BIT8,
                                                       visited_states, cache);

        if (LOGGING(3)) {
            std::stringstream ss;
            ss << "INFO: Used extractors for " << m->state->gress << "," << m->state->name;
            if (m->match) {
                ss << "," << m->match;
            }
            ss << " - " << "8bit:" << extracts_8b << ", 16bit:" << extracts_16b;
            LOG3(ss.str());
        }

        // Count the number of nodes we need to take, the arbiter is taking 4x8bit chunks
        // and 2x16b chunks of data. The result will be ceiled to 16 because that is the
        // depth of the FIFO. After that, we need to apply the 16b padding to a computed number
        // of nodes and the same for 8b padding.
        int pass_16b_nodes = ceil_and_wrap_to_fifo_size(extracts_16b, 2);
        int pass_8b_nodes = ceil_and_wrap_to_fifo_size(extracts_8b, 4);

        // The state counts represent the states _after_ the n2w state. Increment by 1 to account
        // for n2w state.
        if (pass_8b_nodes) pass_8b_nodes++;
        if (pass_16b_nodes) pass_16b_nodes++;

        // Pad extracts: 8b extracts should be padded based on the number of states to flush 16b
        // narrow-to-wide extracts, and 16b extracts should be padded based on the number
        // of states to flush 8b narrow-to-wide extracts.
        std::map<Parser::State::Match *, int> cacheNodeCount;
        pad_nodes_16b_extracts(parser, regs, pass_8b_nodes, m, visited_states, pstate,
                               cacheNodeCount);
        cacheNodeCount.clear();
        pad_nodes_8b_extracts(parser, regs, pass_16b_nodes, m, visited_states, pstate,
                              cacheNodeCount);
    }

    if (LOGGING(1)) {
        pad_collector.printPadInfo();
    }
}

template <>
void Parser::write_config(Target::Tofino::parser_regs &regs, json::map &ctxt_json,
                          bool single_parser) {
    /// remove after 8.7 release
    if (single_parser) {
        for (auto st : all) {
            st->write_config(regs, this, ctxt_json[st->gress == EGRESS ? "egress" : "ingress"]);
        }
    } else {
        ctxt_json["states"] = json::vector();
        for (auto st : all) st->write_config(regs, this, ctxt_json["states"]);
    }

    if (error_count > 0) return;

    int i = 0;
    for (auto ctr : counter_init) {
        if (ctr) ctr->write_config(regs, gress, i);
        ++i;
    }

    for (i = 0; i < checksum_use.size(); i++) {
        for (auto csum : checksum_use[i])
            if (csum) csum->write_config(regs, this);
    }

    if (gress == INGRESS) {
        init_common_regs(this, regs.ingress.prsr_reg, INGRESS);
        // regs.ingress.ing_buf_regs.glb_group.disable();
        // regs.ingress.ing_buf_regs.chan0_group.chnl_drop.disable();
        // regs.ingress.ing_buf_regs.chan0_group.chnl_metadata_fix.disable();
        // regs.ingress.ing_buf_regs.chan1_group.chnl_drop.disable();
        // regs.ingress.ing_buf_regs.chan1_group.chnl_metadata_fix.disable();
        // regs.ingress.ing_buf_regs.chan2_group.chnl_drop.disable();
        // regs.ingress.ing_buf_regs.chan2_group.chnl_metadata_fix.disable();
        // regs.ingress.ing_buf_regs.chan3_group.chnl_drop.disable();
        // regs.ingress.ing_buf_regs.chan3_group.chnl_metadata_fix.disable();

        regs.ingress.prsr_reg.hdr_len_adj.amt = hdr_len_adj;
    }

    if (gress == EGRESS) {
        init_common_regs(this, regs.egress.prsr_reg, EGRESS);
        for (int i = 0; i < 4; i++) regs.egress.epb_prsr_port_regs.chnl_ctrl[i].meta_opt = meta_opt;

        int prsr_max_dph = get_prsr_max_dph();
        if (prsr_max_dph * 16 > Target::PARSER_DEPTH_MAX_BYTES_MULTITHREADED_EGRESS()) {
            if (!options.tof1_egr_parse_depth_checks_disabled)
                warning(lineno,
                        "Egress parser max depth exceeds %d, which requires disabling "
                        "multithreading in the parser",
                        Target::PARSER_DEPTH_MAX_BYTES_MULTITHREADED_EGRESS());
            options.tof1_egr_parse_depth_checks_disabled = true;
        }
        regs.egress.epb_prsr_port_regs.multi_threading.prsr_dph_max = prsr_max_dph;
        regs.egress.prsr_reg.hdr_len_adj.amt = hdr_len_adj;
    }

    // FIXME: The "|| 1" causes the PHV use information to be unconditionally copied
    // into the PHV ownership. This forces the parser ownership to be identical to that
    // in the pipe.
    // Remove to allow different ownership, but make sure that header stacks
    // are processed correctly. All stack elements writeable by the parser must
    // be owned by the parser.
    if (options.match_compiler || 1) {
        phv_use[INGRESS] |= Phv::use(INGRESS);
        phv_use[EGRESS] |= Phv::use(EGRESS);
    }

    for (int i : phv_use[EGRESS]) {
        auto id = Phv::reg(i)->parser_id();
        if (id >= 256) {
            regs.merge.phv_owner.t_owner[id - 256] = 1;
            regs.ingress.prsr_reg.phv_owner.t_owner[id - 256] = 1;
            regs.egress.prsr_reg.phv_owner.t_owner[id - 256] = 1;
        } else if (id < 224) {
            regs.merge.phv_owner.owner[id] = 1;
            regs.ingress.prsr_reg.phv_owner.owner[id] = 1;
            regs.egress.prsr_reg.phv_owner.owner[id] = 1;
        }
    }

    for (int i = 0; i < 224; i++) {
        if (!phv_allow_bitwise_or[i]) {
            regs.ingress.prsr_reg.no_multi_wr.nmw[i] = 1;
            regs.egress.prsr_reg.no_multi_wr.nmw[i] = 1;
        }
        if (phv_allow_bitwise_or[i] || phv_init_valid[i]) regs.merge.phv_valid.vld[i] = 1;
    }

    for (int i = 0; i < 112; i++)
        if (!phv_allow_bitwise_or[256 + i]) {
            regs.ingress.prsr_reg.no_multi_wr.t_nmw[i] = 1;
            regs.egress.prsr_reg.no_multi_wr.t_nmw[i] = 1;
        }

    // if (options.condense_json) {
    //     // FIXME -- removing the uninitialized memory causes problems?
    //     // FIXME -- walle gets the addresses wrong.  Might also require explicit
    //     // FIXME -- zeroing in the driver on real hardware
    //     // regs.memory[INGRESS].disable_if_reset_value();
    //     // regs.memory[EGRESS].disable_if_reset_value();
    //     regs.ingress.disable_if_reset_value();
    //     regs.egress.disable_if_reset_value();
    //     regs.merge.disable_if_reset_value();
    // }

    // Handles the constraint when using narrow extractors to generate wide values
    // (either extracted from the packet or using the constants), then you need to
    // follow the rule the _every_ preceding cycle must do:
    //   0 or 4 8b extractions
    //   0 or 2 or 4 16b extractions
    handle_narrow_to_wide_constraint(this, regs);

    if (error_count == 0 && options.gen_json) {
        /// TODO remove after 8.7 release
        /// TODO Needs fix to simple test harness for parsers node
        /// support
        if (single_parser) {
            if (gress == INGRESS) {
                regs.memory[INGRESS].emit_json(*open_output("memories.all.parser.ingress.cfg.json"),
                                               "ingress");
                regs.ingress.emit_json(*open_output("regs.all.parser.ingress.cfg.json"));
            } else if (gress == EGRESS) {
                regs.memory[EGRESS].emit_json(*open_output("memories.all.parser.egress.cfg.json"),
                                              "egress");
                regs.egress.emit_json(*open_output("regs.all.parser.egress.cfg.json"));
            }
            regs.merge.emit_json(*open_output("regs.all.parse_merge.cfg.json"));
        } else {
            if (gress == INGRESS) {
                regs.memory[INGRESS].emit_json(
                    *open_output("memories.all.parser.ingress.%02x.cfg.json", parser_no),
                    "ingress");
                regs.ingress.emit_json(
                    *open_output("regs.all.parser.ingress.%02x.cfg.json", parser_no));
            }
            if (gress == EGRESS) {
                regs.memory[EGRESS].emit_json(
                    *open_output("memories.all.parser.egress.%02x.cfg.json", parser_no), "egress");
                regs.egress.emit_json(
                    *open_output("regs.all.parser.egress.%02x.cfg.json", parser_no));
            }
            regs.merge.emit_json(*open_output("regs.all.parse_merge.cfg.json"));
        }
    }

    /// TODO remove after 8.7 release
    if (single_parser) {
        for (int i = 0; i < 18; i++) {
            if (gress == INGRESS) {
                TopLevel::regs<Target::Tofino>()->mem_pipe.i_prsr[i].set(
                    "memories.all.parser.ingress", &regs.memory[INGRESS]);
                TopLevel::regs<Target::Tofino>()->reg_pipe.pmarb.ibp18_reg.ibp_reg[i].set(
                    "regs.all.parser.ingress", &regs.ingress);
            } else if (gress == EGRESS) {
                TopLevel::regs<Target::Tofino>()->mem_pipe.e_prsr[i].set(
                    "memories.all.parser.egress", &regs.memory[EGRESS]);
                TopLevel::regs<Target::Tofino>()->reg_pipe.pmarb.ebp18_reg.ebp_reg[i].set(
                    "regs.all.parser.egress", &regs.egress);
            }
        }
    } else {
        if (gress == INGRESS) {
            TopLevel::regs<Target::Tofino>()->parser_ingress.emplace(
                ctxt_json["handle"]->as_number()->val, &regs.ingress);
            TopLevel::regs<Target::Tofino>()->parser_memory[INGRESS].emplace(
                ctxt_json["handle"]->as_number()->val, &regs.memory[INGRESS]);
        } else if (gress == EGRESS) {
            TopLevel::regs<Target::Tofino>()->parser_egress.emplace(
                ctxt_json["handle"]->as_number()->val, &regs.egress);
            TopLevel::regs<Target::Tofino>()->parser_memory[EGRESS].emplace(
                ctxt_json["handle"]->as_number()->val, &regs.memory[EGRESS]);
        }

#if 0
        /// for initiliazing the parser registers in default configuration.
        int start_bit = port_use.ffs();
        do {
            int end_bit = port_use.ffz(start_bit);
            std::cout << "set memories and regs from " << start_bit
            << " to " << end_bit - 1 << std::endl;
            for (auto i = start_bit; i <= end_bit - 1; i++) {
                TopLevel::regs<Target::Tofino>()->mem_pipe.i_prsr[i]
                        .set("memories.all.parser.ingress", &regs.memory[INGRESS]);
                TopLevel::regs<Target::Tofino>()->reg_pipe.pmarb.ibp18_reg.ibp_reg[i]
                        .set("regs.all.parser.ingress", &regs.ingress);
                TopLevel::regs<Target::Tofino>()->mem_pipe.e_prsr[i]
                        .set("memories.all.parser.egress", &regs.memory[EGRESS]);
                TopLevel::regs<Target::Tofino>()->reg_pipe.pmarb.ebp18_reg.ebp_reg[i]
                        .set("regs.all.parser.egress", &regs.egress);
            }
            start_bit = port_use.ffs(end_bit);
        } while (start_bit >= 0);
#endif
    }
    // all parsers share the same parser_merge configuration.
    TopLevel::regs<Target::Tofino>()->reg_pipe.pmarb.prsr_reg.set("regs.all.parse_merge",
                                                                  &regs.merge);
}

template <>
void Parser::gen_configuration_cache(Target::Tofino::parser_regs &regs, json::vector &cfg_cache) {
    std::string reg_fqname;
    std::string reg_name;
    unsigned reg_value;
    std::string reg_value_str;
    unsigned reg_width = 8;

    if (gress == EGRESS) {
        // epb_prsr_port_regs.chnl_ctrl
        for (int i = 0; i < 4; i++) {
            reg_fqname = "pmarb.ebp18_reg.ebp_reg[0].epb_prsr_port_regs.chnl_ctrl[" +
                         std::to_string(i) + "]";
            reg_name = "parser0_chnl_ctrl_" + std::to_string(i);
            reg_value = regs.egress.epb_prsr_port_regs.chnl_ctrl[i];
            if ((reg_value != 0) || (options.match_compiler)) {
                reg_value_str = int_to_hex_string(reg_value, reg_width);
                add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
            }
        }

        // epb_prsr_port_regs.multi_threading
        reg_fqname = "pmarb.ebp18_reg.ebp_reg[0].epb_prsr_port_regs.multi_threading";
        reg_name = "parser0_multi_threading";
        reg_value = regs.egress.epb_prsr_port_regs.multi_threading;
        if ((reg_value != 0) || (options.match_compiler)) {
            reg_value_str = int_to_hex_string(reg_value, reg_width);
            add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
        }
    }
}
