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

#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tables.h"
#include "hashexpr.h"
#include "input_xbar.h"
#include "instruction.h"
#include "lib/algorithm.h"
#include "lib/hex.h"
#include "misc.h"

// template specialization declarations
#include "jbay/gateway.h"
#include "tofino/gateway.h"

static struct {
    unsigned units, bits, half_shift, mask, half_mask;
} range_match_info[] = {{0, 0, 0, 0, 0}, {6, 4, 2, 0xf, 0x3}, {3, 8, 8, 0xffff, 0xff}};

// Dummy value used to start gateway handles. For future use by driver,
// Incremented from inside the gateway table
static uint gateway_handle = 0x70000000;

GatewayTable::Match::Match(value_t *v, value_t &data, range_match_t range_match) {
    if (range_match) {
        for (unsigned i = 0; i < range_match_info[range_match].units; i++)
            range[i] = range_match_info[range_match].mask;
    }
    if (v) {
        lineno = v->lineno;
        if (v->type == tVEC) {
            int last = v->vec.size - 1;
            if (last > static_cast<int>(range_match_info[range_match].units))
                error(lineno, "Too many set values for range match");
            for (int i = 0; i < last; i++)
                if (CHECKTYPE((*v)[last - i - 1], tINT)) {
                    if ((unsigned)(*v)[last - i - 1].i > range_match_info[range_match].mask)
                        error(lineno, "range match set too large");
                    range[i] = (*v)[last - i - 1].i;
                }
            v = &(*v)[last];
        }
        if (v->type == tINT) {
            val.word1 = bitvec(v->i);
            val.word0.setrange(0, 64);
            val.word0 -= val.word1;
        } else if (v->type == tBIGINT) {
            val.word1.setraw(v->bigi.data, v->bigi.size);
            val.word0.setrange(0, v->bigi.size * 64);
            val.word0 -= val.word1;
        } else if (v->type == tMATCH) {
            val = v->m;
        } else if (v->type == tBIGMATCH) {
            val = v->bigm;
        }
    }
    if (data == "run_table") {
        run_table = true;
    } else if (data.type == tSTR || data.type == tVEC) {
        next = data;
    } else if (data.type == tMAP) {
        for (auto &kv : MapIterChecked(data.map)) {
            if (kv.key == "next") {
                next = kv.value;
            } else if (kv.key == "run_table") {
                if (kv.value == "true")
                    run_table = true;
                else if (kv.value == "false")
                    run_table = false;
                else
                    error(kv.value.lineno, "Syntax error, expecting boolean");
            } else if (kv.key == "action") {
                if (CHECKTYPE(kv.value, tSTR)) action = kv.value.s;
            } else {
                error(kv.key.lineno, "Syntax error, expecting gateway action description");
            }
        }
        if (run_table && next.set())
            error(data.lineno, "Can't run table and override next in the same gateway row");
    } else {
        error(data.lineno, "Syntax error, expecting gateway action description");
    }
}

void GatewayTable::setup(VECTOR(pair_t) & data) {
    setup_logical_id();
    if (auto *v = get(data, "range")) {
        if (CHECKTYPE(*v, tINT)) {
            if (v->i == 2) range_match = DC_2BIT;
            if (v->i == 4)
                range_match = DC_4BIT;
            else
                error(v->lineno, "Unknown range match size %" PRId64 " bits", v->i);
        }
    }
    for (auto &kv : MapIterChecked(data, true)) {
        if (kv.key == "name") {
            if (CHECKTYPE(kv.value, tSTR)) gateway_name = kv.value.s;
        } else if (kv.key == "row") {
            if (!CHECKTYPE(kv.value, tINT)) continue;
            if (kv.value.i < 0 || kv.value.i > Target::GATEWAY_ROWS())
                error(kv.value.lineno, "row %" PRId64 " out of range", kv.value.i);
            if (layout.empty()) layout.resize(1);
            layout[0].row = kv.value.i;
            layout[0].lineno = kv.value.lineno;
        } else if (kv.key == "bus") {
            if (!CHECKTYPE(kv.value, tINT)) continue;
            if (kv.value.i < 0 || kv.value.i > 1)
                error(kv.value.lineno, "bus %" PRId64 " out of range", kv.value.i);
            if (layout.empty()) layout.resize(1);
            layout[0].bus[Layout::SEARCH_BUS] = kv.value.i;
            if (layout[0].lineno < 0) layout[0].lineno = kv.value.lineno;
        } else if (kv.key == "payload_row") {
            if (!CHECKTYPE(kv.value, tINT)) continue;
            if (kv.value.i < 0 || kv.value.i > 7)
                error(kv.value.lineno, "row %" PRId64 " out of range", kv.value.i);
            if (layout.size() < 2) layout.resize(2);
            layout[1].row = kv.value.i;
            layout[1].lineno = kv.value.lineno;
        } else if (kv.key == "payload_bus") {
            if (!CHECKTYPE(kv.value, tINT)) continue;
            if (kv.value.i < 0 || kv.value.i > 3)
                error(kv.value.lineno, "bus %" PRId64 " out of range", kv.value.i);
            if (layout.size() < 2) layout.resize(2);
            layout[1].bus[Layout::RESULT_BUS] = kv.value.i;
            if (layout[1].lineno < 0) layout[1].lineno = kv.value.lineno;
        } else if (kv.key == "payload_unit") {
            if (!CHECKTYPE(kv.value, tINT)) continue;
            if (kv.value.i < 0 || kv.value.i > 1)
                error(kv.value.lineno, "payload unit %" PRId64 " out of range", kv.value.i);
            payload_unit = kv.value.i;
        } else if (kv.key == "gateway_unit" || kv.key == "unit") {
            if (!CHECKTYPE(kv.value, tINT)) continue;
            if (kv.value.i < 0 || kv.value.i > 1)
                error(kv.value.lineno, "gateway unit %" PRId64 " out of range", kv.value.i);
            gw_unit = kv.value.i;
        } else if (kv.key == "input_xbar") {
            if (CHECKTYPE(kv.value, tMAP))
                input_xbar.emplace_back(InputXbar::create(this, false, kv.key, kv.value.map));
        } else if (kv.key == "format") {
            if (CHECKTYPEPM(kv.value, tMAP, kv.value.map.size > 0, "non-empty map"))
                format.reset(new Format(this, kv.value.map));
        } else if (kv.key == "always_run") {
            if ((always_run = get_bool(kv.value)) && !Target::SUPPORT_ALWAYS_RUN())
                error(kv.key.lineno, "always_run not supported on %s", Target::name());
        } else if (kv.key == "miss") {
            miss = Match(0, kv.value, range_match);
        } else if (kv.key == "condition") {
            if (CHECKTYPE(kv.value, tMAP)) {
                for (auto &v : kv.value.map) {
                    if (v.key == "expression" && CHECKTYPE(v.value, tSTR))
                        gateway_cond = v.value.s;
                    else if (v.key == "true")
                        cond_true = Match(0, v.value, range_match);
                    else if (v.key == "false")
                        cond_false = Match(0, v.value, range_match);
                }
            }
        } else if (kv.key == "payload") {
            if (CHECKTYPE2(kv.value, tINT, tBIGINT)) payload = get_int64(kv.value);
            /* FIXME -- should also be able to specify payload as <action name>(<args>) */
            have_payload = kv.key.lineno;
        } else if (kv.key == "payload_map") {
            if (kv.value.type == tVEC) {
                if (kv.value.vec.size > Target::GATEWAY_PAYLOAD_GROUPS())
                    error(kv.value.lineno, "payload_map too large (limit %d)",
                          Target::GATEWAY_PAYLOAD_GROUPS());
                for (auto &v : kv.value.vec) {
                    if (v == "_")
                        payload_map.push_back(-1);
                    else if (CHECKTYPE(v, tINT))
                        payload_map.push_back(v.i);
                }
            }
        } else if (kv.key == "match_address") {
            if (CHECKTYPE(kv.value, tINT)) match_address = kv.value.i;
        } else if (kv.key == "match") {
            if (kv.value.type == tVEC) {
                for (auto &v : kv.value.vec) match.emplace_back(gress, stage->stageno, v);
            } else if (kv.value.type == tMAP) {
                for (auto &v : kv.value.map) {
                    if (CHECKTYPE(v.key, tINT)) {
                        if (v.value.type == tCMD && v.value.vec.size == 2 &&
                            v.value.vec[0] == "$valid") {
                            match.emplace_back(v.key.i, gress, stage->stageno, v.value.vec[1],
                                               true);
                        } else {
                            match.emplace_back(v.key.i, gress, stage->stageno, v.value);
                        }
                    }
                }
            } else {
                match.emplace_back(gress, stage->stageno, kv.value);
            }
        } else if (kv.key == "range") {
            /* done above, to be before match parsing */
        } else if (kv.key == "xor") {
            if (kv.value.type == tVEC) {
                for (auto &v : kv.value.vec) xor_match.emplace_back(gress, stage->stageno, v);
            } else if (kv.value.type == tMAP) {
                for (auto &v : kv.value.map)
                    if (CHECKTYPE(v.key, tINT))
                        xor_match.emplace_back(v.key.i, gress, stage->stageno, v.value);
            } else {
                xor_match.emplace_back(gress, stage->stageno, kv.value);
            }
        } else if (kv.key == "long_branch" && Target::LONG_BRANCH_TAGS() > 0) {
            if (options.disable_long_branch) error(kv.key.lineno, "long branches disabled");
            if (CHECKTYPE(kv.value, tMAP)) {
                for (auto &lb : kv.value.map) {
                    if (lb.key.type != tINT || lb.key.i < 0 ||
                        lb.key.i >= Target::LONG_BRANCH_TAGS())
                        error(lb.key.lineno, "Invalid long branch tag %s", value_desc(lb.key));
                    else if (long_branch.count(lb.key.i))
                        error(lb.key.lineno, "Duplicate long branch tag %" PRIi64, lb.key.i);
                    else
                        long_branch.emplace(lb.key.i, lb.value);
                }
            }
        } else if (kv.key == "context_json") {
            setup_context_json(kv.value);
        } else if (kv.key.type == tINT || kv.key.type == tBIGINT || kv.key.type == tMATCH ||
                   (kv.key.type == tVEC && range_match != NONE)) {
            table.emplace_back(&kv.key, kv.value, range_match);
        } else {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
}

bool GatewayTable::check_match_key(MatchKey &key, const std::vector<MatchKey> &vec, bool is_xor) {
    if (!key.val.check()) return false;
    if (key.val->reg.mau_id() < 0)
        error(key.val.lineno, "%s not accessable in mau", key.val->reg.name);
    if (key.offset >= 0) {
        for (auto &okey : vec) {
            if (&okey == &key) break;
            if (key.offset < okey.offset + static_cast<int>(okey.val->size()) &&
                okey.offset < key.offset + static_cast<int>(key.val->size()))
                error(key.val.lineno,
                      "Gateway %s key at offset %d overlaps previous "
                      "value at offset %d",
                      is_xor ? "xor" : "match", key.offset, okey.offset);
        }
    } else if (&key == &vec[0]) {
        key.offset = 0;
    } else {
        auto *prev = &key - 1;
        key.offset = prev->offset + prev->val->size();
    }
    return true;
}

void GatewayTable::verify_format() {
    if (format->log2size > 6)
        error(format->lineno, "Gateway payload format too large (max 64 bits)");
    format->log2size = 6;
    format->pass1(this);
    if (format->groups() > Target::GATEWAY_PAYLOAD_GROUPS())
        error(format->lineno, "Too many groups for gateway payload");
    if (payload_map.empty()) {
        if (format->groups() == 1) {
            payload_map.push_back(0);
        } else {
            payload_map = std::vector<int>(Target::GATEWAY_PAYLOAD_GROUPS(), -1);
            int i = Target::GATEWAY_PAYLOAD_GROUPS() - 2;
            int grp = 0;
            for (auto &row : table) {
                if (!row.run_table && i >= 0) {
                    if (grp >= format->groups() && format->groups() > 1) {
                        error(format->lineno, "Not enough groups in format for payload");
                        grp = 0;
                    }
                    payload_map[i--] = grp++;
                }
            }
            if (!miss.run_table) payload_map.back() = format->groups() - 1;
        }
    }
    for (auto pme : payload_map) {
        if (pme < -1 || pme >= int(format->groups()))
            error(format->lineno, "Invalid format group %d in payload_map", pme);
    }
    if (match_table) {
        if (match_table->table_type() == TERNARY) {
            if (format->groups() > 1)
                error(format->lineno,
                      "Can't have mulitple payload format groups when attached "
                      "to a ternary table");
        } else if (!match_table->format) {
            // ok
        } else if (auto *srm = match_table->to<SRamMatchTable>()) {
            int groups = std::min(format->groups(), match_table->format->groups());
            bool err = false;
            for (auto &field : *format) {
                if (auto match_field = match_table->format->field(field.first)) {
                    int match_group = -1;
                    for (auto gw_group : payload_map) {
                        ++match_group;
                        if (gw_group < 0) continue;
                        int em_group = match_group;
                        if (!srm->word_info.empty()) {
                            if (match_group < srm->word_info[0].size())
                                em_group = srm->word_info[0][match_group];
                            else
                                em_group = -1;
                        }
                        if (em_group < 0) continue;
                        if (field.second.by_group[gw_group]->bits !=
                            match_field->by_group[em_group]->bits) {
                            if (!err) {
                                error(format->lineno,
                                      "Gateway format inconsistent with table "
                                      "%s it is attached to",
                                      match_table->name());
                                error(match_table->format->lineno, "field %s inconsistent",
                                      field.first.c_str());
                                err = true;
                                break;
                            }
                        }
                    }
                } else {
                    if (!err)
                        error(format->lineno,
                              "Gateway format inconsistent with table %s it is "
                              "attached to",
                              match_table->name());
                    error(match_table->format->lineno, "No field %s in match table format",
                          field.first.c_str());
                    err = true;
                }
            }
        }
    } else if (layout.size() > 1) {
        if (!layout[1].bus.count(Layout::RESULT_BUS)) {
            error(layout[1].lineno, "No result bus for gateway payload");
        } else {
            int result_bus = layout[1].bus.at(Layout::RESULT_BUS);
            if (result_bus > 3)
                error(layout[1].lineno, "Invalid bus %d for gateway payload", result_bus);
            if ((result_bus & 2) && format->groups() > 1)
                error(format->lineno,
                      "Can't have mulitple payload format groups when using "
                      "ternary indirect bus");
        }
    }
}

void GatewayTable::pass1() {
    LOG1("### Gateway table " << name() << " pass1 " << loc());
    if (!match_table) {
        // needs to happen before Actions::pass1, but will have been called from the
        // match table if this gateway is attached to one.
        setup_map_indexing(this);
    }
    Table::pass1();
#if 0
    // redundant with (and supercedes) choose_logical_id in pass2.  That function is much
    // better, taking dependencies into account, so logical_id should not be allocated here
    alloc_id("logical", logical_id, stage->pass1_logical_id,
                 LOGICAL_TABLES_PER_STAGE, true, stage->logical_id_use);
#endif
    if (always_run && match_table)
        error(lineno, "always_run set on non-standalone gateway for %s", match_table->name());
    if (gw_unit >= 0) {
        if (auto *old = stage->gw_unit_use[layout[0].row][gw_unit])
            error(layout[0].lineno, "gateway %d.%d already in use by table %s", layout[0].row,
                  gw_unit, old->name());
        else
            stage->gw_unit_use[layout[0].row][gw_unit] = this;
    }
    for (auto &ixb : input_xbar) {
        ixb->pass1();
        if (Target::GATEWAY_SINGLE_XBAR_GROUP() && ixb->match_group() < 0)
            error(ixb->lineno, "Gateway match keys must be in a single ixbar group");
    }
    for (auto &k : match)
        if (!check_match_key(k, match, false)) break;
    for (auto &k : xor_match)
        if (!check_match_key(k, xor_match, true)) break;
    std::sort(match.begin(), match.end());
    std::sort(xor_match.begin(), xor_match.end());
    if (table.size() > 4) error(lineno, "Gateway can only have 4 match entries max");
    for (auto &line : table) check_next(line.next);
    check_next(miss.next);
    check_next(cond_false.next);
    check_next(cond_true.next);
    if (format) verify_format();

    if (error_count > 0) return;
    /* FIXME -- the rest of this function is a hack -- sometimes the compiler wants to
     * generate matches just covering the bits it names in the match and other times it wants
     * to create the whole tcam value.  Need to fix the asm syntax to be sensible and fix the
     * compiler's output.
     * Part of the issue is that in tofino1/2 we copy the word0/word1 bits directly to
     * the tcam, so we need to treat unspecified bits as don't care.  Another part is that
     * integer constants used as matches get padded with 0 out to a mulitple of 64 bits,
     * and those should also be don't care where they don't get matched.
     */
    bitvec ignore(0, Target::GATEWAY_MATCH_BITS());
    int shift = -1;
    int maxbit = 0;
    for (auto &r : match) {
        if (range_match && r.offset >= 32) {
            continue;
        }
        ignore.clrrange(r.offset, r.val->size());
        if (shift < 0 || shift > r.offset) shift = r.offset;
        if (maxbit < r.offset + r.val->size()) maxbit = r.offset + r.val->size();
    }
    if (shift < 0) shift = 0;
    LOG3("shift=" << shift << " ignore=0x" << ignore);
    for (auto &line : table) {
        bitvec matching = (line.val.word0 ^ line.val.word1) << shift;
        matching -= (line.val.word0 << shift) - bitvec(0, maxbit);  // ignore leading 0s
        if (matching & ignore)
            warning(line.lineno, "Trying to match on bits not in match of gateway");
        line.val.word0 = (line.val.word0 << shift) | ignore;
        line.val.word1 = (line.val.word1 << shift) | ignore;
    }
}

int GatewayTable::find_next_lut_entry(Table *tbl, const Match &match) {
    int rv = 0;
    for (auto &e : tbl->hit_next) {
        if (e == match.next) return rv;
        ++rv;
    }
    for (auto &e : tbl->extra_next_lut) {
        if (e == match.next) return rv;
        ++rv;
    }
    tbl->extra_next_lut.push_back(match.next);
    if (rv == Target::NEXT_TABLE_SUCCESSOR_TABLE_DEPTH())
        error(tbl->lineno, "Too many next table map entries in table %s", tbl->name());
    return rv;
}

void GatewayTable::pass2() {
    LOG1("### Gateway table " << name() << " pass2 " << loc());
    if (logical_id < 0) {
        if (match_table)
            logical_id = match_table->logical_id;
        else
            choose_logical_id();
    }
    for (auto &ixb : input_xbar) ixb->pass2();
    need_next_map_lut = miss.next.need_next_map_lut();
    for (auto &e : table) need_next_map_lut |= e.next.need_next_map_lut();
    if (need_next_map_lut) {
        Table *tbl = match_table;
        if (!tbl) tbl = this;
        for (auto &e : table)
            if (!e.run_table && e.next_map_lut < 0) e.next_map_lut = find_next_lut_entry(tbl, e);
        if (!miss.run_table && miss.next_map_lut < 0)
            miss.next_map_lut = find_next_lut_entry(tbl, miss);
    }
}

void GatewayTable::pass3() {
    LOG1("### Gateway table " << name() << " pass3 " << loc());
    if (match_table)
        physical_ids = match_table->physical_ids;
    else
        allocate_physical_ids();
}

static unsigned match_input_use(const std::vector<GatewayTable::MatchKey> &match) {
    unsigned rv = 0;
    for (auto &r : match) {
        unsigned lo = r.offset;
        unsigned hi = lo + r.val->size() - 1;
        if (lo < 32) {
            rv |= (((UINT32_C(1) << (hi / 8 - lo / 8 + 1)) - 1) << lo / 8) & 0xf;
            lo = 32;
        }
        if (lo <= hi) rv |= ((UINT32_C(1) << (hi - lo + 1)) - 1) << (lo - 24);
    }
    return rv;
}

/* caluclate match_bus byte use (8 bytes/bits) + hash output use (12 bits) */
unsigned GatewayTable::input_use() const {
    unsigned rv = match_input_use(match) | match_input_use(xor_match);
    if (!xor_match.empty()) rv |= (rv & 0xf) << 4;
    return rv;
}

bool GatewayTable::is_branch() const {
    for (auto &line : table)
        if (line.next.next_table() != nullptr) return true;
    if (!miss.run_table && miss.next.next_table() != nullptr) return true;
    return false;
}

/* FIXME -- how to deal with (or even specify) matches in the upper 24 bits coming from
 * the hash bus?   Currently we assume that the input_xbar is declared to set up the
 * hash signals correctly so that we can just match them.  Should at least check it
 * somewhere, somehow. We do some checking in check_match_key above, but is that enough?
 */
template <class REGS>
static bool setup_vh_xbar(REGS &regs, Table *table, Table::Layout &row, int base,
                          std::vector<GatewayTable::MatchKey> &match, int group) {
    auto &rams_row = regs.rams.array.row[row.row];
    auto &byteswizzle_ctl =
        rams_row.exactmatch_row_vh_xbar_byteswizzle_ctl[row.bus.at(Table::Layout::SEARCH_BUS)];
    for (auto &r : match) {
        if (r.offset >= 32) break; /* skip hash matches */
        for (int bit = 0; bit < r.val->size(); ++bit) {
            int ibyte = table->find_on_ixbar(*Phv::Ref(r.val, bit, bit), group);
            if (ibyte < 0) {
                error(r.val.lineno, "Can't find %s(%d) on ixbar", r.val.desc().c_str(), bit);
                return false;
            }
            unsigned byte = base + (r.offset + bit) / 8;
            byteswizzle_ctl[byte][(r.val->lo + bit) & 7] = 0x10 + ibyte;
        }
    }
    return true;
}

template <class REGS>
void enable_gateway_payload_exact_shift_ovr(REGS &regs, int bus) {
    regs.rams.match.merge.gateway_payload_exact_shift_ovr[bus / 8] |= 1U << bus % 8;
}

template <class REGS>
void GatewayTable::payload_write_regs(REGS &regs, int row, int type, int bus) {
    auto &merge = regs.rams.match.merge;
    auto &xbar_ctl = merge.gateway_to_pbus_xbar_ctl[row * 2 + bus];
    if (type) {
        xbar_ctl.tind_logical_select = logical_id;
        xbar_ctl.tind_inhibit_enable = 1;
    } else {
        xbar_ctl.exact_logical_select = logical_id;
        xbar_ctl.exact_inhibit_enable = 1;
    }
    if (have_payload >= 0 || match_address >= 0) {
        BUG_CHECK(payload_unit == bus);
        if (type)
            merge.gateway_payload_tind_pbus[row] |= 1 << bus;
        else
            merge.gateway_payload_exact_pbus[row] |= 1 << bus;
    }
    if (have_payload >= 0) {
        merge.gateway_payload_data[row][bus][0][type] = payload & 0xffffffff;
        merge.gateway_payload_data[row][bus][1][type] = payload >> 32;
        merge.gateway_payload_data[row][bus][0][type ^ 1] = payload & 0xffffffff;
        merge.gateway_payload_data[row][bus][1][type ^ 1] = payload >> 32;
    }
    if (match_address >= 0) {
        merge.gateway_payload_match_adr[row][bus][type] = match_address;
        merge.gateway_payload_match_adr[row][bus][type ^ 1] = match_address;
    } else if (options.target == TOFINO) {
        // For Tofino A0, there is a bug in snapshot that cannot distinguish if a
        // gateway is inhibiting a table To work around this, configure the
        // gateway_payload_match_adr to an invalid value. Add a command line flag
        // if this is only a tofino A0 issue?.
        merge.gateway_payload_match_adr[row][bus][type] = 0x7ffff;
        merge.gateway_payload_match_adr[row][bus][type ^ 1] = 0x7ffff;
    }

    int groups = format ? format->groups() : 1;
    if (groups > 1 || payload_map.size() > 1) {
        BUG_CHECK(type == 0);  // only supported on exact result busses
        enable_gateway_payload_exact_shift_ovr(regs, row * 2 + bus);
    }

    int tcam_shift = 0;
    if (type != 0 && format) {
        auto match_table = get_match_table();
        if (match_table) {
            auto ternary_table = match_table->to<TernaryMatchTable>();
            if (ternary_table && ternary_table->has_indirect()) {
                tcam_shift = format->log2size - 2;
            }
        }
    }

    if (format) {
        if (auto *attached = get_attached()) {
            for (auto &st : attached->stats) {
                if (type == 0) {
                    for (unsigned i = 0; i < payload_map.size(); ++i) {
                        auto grp = payload_map.at(i);
                        if (grp < 0) continue;
                        merge.mau_stats_adr_exact_shiftcount[row * 2 + bus][i] =
                            st->determine_shiftcount(st, grp, 0, 0);
                    }
                } else {
                    merge.mau_stats_adr_tcam_shiftcount[row * 2 + bus] =
                        st->determine_shiftcount(st, 0, 0, tcam_shift);
                }
                break;
            }

            for (auto &m : attached->meters) {
                if (type == 0) {
                    for (unsigned i = 0; i < payload_map.size(); ++i) {
                        auto grp = payload_map.at(i);
                        if (grp < 0) continue;
                        m->to<MeterTable>()->setup_exact_shift(regs, row * 2 + bus, grp, 0, i, m,
                                                               attached->meter_color);
                    }
                } else {
                    m->to<MeterTable>()->setup_tcam_shift(regs, row * 2 + bus, tcam_shift, m,
                                                          attached->meter_color);
                }
                break;
            }
            for (auto &s : attached->statefuls) {
                if (type == 0) {
                    for (unsigned i = 0; i < payload_map.size(); ++i) {
                        auto grp = payload_map.at(i);
                        if (grp < 0) continue;
                        merge.mau_meter_adr_exact_shiftcount[row * 2 + bus][i] =
                            s->determine_shiftcount(s, grp, 0, 0);
                    }
                } else {
                    merge.mau_meter_adr_tcam_shiftcount[row * 2 + bus] =
                        s->determine_shiftcount(s, 0, 0, tcam_shift);
                }
                break;
            }
        }
    }

    if (match_table && match_table->instruction) {
        if (auto field = match_table->instruction.args[0].field()) {
            if (type == 0) {
                for (unsigned i = 0; i < payload_map.size(); ++i) {
                    auto grp = payload_map.at(i);
                    if (grp < 0) continue;
                    merge.mau_action_instruction_adr_exact_shiftcount[row * 2 + bus][i] =
                        field->by_group[grp]->bit(0);
                }
            } else {
                merge.mau_action_instruction_adr_tcam_shiftcount[row * 2 + bus] =
                    field->bit(0) + tcam_shift;
            }
        }
    } else if (auto *action = format ? format->field("action") : nullptr) {
        if (type == 0) {
            for (unsigned i = 0; i < payload_map.size(); ++i) {
                auto grp = payload_map.at(i);
                if (grp < 0) continue;
                merge.mau_action_instruction_adr_exact_shiftcount[row * 2 + bus][i] =
                    action->by_group[grp]->bit(0);
            }
        } else {
            merge.mau_action_instruction_adr_tcam_shiftcount[row * 2 + bus] =
                action->bit(0) + tcam_shift;
        }
    }

    if (format && format->immed) {
        if (type == 0) {
            for (unsigned i = 0; i < payload_map.size(); ++i) {
                auto grp = payload_map.at(i);
                if (grp < 0) continue;
                merge.mau_immediate_data_exact_shiftcount[row * 2 + bus][i] =
                    format->immed->by_group[grp]->bit(0);
            }
        } else {
            merge.mau_immediate_data_tcam_shiftcount[row * 2 + bus] =
                format->immed->bit(0) + tcam_shift;
        }
        // FIXME -- may be redundant witehr writing this for the match table,
        // but should always be consistent
        merge.mau_immediate_data_mask[type][row * 2 + bus] = bitMask(format->immed_size);
        merge.mau_payload_shifter_enable[type][row * 2 + bus].immediate_data_payload_shifter_en = 1;
    }

    if (type) {
        merge.tind_bus_prop[row * 2 + bus].tcam_piped = 1;
        merge.tind_bus_prop[row * 2 + bus].thread = gress;
        merge.tind_bus_prop[row * 2 + bus].enabled = 1;
    } else {
        merge.exact_match_phys_result_en[row / 4U] |= 1U << (row % 4U * 2 + bus);
        merge.exact_match_phys_result_thread[row / 4U] |= gress << (row % 4U * 2 + bus);
        if (stage->tcam_delay(gress))
            merge.exact_match_phys_result_delay[row / 4U] |= 1U << (row % 4U * 2 + bus);
    }
}

template <class REGS>
void GatewayTable::standalone_write_regs(REGS &regs) {}

template <class REGS>
void GatewayTable::write_regs_vt(REGS &regs) {
    LOG1("### Gateway table " << name() << " write_regs " << loc());
    auto &row = layout[0];
    for (auto &ixb : input_xbar) {
        // FIXME -- if there's no ixbar in the gateway, we should look for a group with
        // all the match/xor values across all the exact match groups in the stage and use
        // that.
        ixb->write_regs(regs);
        if (!setup_vh_xbar(regs, this, row, 0, match, ixb->match_group()) ||
            !setup_vh_xbar(regs, this, row, 4, xor_match, ixb->match_group()))
            return;
    }
    auto &row_reg = regs.rams.array.row[row.row];
    auto &gw_reg = row_reg.gateway_table[gw_unit];
    auto &merge = regs.rams.match.merge;
    int search_bus = row.bus.at(Layout::SEARCH_BUS);
    if (search_bus == 0) {
        gw_reg.gateway_table_ctl.gateway_table_input_data0_select = 1;
        gw_reg.gateway_table_ctl.gateway_table_input_hash0_select = 1;
    } else {
        BUG_CHECK(search_bus == 1);
        gw_reg.gateway_table_ctl.gateway_table_input_data1_select = 1;
        gw_reg.gateway_table_ctl.gateway_table_input_hash1_select = 1;
    }
    for (auto &ixb : input_xbar) {
        if (ixb->hash_group() >= 0)
            setup_muxctl(row_reg.vh_adr_xbar.exactmatch_row_hashadr_xbar_ctl[search_bus],
                         ixb->hash_group());
        if (ixb->match_group() >= 0 && gateway_needs_ixbar_group()) {
            auto &vh_xbar_ctl = row_reg.vh_xbar[search_bus].exactmatch_row_vh_xbar_ctl;
            setup_muxctl(vh_xbar_ctl, ixb->match_group());
            /* vh_xbar_ctl.exactmatch_row_vh_xbar_thread = gress; */ }
    }
    gw_reg.gateway_table_ctl.gateway_table_logical_table = logical_id;
    gw_reg.gateway_table_ctl.gateway_table_thread = timing_thread(gress);
    for (auto &r : xor_match)
        gw_reg.gateway_table_matchdata_xor_en |= bitMask(r.val->size()) << r.offset;
    int idx = 3;
    gw_reg.gateway_table_ctl.gateway_table_mode = range_match;
    for (auto &line : table) {
        BUG_CHECK(idx >= 0);
        /* FIXME -- hardcoding version/valid to always */
        gw_reg.gateway_table_vv_entry[idx].gateway_table_entry_versionvalid0 = 0x3;
        gw_reg.gateway_table_vv_entry[idx].gateway_table_entry_versionvalid1 = 0x3;
        gw_reg.gateway_table_entry_matchdata[idx][0] = line.val.word0.getrange(0, 32);
        gw_reg.gateway_table_entry_matchdata[idx][1] = line.val.word1.getrange(0, 32);
        if (range_match) {
            auto &info = range_match_info[range_match];
            for (unsigned i = 0; i < range_match_info[range_match].units; i++) {
                gw_reg.gateway_table_data_entry[idx][0] |= (line.range[i] & info.half_mask)
                                                           << (i * info.bits);
                gw_reg.gateway_table_data_entry[idx][1] |=
                    ((line.range[i] >> info.half_shift) & info.half_mask) << (i * info.bits);
            }
        } else {
            gw_reg.gateway_table_data_entry[idx][0] = line.val.word0.getrange(32, 24);
            gw_reg.gateway_table_data_entry[idx][1] = line.val.word1.getrange(32, 24);
        }
        if (!line.run_table) {
            merge.gateway_inhibit_lut[logical_id] |= 1 << idx;
        }
        idx--;
    }
    if (!miss.run_table) {
        merge.gateway_inhibit_lut[logical_id] |= 1 << 4;
    }
    write_next_table_regs(regs);
    merge.gateway_en |= 1 << logical_id;
    setup_muxctl(merge.gateway_to_logicaltable_xbar_ctl[logical_id], row.row * 2 + gw_unit);
    if (layout.size() > 1) {
        int result_bus = layout[1].bus.at(Layout::RESULT_BUS);
        payload_write_regs(regs, layout[1].row, result_bus >> 1, result_bus & 1);
    }
    if (Table *tbl = match_table) {
        bool tind_bus = false;
        auto bus_type = Layout::RESULT_BUS;
        auto *tmatch = dynamic_cast<TernaryMatchTable *>(tbl);
        if (tmatch) {
            tind_bus = true;
            bus_type = Layout::TIND_BUS;
            tbl = tmatch->indirect;
        } else if (auto *hashaction = dynamic_cast<HashActionTable *>(tbl)) {
            tind_bus = hashaction->layout[0].bus.at(bus_type) >= 2;
        }
        if (tbl) {
            for (auto &row : tbl->layout) {
                if (row.bus.count(bus_type)) {
                    int bus = row.bus.at(bus_type);
                    auto &xbar_ctl = merge.gateway_to_pbus_xbar_ctl[row.row * 2 + (bus & 1)];
                    if (tind_bus) {
                        xbar_ctl.tind_logical_select = logical_id;
                        xbar_ctl.tind_inhibit_enable = 1;
                    } else {
                        xbar_ctl.exact_logical_select = logical_id;
                        xbar_ctl.exact_inhibit_enable = 1;
                    }
                }
            }
        } else {
            BUG_CHECK(tmatch);
            auto &xbar_ctl = merge.gateway_to_pbus_xbar_ctl[tmatch->indirect_bus];
            xbar_ctl.tind_logical_select = logical_id;
            xbar_ctl.tind_inhibit_enable = 1;
        }
    } else {
        if (gress != GHOST) merge.predication_ctl[gress].table_thread |= 1 << logical_id;
        if (gress == INGRESS || gress == GHOST) {
            merge.logical_table_thread[0].logical_table_thread_ingress |= 1 << logical_id;
            merge.logical_table_thread[1].logical_table_thread_ingress |= 1 << logical_id;
            merge.logical_table_thread[2].logical_table_thread_ingress |= 1 << logical_id;
        } else if (gress == EGRESS) {
            regs.dp.imem_table_addr_egress |= 1 << logical_id;
            merge.logical_table_thread[0].logical_table_thread_egress |= 1 << logical_id;
            merge.logical_table_thread[1].logical_table_thread_egress |= 1 << logical_id;
            merge.logical_table_thread[2].logical_table_thread_egress |= 1 << logical_id;
        }
        auto &adrdist = regs.rams.match.adrdist;
        adrdist.adr_dist_table_thread[timing_thread(gress)][0] |= 1 << logical_id;
        adrdist.adr_dist_table_thread[timing_thread(gress)][1] |= 1 << logical_id;
        // FIXME -- allow table_counter on standalone gateay?  What can it count?
        if (options.match_compiler)
            merge.mau_table_counter_ctl[logical_id / 8U].set_subfield(4, 3 * (logical_id % 8U), 3);
        standalone_write_regs(regs);
    }
    if (stage->tcam_delay(gress) > 0) merge.exact_match_logical_result_delay |= 1 << logical_id;
}

std::set<std::string> gateways_in_json;
void GatewayTable::gen_tbl_cfg(json::vector &out) const {
    // Avoid adding gateway table multiple times to the json. The gateway table
    // gets called multiple times in some cases based on how it is attached or
    // associated with a match table, we should only output it to json once.
    auto gwName = gateway_name.empty() ? name() : gateway_name;
    if (gateways_in_json.count(gwName)) return;
    LOG3("### Gateway table " << gwName << " gen_tbl_cfg " << loc());
    json::map gTable;
    gTable["direction"] = P4Table::direction_name(gress);
    gTable["attached_to"] = match_table ? match_table->p4_name() : "-";
    gTable["handle"] = gateway_handle++;
    gTable["name"] = gwName;
    gTable["table_type"] = "condition";

    json::vector gStageTables;
    json::map gStageTable;

    json::map &next_table_ids = gStageTable["next_tables"];
    json::map &next_table_names = gStageTable["next_table_names"];

    auto &condTNext = cond_true.next;
    auto &condFNext = cond_false.next;
    if (Target::LONG_BRANCH_TAGS() > 0) {
        json::vector &next_table_names_true = next_table_names["true"];
        json::vector &next_table_names_false = next_table_names["false"];
        json::vector &next_table_ids_true = next_table_ids["true"];
        json::vector &next_table_ids_false = next_table_ids["false"];
        if (condTNext.size() == 0) {
            next_table_names_true.push_back(condTNext.next_table_name());
            next_table_ids_true.push_back(condTNext.next_table_id());
        } else {
            for (auto t : condTNext) {
                next_table_names_true.push_back(t.name);
                next_table_ids_true.push_back(t->table_id());
            }
        }
        if (condFNext.size() == 0) {
            next_table_names_false.push_back(condFNext.next_table_name());
            next_table_ids_false.push_back(condFNext.next_table_id());
        } else {
            for (auto t : condFNext) {
                next_table_names_false.push_back(t.name);
                next_table_ids_false.push_back(t->table_id());
            }
        }
    } else {
        next_table_ids["false"] = json::string(condFNext.next_table_id());
        next_table_ids["true"] = json::string(condTNext.next_table_id());
        next_table_names["false"] = json::string(condFNext.next_table_name());
        next_table_names["true"] = json::string(condTNext.next_table_name());
    }

    json::map mra;
    mra["memory_unit"] = gw_memory_unit();
    mra["memory_type"] = "gateway";
    mra["payload_buses"] = json::vector();
    gStageTable["memory_resource_allocation"] = std::move(mra);
    json::vector pack_format;  // For future use
    gStageTable["pack_format"] = std::move(pack_format);

    gStageTable["logical_table_id"] = logical_id;
    gStageTable["stage_number"] = stage->stageno;
    gStageTable["stage_table_type"] = "gateway";
    gStageTable["size"] = 0;
    gStageTables.push_back(std::move(gStageTable));

    json::vector condition_fields;
    for (auto m : match) {
        json::map condition_field;
        condition_field["name"] = m.val.name();
        condition_field["start_bit"] = m.offset;
        condition_field["bit_width"] = m.val.size();
        condition_fields.push_back(std::move(condition_field));
    }

    gTable["stage_tables"] = std::move(gStageTables);
    gTable["condition_fields"] = std::move(condition_fields);
    gTable["condition"] = gateway_cond;
    gTable["size"] = 0;
    out.push_back(std::move(gTable));
    gateways_in_json.insert(gwName);
}

DEFINE_TABLE_TYPE_WITH_SPECIALIZATION(GatewayTable, TARGET_CLASS)
