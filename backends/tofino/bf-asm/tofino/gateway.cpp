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

#include "backends/tofino/bf-asm/tofino/gateway.h"

#include "backends/tofino/bf-asm/hashexpr.h"
#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/tofino/ternary_match.h"
#include "lib/hex.h"

/* Tofino1/2 Gateway table support
 * GatewayTable uses the Table::Layout in a somewhat hacky way to track the gateway match
 * and payload blocks.  Layout may have either one or two entries.
 * layout[0] is the layout for the gateway match -- which row and search bus is being used
 * layout[1] is the layout for the payload -- which row and result bus is being used.
 * if layout.size() == 1, there is no payload.
 * The payload result bus is stored as bus[RESULT_BUS] even though it may be either a
 * match result bus or a tind bus -- the second bit (so busses 2 and 3) are the tind
 * busses as that is the way they they are encode in some registers.  It should perhaps be
 * changed to use the bus_type_t to track whether it is a match RESULT_BUS or a TIND_BUS
 */

bool Target::Tofino::GatewayTable::check_match_key(MatchKey &key, const std::vector<MatchKey> &vec,
                                                   bool is_xor) {
    if (!::GatewayTable::check_match_key(key, vec, is_xor)) return false;
    if (key.offset < 32 && (key.offset & 7) != (key.val->lo & 7))
        error(key.val.lineno, "Gateway %s key %s misaligned within byte", is_xor ? "xor" : "match",
              key.val.name());
    if (key.offset + key.val->size() > (is_xor ? 32 : 44)) {
        error(key.val.lineno, "Gateway %s key too big", is_xor ? "xor" : "match");
        return false;
    }
    if (key.offset >= 32 && !input_xbar.empty()) {
        BUG_CHECK(input_xbar.size() == 1, "%s does not have one input xbar", name());
        auto hash = input_xbar[0]->hash_column(key.offset + 8);
        if (hash.size() != 1 || hash[0]->bit || !hash[0]->fn ||
            !hash[0]->fn->match_phvref(key.val)) {
            // FIXME: hash.size() maybe zero when key.valid is true.
            // which means the key.offset is incorrect.
            if (!key.valid) {
                error(key.val.lineno, "Gateway %s key %s not in matching hash column",
                      is_xor ? "xor" : "match", key.val.name());
                return false;
            }
        }
    }
    return true;
}

void Target::Tofino::GatewayTable::pass1() {
    ::GatewayTable::pass1();
    /* in a gateway, the layout has one or two rows -- layout[0] specifies the gateway, and
     * layout[1] specifies the payload. There will be no columns in either row.
     */
    if (layout.empty() || layout[0].row < 0)
        error(lineno, "No row specified in gateway");
    else if (!layout[0].bus.count(Layout::SEARCH_BUS) && (!match.empty() || !xor_match.empty()))
        error(lineno, "No bus specified in gateway to read from");
    if (payload_unit >= 0 && have_payload < 0 && match_address < 0)
        error(lineno, "payload_unit with no payload or match address in gateway");
    if (layout.size() > 1) {
        if (layout[1].bus.count(Layout::RESULT_BUS) && (have_payload >= 0 || match_address >= 0)) {
            int result_bus = layout[1].bus.at(Layout::RESULT_BUS);
            if (payload_unit < 0) {
                payload_unit = result_bus & 1;
            } else if (payload_unit != (result_bus & 1)) {
                error(layout[1].lineno, "payload unit %d cannot write to result bus %d",
                      payload_unit, result_bus);
            }
        }
        if (layout[1].row < 0) {
            error(layout[1].lineno, "payload_bus with no payload_row in gateway");
        } else if (Table *tbl = match_table) {
            if (auto *tmatch = dynamic_cast<TernaryMatchTable *>(tbl)) tbl = tmatch->indirect;
            if (tbl && !tbl->layout.empty()) {
                for (auto &r : tbl->layout) {
                    if (r.row != layout[1].row) continue;
                    if (!r.bus.count(Layout::RESULT_BUS)) continue;
                    int match_rbus = r.bus.at(Layout::RESULT_BUS);
                    if (payload_unit >= 0 && payload_unit != (match_rbus & 1)) continue;
                    if (!layout[1].bus.count(Layout::RESULT_BUS))
                        layout[1].bus[Layout::RESULT_BUS] = match_rbus;
                    if (match_rbus == layout[1].bus.at(Layout::RESULT_BUS)) {
                        if (tbl->to<TernaryIndirectTable>()) layout[1].bus[Layout::RESULT_BUS] |= 2;
                        break;
                    }
                }
            }
        } else if (have_payload >= 0 || match_address >= 0) {
            if (payload_unit) {
                if (auto *old = stage->gw_payload_use[layout[1].row][payload_unit])
                    error(layout[1].lineno, "payload %d.%d already in use by table %s",
                          layout[1].row, payload_unit, old->name());
                else
                    stage->gw_payload_use[layout[1].row][payload_unit] = this;
            }
        } else if (payload_unit >= 0) {
            error(lineno, "payload_unit with no payload or match address in gateway");
        }
    } else if ((have_payload >= 0 || match_address >= 0) && !match_table) {
        error(have_payload, "payload on standalone gateway requires explicit payload_row");
    } else if (payload_unit >= 0 && match_table) {
        bool ternary = false;
        Table *tbl = match_table;
        if (auto *tmatch = dynamic_cast<TernaryMatchTable *>(tbl)) {
            ternary = true;
            tbl = tmatch->indirect;
        }
        if (!tbl || tbl->layout.empty()) {
            error(lineno, "No result busses in table %s for gateway payload", match_table->name());
        } else {
            for (auto &r : tbl->layout) {
                auto match_rbus = r.bus.count(Layout::RESULT_BUS) ? r.bus.at(Layout::RESULT_BUS)
                                                                  : r.bus.at(Layout::SEARCH_BUS);
                if (match_rbus >= 0 && payload_unit != (match_rbus & 1)) continue;
                if (!stage->gw_payload_use[r.row][payload_unit]) {
                    layout.resize(2);
                    layout[1].row = r.row;
                    if (r.bus.count(Layout::RESULT_BUS))
                        layout[1].bus[Layout::RESULT_BUS] = r.bus.at(Layout::RESULT_BUS);
                    else
                        layout[1].bus[Layout::RESULT_BUS] =
                            r.bus.at(Layout::SEARCH_BUS) | (ternary ? 2 : 0);
                    stage->gw_payload_use[r.row][payload_unit] = this;
                    break;
                }
            }
            if (layout.size() < 2)
                error(lineno, "No row in table %s has payload unit %d free", tbl->name(),
                      payload_unit);
        }
    }
    if (layout.size() > 1 && layout[1].bus.count(Layout::RESULT_BUS)) {
        int result_bus = layout[1].bus.at(Layout::RESULT_BUS);
        Table *tbl = match_table;
        if (auto *tmatch = dynamic_cast<TernaryMatchTable *>(tbl)) tbl = tmatch->indirect;
        if (!tbl) tbl = this;
        auto &bus_use =
            (result_bus & 2) ? stage->tcam_indirect_bus_use : stage->match_result_bus_use;
        auto *old = bus_use[layout[1].row][result_bus & 1];
        if (old && old != tbl)
            error(layout[1].lineno,
                  "Gateway payload result bus %d conflict on row %d between "
                  "%s and %s",
                  result_bus, layout[1].row, name(), old->name());
        bus_use[layout[1].row][result_bus & 1] = tbl;
    }
}

void Target::Tofino::GatewayTable::pass2() {
    ::GatewayTable::pass2();
    if (gw_unit < 0) {
        if (layout[0].bus.count(Layout::SEARCH_BUS) &&
            !stage->gw_unit_use[layout[0].row][layout[0].bus.at(Layout::SEARCH_BUS)]) {
            gw_unit = layout[0].bus.at(Layout::SEARCH_BUS);
        } else {
            for (int i = 0; i < 2; ++i) {
                if (!stage->gw_unit_use[layout[0].row][i] &&
                    !stage->sram_search_bus_use[layout[0].row][i]) {
                    gw_unit = i;
                    break;
                }
            }
        }
        if (gw_unit < 0)
            error(layout[0].lineno, "No gateway units available on row %d", layout[0].row);
        else
            stage->gw_unit_use[layout[0].row][gw_unit] = this;
    }
    if (!layout[0].bus.count(Layout::SEARCH_BUS) && gw_unit >= 0)
        layout[0].bus[Layout::SEARCH_BUS] = gw_unit;
    if (payload_unit < 0 && (have_payload >= 0 || match_address >= 0)) {
        if (layout.size() > 1) {
            if (!layout[1].bus.count(Layout::RESULT_BUS)) {
                if (!stage->gw_payload_use[layout[1].row][0])
                    payload_unit = 0;
                else if (!stage->gw_payload_use[layout[1].row][1])
                    payload_unit = 1;
            } else {
                int u = layout[1].bus.at(Layout::RESULT_BUS) & 1;
                if (!stage->gw_payload_use[layout[1].row][u]) payload_unit = u;
            }
            if (payload_unit >= 0)
                stage->gw_payload_use[layout[1].row][payload_unit] = this;
            else
                error(lineno, "No payload available on row %d", layout[1].row);
        } else if (Table *tbl = match_table) {
            bool ternary = false;
            if (auto *tmatch = dynamic_cast<TernaryMatchTable *>(tbl)) {
                tbl = tmatch->indirect;
                ternary = true;
            }
            if (tbl && !tbl->layout.empty()) {
                for (auto &row : tbl->layout) {
                    auto match_rbus = row.bus.at(ternary ? Layout::TIND_BUS : Layout::RESULT_BUS);
                    BUG_CHECK(match_rbus >= 0, "alloc_busses on the match table must be run first");
                    if (stage->gw_payload_use[row.row][match_rbus]) {
                        continue;
                    } else {
                        payload_unit = match_rbus;
                    }
                    stage->gw_payload_use[row.row][payload_unit] = this;
                    layout.resize(2);
                    layout[1].row = row.row;
                    layout[1].bus[Layout::RESULT_BUS] = match_rbus | (ternary ? 2 : 0);
                    break;
                }
                if (payload_unit < 0)
                    error(lineno, "No row in table %s has a free payload unit", tbl->name());
            } else {
                error(lineno, "No result busses in table %s for gateway payload",
                      match_table->name());
            }
        }
    }
    if (payload_unit >= 0 && !layout[1].bus.count(Layout::RESULT_BUS)) {
        BUG_CHECK(layout.size() > 1, "layout size <= 1");
        int row = layout[1].row;
        Table *tbl = match_table;
        int ternary = tbl ? 0 : -1;
        if (auto *tmatch = dynamic_cast<TernaryMatchTable *>(tbl)) {
            ternary = 1;
            tbl = tmatch->indirect ? tmatch->indirect : tmatch;
        }
        if (!tbl) tbl = this;
        for (int i = payload_unit; i < 4; i += 2) {
            if (ternary >= 0 && (i >> 1) != ternary) continue;
            auto &result_bus = (i & 2) ? stage->tcam_indirect_bus_use : stage->match_result_bus_use;
            if (!result_bus[row][i & 1] || result_bus[row][i & 1] == tbl) {
                layout[1].bus[Layout::RESULT_BUS] = i;
                result_bus[row][i & 1] = tbl;
                break;
            }
        }
        if (!layout[1].bus.count(Layout::RESULT_BUS)) {
            error(lineno, "No result bus available for gateway payload of table %s on row %d",
                  name(), layout[1].row);
        }
    }
}

void Target::Tofino::GatewayTable::pass3() {
    ::GatewayTable::pass3();
    if (layout[0].bus.count(Layout::SEARCH_BUS)) {
        int search_bus = layout[0].bus.at(Layout::SEARCH_BUS);
        auto *tbl = stage->sram_search_bus_use[layout[0].row][search_bus];
        // Sharing with an exact match -- make sure it is ok
        if (!tbl) return;
        for (auto &ixb : input_xbar) {
            auto *sram_tbl = tbl->to<SRamMatchTable>();
            BUG_CHECK(sram_tbl,
                      "%s is not an SRamMatch table even though it is using a "
                      "search bus?",
                      tbl->name());
            SRamMatchTable::WayRam *way = nullptr;
            for (auto &row : sram_tbl->layout) {
                if (row.row == layout[0].row && row.bus.at(Layout::SEARCH_BUS) == search_bus) {
                    if (row.memunits.empty()) {
                        // FIXME -- not really used, so we don't need to check the
                        // match/hash group.  Should this be an asm error?
                        return;
                    }
                    way = &sram_tbl->way_map.at(row.memunits[0]);
                    break;
                }
            }
            BUG_CHECK(way, "%s claims to use search bus %d.%d, but we can't find it in the layout",
                      sram_tbl->name(), layout[0].row, search_bus);
            if (ixb->hash_group() >= 0 && sram_tbl->ways[way->way].group_xme >= 0 &&
                ixb->hash_group() != sram_tbl->ways[way->way].group_xme) {
                error(layout[0].lineno,
                      "%s sharing search bus %d.%d with %s, but wants a "
                      "different hash group",
                      name(), layout[0].row, search_bus, tbl->name());
            }
            if (ixb->match_group() >= 0 && sram_tbl->word_ixbar_group[way->word] >= 0 &&
                gateway_needs_ixbar_group() &&
                ixb->match_group() != sram_tbl->word_ixbar_group[way->word]) {
                error(layout[0].lineno,
                      "%s sharing search bus %d.%d with %s, but wants a "
                      "different match group",
                      name(), layout[0].row, search_bus, tbl->name());
            }
        }
    }
}

template <>
void enable_gateway_payload_exact_shift_ovr(Target::Tofino::mau_regs & /*regs*/, int /*bus*/) {
    BUG("enable_gateway_payload_exact_shift_ovr not supported");
}

void Target::Tofino::GatewayTable::write_next_table_regs(Target::Tofino::mau_regs &regs) {
    auto &merge = regs.rams.match.merge;
    int idx = 3;
    if (need_next_map_lut) error(lineno, "Tofino does not support using next_map_lut in gateways");
    for (auto &line : table) {
        BUG_CHECK(idx >= 0, "Index is negative");
        if (!line.run_table)
            merge.gateway_next_table_lut[logical_id][idx] = line.next.next_table_id();
        --idx;
    }
    if (!miss.run_table) merge.gateway_next_table_lut[logical_id][4] = miss.next.next_table_id();
}
