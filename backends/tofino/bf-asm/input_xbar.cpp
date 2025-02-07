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

#include "input_xbar.h"

#include <config.h>
#include <stdlib.h>

#include <fstream>

#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"
#include "hashexpr.h"
#include "lib/log.h"
#include "lib/range.h"
#include "misc.h"
#include "power_ctl.h"

// template specialization declarations
#include "backends/tofino/bf-asm/jbay/input_xbar.h"
#include "backends/tofino/bf-asm/tofino/input_xbar.h"

void HashCol::dbprint(std::ostream &out) const {
    out << "HashCol: " << " lineno: " << lineno << " bit: " << bit << " data: " << data
        << " valid: " << valid;
    if (fn) out << " fn: " << *fn << std::endl;
}

DynamicIXbar::DynamicIXbar(const Table *tbl, const pair_t &data) {
    if (CHECKTYPE(data.key, tINT)) {
        bit = data.key.i;
        if (bit < 0 || bit >= Target::DYNAMIC_CONFIG_INPUT_BITS())
            error(data.key.lineno, "Invalid dynamic config bit %d", bit);
    }
    if (CHECKTYPE2(data.value, tMAP, tMATCH)) {
        if (data.value.type == tMAP) {
            for (auto &kv : data.value.map)
                if (CHECKTYPE(kv.value, tMATCH))
                    match_phv.emplace_back(Phv::Ref(tbl->gress, tbl->stage->stageno, data.key),
                                           data.value.m);
        } else {
            match = data.value.m;
        }
    }
}

int InputXbar::group_max_index(Group::type_t t) const {
    switch (t) {
        case Group::EXACT:
            return EXACT_XBAR_GROUPS;
        case Group::TERNARY:
            return TCAM_XBAR_GROUPS;
        case Group::BYTE:
            return BYTE_XBAR_GROUPS;
        default:
            BUG("invalid group type for %s: %s", Target::name(), group_type(t));
    }
    return 0;
}

InputXbar::Group InputXbar::group_name(bool tern, const value_t &key) const {
    if (CHECKTYPE(key, tCMD)) {
        int index = 1;
        if (key[0] != "group" && (key[1] == "group" || key[1] == "table")) ++index;
        if (PCHECKTYPE(key.vec.size == index + 1, key[index], tINT)) {
            index = key[index].i;
            if (key[0] == "group") return Group(tern ? Group::TERNARY : Group::EXACT, index);
            if (key[0] == "exact" && key[1] == "group") return Group(Group::EXACT, index);
            if (key[0] == "ternary" && key[1] == "group") return Group(Group::TERNARY, index);
            if (key[0] == "byte" && key[1] == "group") return Group(Group::BYTE, index);
        }
    }
    return Group(Group::INVALID, 0);
}

int InputXbar::group_size(Group::type_t t) const {
    switch (t) {
        case Group::EXACT:
            return EXACT_XBAR_GROUP_SIZE;
        case Group::TERNARY:
            return TCAM_XBAR_GROUP_SIZE;
        case Group::BYTE:
            return BYTE_XBAR_GROUP_SIZE;
        default:
            BUG("invalid group type for %s: %s", Target::name(), group_type(t));
    }
    return 0;
}

const char *InputXbar::group_type(Group::type_t t) const {
    switch (t) {
        case Group::EXACT:
            return "exact";
        case Group::TERNARY:
            return "ternary";
        case Group::BYTE:
            return "byte";
        case Group::GATEWAY:
            return "gateway";
        case Group::XCMP:
            return "xcmp";
        default:
            return "";
    }
}

void InputXbar::parse_group(Table *t, Group gr, const value_t &value) {
    BUG_CHECK(gr.index >= 0, "invalid group");
    auto &group = groups[gr];
    if (value.type == tVEC) {
        for (auto &reg : value.vec) group.emplace_back(Phv::Ref(t->gress, t->stage->stageno, reg));
    } else if (value.type == tMAP) {
        for (auto &reg : value.map) {
            if (!CHECKTYPE2(reg.key, tINT, tRANGE)) continue;
            int lo = -1, hi = -1;
            if (reg.key.type == tINT) {
                lo = reg.key.i;
            } else {
                lo = reg.key.lo;
                hi = reg.key.hi;
            }
            if (lo < 0 || lo >= group_size(gr.type)) {
                error(reg.key.lineno, "Invalid offset for %s group", group_type(gr.type));
            } else if (gr.type == Group::TERNARY && lo >= 40) {
                if (hi >= lo) hi -= 40;
                groups[Group(Group::BYTE, gr.index / 2)].emplace_back(
                    Phv::Ref(t->gress, t->stage->stageno, reg.value), lo - 40, hi);
            } else {
                group.emplace_back(Phv::Ref(t->gress, t->stage->stageno, reg.value), lo, hi);
            }
        }
    } else {
        group.emplace_back(Phv::Ref(t->gress, t->stage->stageno, value));
    }
}

void InputXbar::parse_hash_group(HashGrp &hash_group, const value_t &value) {
    if (value.type == tINT && (unsigned)value.i < Target::EXACT_HASH_TABLES()) {
        hash_group.tables |= 1U << value.i;
        return;
    }
    if (!CHECKTYPE2(value, tVEC, tMAP)) return;
    const VECTOR(value_t) *tbl = 0;
    if (value.type == tMAP) {
        for (auto &el : MapIterChecked(value.map)) {
            if (el.key == "seed") {
                if (!CHECKTYPE2(el.value, tINT, tBIGINT)) continue;
                if (el.value.type == tBIGINT) {
                    int shift = 0;
                    for (int i = 0; i < el.value.bigi.size; ++i) {
                        if (shift >= 64) {
                            error(el.key.lineno, "Invalid seed %s too large",
                                  value_desc(&el.value));
                            break;
                        }
                        hash_group.seed |= el.value.bigi.data[i] << shift;
                        shift += CHAR_BIT * sizeof(el.value.bigi.data[i]);
                    }
                } else {
                    hash_group.seed |= el.value.i & 0xFFFFFFFF;
                }
            } else if (el.key == "table") {
                if (el.value.type == tINT) {
                    if (el.value.i < 0 || el.value.i >= Target::EXACT_HASH_TABLES())
                        error(el.value.lineno, "invalid hash group descriptor");
                    else
                        hash_group.tables |= 1U << el.value.i;
                } else if (CHECKTYPE(el.value, tVEC)) {
                    tbl = &el.value.vec;
                }
            } else if (el.key == "seed_parity") {
                if (el.value.type == tSTR && el.value == "true") hash_group.seed_parity = true;
            } else {
                error(el.key.lineno, "invalid hash group descriptor");
            }
        }
    } else {
        tbl = &value.vec;
    }
    if (tbl) {
        for (auto &v : *tbl) {
            if (!CHECKTYPE(v, tINT)) continue;
            if (v.i < 0 || v.i >= Target::EXACT_HASH_TABLES()) {
                error(v.lineno, "invalid hash group descriptor");
            } else {
                hash_group.tables |= 1U << v.i;
            }
        }
    }
}

void InputXbar::parse_hash_table(Table *t, HashTable ht, const value_t &value) {
    if (!CHECKTYPE(value, tMAP)) return;
    for (auto &c : value.map) {
        if (c.key.type == tINT) {
            setup_hash(hash_tables[ht], ht, t->gress, t->stage->stageno, c.value, c.key.lineno,
                       c.key.i, c.key.i);
        } else if (c.key.type == tRANGE) {
            setup_hash(hash_tables[ht], ht, t->gress, t->stage->stageno, c.value, c.key.lineno,
                       c.key.lo, c.key.hi);
        } else if (CHECKTYPEM(c.key, tCMD, "hash column decriptor")) {
            if (c.key.vec.size != 2 || c.key[0] != "valid" || c.key[1].type != tINT ||
                options.target != TOFINO) {
                error(c.key.lineno, "Invalid hash column descriptor");
                continue;
            }
            int col = c.key[1].i;
            if (col < 0 || col >= 52) {
                error(c.key.lineno, "Hash column out of range");
                continue;
            }
            if (!CHECKTYPE(c.value, tINT)) continue;
            if (hash_tables[ht][col].valid)
                error(c.key.lineno, "Hash table %d column %d valid duplicated", ht.index, col);
            else if (c.value.i >= 0x10000)
                error(c.value.lineno, "Hash valid value out of range");
            else
                hash_tables[ht][col].valid = c.value.i;
        }
    }
}

void InputXbar::setup_hash(std::map<int, HashCol> &hash_table, HashTable ht, gress_t gress,
                           int stage, value_t &what, int lineno, int lo, int hi) {
    if (lo < 0 || lo >= hash_num_columns(ht) || hi < 0 || hi >= hash_num_columns(ht)) {
        error(lineno, "Hash column out of range");
        return;
    }
    if (lo == hi) {
        if (what.type == tINT || what.type == tBIGINT) {
            hash_table[lo].data = get_bitvec(what, 64, "Hash column value out of range");
            return;
        } else if ((what.type == tSTR) && (what == "parity")) {
            options.disable_gfm_parity = false;
            hash_table_parity[ht] = lo;
            return;
        }
    } else if (what.type == tINT && what.i == 0) {
        for (int i = lo; i <= hi; ++i) {
            hash_table[i].data.setraw(what.i);
        }
        return;
    }
    HashExpr *fn = HashExpr::create(gress, stage, what);  // TODO Set the crcSize.
    if (!fn) return;
    fn->build_algorithm();
    int width = fn->width();
    if (width && width != abs(hi - lo) + 1)
        error(what.lineno, "hash expression width mismatch (%d != %d)", width, abs(hi - lo) + 1);
    int bit = 0;
    int errlo = -1;
    bool fn_assigned = false;
    for (int col : Range(lo, hi)) {
        if (hash_table[col].data || hash_table[col].fn) {
            if (errlo < 0) errlo = col;
        } else {
            if (errlo >= 0) {
                if (errlo == col - 1) {
                    error(lineno, "%s column %d duplicated", ht.toString().c_str(), errlo);
                } else {
                    error(lineno, "%s column %d..%d duplicated", ht.toString().c_str(), errlo,
                          col - 1);
                }
                errlo = -1;
            }
            hash_table[col].lineno = what.lineno;
            hash_table[col].fn = fn;
            hash_table[col].bit = bit++;
            fn_assigned = true;
        }
    }

    if (!fn_assigned) delete fn;

    if (errlo >= 0) {
        error(lineno, "%s column %d..%d duplicated", ht.toString().c_str(), errlo, hi);
    }
}

void InputXbar::input(Table *t, bool tern, const VECTOR(pair_t) & data) {
    for (auto &kv : data) {
        if ((kv.key.type == tSTR) && (kv.key == "random_seed")) {
            random_seed = kv.value.i;
            continue;
        }
        if (kv.key.type == tCMD && kv.key.vec.size == 2 && kv.key[1] == "unit" &&
            parse_unit(t, kv)) {
            continue;
        }
        if (auto grp = group_name(tern, kv.key)) {
            if (grp.index >= group_max_index(grp.type)) {
                error(kv.key.lineno, "invalid group descriptor");
                continue;
            }
            parse_group(t, grp, kv.value);
        } else if (kv.key.type == tCMD && kv.key[0] == "hash") {
            if (!CHECKTYPE(kv.key.vec.back(), tINT)) continue;
            int index = kv.key.vec.back().i;
            if (kv.key[1] == "group") {
                if (index >= Target::EXACT_HASH_GROUPS()) {
                    error(kv.key.lineno, "invalid hash group descriptor");
                    continue;
                }
                if (hash_groups[index].lineno >= 0) {
                    // FIXME -- should be an error? but the compiler generates it this way
                    warning(kv.key.lineno, "duplicate hash group %d, will merge with", index);
                    warning(hash_groups[index].lineno, "previous definition here");
                }
                hash_groups[index].lineno = kv.key.lineno;
                parse_hash_group(hash_groups[index], kv.value);
            } else if (index >= Target::EXACT_HASH_TABLES()) {
                error(kv.key.lineno, "invalid hash descriptor");
            } else {
                parse_hash_table(t, HashTable(HashTable::EXACT, index), kv.value);
            }
        } else if (kv.key.type == tCMD && kv.key[1] == "hash" && parse_hash(t, kv)) {
            continue;
        } else {
            error(kv.key.lineno, "expecting a group or hash descriptor");
        }
    }
}

std::unique_ptr<InputXbar> InputXbar::create(Table *table, const value_t *key) {
    if (key && key->type != tSTR)
        error(key->lineno, "%s does not support dynamic key mux", Target::name());
    return std::unique_ptr<InputXbar>(new InputXbar(table, key ? key->lineno : -1));
}

std::unique_ptr<InputXbar> InputXbar::create(Table *table, bool tern, const value_t &key,
                                             const VECTOR(pair_t) & data) {
    auto rv = create(table, &key);
    rv->input(table, tern, data);
    return rv;
}

unsigned InputXbar::tcam_width() {
    unsigned words = 0, bytes = 0;
    for (auto &group : groups) {
        if (group.first.type != Group::TERNARY) {
            if (group.first.type == Group::BYTE) ++bytes;
            continue;
        }
        unsigned in_word = 0, in_byte = 0;
        for (auto &input : group.second) {
            if (input.lo < 40) in_word = 1;
            if (input.lo >= 40 || input.hi >= 40) in_byte = 1;
        }
        words += in_word;
        bytes += in_byte;
    }
    if (bytes * 2 > words) error(lineno, "Too many byte groups in tcam input xbar");
    return words;
}

int InputXbar::tcam_byte_group(int idx) {
    for (auto &group : groups) {
        if (group.first.type != Group::TERNARY) continue;
        for (auto &input : group.second)
            if (input.lo >= 40 || input.hi >= 40) {
                if (--idx < 0) return group.first.index / 2;
                break;
            }
    }
    return -1;
}

int InputXbar::tcam_word_group(int idx) {
    for (auto &group : groups) {
        if (group.first.type != Group::TERNARY) continue;
        for (auto &input : group.second)
            if (input.lo < 40) {
                if (--idx < 0) return group.first.index;
                break;
            }
    }
    return -1;
}

const std::map<int, HashCol> &InputXbar::get_hash_table(HashTable id) {
    for (auto &ht : hash_tables)
        if (ht.first == id) return ht.second;
    warning(lineno, "%s does not exist in table %s", id.toString().c_str(), table->name());
    static const std::map<int, HashCol> empty_hash_table = {};
    return empty_hash_table;
}

bool InputXbar::conflict(const std::vector<Input> &a, const std::vector<Input> &b) {
    for (auto &i1 : a) {
        if (i1.lo < 0) continue;
        for (auto &i2 : b) {
            if (i2.lo < 0) continue;
            if (i2.lo > i1.hi || i1.lo > i2.hi) continue;
            if (i1.what->reg != i2.what->reg) return true;
            if (i1.lo - i1.what->lo != i2.lo - i2.what->lo) return true;
        }
    }
    return false;
}

bool InputXbar::conflict(const std::map<int, HashCol> &a, const std::map<int, HashCol> &b,
                         int *col) {
    for (auto &acol : a) {
        if (auto bcol = ::getref(b, acol.first)) {
            if (acol.second.data != bcol->data || acol.second.valid != bcol->valid) {
                if (col) *col = acol.first;
                return true;
            }
        }
    }
    return false;
}

bool InputXbar::conflict(const HashGrp &a, const HashGrp &b) {
    if (a.tables != b.tables) return true;
    if (a.seed && b.seed && a.seed != b.seed) return true;
    return false;
}

uint64_t InputXbar::hash_columns_used(HashTable hash) {
    uint64_t rv = 0;
    if (hash_tables.count(hash))
        for (auto &col : hash_tables[hash]) rv |= UINT64_C(1) << col.first;
    return rv;
}

/* FIXME -- this is questionable, but the compiler produces hash groups that conflict
 * FIXME -- so we try to tag ones that may be ok as merely warnings */
bool InputXbar::can_merge(HashGrp &a, HashGrp &b) {
    unsigned both = a.tables & b.tables;
    uint64_t both_cols = 0, a_cols = 0, b_cols = 0;
    for (unsigned i = 0; i < 16; i++) {
        unsigned mask = 1U << i;
        if (!((a.tables | b.tables) & mask)) continue;
        for (InputXbar *other : table->stage->hash_table_use[i]) {
            if (both & mask) both_cols |= other->hash_columns_used(i);
            if (a.tables & mask) a_cols |= other->hash_columns_used(i);
            if (b.tables & mask) b_cols |= other->hash_columns_used(i);
            for (auto htp : hash_table_parity) {
                if (other->hash_table_parity.count(htp.first) &&
                    other->hash_table_parity.at(htp.first) != htp.second)
                    return false;
            }
        }
    }
    a_cols &= ~both_cols;
    b_cols &= ~both_cols;
    if (a_cols & b_cols) return false;
    if ((a_cols & b.seed & ~a.seed) || (b_cols & a.seed & ~b.seed)) return false;
    if (a.tables && b.tables) {
        a.tables |= b.tables;
        b.tables |= a.tables;
    }
    if (a.seed && b.seed) {
        a.seed |= b.seed;
        b.seed |= a.seed;
    }
    return true;
}

static int tcam_swizzle_offset[4][4] = {
    {0, +1, -2, -1},
    {+3, 0, +1, -2},
    {+2, -1, 0, -3},
    {+1, +2, -1, 0},
};

// FIXME -- when swizlling 16 bit PHVs, there are 2 places we could copy from, but
// FIXME -- we only consider the closest/easiest
static int tcam_swizzle_16[2][2]{{0, -1}, {+1, 0}};

int InputXbar::tcam_input_use(int out_byte, int phv_byte, int phv_size) {
    int rv = out_byte;
    BUG_CHECK(phv_byte >= 0 && phv_byte < phv_size / 8);
    switch (phv_size) {
        case 8:
            break;
        case 32:
            rv += tcam_swizzle_offset[out_byte & 3][phv_byte];
            break;
        case 16:
            rv += tcam_swizzle_16[out_byte & 1][phv_byte];
            break;
        default:
            BUG();
    }
    return rv;
}

void InputXbar::tcam_update_use(TcamUseCache &use) {
    if (use.ixbars_added.count(this)) return;
    use.ixbars_added.insert(this);
    for (auto &group : groups) {
        if (group.first.type == Group::EXACT) continue;
        for (auto &input : group.second) {
            if (input.lo < 0) continue;
            int group_base = (group.first.index * 11 + 1) / 2U;
            int half_byte = 5 + 11 * (group.first.index / 2U);
            if (group.first.type == Group::BYTE) {
                group_base = 5 + 11 * group.first.index;
                half_byte = -1;
            }
            int group_byte = input.lo / 8;
            for (int phv_byte = input.what->lo / 8; phv_byte <= input.what->hi / 8;
                 phv_byte++, group_byte++) {
                BUG_CHECK(group_byte <= 5);
                int out_byte = group_byte == 5 ? half_byte : group_base + group_byte;
                int in_byte = tcam_input_use(out_byte, phv_byte, input.what->reg.size);
                use.tcam_use.emplace(in_byte, std::pair<const Input &, int>(input, phv_byte));
            }
        }
    }
}

void InputXbar::check_input(InputXbar::Group group, Input &input, TcamUseCache &use) {
    if (group.type == Group::EXACT) {
        if (input.lo % input.what->reg.size != input.what->lo)
            error(input.what.lineno, "%s misaligned on input_xbar", input.what.name());
        return;
    }
    unsigned bit_align_mask = input.lo >= 40 ? 3 : 7;
    unsigned byte_align_mask = (input.what->reg.size - 1) >> 3;
    int group_base = (group.index * 11 + 1) / 2U;
    int half_byte = 5 + 11 * (group.index / 2U);
    if (group.type == Group::BYTE) {
        bit_align_mask = 3;
        group_base = 5 + 11 * group.index;
        half_byte = -1;
    }
    int group_byte = input.lo / 8;
    if ((input.lo ^ input.what->lo) & bit_align_mask) {
        error(input.what.lineno, "%s misaligned on input_xbar", input.what.name());
        return;
    }
    for (int phv_byte = input.what->lo / 8; phv_byte <= input.what->hi / 8;
         phv_byte++, group_byte++) {
        BUG_CHECK(group_byte <= 5);
        int out_byte = group_byte == 5 ? half_byte : group_base + group_byte;
        int in_byte = tcam_input_use(out_byte, phv_byte, input.what->reg.size);
        if (in_byte < 0 || in_byte >= TCAM_XBAR_INPUT_BYTES) {
            error(input.what.lineno, "%s misaligned on input_xbar", input.what.name());
            break;
        }
        auto *tbl = table->stage->tcam_ixbar_input[in_byte];
        if (tbl) {
            BUG_CHECK(tbl->input_xbar.size() == 1, "%s does not have one input xbar", tbl->name());
            tbl->input_xbar[0]->tcam_update_use(use);
        }
        if (use.tcam_use.count(in_byte)) {
            if (use.tcam_use.at(in_byte).first.what->reg != input.what->reg ||
                use.tcam_use.at(in_byte).second != phv_byte) {
                error(input.what.lineno, "Use of tcam ixbar for %s", input.what.name());
                error(use.tcam_use.at(in_byte).first.what.lineno, "...conflicts with %s",
                      use.tcam_use.at(in_byte).first.what.name());
                break;
            }
        } else {
            use.tcam_use.emplace(in_byte, std::pair<const Input &, int>(input, phv_byte));
            table->stage->tcam_ixbar_input[in_byte] = tbl;
        }
    }
}

bool InputXbar::copy_existing_hash(HashTable ht, std::pair<const int, HashCol> &col) {
    for (InputXbar *other : table->stage->hash_table_use[ht.index]) {
        if (other == this) continue;
        if (other->hash_tables.count(ht)) {
            auto &o = other->hash_tables.at(ht);
            if (o.count(col.first)) {
                auto ocol = o.at(col.first);
                if (ocol.fn && *ocol.fn == *col.second.fn) {
                    col.second.data = ocol.data;
                    return true;
                }
            }
        }
    }
    return false;
}

void InputXbar::gen_hash_column(std::pair<const int, HashCol> &col,
                                std::pair<const HashTable, std::map<int, HashCol>> &hash) {
    col.second.fn->gen_data(col.second.data, col.second.bit, this, hash.first);
}

void InputXbar::pass1() {
    TcamUseCache tcam_use;
    tcam_use.ixbars_added.insert(this);
    if (random_seed >= 0) srandom(random_seed);
    for (auto &group : groups) {
        for (auto &input : group.second) {
            if (!input.what.check()) continue;
            if (input.what->reg.ixbar_id() < 0)
                error(input.what.lineno, "%s not accessable in input xbar", input.what->reg.name);
            table->stage->match_use[table->gress][input.what->reg.uid] = 1;
            if (input.lo < 0 && group.first.type == Group::BYTE) input.lo = input.what->lo % 8U;
            if (input.lo >= 0) {
                if (input.hi >= 0) {
                    if (input.size() != input.what->size())
                        error(input.what.lineno, "Input xbar size doesn't match register size");
                } else {
                    input.hi = input.lo + input.what->size() - 1;
                }
                if (input.lo >= group_size(group.first.type))
                    error(input.what.lineno, "placing %s off the top of the input xbar",
                          input.what.name());
            }
            check_input(group.first, input, tcam_use);
        }
        auto &use = table->stage->ixbar_use;
        for (InputXbar *other : use[group.first]) {
            if (other->groups.count(group.first) &&
                conflict(other->groups.at(group.first), group.second)) {
                error(lineno, "Input xbar group %d conflict in stage %d", group.first.index,
                      table->stage->stageno);
                warning(other->lineno, "conflicting group definition here");
            }
        }
        use[group.first].push_back(this);
    }
    for (auto &hash : hash_tables) {
        bool ok = true;
        HashExpr *prev = 0;
        for (auto &col : hash.second) {
            if (col.second.fn && col.second.fn != prev)
                ok = (prev = col.second.fn)->check_ixbar(this, hash.first);
            if (ok && col.second.fn && !copy_existing_hash(hash.first, col)) {
                gen_hash_column(col, hash);
            }
        }
        bool add_to_use = true;
        for (InputXbar *other : table->stage->hash_table_use[hash.first.uid()]) {
            if (other == this) {
                add_to_use = false;
                continue;
            }
            int column;
            if (other->hash_tables.count(hash.first) &&
                conflict(other->hash_tables[hash.first], hash.second, &column)) {
                error(hash.second.at(column).lineno, "%s column %d conflict in stage %d",
                      hash.first.toString().c_str(), column, table->stage->stageno);
                error(other->hash_tables[hash.first].at(column).lineno,
                      "conflicting hash definition here");
            }
        }
        if (add_to_use) table->stage->hash_table_use[hash.first.uid()].push_back(this);
    }
    for (auto &group : hash_groups) {
        bool add_to_use = true;
        for (InputXbar *other : table->stage->hash_group_use[group.first]) {
            if (other == this) {
                add_to_use = false;
                break;
            }
            if (other->hash_groups.count(group.first) &&
                conflict(other->hash_groups[group.first], group.second)) {
                if (can_merge(other->hash_groups[group.first], group.second))
                    warning(group.second.lineno,
                            "Input xbar hash group %d mergeable conflict "
                            "in stage %d",
                            group.first, table->stage->stageno);
                else
                    error(group.second.lineno, "Input xbar hash group %d conflict in stage %d",
                          group.first, table->stage->stageno);
                warning(other->hash_groups[group.first].lineno,
                        "conflicting hash group definition here");
            }
        }
        if (add_to_use) table->stage->hash_group_use[group.first].push_back(this);
    }
}

void InputXbar::add_use(unsigned &byte_use, std::vector<Input> &inputs) {
    for (auto &i : inputs) {
        if (i.lo < 0) continue;
        for (int byte = i.lo / 8; byte <= i.hi / 8; byte++) byte_use |= 1 << byte;
        ;
    }
}

const InputXbar::Input *InputXbar::GroupSet::find(Phv::Slice sl) const {
    for (InputXbar *i : use)
        if (auto rv = i->find(sl, group)) return rv;
    return 0;
}

std::vector<const InputXbar::Input *> InputXbar::GroupSet::find_all(Phv::Slice sl) const {
    std::vector<const Input *> rv;
    for (const InputXbar *i : use) {
        auto vec = i->find_all(sl, group);
        rv.insert(rv.end(), vec.begin(), vec.end());
    }
    return rv;
}

void InputXbar::GroupSet::dbprint(std::ostream &out) const {
    std::map<unsigned, const InputXbar::Input *> byte_use;
    for (const InputXbar *ixbar : use) {
        if (ixbar->groups.count(group)) {
            for (auto &i : ixbar->groups.at(group)) {
                if (i.lo < 0) continue;
                for (int byte = i.lo / 8; byte <= i.hi / 8; byte++) byte_use[byte] = &i;
            }
        }
    }
    const InputXbar::Input *prev = 0;
    for (auto &in : byte_use) {
        if (prev == in.second) continue;
        if (prev) out << ", ";
        prev = in.second;
        out << prev->what << ':' << prev->lo << ".." << prev->hi;
    }
}

void InputXbar::pass2() {
    auto &use = table->stage->ixbar_use;
    for (auto &group : groups) {
        unsigned bytes_in_use = 0;
        for (auto &input : group.second) {
            if (input.lo >= 0) continue;
            if (auto *at = GroupSet(use, group.first).find(*input.what)) {
                input.lo = at->lo;
                input.hi = at->hi;
                LOG1(input.what << " found in bytes " << at->lo / 8 << ".." << at->hi / 8 << " of "
                                << group.first << " in stage " << table->stage->stageno);
                continue;
            }
            if (bytes_in_use == 0)
                for (InputXbar *other : table->stage->ixbar_use[group.first])
                    if (other->groups.count(group.first))
                        add_use(bytes_in_use, other->groups.at(group.first));
            int need = input.what->hi / 8U - input.what->lo / 8U + 1;
            unsigned mask = (1U << need) - 1;
            int max = (group_size(group.first.type) + 7) / 8 - need;
            for (int i = 0; i <= max; i++, mask <<= 1)
                if (!(bytes_in_use & mask)) {
                    input.lo = i * 8 + input.what->lo % 8U;
                    input.hi = (i + need - 1) * 8 + input.what->hi % 8U;
                    bytes_in_use |= mask;
                    LOG1("Putting " << input.what << " in bytes " << i << ".." << i + need - 1
                                    << " of " << group.first << " in stage "
                                    << table->stage->stageno);
                    break;
                }
            if (input.lo < 0) {
                error(input.what.lineno, "No space in input xbar %s group %d for %s",
                      group_type(group.first.type), group.first.index, input.what.name());
                LOG1("Failed to put " << input.what << " into " << group.first << " in stage "
                                      << table->stage->stageno);
                LOG1("  inuse: " << GroupSet(use, group.first));
            }
        }
    }
    for (auto &hash : hash_tables) {
        for (auto &col : hash.second) {
            if (!col.second.data && col.second.fn) {
                gen_hash_column(col, hash);
            }
        }
    }
}

template <class REGS>
void InputXbar::write_regs(REGS &regs) {
    LOG1("### Input xbar " << table->name() << " write_regs " << table->loc());
    auto &xbar = regs.dp.xbar_hash.xbar;
    auto gress = timing_thread(table->gress);
    for (auto &group : groups) {
        if (group.second.empty()) continue;
        LOG1("  # Input xbar group " << group.first);
        unsigned group_base = 0;
        unsigned half_byte = 0;
        unsigned bytes_used = 0;
        switch (group.first.type) {
            case Group::EXACT:
                group_base = group.first.index * 16U;
                break;
            case Group::TERNARY:
                group_base = 128 + (group.first.index * 11 + 1) / 2U;
                half_byte = 133 + 11 * (group.first.index / 2U);
                xbar.mau_match_input_xbar_ternary_match_enable[gress] |=
                    1 << (group.first.index) / 2U;
                break;
            case Group::BYTE:
                group_base = 133 + 11 * group.first.index;
                xbar.mau_match_input_xbar_ternary_match_enable[gress] |= 1 << (group.first.index);
                break;
            default:
                BUG();
        }
        for (auto &input : group.second) {
            BUG_CHECK(input.lo >= 0);
            unsigned word_group = 0, word_index = 0, swizzle_mask = 0;
            bool hi_enable = false;
            switch (input.what->reg.size) {
                case 8:
                    word_group = (input.what->reg.ixbar_id() - 64) / 8U;
                    word_index = (input.what->reg.ixbar_id() - 64) % 8U + (word_group & 4) * 2;
                    swizzle_mask = 0;
                    break;
                case 16:
                    word_group = (input.what->reg.ixbar_id() - 128) / 12U;
                    word_index =
                        (input.what->reg.ixbar_id() - 128) % 12U + 16 + (word_group & 4) * 3;
                    swizzle_mask = 1;
                    break;
                case 32:
                    word_group = input.what->reg.ixbar_id() / 8U;
                    word_index = input.what->reg.ixbar_id() % 8U;
                    hi_enable = word_group & 4;
                    swizzle_mask = 3;
                    break;
                default:
                    BUG();
            }
            word_group &= 3;
            unsigned phv_byte = input.what->lo / 8U;
            unsigned phv_size = input.what->reg.size / 8U;
            for (unsigned byte = input.lo / 8U; byte <= input.hi / 8U; byte++, phv_byte++) {
                bytes_used |= 1U << byte;
                unsigned i = group_base + byte;
                if (half_byte && byte == 5) i = half_byte;
                if (i % phv_size != phv_byte) {
                    if (group.first.type != Group::EXACT) {
                        int off;
                        if (phv_size == 2)
                            off = (i & 2) ? -1 : 1;
                        else
                            off = tcam_swizzle_offset[i & 3][phv_byte];
                        xbar.tswizzle.tcam_byte_swizzle_ctl[(i & 0x7f) / 4U].set_subfield(
                            off & 3U, 2 * (i % 4U), 2);
                        i += off;
                    } else {
                        error(input.what.lineno, "misaligned phv access on input_xbar");
                    }
                }
                if (input.what->reg.ixbar_id() < 64) {
                    BUG_CHECK(input.what->reg.size == 32);
                    xbar.match_input_xbar_32b_ctl[word_group][i].match_input_xbar_32b_ctl_address =
                        word_index;
                    if (hi_enable)
                        xbar.match_input_xbar_32b_ctl[word_group][i]
                            .match_input_xbar_32b_ctl_hi_enable = 1;
                    else
                        xbar.match_input_xbar_32b_ctl[word_group][i]
                            .match_input_xbar_32b_ctl_lo_enable = 1;
                } else {
                    xbar.match_input_xbar_816b_ctl[word_group][i]
                        .match_input_xbar_816b_ctl_address = word_index;
                    xbar.match_input_xbar_816b_ctl[word_group][i].match_input_xbar_816b_ctl_enable =
                        1;
                }
                if ((i ^ phv_byte) & swizzle_mask)
                    error(input.what.lineno, "Need tcam swizzle for %s",
                          input.what.toString().c_str());
            }
            auto &power_ctl = regs.dp.match_input_xbar_din_power_ctl;
            // we do in fact want mau_id, not ixbar_id here!
            set_power_ctl_reg(power_ctl, input.what->reg.mau_id());
        }
        if (group.first.type == Group::EXACT) {
            unsigned enable = 0;
            if (bytes_used & 0xff) enable |= 1;
            if (bytes_used & 0xff00) enable |= 2;
            enable <<= group.first.index * 2;
            regs.dp.mau_match_input_xbar_exact_match_enable[gress].rewrite();
            regs.dp.mau_match_input_xbar_exact_match_enable[gress] |= enable;
        }
    }
    auto &hash = regs.dp.xbar_hash.hash;
    for (auto &ht : hash_tables) {
        if (ht.second.empty()) continue;
        LOG1("  # Input xbar hash table " << ht.first);
        write_galois_matrix(regs, ht.first, ht.second);
    }
    for (auto &hg : hash_groups) {
        LOG1("  # Input xbar hash group " << hg.first);
        int grp = hg.first;
        if (hg.second.tables) {
            hash.parity_group_mask[grp][0] = hg.second.tables & 0xff;
            hash.parity_group_mask[grp][1] = (hg.second.tables >> 8) & 0xff;
            regs.dp.mau_match_input_xbar_exact_match_enable[gress].rewrite();
            regs.dp.mau_match_input_xbar_exact_match_enable[gress] |= hg.second.tables;
        }
        if (hg.second.seed) {
            for (int bit = 0; bit < 52; ++bit) {
                if ((hg.second.seed >> bit) & 1) {
                    hash.hash_seed[bit] |= UINT64_C(1) << grp;
                }
            }
        }
        if (gress == INGRESS)
            regs.dp.hashout_ctl.hash_group_ingress_enable |= 1 << grp;
        else
            regs.dp.hashout_ctl.hash_group_egress_enable |= 1 << grp;
        // Set hash parity check if enabled. The hash parity column data is set
        // in pass2
        if (hg.second.tables && !options.disable_gfm_parity) {
            // Enable check if parity bit is set on all tables in hash group
            int parity_bit = -1;
            for (int index : bitvec(hg.second.tables)) {
                HashTable ht(HashTable::EXACT, index);
                if (!hash_table_parity.count(ht)) {
                    continue;
                } else {
                    if (parity_bit == -1) {
                        parity_bit = hash_table_parity[ht];
                    } else {
                        if (hash_table_parity[ht] != parity_bit)
                            error(hg.second.lineno,
                                  "Hash tables within a hash group "
                                  "do not have the same parity bit - %d",
                                  grp);
                    }
                }
            }
            if (parity_bit >= 0) {
                regs.dp.hashout_ctl.hash_parity_check_enable |= 1 << grp;
                // Hash seed must have even parity for the group. Loop through
                // all bits set on the group for hash seed to determine if the
                // parity bit must be set
                int seed_parity = 0;
                for (int bit = 0; bit < 52; ++bit) {
                    auto seed_bit = (hash.hash_seed[bit] >> grp) & 0x1;
                    seed_parity ^= seed_bit;
                }
                if (seed_parity) {  // flip parity bit setup on group for even parity
                    if (!hg.second.seed_parity)
                        warning(hg.second.lineno,
                                "hash group %d has parity enabled, but setting seed_parity"
                                " is disabled, changing seed to even parity",
                                grp);
                    hash.hash_seed[parity_bit] ^= (1 << grp);
                }
            }
        }
    }
}

template void InputXbar::write_regs(Target::Tofino::mau_regs &);
#if HAVE_JBAY
template void InputXbar::write_regs(Target::JBay::mau_regs &);
#endif /* HAVE_JBAY */

template <class REGS>
void InputXbar::write_xmu_regs(REGS &regs) {
    BUG("no XMU regs for %s", Target::name());
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void InputXbar::write_xmu_regs, mau_regs &)

const InputXbar::Input *InputXbar::find(Phv::Slice sl, Group grp, Group *found) const {
    const InputXbar::Input *rv = nullptr;
    if (groups.count(grp)) {
        for (auto &in : groups.at(grp)) {
            if (in.lo < 0) continue;
            if (in.what->reg.uid != sl.reg.uid) continue;
            if (in.what->lo / 8U > sl.lo / 8U) continue;
            if (in.what->hi / 8U < sl.hi / 8U) continue;
            rv = &in;
            if (in.what->lo > sl.lo) continue;
            if (in.what->hi < sl.hi) continue;
            if (found) *found = grp;
            return &in;
        }
    } else if (grp.index == -1) {
        for (auto &g : Keys(groups)) {
            if (g.type != grp.type) continue;
            if ((rv = find(sl, g))) {
                if (found) *found = g;
                return rv;
            }
        }
    }
    return rv;
}

int InputXbar::find_offset(const MatchSource *, Group, int) const {
    BUG("find_offset should not be needed on %s", Target::name());
}

std::vector<const InputXbar::Input *> InputXbar::find_all(Phv::Slice sl, Group grp) const {
    std::vector<const InputXbar::Input *> rv;
    if (groups.count(grp)) {
        for (auto &in : groups.at(grp)) {
            if (in.lo < 0) continue;
            if (in.what->reg.uid != sl.reg.uid) continue;
            if (in.what->lo / 8U > sl.lo / 8U) continue;
            if (in.what->hi / 8U < sl.hi / 8U) continue;
            rv.push_back(&in);
        }
    } else if (grp.index == -1) {
        for (auto &g : Keys(groups)) {
            if (g.type != grp.type) continue;
            auto tmp = find_all(sl, g);
            rv.insert(rv.end(), tmp.begin(), tmp.end());
        }
    }
    return rv;
}

/**
 * InputXbar::find_hash_inputs: find all of the ixbar inputs that feed a particular phv slice
 * to a hash table
 * @param sl            the PHV container slice we're interested in
 * @param hash_table    which hash table we want the input for (-1 for all hash tables)
 */
std::vector<const InputXbar::Input *> InputXbar::find_hash_inputs(Phv::Slice sl,
                                                                  HashTable ht) const {
    /* code for tofino1/2 -- all hash tables take input from exact ixbar groups, with
     * two hash tables per group (even in lower bits and odd in upper bits)
     */
    BUG_CHECK(ht.type == HashTable::EXACT, "not an exact hash table: %s", ht.toString().c_str());
    auto rv = find_all(sl, Group(Group::EXACT, ht.index >= 0 ? ht.index / 2 : -1));
    if (ht.index >= 0) {
        unsigned upper = ht.index % 2;
        for (auto it = rv.begin(); it != rv.end();) {
            unsigned bit = (*it)->lo + (sl.lo - (*it)->what->lo);
            if (bit / 64 != upper || (bit + sl.size() - 1) / 64 != upper)
                it = rv.erase(it);
            else
                ++it;
        }
    }
    return rv;
}

bitvec InputXbar::hash_group_bituse(int grp) const {
    bitvec rv;
    unsigned tables = 0;
    for (auto &g : hash_groups) {
        if (grp == -1 || static_cast<int>(g.first) == grp) {
            tables |= g.second.tables;
            rv |= g.second.seed;
        }
    }
    for (auto &tbl : hash_tables) {
        if (tbl.first.type != HashTable::EXACT) continue;
        if (!((tables >> tbl.first.index) & 1)) continue;
        // Skip parity bit if set on hash table
        auto hash_parity_bit = -1;
        if (hash_table_parity.count(tbl.first)) {
            hash_parity_bit = hash_table_parity.at(tbl.first);
        }
        for (auto &col : tbl.second) {
            if (col.first == hash_parity_bit) continue;
            rv[col.first] = 1;
        }
    }
    return rv;
}

// Used by LPF/WRED meters to determine the bytemask input
bitvec InputXbar::bytemask() {
    bitvec bytemask;
    // Only one ixbar group allowed for a meter input
    if (match_group() == -1) return bytemask;
    for (auto group : groups) {
        auto &inputs = group.second;
        for (auto &input : inputs) {
            int byte_lo = input.lo / 8;
            int byte_hi = input.hi / 8;
            int byte_size = byte_hi - byte_lo + 1;
            bytemask.setrange(byte_lo, byte_size);
        }
    }
    return bytemask;
}

std::vector<const HashCol *> InputXbar::hash_column(int col, int grp) const {
    unsigned tables = 0;
    std::vector<const HashCol *> rv;
    for (auto &g : hash_groups)
        if (grp == -1 || static_cast<int>(g.first) == grp) tables |= g.second.tables;
    for (auto &tbl : hash_tables) {
        if (tbl.first.type != HashTable::EXACT) continue;
        if (!((tables >> tbl.first.index) & 1)) continue;
        if (const HashCol *c = getref(tbl.second, col)) rv.push_back(c);
    }
    return rv;
}

bool InputXbar::log_hashes(std::ofstream &out) const {
    bool logged = false;
    for (auto &ht : hash_tables) {
        // ht.first is HashTable
        // ht.second is std::map<int, HashCol>, key is col
        if (ht.second.empty()) continue;
        out << std::endl << ht.first << std::endl;
        logged = true;
        for (auto &col : ht.second) {
            // col.first is hash result bit
            // col.second is bits XOR'd in
            out << "result[" << col.first << "] = ";
            out << get_seed_bit(ht.first.index / 2, col.first);
            for (const auto &bit : col.second.data) {
                if (auto ref = get_hashtable_bit(ht.first, bit)) {
                    std::string field_name = ref.name();
                    auto field_bit = remove_name_tail_range(field_name) + ref.lobit();
                    out << " ^ " << field_name << "[" << field_bit << "]";
                }
            }
            out << std::endl;
        }
    }
    return logged;
}

std::string InputXbar::HashTable::toString() const {
    std::stringstream tmp;
    tmp << *this;
    return tmp.str();
}

unsigned InputXbar::HashTable::uid() const {
    switch (type) {
        case EXACT:
            BUG_CHECK(index < Target::EXACT_HASH_TABLES(), "index too large: %s",
                      toString().c_str());
            return index;
        case XCMP:
            return index + Target::EXACT_HASH_TABLES();
        default:
            BUG("invalid type: %s", toString().c_str());
    }
}
