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

#include "backends/tofino/bf-asm/action_bus.h"
#include "backends/tofino/bf-asm/input_xbar.h"
#include "backends/tofino/bf-asm/instruction.h"
#include "backends/tofino/bf-asm/mask_counter.h"
#include "backends/tofino/bf-asm/misc.h"
#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"
#include "lib/algorithm.h"
#include "lib/hex.h"

Table::Format::Field *SRamMatchTable::lookup_field(const std::string &n,
                                                   const std::string &act) const {
    auto *rv = format ? format->field(n) : nullptr;
    if (!rv && gateway) rv = gateway->lookup_field(n, act);
    if (!rv && !act.empty()) {
        if (auto call = get_action()) rv = call->lookup_field(n, act);
    }
    if (!rv && n == "immediate" && !::Phv::get(gress, stage->stageno, n)) {
        static Format::Field default_immediate(nullptr, 32, Format::Field::USED_IMMED);
        rv = &default_immediate;
    }
    return rv;
}

const char *SRamMatchTable::Ram::desc() const {
    static char buffer[256], *p = buffer;
    char *end = buffer + sizeof(buffer), *rv;
    do {
        if (end - p < 7) p = buffer;
        rv = p;
        if (stage >= 0)
            p += snprintf(p, end - p, "Ram %d,%d,%d", stage, row, col);
        else if (row >= 0)
            p += snprintf(p, end - p, "Ram %d,%d", row, col);
        else
            p += snprintf(p, end - p, "Lamb %d", col);
    } while (p++ >= end);
    return rv;
}

/* calculate the 18-bit byte/nybble mask tofino uses for matching in a 128-bit word */
static unsigned tofino_bytemask(int lo, int hi) {
    unsigned rv = 0;
    for (unsigned i = lo / 4U; i <= hi / 4U; i++) rv |= 1U << (i < 28 ? i / 2 : i - 14);
    return rv;
}

/**
 * Determining the result bus for an entry, if that entry has no overhead.  The result bus
 * is still needed to get the direct address location to find action data / run an
 * instruction, etc.
 *
 * This section maps the allocation scheme used in the TableFormat::Use in p4c, found
 * in the function result_bus_words
 */
void SRamMatchTable::no_overhead_determine_result_bus_usage() {
    bitvec result_bus_words;

    for (int i = 0; i < static_cast<int>(group_info.size()); i++) {
        BUG_CHECK(group_info[i].overhead_word < 0);
        if (group_info[i].match_group.size() == 1) {
            group_info[i].result_bus_word = group_info[i].match_group.begin()->first;
            result_bus_words.setbit(group_info[i].result_bus_word);
        }
    }

    for (int i = 0; i < static_cast<int>(group_info.size()); i++) {
        if (group_info[i].overhead_word < 0 && group_info[i].match_group.size() > 1) {
            bool result_bus_set = false;
            for (auto match_group : group_info[i].match_group) {
                if (result_bus_words.getbit(match_group.first)) {
                    group_info[i].result_bus_word = match_group.first;
                    result_bus_set = true;
                }
            }
            if (!result_bus_set)
                group_info[i].result_bus_word = group_info[i].match_group.begin()->first;
            LOG1("  format group " << i << " no overhead multiple match groups");
        }
    }
}

void SRamMatchTable::verify_format(Target::Tofino) {
    if (format->log2size < 7) format->log2size = 7;
    format->pass1(this);
    group_info.resize(format->groups());
    unsigned fmt_width = (format->size + 127) / 128;
    if (word_info.size() > fmt_width) {
        warning(format->lineno, "Match group map wider than format, padding out format");
        format->size = word_info.size() * 128;
        fmt_width = word_info.size();
        while ((1U << format->log2size) < format->size) ++format->log2size;
    }
    for (unsigned i = 0; i < format->groups(); i++) {
        auto &info = group_info[i];
        info.tofino_mask.resize(fmt_width);
        if (Format::Field *match = format->field("match", i)) {
            for (auto &piece : match->bits) {
                unsigned word = piece.lo / 128;
                if (word != piece.hi / 128)
                    error(format->lineno,
                          "'match' field must be explictly split across "
                          "128-bit boundary in table %s",
                          name());
                info.tofino_mask[word] |= tofino_bytemask(piece.lo % 128, piece.hi % 128);
                info.match_group[word] = -1;
            }
        }
        if (auto *version = format->field("version", i)) {
            if (version->bits.size() != 1) error(format->lineno, "'version' field cannot be split");
            auto &piece = version->bits[0];
            unsigned word = piece.lo / 128;
            if (version->size != 4 || (piece.lo % 4) != 0)
                error(format->lineno,
                      "'version' field not 4 bits and nibble aligned "
                      "in table %s",
                      name());
            info.tofino_mask[word] |= tofino_bytemask(piece.lo % 128, piece.hi % 128);
            info.match_group[word] = -1;
        }
        for (unsigned j = 0; j < i; j++)
            for (unsigned word = 0; word < fmt_width; word++)
                if (group_info[j].tofino_mask[word] & info.tofino_mask[word]) {
                    int bit = ffs(group_info[j].tofino_mask[word] & info.tofino_mask[word]) - 1;
                    if (bit >= 14) bit += 14;
                    error(format->lineno, "Match groups %d and %d both use %s %d in word %d", i, j,
                          bit > 20 ? "nibble" : "byte", bit, word);
                    break;
                }
        for (auto it = format->begin(i); it != format->end(i); it++) {
            Format::Field &f = it->second;
            if (it->first == "match" || it->first == "version" || it->first == "proxy_hash")
                continue;
            if (f.bits.size() != 1) {
                error(format->lineno, "Can't deal with split field %s", it->first.c_str());
                continue;
            }
            unsigned limit = Target::MAX_OVERHEAD_OFFSET();
            if (it->first == "next") limit = Target::MAX_OVERHEAD_OFFSET_NEXT();
            unsigned word = f.bit(0) / 128;
            if (info.overhead_word < 0) {
                info.overhead_word = word;
                format->overhead_word = word;
                LOG5("Setting overhead word for format : " << word);
                info.overhead_bit = f.bit(0) % 128;
                info.match_group[word] = -1;
            } else if (info.overhead_word != static_cast<int>(word)) {
                error(format->lineno, "Match overhead group %d split across words", i);
            } else if (word != f.bit(f.size - 1) / 128 || f.bit(f.size - 1) % 128 >= limit) {
                error(format->lineno, "Match overhead field %s(%d) not in bottom %d bits",
                      it->first.c_str(), i, limit);
            }
            if (!info.match_group.count(word))
                error(format->lineno, "Match overhead in group %d in word with no match?", i);
            if ((unsigned)info.overhead_bit > f.bit(0) % 128) info.overhead_bit = f.bit(0) % 128;
        }
        info.vpn_offset = i;
    }
    if (word_info.empty()) {
        word_info.resize(fmt_width);
        if (format->field("next")) {
            /* 'next' for match group 0 must be in bit 0, so make the format group with
             * overhead in bit 0 match group 0 in its overhead word */
            for (unsigned i = 0; i < group_info.size(); i++) {
                if (group_info[i].overhead_bit == 0) {
                    BUG_CHECK(error_count > 0 || word_info[group_info[i].overhead_word].empty());
                    group_info[i].match_group[group_info[i].overhead_word] = 0;
                    word_info[group_info[i].overhead_word].push_back(i);
                }
            }
        }
        for (unsigned i = 0; i < group_info.size(); i++) {
            if (group_info[i].match_group.size() > 1) {
                for (auto &mgrp : group_info[i].match_group) {
                    if (mgrp.second >= 0) continue;
                    if ((mgrp.second = word_info[mgrp.first].size()) > 1)
                        error(format->lineno, "Too many multi-word groups using word %d",
                              mgrp.first);
                    word_info[mgrp.first].push_back(i);
                }
            }
        }
    } else {
        if (word_info.size() != fmt_width)
            error(mgm_lineno, "Match group map doesn't match format size");
        for (unsigned i = 0; i < word_info.size(); i++) {
            for (unsigned j = 0; j < word_info[i].size(); j++) {
                int grp = word_info[i][j];
                if (grp < 0 || (unsigned)grp >= format->groups()) {
                    error(mgm_lineno, "Invalid group number %d", grp);
                } else if (!group_info[grp].match_group.count(i)) {
                    error(mgm_lineno, "Format group %d doesn't match in word %d", grp, i);
                } else {
                    group_info[grp].match_group[i] = j;
                    auto *next = format->field("next", grp);
                    if (!next && hit_next.size() > 1) next = format->field("action", grp);
                    if (next) {
                        if (next->bit(0) / 128 != i) continue;
                        static unsigned limit[5] = {0, 8, 32, 32, 32};
                        unsigned bit = next->bit(0) % 128U;
                        if (!j && bit)
                            error(mgm_lineno,
                                  "Next(%d) field must start at bit %d to be in "
                                  "match group 0",
                                  grp, i * 128);
                        else if (j && (!bit || bit > limit[j]))
                            warning(mgm_lineno,
                                    "Next(%d) field must start in range %d..%d "
                                    "to be in match group %d",
                                    grp, i * 128 + 1, i * 128 + limit[j], j);
                    }
                }
            }
        }
    }
    if (hit_next.size() > 1 && !format->field("next") && !format->field("action"))
        error(format->lineno, "No 'next' field in format");
    if (error_count > 0) return;

    for (int i = 0; i < static_cast<int>(group_info.size()); i++) {
        if (group_info[i].match_group.size() == 1) {
            for (auto &mgrp : group_info[i].match_group) {
                if (mgrp.second >= 0) continue;
                if ((mgrp.second = word_info[mgrp.first].size()) > 4)
                    error(format->lineno, "Too many match groups using word %d", mgrp.first);
                word_info[mgrp.first].push_back(i);
            }
        }
        // Determining the result bus word, where the overhead is supposed to be
    }

    bool has_overhead_word = false;
    bool overhead_word_set = false;
    for (int i = 0; i < static_cast<int>(group_info.size()); i++) {
        if (overhead_word_set) BUG_CHECK((group_info[i].overhead_word >= 0) == has_overhead_word);
        if (group_info[i].overhead_word >= 0) {
            has_overhead_word = true;
            group_info[i].result_bus_word = group_info[i].overhead_word;
        }
        overhead_word_set = true;
    }

    if (!has_overhead_word) no_overhead_determine_result_bus_usage();

    /**
     * Determining the result bus for an entry, if that entry has no overhead.  The result bus
     * is still needed to get the direct address location to find action data / run an
     * instruction, etc.
     *
     * This section maps the allocation scheme used in the TableFormat::Use in p4c, found
     * in the function result_bus_words
     */

    for (int i = 0; i < static_cast<int>(group_info.size()); i++) {
        LOG1("  masks: " << hexvec(group_info[i].tofino_mask));
        for (auto &mgrp : group_info[i].match_group)
            LOG1("    match group " << mgrp.second << " in word " << mgrp.first);
    }

    for (unsigned i = 0; i < word_info.size(); i++)
        LOG1("  word " << i << " groups: " << word_info[i]);
    if (options.match_compiler && 0) {
        /* hack to match the compiler's nibble usage -- if any of the top 4 nibbles is
         * unused in a word, mark it as used by any group that uses the other nibble of the
         * byte, UNLESS it is used for the version.  This is ok, as the unused nibble will
         * end up being masked off by the match_mask anyways */
        for (unsigned word = 0; word < word_info.size(); word++) {
            unsigned used_nibbles = 0;
            for (auto group : word_info[word])
                used_nibbles |= group_info[group].tofino_mask[word] >> 14;
            for (unsigned nibble = 0; nibble < 4; nibble++) {
                if (!((used_nibbles >> nibble) & 1) && ((used_nibbles >> (nibble ^ 1)) & 1)) {
                    LOG1("  ** fixup nibble " << nibble << " in word " << word);
                    for (auto group : word_info[word])
                        if ((group_info[group].tofino_mask[word] >> (14 + (nibble ^ 1))) & 1) {
                            if (auto *version = format->field("version", group)) {
                                if (version->bit(0) == word * 128 + (nibble ^ 1) * 4 + 112) {
                                    LOG1("      skip group " << group << " (version)");
                                    continue;
                                }
                            }
                            group_info[group].tofino_mask[word] |= 1 << (14 + nibble);
                            LOG1("      adding to group " << group);
                        }
                }
            }
        }
    }

    verify_match(fmt_width);
}

void SRamMatchTable::verify_format_pass2(Target::Tofino) {}

/**
 * Guarantees that each match field is a PHV field, which is the standard unless the table is
 * a proxy hash table.
 */
bool SRamMatchTable::verify_match_key() {
    for (auto *match_key : match) {
        auto phv_p = dynamic_cast<Phv::Ref *>(match_key);
        if (phv_p == nullptr) {
            error(match_key->get_lineno(), "A non PHV match key in table %s", name());
            continue;
        }
        auto phv_ref = *phv_p;
        if (phv_ref.check() && phv_ref->reg.mau_id() < 0)
            error(phv_ref.lineno, "%s not accessable in mau", phv_ref->reg.name);
    }
    auto match_format = format->field("match");
    if (match_format && match.empty()) {
        BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
        for (auto ixbar_element : *input_xbar[0]) {
            match.emplace_back(new Phv::Ref(ixbar_element.second.what));
        }
    }
    return error_count == 0;
}

std::unique_ptr<json::map> SRamMatchTable::gen_memory_resource_allocation_tbl_cfg(
    const Way &way) const {
    json::map mra;
    unsigned vpn_ctr = 0;
    unsigned fmt_width = format ? (format->size + 127) / 128 : 0;
    unsigned ramdepth = way.isLamb() ? LAMB_DEPTH_BITS : SRAM_DEPTH_BITS;
    if (hash_fn_ids.count(way.group_xme) > 0)
        mra["hash_function_id"] = hash_fn_ids.at(way.group_xme);
    mra["hash_entry_bit_lo"] = way.index;
    mra["hash_entry_bit_hi"] = way.index + ramdepth + way.subword_bits - 1;
    mra["number_entry_bits"] = ramdepth;
    mra["number_subword_bits"] = way.subword_bits;
    if (way.select) {
        int lo = way.select.min().index(), hi = way.select.max().index();
        mra["hash_select_bit_lo"] = lo;
        mra["hash_select_bit_hi"] = hi;
        if (way.select.popcount() != hi - lo + 1) {
            warning(way.lineno, "driver does not support discontinuous bits in a way mask");
            mra["hash_select_bit_mask"] = way.select.getrange(lo, 32);
        }
    } else {
        mra["hash_select_bit_lo"] = mra["hash_select_bit_hi"] = 40;
    }
    mra["number_select_bits"] = way.select.popcount();
    mra["memory_type"] = way.isLamb() ? "lamb" : "sram";
    json::vector mem_units;
    json::vector &mem_units_and_vpns = mra["memory_units_and_vpns"] = json::vector();
    int way_uses_lambs = -1;  // don't know yet
    for (auto &ram : way.rams) {
        if (ram.isLamb()) {
            BUG_CHECK(way_uses_lambs != 0, "mixed lambs and memories in a way");
            way_uses_lambs = 1;
        } else {
            BUG_CHECK(way_uses_lambs != 1, "mixed lambs and memories in a way");
            way_uses_lambs = 0;
            if (mem_units.empty())
                vpn_ctr = layout_get_vpn(ram);
            else
                BUG_CHECK(vpn_ctr == layout_get_vpn(ram));
        }
        mem_units.push_back(json_memunit(ram));
        if (mem_units.size() == fmt_width) {
            json::map tmp;
            tmp["memory_units"] = std::move(mem_units);
            mem_units = json::vector();
            json::vector vpns;
            for (auto &grp : group_info) vpns.push_back(vpn_ctr + grp.vpn_offset);
            vpn_ctr += group_info.size();
            if (group_info.empty())  // FIXME -- can this happen?
                vpns.push_back(vpn_ctr++);
            tmp["vpns"] = std::move(vpns);
            mem_units_and_vpns.push_back(std::move(tmp));
        }
    }
    BUG_CHECK(mem_units.empty());
    return json::mkuniq<json::map>(std::move(mra));
}

/**
 * The purpose of this function is to generate the hash_functions JSON node.  The hash functions
 * are for the driver to determine what RAM/RAM line to write the match data into during entry
 * adds.
 *
 * The JSON nodes for the hash functions are the following:
 *     - hash_bits - A vector determining what each bit is calculated from.  Look at the comments
 *           over the function gen_hash_bits
 *     The following two fields are required for High Availability mode and Entry Reads from HW
 *     - ghost_bit_to_hash_bit - A vector describing where the ghost bits are in the hash matrix
 *     - ghost_bit_info - A vector indicating which p4 fields are used as the ghost bits
 *     The following field is only necessary for dynamic_key_masks
 *     - hash_function_number - which of the 8 hash functions this table is using.
 *
 * The order of the hash functions must coordinate to the order of the hash_function_ids used
 * in the Way JSON, as this is how a single way knows which hash function to use for its lookup
 */
void SRamMatchTable::add_hash_functions(json::map &stage_tbl) const {
    BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
    auto &ht = input_xbar[0]->get_hash_tables();
    if (ht.size() == 0) return;
    // Output cjson node only if hash tables present
    std::map<int, bitvec> hash_bits_per_group;
    for (auto &way : ways) {
        int depth = way.isLamb() ? LAMB_DEPTH_BITS : SRAM_DEPTH_BITS;
        if (format->field("match")) {
            // cuckoo or BPH
        } else {
            depth += ceil_log2(format->groups());
            if (format->size < 128) depth += 7 - ceil_log2(format->size);
        }
        bitvec way_impact;
        way_impact.setrange(way.index, depth);
        way_impact |= way.select;
        hash_bits_per_group[way.group_xme] |= way_impact;
    }

    // Order so that the order is the same of the hash_function_ids in the ways
    // FIXME -- this seems pointless, as iterating over a std::map will always be
    // in order.  So this loop could go away and the later loop be over hash_bits_per_group
    std::vector<std::pair<int, bitvec>> hash_function_to_hash_bits(hash_fn_ids.size());
    for (auto entry : hash_bits_per_group) {
        int hash_fn_id = hash_fn_ids.at(entry.first);
        if (hash_fn_id >= hash_fn_ids.size()) BUG();
        hash_function_to_hash_bits[hash_fn_id] = entry;
    }

    json::vector &hash_functions = stage_tbl["hash_functions"] = json::vector();
    for (auto entry : hash_function_to_hash_bits) {
        int hash_group_no = entry.first;

        json::map hash_function;
        json::vector &hash_bits = hash_function["hash_bits"] = json::vector();
        hash_function["hash_function_number"] = hash_group_no;
        json::vector &ghost_bits_to_hash_bits = hash_function["ghost_bit_to_hash_bit"] =
            json::vector();
        json::vector &ghost_bits_info = hash_function["ghost_bit_info"] = json::vector();
        // Get the hash group data
        if (auto *hash_group = input_xbar[0]->get_hash_group(hash_group_no)) {
            // Process only hash tables used per hash group
            for (unsigned id : bitvec(hash_group->tables)) {
                auto hash_table = input_xbar[0]->get_hash_table(id);
                gen_hash_bits(hash_table, InputXbar::HashTable(InputXbar::HashTable::EXACT, id),
                              hash_bits, hash_group_no, entry.second);
            }
        } else {
            for (auto &ht : input_xbar[0]->get_hash_tables())
                gen_hash_bits(ht.second, ht.first, hash_bits, hash_group_no, entry.second);
        }
        gen_ghost_bits(hash_group_no, ghost_bits_to_hash_bits, ghost_bits_info);
        hash_functions.push_back(std::move(hash_function));
    }
}

void SRamMatchTable::verify_match(unsigned fmt_width) {
    if (!verify_match_key()) return;
    // Build the match_by_bit
    unsigned bit = 0;
    for (auto &r : match) {
        match_by_bit.emplace(bit, r);
        bit += r->size();
    }
    auto match_format = format->field("match");
    if ((unsigned)bit != (match_format ? match_format->size : 0))
        warning(match[0]->get_lineno(),
                "Match width %d for table %s doesn't match format match "
                "width %d",
                bit, name(), match_format ? match_format->size : 0);
    match_in_word.resize(fmt_width);
    for (unsigned i = 0; i < format->groups(); i++) {
        Format::Field *match = format->field("match", i);
        if (!match) continue;
        unsigned bit = 0;
        for (auto &piece : match->bits) {
            auto mw = --match_by_bit.upper_bound(bit);
            int lo = bit - mw->first;
            while (mw != match_by_bit.end() && mw->first < bit + piece.size()) {
                if ((piece.lo + mw->first - bit) % 8U != (mw->second->slicelobit() % 8U))
                    error(mw->second->get_lineno(),
                          "bit within byte misalignment matching %s in "
                          "match group %d of table %s",
                          mw->second->name(), i, name());
                int hi =
                    std::min((unsigned)mw->second->size() - 1, bit + piece.size() - mw->first - 1);
                BUG_CHECK((unsigned)piece.lo / 128 < fmt_width);
                // merge_phv_vec(match_in_word[piece.lo/128], Phv::Ref(mw->second, lo, hi));

                if (auto phv_p = dynamic_cast<Phv::Ref *>(mw->second)) {
                    auto phv_ref = *phv_p;
                    auto vec = split_phv_bytes(Phv::Ref(phv_ref, lo, hi));
                    for (auto ref : vec) {
                        match_in_word[piece.lo / 128].emplace_back(new Phv::Ref(ref));
                    }

                } else if (auto hash_p = dynamic_cast<HashMatchSource *>(mw->second)) {
                    match_in_word[piece.lo / 128].push_back(new HashMatchSource(*hash_p));
                } else {
                    BUG();
                }
                lo = 0;
                ++mw;
            }
            bit += piece.size();
        }
    }
    for (unsigned i = 0; i < fmt_width; i++) {
        std::string match_word_info = "[ ";
        std::string sep = "";
        for (auto entry : match_in_word[i]) {
            match_word_info += sep + entry->toString();
            sep = ", ";
        }
        LOG1("  match in word " << i << ": " << match_word_info);
    }
}

bool SRamMatchTable::parse_ram(const value_t &v, std::vector<Ram> &res) {
    if (!CHECKTYPE(v, tVEC)) return true;  // supress added message
    for (auto &el : v.vec)                 // all elements must be positive integers
        if (el.type != tINT || el.i < 0) return false;
    switch (v.vec.size) {
        case 1:  // lamb unit
            if (v[0].i < Target::SRAM_LAMBS_PER_STAGE()) {
                res.emplace_back(v[0].i);
                return true;
            }
            break;
        case 2:                                       // row, col
            if (Target::SRAM_GLOBAL_ACCESS()) break;  // stage required
            if (v[0].i < Target::SRAM_ROWS(gress) && v[1].i < Target::SRAM_UNITS_PER_ROW()) {
                res.emplace_back(v[0].i, v[1].i);
                return true;
            }
            break;
        case 3:  // stage, row, col
            if (Target::SRAM_GLOBAL_ACCESS() && v[0].i < Target::NUM_STAGES(gress) &&
                v[1].i < Target::SRAM_ROWS(gress) && v[2].i < Target::SRAM_UNITS_PER_ROW()) {
                res.emplace_back(v[0].i, v[1].i, v[2].i);
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool SRamMatchTable::parse_way(const value_t &v) {
    Way way = {};
    way.lineno = v.lineno;
    if (!CHECKTYPE2(v, tVEC, tMAP)) return true;  // supress added message
    if (v.type == tVEC) {
        // DEPRECATED -- old style "raw" way for tofino1/2
        if (v.vec.size < 3 || v[0].type != tINT || v[1].type != tINT || v[2].type != tINT ||
            v[0].i < 0 || v[1].i < 0 || v[2].i < 0 || v[0].i >= Target::EXACT_HASH_GROUPS() ||
            v[1].i >= EXACT_HASH_ADR_GROUPS || v[2].i >= (1 << EXACT_HASH_SELECT_BITS)) {
            return false;
        }
        way.group_xme = v[0].i;
        way.index = v[1].i * EXACT_HASH_ADR_BITS;
        way.select = bitvec(v[2].i) << EXACT_HASH_FIRST_SELECT_BIT;
        for (int i = 3; i < v.vec.size; i++) {
            if (!CHECKTYPE(v[i], tVEC)) return true;  // supress added message
            if (!parse_ram(v[i], way.rams)) error(v[i].lineno, "invalid ram in way");
        }
    } else {
        int index_size = 0;
        for (auto &kv : MapIterChecked(v.map)) {
            if ((kv.key == "group" || kv.key == "xme") && CHECKTYPE(kv.value, tINT)) {
                if ((way.group_xme = kv.value.i) >= Target::IXBAR_HASH_GROUPS())
                    error(kv.value.lineno, "%s %ld out of range", kv.key.s, kv.value.i);
            } else if (kv.key == "index") {
                if (!CHECKTYPE2(kv.value, tINT, tRANGE)) continue;
                if (kv.value.type == tINT) {
                    way.index = kv.value.i;
                } else {
                    way.index = kv.value.range.lo;
                    way.index_hi = kv.value.range.hi;
                    index_size = kv.value.range.hi - kv.value.range.lo + 1;
                }
                if (way.index > Target::IXBAR_HASH_INDEX_MAX() ||
                    way.index % Target::IXBAR_HASH_INDEX_STRIDE() != 0)
                    error(kv.value.lineno, "invalid way index %d", way.index);
            } else if (kv.key == "select") {
                if (kv.value.type == tCMD && kv.value == "&") {
                    if (CHECKTYPE2(kv.value[1], tINT, tRANGE) && CHECKTYPE(kv.value[2], tINT)) {
                        way.select = bitvec(kv.value[2].i);
                        if (kv.value[1].type == tINT) {
                            way.select <<= kv.value[1].i;
                        } else {
                            way.select <<= kv.value[1].range.lo;
                            if (kv.value[1].range.hi < way.select.max().index())
                                error(kv.value.lineno, "invalid select mask for range");
                        }
                    }
                } else if (kv.value.type == tRANGE) {
                    way.select.setrange(kv.value.range.lo,
                                        kv.value.range.hi - kv.value.range.lo + 1);
                } else {
                    error(kv.value.lineno, "invalid select %s", value_desc(&kv.value));
                }
            } else if (kv.key == "rams" && CHECKTYPE(kv.value, tVEC)) {
                for (auto &ram : kv.value.vec) {
                    if (!CHECKTYPE(ram, tVEC)) break;
                    if (!parse_ram(ram, way.rams)) error(ram.lineno, "invalid ram in way");
                }
            }
        }
        if (index_size) {
            // FIXME -- currently this code is assuming the index bits cover just the ram index
            // bits and the subword bits, and not any select bits.  Perhaps that is wrong an it
            // should include the select bits.
            if (way.rams.empty()) {
                error(v.lineno, "no rams in way");
            } else {
                way.subword_bits = index_size - (way.isLamb() ? LAMB_DEPTH_BITS : SRAM_DEPTH_BITS);
                if (way.subword_bits < 0) error(v.lineno, "index range too small for way rams");
            }
        }
    }
    ways.push_back(way);
    return true;
}

void SRamMatchTable::common_sram_setup(pair_t &kv, const VECTOR(pair_t) & data) {
    if (kv.key == "ways") {
        if (!CHECKTYPE(kv.value, tVEC)) return;
        for (auto &w : kv.value.vec)
            if (!parse_way(w)) error(w.lineno, "invalid way descriptor");
    } else if (kv.key == "match") {
        if (kv.value.type == tVEC) {
            for (auto &v : kv.value.vec) {
                if (v == "hash_group")
                    match.emplace_back(new HashMatchSource(v));
                else
                    match.emplace_back(new Phv::Ref(gress, stage->stageno, v));
            }
        } else {
            if (kv.value == "hash_group")
                match.emplace_back(new HashMatchSource(kv.value));
            else
                match.emplace_back(new Phv::Ref(gress, stage->stageno, kv.value));
        }
    } else if (kv.key == "match_group_map") {
        mgm_lineno = kv.value.lineno;
        if (CHECKTYPE(kv.value, tVEC)) {
            word_info.resize(kv.value.vec.size);
            for (int i = 0; i < kv.value.vec.size; i++)
                if (CHECKTYPE(kv.value[i], tVEC)) {
                    if (kv.value[i].vec.size > 5)
                        error(kv.value[i].lineno, "Too many groups for word %d", i);
                    for (auto &v : kv.value[i].vec)
                        if (CHECKTYPE(v, tINT)) word_info[i].push_back(v.i);
                }
        }
    } else {
        warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key), name());
    }
}

void SRamMatchTable::common_sram_checks() {
    if (Target::SRAM_GLOBAL_ACCESS())
        alloc_global_srams();
    else
        alloc_rams(false, stage->sram_use, &stage->sram_search_bus_use);
    if (layout_size() > 0 && !format) error(lineno, "No format specified in table %s", name());
    if (!action.set() && !actions)
        error(lineno, "Table %s has neither action table nor immediate actions", name());
    if (actions && !action_bus) action_bus = ActionBus::create();
    if (input_xbar.empty()) input_xbar.emplace_back(InputXbar::create(this));
}

void SRamMatchTable::alloc_global_busses() { BUG(); }

void SRamMatchTable::pass1() {
    LOG1("### SRam match table " << name() << " pass1 " << loc());
    if (format) {
        verify_format();
        setup_ways();
        determine_word_and_result_bus();
    }
    if (Target::SRAM_GLOBAL_ACCESS())
        alloc_global_busses();
    else
        alloc_busses(stage->sram_search_bus_use, Layout::SEARCH_BUS);
    MatchTable::pass1();
    if (action_enable >= 0)
        if (action.args.size() < 1 || action.args[0].size() <= (unsigned)action_enable)
            error(lineno, "Action enable bit %d out of range for action selector", action_enable);
    if (gateway) {
        if (!gateway->layout.empty()) {
            for (auto &row : layout) {
                if (row.row == gateway->layout[0].row && row.bus == gateway->layout[0].bus &&
                    !row.memunits.empty()) {
                    unsigned gw_use = gateway->input_use() & 0xff;
                    auto &way = way_map.at(row.memunits[0]);
                    for (auto &grp : group_info) {
                        if (gw_use & grp.tofino_mask[way.word]) {
                            error(gateway->lineno,
                                  "match bus conflict between match and gateway"
                                  " on table %s",
                                  name());
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

void SRamMatchTable::setup_hash_function_ids() {
    unsigned hash_fn_id = 0;
    for (auto &w : ways) {
        if (hash_fn_ids.count(w.group_xme) == 0) hash_fn_ids[w.group_xme] = hash_fn_id++;
    }
}

void SRamMatchTable::setup_ways() {
    unsigned fmt_width = (format->size + 127) / 128;
    if (ways.empty()) {
        error(lineno, "No ways defined in table %s", name());
    } else if (ways[0].rams.empty()) {
        for (auto &w : ways)
            if (!w.rams.empty()) {
                error(w.lineno, "Must specify rams for all ways in tabls %s, or none", name());
                return;
            }
        if (layout.size() % fmt_width != 0) {
            error(lineno, "Rows is not a multiple of width in table %s", name());
            return;
        }
        for (unsigned i = 0; i < layout.size(); ++i) {
            unsigned first = (i / fmt_width) * fmt_width;
            if (layout[i].memunits.size() != layout[first].memunits.size())
                error(layout[i].lineno, "Row size mismatch within wide table %s", name());
        }
        if (error_count > 0) return;
        unsigned ridx = 0, cidx = 0;
        for (auto &way : ways) {
            if (ridx >= layout.size()) {
                error(way.lineno, "Not enough rams for ways in table %s", name());
                break;
            }
            unsigned size = 1U << way.select.popcount();
            for (unsigned i = 0; i < size; i++) {
                for (unsigned word = 0; word < fmt_width; ++word) {
                    BUG_CHECK(ridx + word < layout.size());
                    auto &row = layout[ridx + word];
                    BUG_CHECK(cidx < row.memunits.size());
                    way.rams.push_back(row.memunits[cidx]);
                }
                if (++cidx == layout[ridx].memunits.size()) {
                    ridx += fmt_width;
                    cidx = 0;
                }
            }
        }
        if (ridx < layout.size())
            error(ways[0].lineno, "Too many rams for ways in table %s", name());
    } else {
        std::set<Ram> rams;
        for (auto &row : layout) {
            for (auto &unit : row.memunits) {
                BUG_CHECK(!rams.count(unit), "%s duplicate in table", unit.desc());
                rams.insert(unit);
            }
        }
        int way = -1;
        for (auto &w : ways) {
            ++way;
            int index = -1;
            if (table_type() != ATCAM) {
                if ((w.rams.size() != (1U << w.select.popcount()) * fmt_width))
                    error(w.lineno, "Depth of way doesn't match number of rams in table %s",
                          name());
            } else {
                // Allowed to not fully match, as the partition index can be set from the
                // control plane
                if (!((w.rams.size() <= (1U << w.select.popcount()) * fmt_width) &&
                      (w.rams.size() % fmt_width) == 0))
                    error(w.lineno, "RAMs in ATCAM is not a legal multiple of the format width %s",
                          name());
            }
            for (auto &ram : w.rams) {
                ++index;
                if (way_map.count(ram)) {
                    if (way == way_map.at(ram).way)
                        error(w.lineno, "%s used twice in way %d of table %s", ram.desc(), way,
                              name());
                    else
                        error(w.lineno, "%s used ways %d and %d of table %s", ram.desc(), way,
                              way_map.at(ram).way, name());
                    continue;
                }
                way_map[ram].way = way;
                if (!ram.isLamb() && !rams.count(ram))
                    error(w.lineno, "%s in way %d not part of table %s", ram.desc(), way, name());
                rams.erase(ram);
            }
        }
        for (const auto &unit : rams) {
            error(lineno, "%s not in any way of table %s", unit.desc(), name());
        }
    }
    if (error_count > 0) return;
    int way = 0;
    for (auto &w : ways) {
        MaskCounter bank(w.select.getrange(EXACT_HASH_FIRST_SELECT_BIT, 32));
        unsigned index = 0, word = 0;
        int col = -1;
        for (auto &ram : w.rams) {
            auto &wm = way_map[ram];
            wm.way = way;
            wm.index = index;
            wm.word = fmt_width - word - 1;
            wm.bank = bank;
            if (word && col != ram.col)
                error(w.lineno, "Wide exact match split across columns %d and %d", col, ram.col);
            col = ram.col;
            ++index;
            if (++word == fmt_width) {
                word = 0;
                bank++;
            }
        }
        ++way;
    }
    setup_hash_function_ids();
}

/**
 * Either fills out the word/result bus information each row, if it is not provided directly by
 * the compiler, or verifies that the word/result_bus information matches directly with
 * what has been calculated through the way information provided.
 */
void SRamMatchTable::determine_word_and_result_bus() {
    for (auto &row : layout) {
        int word = -1;
        bool word_set = false;
        for (auto &ram : row.memunits) {
            auto &way = way_map.at(ram);
            if (word_set) {
                BUG_CHECK(word == way.word);
            } else {
                word = way.word;
                word_set = true;
            }
        }
        if (row.word_initialized()) {
            if (word != row.word)
                error(lineno, "Word on row %d bus %d does not align with word in RAM", row.row,
                      row.bus.at(Layout::SEARCH_BUS));
        } else {
            row.word = word;
        }
    }

    for (auto &row : layout) {
        bool result_bus_needed = false;
        if (row.word < 0) {
            // row with no rams -- assume it needs a result bus for the payload
            result_bus_needed = true;
        } else {
            for (auto group_in_word : word_info.at(row.word)) {
                if (group_info[group_in_word].result_bus_word == row.word) result_bus_needed = true;
            }
        }
        if (!row.bus.count(Layout::RESULT_BUS) && result_bus_needed)
            row.bus[Layout::RESULT_BUS] = row.bus.at(Layout::SEARCH_BUS);
        if (row.bus.count(Layout::RESULT_BUS)) {
            auto *old = stage->match_result_bus_use[row.row][row.bus.at(Layout::RESULT_BUS)];
            if (old && old != this)
                error(row.lineno,
                      "inconsistent use of match result bus %d on row %d between "
                      "table %s and %s",
                      row.row, row.bus.at(Layout::RESULT_BUS), name(), old->name());
            stage->match_result_bus_use[row.row][row.bus.at(Layout::RESULT_BUS)] = this;
        }
    }
}

int SRamMatchTable::determine_pre_byteswizzle_loc(MatchSource *ms, int lo, int hi, int word) {
    auto phv_p = dynamic_cast<Phv::Ref *>(ms);
    BUG_CHECK(phv_p);
    auto phv_ref = *phv_p;
    Phv::Slice sl(*phv_ref, lo, hi);
    BUG_CHECK(word_ixbar_group[word] >= 0);
    return find_on_ixbar(sl, word_ixbar_group[word]);
}

template <class REGS>
void SRamMatchTable::write_attached_merge_regs(REGS &regs, int bus, int word, int word_group) {
    int group = word_info[word][word_group];
    auto &merge = regs.rams.match.merge;
    for (auto &st : attached.stats) {
        if (group_info[group].result_bus_word == static_cast<int>(word)) {
            merge.mau_stats_adr_exact_shiftcount[bus][word_group] =
                st->to<CounterTable>()->determine_shiftcount(st, group, word, 0);
        } else if (options.match_compiler) {
            /* unused, so should not be set... */
            merge.mau_stats_adr_exact_shiftcount[bus][word_group] = 7;
        }
        break; /* all must be the same, only config once */
    }
    for (auto &m : attached.meters) {
        if (group_info[group].overhead_word == static_cast<int>(word) ||
            group_info[group].overhead_word == -1) {
            m->to<MeterTable>()->setup_exact_shift(regs, bus, group, word, word_group, m,
                                                   attached.meter_color);
        } else if (options.match_compiler) {
            /* unused, so should not be set... */
            merge.mau_meter_adr_exact_shiftcount[bus][word_group] = 16;
        }
        break; /* all must be the same, only config once */
    }
    for (auto &s : attached.statefuls) {
        if (group_info[group].overhead_word == static_cast<int>(word) ||
            group_info[group].overhead_word == -1) {
            merge.mau_meter_adr_exact_shiftcount[bus][word_group] =
                s->to<StatefulTable>()->determine_shiftcount(s, group, word, 0);
        } else if (options.match_compiler) {
            /* unused, so should not be set... */
            merge.mau_meter_adr_exact_shiftcount[bus][word_group] = 16;
        }
        break; /* all must be the same, only config once */
    }
}

template <class REGS>
void SRamMatchTable::write_regs_vt(REGS &regs) {
    LOG1("### SRam match table " << name() << " write_regs " << loc());
    MatchTable::write_regs(regs, 0, this);
    auto &merge = regs.rams.match.merge;
    unsigned fmt_width = format ? (format->size + 127) / 128 : 0;
    bitvec match_mask;
    match_mask.setrange(0, 128 * fmt_width);
    version_nibble_mask.setrange(0, 32 * fmt_width);
    for (unsigned i = 0; format && i < format->groups(); i++) {
        if (Format::Field *match = format->field("match", i)) {
            for (auto &piece : match->bits) match_mask.clrrange(piece.lo, piece.hi + 1 - piece.lo);
        }
        if (Format::Field *version = format->field("version", i)) {
            match_mask.clrrange(version->bit(0), version->size);
            version_nibble_mask.clrrange(version->bit(0) / 4, 1);
        }
    }
    Format::Field *next = format ? format->field("next") : nullptr;
    if (format && !next && hit_next.size() > 1) next = format->field("action");

    /* iterating through rows in the sram array;  while in this loop, 'row' is the
     * row we're on, 'word' is which word in a wide full-way the row is for, and 'way'
     * is which full-way of the match table the row is for.  For compatibility with the
     * compiler, we iterate over rows and ways in order, and words from msb to lsb (reversed) */
    int index = -1;
    for (auto &row : layout) {
        index++; /* index of the row in the layout */
        int search_bus = ::get(row.bus, Layout::SEARCH_BUS, -1);
        /* setup match logic in rams */
        auto &rams_row = regs.rams.array.row[row.row];
        auto &vh_adr_xbar = rams_row.vh_adr_xbar;
        bool first = true;
        int hash_group = -1;
        unsigned word = ~0;
        auto vpn_iter = row.vpns.begin();
        for (auto &memunit : row.memunits) {
            int col = memunit.col;
            BUG_CHECK(memunit.stage == INT_MIN && memunit.row == row.row, "bogus %s in row %d",
                      memunit.desc(), row.row);
            auto &way = way_map.at(memunit);
            if (first) {
                hash_group = ways[way.way].group_xme;
                word = way.word;
                setup_muxctl(vh_adr_xbar.exactmatch_row_hashadr_xbar_ctl[search_bus], hash_group);
                first = false;
            } else if (hash_group != ways[way.way].group_xme || int(word) != way.word) {
                auto first_way = way_map.at(row.memunits[0]);
                error(ways[way.way].lineno,
                      "table %s ways #%d and #%d use the same row bus "
                      "(%d.%d) but different %s",
                      name(), first_way.way, way.way, row.row, search_bus,
                      int(word) == way.word ? "hash groups" : "word order");
                hash_group = ways[way.way].group_xme;
                word = way.word;
            }
            setup_muxctl(vh_adr_xbar.exactmatch_mem_hashadr_xbar_ctl[col],
                         ways[way.way].index / EXACT_HASH_ADR_BITS + search_bus * 5);
            if (options.match_compiler || ways[way.way].select) {
                // Glass always sets this.  When mask == 0, bank will also be 0, and the
                // comparison will always match, so the bus need not be read (inp_sel).
                // CSR suggests it should NOT be set if not needed to save power.
                auto &bank_enable = vh_adr_xbar.exactmatch_bank_enable[col];
                bank_enable.exactmatch_bank_enable_bank_mask =
                    ways[way.way].select.getrange(EXACT_HASH_FIRST_SELECT_BIT, 32);
                bank_enable.exactmatch_bank_enable_bank_id = way.bank;
                bank_enable.exactmatch_bank_enable_inp_sel |= 1 << search_bus;
            }
            auto &ram = rams_row.ram[col];
            for (unsigned i = 0; i < 4; i++)
                ram.match_mask[i] = match_mask.getrange(way.word * 128U + i * 32, 32);

            if (next) {
                for (int group : word_info[way.word]) {
                    if (group_info[group].result_bus_word != way.word) continue;
                    int pos = (next->by_group[group]->bit(0) % 128) - 1;
                    auto &n = ram.match_next_table_bitpos;
                    switch (group_info[group].result_bus_word_group()) {
                        case 0:
                            break;
                        case 1:
                            n.match_next_table1_bitpos = pos;
                            break;
                        case 2:
                            n.match_next_table2_bitpos = pos;
                            break;
                        case 3:
                            n.match_next_table3_bitpos = pos;
                            break;
                        case 4:
                            n.match_next_table4_bitpos = pos;
                            break;
                        default:
                            BUG();
                    }
                }
            }

            ram.unit_ram_ctl.match_ram_logical_table = logical_id;
            ram.unit_ram_ctl.match_ram_write_data_mux_select = 7; /* unused */
            ram.unit_ram_ctl.match_ram_read_data_mux_select = 7;  /* unused */
            ram.unit_ram_ctl.match_ram_matchdata_bus1_sel = search_bus;
            if (row.bus.count(Layout::RESULT_BUS))
                ram.unit_ram_ctl.match_result_bus_select = 1 << row.bus.at(Layout::RESULT_BUS);
            if (auto cnt = word_info[way.word].size())
                ram.unit_ram_ctl.match_entry_enable = ~(~0U << cnt);
            auto &unitram_config =
                regs.rams.map_alu.row[row.row].adrmux.unitram_config[col / 6][col % 6];
            unitram_config.unitram_type = 1;
            unitram_config.unitram_logical_table = logical_id;
            switch (gress) {
                case INGRESS:
                case GHOST:
                    unitram_config.unitram_ingress = 1;
                    break;
                case EGRESS:
                    unitram_config.unitram_egress = 1;
                    break;
                default:
                    BUG();
            }
            unitram_config.unitram_enable = 1;

            int vpn = *vpn_iter++;
            std::vector<int> vpn01;
            auto groups_in_word = word_info[way.word];
            // Action format is made up of multiple groups (groups_in_format) which can be spread
            // across multiple words. The match_group_map specifies which groups are within each
            // word. For an N pack across M words if N > M, we have one or more words with multiple
            // groups.
            // Below code assigns VPN for each group(groups_in_word) within a word which are indexed
            // separately from groups_in_format.
            // E.g.
            // format: {
            //   action(0): 0..0, immediate(0): 2..9,   version(0): 112..115, match(0): 18..71,
            //   action(1): 1..1, immediate(1): 10..17, version(1): 116..119,
            //         match(1): [ 194..199, 72..111, 120..127  ],
            //   action(2): 128..128, immediate(2): 129..136, version(2): 240..243,
            //         match(2): 138..191,
            //   action(3): 256..256, immediate(3): 257..264, version(3): 368..371,
            //          match(3): 266..319,
            //   action(4): 384..384, immediate(4): 385..392, version(4): 496..499,
            //      match(4): 394..447 }
            //   match_group_map: [ [ 1, 0 ], [ 1, 2], [ 3 ], [ 4 ]  ]
            //                        ^         ^
            // }
            // In the above example the "format" specifies the 5 groups packed across 4 RAMs.These
            // are the groups_in_format
            // The "match_group_map" specifies the groups within each word.
            // Group 1 is spread across word 0 & word 1.
            // Within word 0 - group 1 is group_in_word 0 and group 0 is group_in_word 1
            // Within word 1 - group 1 is group_in_word 0 and group 2 is group_in_word 1
            // This distinction is used while specifying the config register in setting the subfield
            // on match_ram_vpn_lsbs.
            for (auto group_in_word = 0; group_in_word < groups_in_word.size(); group_in_word++) {
                auto group_in_format = groups_in_word[group_in_word];
                int overhead_word = group_info[group_in_format].overhead_word;
                int group_vpn = vpn + group_info[group_in_format].vpn_offset;
                bool ok = false;
                for (unsigned i = 0; i < vpn01.size(); ++i) {
                    if (vpn01[i] == group_vpn >> 2) {
                        ok = true;
                        group_vpn = (group_vpn & 3) + (i << 2);
                        break;
                    }
                }
                if (!ok) {
                    if (vpn01.size() >= 2) {
                        error(mgm_lineno > 0 ? mgm_lineno : lineno,
                              "Too many diverse vpns in table layout for %s", name());
                        break;
                    }
                    vpn01.push_back(group_vpn >> 2);
                    group_vpn &= 3;
                    if (vpn01.size() == 1) {
                        ram.match_ram_vpn.match_ram_vpn0 = vpn01.back();
                    } else {
                        ram.match_ram_vpn.match_ram_vpn1 = vpn01.back();
                        group_vpn |= 4;
                    }
                }
                ram.match_ram_vpn.match_ram_vpn_lsbs.set_subfield(group_vpn, group_in_word * 3, 3);
            }

            int word_group = 0;
            for (int group : word_info[way.word]) {
                unsigned mask = group_info[group].tofino_mask[way.word];
                ram.match_bytemask[word_group].mask_bytes_0_to_13 = ~mask & 0x3fff;
                ram.match_bytemask[word_group].mask_nibbles_28_to_31 = ~(mask >> 14) & 0xf;
                word_group++;
            }
            for (; word_group < 5; word_group++) {
                ram.match_bytemask[word_group].mask_bytes_0_to_13 = 0x3fff;
                ram.match_bytemask[word_group].mask_nibbles_28_to_31 = 0xf;
            }
            if (gress == EGRESS)
                regs.cfg_regs.mau_cfg_uram_thread[col / 4U] |= 1U << (col % 4U * 8U + row.row);
            rams_row.emm_ecc_error_uram_ctl[timing_thread(gress)] |= 1U << (col - 2);
        }
        /* setup input xbars to get data to the right places on the bus(es) */
        bool using_match = false;
        // Loop for determining the config to indicate which bytes from the search bus
        // are compared to the bytes on the RAM line
        if (!row.memunits.empty()) {
            auto &byteswizzle_ctl = rams_row.exactmatch_row_vh_xbar_byteswizzle_ctl[search_bus];
            for (unsigned i = 0; format && i < format->groups(); i++) {
                if (Format::Field *match = format->field("match", i)) {
                    unsigned bit = 0;
                    for (auto &piece : match->bits) {
                        if (piece.lo / 128U != word) {
                            bit += piece.size();
                            continue;
                        }
                        using_match = true;
                        for (unsigned fmt_bit = piece.lo; fmt_bit <= piece.hi;) {
                            unsigned byte = (fmt_bit % 128) / 8;
                            unsigned bits_in_byte = (byte + 1) * 8 - (fmt_bit % 128);
                            if (fmt_bit + bits_in_byte > piece.hi + 1)
                                bits_in_byte = piece.hi + 1 - fmt_bit;
                            auto it = --match_by_bit.upper_bound(bit);
                            int lo = bit - it->first;
                            int hi = lo + bits_in_byte - 1;
                            int bus_loc = determine_pre_byteswizzle_loc(it->second, lo, hi, word);
                            BUG_CHECK(bus_loc >= 0 && bus_loc < 16);
                            for (unsigned b = 0; b < bits_in_byte; b++, fmt_bit++)
                                byteswizzle_ctl[byte][fmt_bit % 8U] = 0x10 + bus_loc;
                            bit += bits_in_byte;
                        }
                    }
                    BUG_CHECK(bit == match->size);
                }
                if (Format::Field *version = format->field("version", i)) {
                    if (version->bit(0) / 128U != word) continue;
                    ///> if no match, but a version/valid is, the vh_xbar needs to be
                    ///> enabled.  This was preventing anything from running
                    using_match = true;
                    for (unsigned j = 0; j < version->size; ++j) {
                        unsigned bit = version->bit(j);
                        unsigned byte = (bit % 128) / 8;
                        byteswizzle_ctl[byte][bit % 8U] = 8;
                    }
                }
            }
            if (using_match) {
                auto &vh_xbar_ctl = rams_row.vh_xbar[search_bus].exactmatch_row_vh_xbar_ctl;
                if (word_ixbar_group[word] >= 0) {
                    setup_muxctl(vh_xbar_ctl, word_ixbar_group[word]);
                } else {
                    // Need the bus for version/valid, but don't care what other data is on it.  So
                    // just set the enable without actually selecting an input -- if another table
                    // is sharing the bus, it will set it, otherwise we'll get ixbar group 0
                    vh_xbar_ctl.exactmatch_row_vh_xbar_enable = 1;
                }
                vh_xbar_ctl.exactmatch_row_vh_xbar_thread = timing_thread(gress);
            }
        }
        /* setup match central config to extract results of the match */
        ssize_t r_bus = -1;
        if (row.bus.count(Layout::RESULT_BUS)) r_bus = row.row * 2 + row.bus.at(Layout::RESULT_BUS);
        // If the result bus is not to be used, then the registers are not necessary to set up
        // for shift/mask/default etc.
        /* FIXME -- factor this where possible with ternary match code */
        if (action) {
            if (auto adt = action->to<ActionTable>()) {
                if (r_bus >= 0) {
                    /* FIXME -- support for multiple sizes of action data? */
                    merge.mau_actiondata_adr_mask[0][r_bus] = adt->determine_mask(action);
                    merge.mau_actiondata_adr_vpn_shiftcount[0][r_bus] =
                        adt->determine_vpn_shiftcount(action);
                }
            }
        }

        if (format && word < word_info.size()) {
            for (unsigned word_group = 0; word_group < word_info[word].size(); word_group++) {
                int group = word_info[word][word_group];
                if (group_info[group].result_bus_word == static_cast<int>(word)) {
                    BUG_CHECK(r_bus >= 0);
                    if (format->immed) {
                        BUG_CHECK(format->immed->by_group[group]->bit(0) / 128U == word);
                        merge.mau_immediate_data_exact_shiftcount[r_bus][word_group] =
                            format->immed->by_group[group]->bit(0) % 128;
                    }
                    if (instruction) {
                        int shiftcount = 0;
                        if (auto field = instruction.args[0].field()) {
                            assert(field->by_group[group]->bit(0) / 128U == word);
                            shiftcount = field->by_group[group]->bit(0) % 128U;
                        } else if (auto field = instruction.args[1].field()) {
                            assert(field->by_group[group]->bit(0) / 128U == word);
                            shiftcount = field->by_group[group]->bit(0) % 128U;
                        }
                        merge.mau_action_instruction_adr_exact_shiftcount[r_bus][word_group] =
                            shiftcount;
                    }
                }
                /* FIXME -- factor this where possible with ternary match code */
                if (action) {
                    if (group_info[group].result_bus_word == static_cast<int>(word)) {
                        BUG_CHECK(r_bus >= 0);
                        merge.mau_actiondata_adr_exact_shiftcount[r_bus][word_group] =
                            action->determine_shiftcount(action, group, word, 0);
                    }
                }
                if (attached.selector) {
                    if (group_info[group].result_bus_word == static_cast<int>(word)) {
                        BUG_CHECK(r_bus >= 0);
                        auto sel = get_selector();
                        merge.mau_meter_adr_exact_shiftcount[r_bus][word_group] =
                            sel->determine_shiftcount(attached.selector, group, word, 0);
                        merge.mau_selectorlength_shiftcount[0][r_bus] =
                            sel->determine_length_shiftcount(attached.selector_length, group, word);
                        merge.mau_selectorlength_mask[0][r_bus] =
                            sel->determine_length_mask(attached.selector_length);
                        merge.mau_selectorlength_default[0][r_bus] =
                            sel->determine_length_default(attached.selector_length);
                    }
                }
                if (idletime) {
                    if (group_info[group].result_bus_word == static_cast<int>(word)) {
                        BUG_CHECK(r_bus >= 0);
                        merge.mau_idletime_adr_exact_shiftcount[r_bus][word_group] =
                            idletime->direct_shiftcount();
                    }
                }
                if (r_bus >= 0) write_attached_merge_regs(regs, r_bus, word, word_group);
            }
        } else if (format) {
            // If we have a result bus without any attached memories, program
            // the registers on this row because a subset of the registers have been
            // programmed elsewhere and it can break things if we have a partial configuration.
            // FIXME: avoid programming any registers if we don't actually use the result bus.
            if (r_bus >= 0) write_attached_merge_regs(regs, r_bus, 0, 0);
        }
        for (auto &ram : row.memunits) {
            int word_group = 0;
            auto &merge_col = merge.col[ram.col];
            for (int group : word_info[word]) {
                int result_bus_word = group_info[group].result_bus_word;
                if (int(word) == result_bus_word) {
                    BUG_CHECK(r_bus >= 0);
                    merge_col.row_action_nxtable_bus_drive[row.row] |= 1 << (r_bus % 2);
                }
                if (word_group < 2) {
                    auto &way = way_map.at(ram);
                    int idx = way.index + word - result_bus_word;
                    int overhead_row = ways[way.way].rams[idx].row;
                    auto &hitmap_ixbar = merge_col.hitmap_output_map[2 * row.row + word_group];
                    setup_muxctl(hitmap_ixbar,
                                 overhead_row * 2 + group_info[group].result_bus_word_group());
                }
                ++word_group;
            }
            // setup_muxctl(merge.col[ram.col].hitmap_output_map[bus],
            //                layout[index+word].row*2 + layout[index+word].bus);
        }
        // if (gress == EGRESS)
        //     merge.exact_match_delay_config.exact_match_bus_thread |= 1 << bus;
        if (r_bus >= 0) {
            merge.exact_match_phys_result_en[r_bus / 8U] |= 1U << (r_bus % 8U);
            merge.exact_match_phys_result_thread[r_bus / 8U] |= timing_thread(gress)
                                                                << (r_bus % 8U);
            if (stage->tcam_delay(gress))
                merge.exact_match_phys_result_delay[r_bus / 8U] |= 1U << (r_bus % 8U);
        }
    }

    merge.exact_match_logical_result_en |= 1 << logical_id;
    if (stage->tcam_delay(gress) > 0) merge.exact_match_logical_result_delay |= 1 << logical_id;
    if (actions) actions->write_regs(regs, this);
    if (gateway) gateway->write_regs(regs);
    if (idletime) idletime->write_regs(regs);
    for (auto &hd : hash_dist) hd.write_regs(regs, this);
}
FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, void SRamMatchTable::write_regs, (mau_regs & regs),
                      { write_regs_vt(regs); })

std::string SRamMatchTable::get_match_mode(const Phv::Ref &pref, int offset) const {
    return "unused";
}

void SRamMatchTable::add_field_to_pack_format(json::vector &field_list, int basebit,
                                              std::string name, const Table::Format::Field &field,
                                              const Table::Actions::Action *act) const {
    if (name != "match") {
        // FIXME -- tofino always pads out the wordsize so basebit is always 0.
        basebit = 0;
        Table::add_field_to_pack_format(field_list, basebit, name, field, act);
        return;
    }
    LOG3("Adding fields for " << name << " - " << field << " to pack format for SRAM table "
                              << this->name() << " in action : " << act);
    unsigned bit = 0;
    for (auto &piece : field.bits) {
        auto mw = --match_by_bit.upper_bound(bit);
        int lo = bit - mw->first;
        int lsb_mem_word_idx = piece.lo / MEM_WORD_WIDTH;
        int msb_mem_word_idx = piece.hi / MEM_WORD_WIDTH;
        int offset = piece.lo % MEM_WORD_WIDTH;
        while (mw != match_by_bit.end() && mw->first < bit + piece.size()) {
            std::string source = "";
            std::string immediate_name = "";
            std::string mw_name = mw->second->name();
            int start_bit = 0;

            get_cjson_source(mw_name, source, start_bit);
            if (source == "")
                error(lineno, "Cannot determine proper source for field %s", name.c_str());
            std::string field_name, global_name = "";
            std::string match_mode;
            if (auto phv_p = dynamic_cast<Phv::Ref *>(mw->second)) {
                field_name = mw->second->name();
                // If the name has a slice in it, remove it and add the lo bit of
                // the slice to field_bit.  This takes the place of
                // canon_field_list(), rather than extracting the slice component
                // of the field name, if present, and appending it to the key name.
                int slice_offset = remove_name_tail_range(field_name);
                start_bit = lo + slice_offset + mw->second->fieldlobit();
                global_name = field_name;
                auto p = find_p4_param(field_name, "", start_bit);
                if (!p && !p4_params_list.empty()) {
                    warning(lineno,
                            "Cannot find field name %s in p4_param_order "
                            "for table %s",
                            field_name.c_str(), this->name());
                } else if (p && !p->key_name.empty()) {
                    field_name = p->key_name;
                }
                match_mode = get_match_mode(*phv_p, mw->first);
            } else if (dynamic_cast<HashMatchSource *>(mw->second)) {
                field_name = "--proxy_hash--";
                match_mode = "unused";
                start_bit = mw->second->fieldlobit();
            } else {
                BUG();
            }

            field_list.push_back(json::map{{"field_name", json::string(field_name)},
                                           {"global_name", json::string(global_name)},
                                           {"source", json::string(source)},
                                           {"lsb_mem_word_offset", json::number(offset)},
                                           {"start_bit", json::number(start_bit)},
                                           {"immediate_name", json::string(immediate_name)},
                                           {"lsb_mem_word_idx", json::number(lsb_mem_word_idx)},
                                           {"msb_mem_word_idx", json::number(msb_mem_word_idx)},
                                           // FIXME-JSON
                                           {"match_mode", json::string(match_mode)},
                                           {"enable_pfe", json::False()},  // FIXME-JSON
                                           {"field_width", json::number(mw->second->size())}});
            LOG5("Adding json field  " << field_list.back());
            offset += mw->second->size();
            lo = 0;
            ++mw;
        }
        bit += piece.size();
    }
}

void SRamMatchTable::add_action_cfgs(json::map &tbl, json::map &stage_tbl) const {
    if (actions) {
        actions->gen_tbl_cfg(tbl["actions"]);
        actions->add_action_format(this, stage_tbl);
    } else if (action && action->actions) {
        action->actions->gen_tbl_cfg(tbl["actions"]);
        action->actions->add_action_format(this, stage_tbl);
    }
}

unsigned SRamMatchTable::get_format_width() const {
    return format ? (format->size + 127) / 128 : 0;
}

unsigned SRamMatchTable::get_number_entries() const {
    unsigned fmt_width = get_format_width();
    unsigned number_entries = 0;
    if (format) number_entries = layout_size() / fmt_width * format->groups() * entry_ram_depth();
    return number_entries;
}

json::map *SRamMatchTable::add_common_sram_tbl_cfgs(json::map &tbl, std::string match_type,
                                                    std::string stage_table_type) const {
    common_tbl_cfg(tbl);
    json::map &match_attributes = tbl["match_attributes"];
    json::vector &stage_tables = match_attributes["stage_tables"];
    json::map *stage_tbl_ptr =
        add_stage_tbl_cfg(match_attributes, stage_table_type.c_str(), get_number_entries());
    json::map &stage_tbl = *stage_tbl_ptr;
    // This is a only a glass required field, as it is only required when no default action
    // is specified, which is impossible for Brig through p4-16
    stage_tbl["default_next_table"] = Stage::end_of_pipe();
    match_attributes["match_type"] = match_type;
    add_hash_functions(stage_tbl);
    add_action_cfgs(tbl, stage_tbl);
    add_result_physical_buses(stage_tbl);
    MatchTable::gen_idletime_tbl_cfg(stage_tbl);
    merge_context_json(tbl, stage_tbl);
    add_all_reference_tables(tbl);
    return stage_tbl_ptr;
}

int SRamMatchTable::find_problematic_vpn_offset() const {
    // Any single word of a match that contains 3 or more groups whose min and max vpn_offset
    // differs by more than 5 is going to be a problem.  We need to permute the offsets so that
    // does not happen
    if (group_info.size() <= 6) return -1;  // can't differ by more than 5
    for (auto &word : word_info) {
        if (word.size() <= 2) continue;  // can't be a problem
        int minvpn = -1, maxvpn = -1, avg = 0;
        for (auto group : word) {
            int vpn_offset = group_info[group].vpn_offset;
            if (minvpn < 0)
                minvpn = maxvpn = vpn_offset;
            else if (minvpn > vpn_offset)
                minvpn = vpn_offset;
            else if (maxvpn < vpn_offset)
                maxvpn = vpn_offset;
            avg += vpn_offset;
        }
        if (maxvpn - minvpn > 5) {
            if (minvpn + maxvpn > (2 * avg) / word.size())
                minvpn = maxvpn;  // look for the max to move, instead of min
            for (auto group : word) {
                if (group_info[group].vpn_offset == minvpn) return group;
            }
            BUG("failed to find the group vpn we just saw");
        }
    }
    return -1;  // no problem found
}

void SRamMatchTable::alloc_vpns() {
    if (error_count > 0 || no_vpns || layout_size() == 0 || layout[0].vpns.size() > 0) return;
    int period, width, depth;
    const char *period_name;
    vpn_params(width, depth, period, period_name);
    std::map<Ram, int *> vpn_for;
    for (auto &row : layout) {
        row.vpns.resize(row.memunits.size());
        int i = 0;
        for (auto &ram : row.memunits) vpn_for[ram] = &row.vpns[i++];
    }
    int vpn = 0, word = 0;
    for (auto &way : ways) {
        for (auto unit : way.rams) {
            *vpn_for[unit] = vpn;
            if (++word == width) {
                word = 0;
                vpn += period;
            }
        }
    }

    int fix = find_problematic_vpn_offset();
    if (fix >= 0) {
        // Swap it with the middle one.  That should fix all the cases we've seen
        int middle = group_info.size() / 2;
        BUG_CHECK(middle != fix, "vpn_offset fix doesn't work");
        std::swap(group_info[fix].vpn_offset, group_info[middle].vpn_offset);
        BUG_CHECK(find_problematic_vpn_offset() < 0, "vpn_offset fix did not work");
    }
}
