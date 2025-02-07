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
#include "misc.h"

void IdletimeTable::setup(VECTOR(pair_t) & data) {
    setup_layout(layout, data);
    for (auto &kv : MapIterChecked(data, true)) {
        if (kv.key == "precision") {
            if (CHECKTYPE(kv.value, tINT)) {
                precision = kv.value.i;
                if (precision != 1 && precision != 2 && precision != 3 && precision != 6)
                    error(kv.value.lineno, "Invalid idletime precision %d", precision);
            }
        } else if (kv.key == "sweep_interval") {
            if (CHECKTYPE(kv.value, tINT)) sweep_interval = kv.value.i;
        } else if (kv.key == "notification") {
            if (kv.value == "disable")
                disable_notification = true;
            else if (kv.value == "two_way")
                two_way_notification = true;
            else if (kv.value != "enable")
                error(kv.value.lineno, "Unknown notification style '%s'", value_desc(kv.value));
        } else if (kv.key == "per_flow_enable") {
            per_flow_enable = get_bool(kv.value);
        } else if (kv.key == "context_json") {
            setup_context_json(kv.value);
        } else if (kv.key == "row" || kv.key == "column" || kv.key == "bus") {
            /* already done in setup_layout */
        } else {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
    alloc_rams(false, stage->mapram_use);
    for (auto &r : layout) {
        if (!r.bus.count(Layout::IDLE_BUS)) continue;
        int &idle_bus = r.bus.at(Layout::IDLE_BUS);
        if (idle_bus >= IDLETIME_BUSSES) {
            error(r.lineno, "bus %d invalid", idle_bus);
            continue;
        }
        if (r.row >= 4 && idle_bus < 10)
            idle_bus += 10;
        else if (r.row < 4 && idle_bus >= 10)
            error(r.lineno, "idletime bus %d not accessable on row %d", idle_bus, r.row);
        if (Table *old = stage->idletime_bus_use[idle_bus]) {
            if (old != this)
                error(r.lineno,
                      "Table %s trying to use idletime bus %d which is already in "
                      "use by table %s",
                      name(), idle_bus, old->name());
        } else {
            stage->idletime_bus_use[idle_bus] = this;
        }
    }
}

void IdletimeTable::pass1() {
    LOG1("### Idletime table " << name() << " pass1 " << loc());
    alloc_vpns();
}

void IdletimeTable::pass2() { LOG1("### Idletime table " << name() << " pass2 " << loc()); }

void IdletimeTable::pass3() { LOG1("### Idletime table " << name() << " pass3 " << loc()); }

// This is the same as AttachedTable::json_memunit, but IdletimeTable is not a derived class
// of AttachedTable, so we duplicate it
int IdletimeTable::json_memunit(const MemUnit &r) const {
    if (r.stage >= 0) {
        return r.stage * Target::SRAM_STRIDE_STAGE() + r.row * Target::SRAM_STRIDE_ROW() +
               r.col * Target::SRAM_STRIDE_COLUMN();
    } else if (r.row >= 0) {
        // per-stage logical sram
        return r.row * Target::SRAM_LOGICAL_UNITS_PER_ROW() + r.col;
    } else {
        // lamb
        return r.col;
    }
}

static int precision_bits[] = {0, 0, 1, 2, 0, 0, 3};

template <class REGS>
void IdletimeTable::write_merge_regs_vt(REGS &regs, int type, int bus) {
    auto &merge = regs.rams.match.merge;
    merge.mau_payload_shifter_enable[type][bus].idletime_adr_payload_shifter_en = 1;
    merge.mau_idletime_adr_mask[type][bus] =
        (~1U << precision_bits[precision]) & ((1U << IDLETIME_ADDRESS_BITS) - 1);
    merge.mau_idletime_adr_default[type][bus] =
        (1U << IDLETIME_ADDRESS_PER_FLOW_ENABLE_START_BIT) | ((1 << precision_bits[precision]) - 1);
}

FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, void IdletimeTable::write_merge_regs,
                      (mau_regs & regs, int type, int bus),
                      { write_merge_regs_vt(regs, type, bus); })

int IdletimeTable::precision_shift() const { return precision_bits[precision] + 1; }
int IdletimeTable::direct_shiftcount() const { return 67 - precision_bits[precision]; }

template <class REGS>
void IdletimeTable::write_regs_vt(REGS &regs) {
    LOG1("### Idletime table " << name() << " write_regs " << loc());
    auto &map_alu = regs.rams.map_alu;
    auto &adrdist = regs.rams.match.adrdist;
    int minvpn = 1000000, maxvpn = -1;
    for (Layout &logical_row : layout)
        for (auto v : logical_row.vpns) {
            if (v < minvpn) minvpn = v;
            if (v > maxvpn) maxvpn = v;
        }
    // regs.cfg_regs.mau_cfg_lt_has_idle |= 1 << logical_id;
    for (Layout &row : layout) {
        int idle_bus = row.bus.at(Layout::IDLE_BUS);
        auto &map_alu_row = map_alu.row[row.row];
        auto &adrmux = map_alu_row.adrmux;
        auto vpn = row.vpns.begin();
        for (auto &memunit : row.memunits) {
            int col = memunit.col;
            BUG_CHECK(memunit.stage == INT_MIN && memunit.row == row.row, "bogus %s in row %d",
                      memunit.desc(), row.row);
            setup_muxctl(map_alu_row.vh_xbars.adr_dist_idletime_adr_xbar_ctl[col], idle_bus % 10);
            auto &mapram_cfg = adrmux.mapram_config[col];
            // auto &mapram_ctl = adrmux.mapram_ctl[col];
            if (disable_notification) mapram_cfg.idletime_disable_notification = 1;
            if (two_way_notification) mapram_cfg.two_way_idletime_notification = 1;
            if (per_flow_enable) mapram_cfg.per_flow_idletime = 1;
            mapram_cfg.idletime_bitwidth = precision_bits[precision];
            mapram_cfg.mapram_type = MapRam::IDLETIME;
            mapram_cfg.mapram_logical_table = logical_id;
            mapram_cfg.mapram_vpn_members = 0;  // FIXME
            mapram_cfg.mapram_vpn = *vpn++;
            if (gress == INGRESS)
                mapram_cfg.mapram_ingress = 1;
            else
                mapram_cfg.mapram_egress = 1;
            mapram_cfg.mapram_enable = 1;
            if ((precision == 1) || (precision == 2)) {
                mapram_cfg.mapram_parity_generate = 1;
                mapram_cfg.mapram_parity_check = 1;
            } else {
                if ((precision != 3) && (precision != 6))
                    error(lineno, "Unknown idletime precision = %d", precision);
                mapram_cfg.mapram_ecc_generate = 1;
                mapram_cfg.mapram_ecc_check = 1;
            }
            auto &adrmux_ctl = adrmux.ram_address_mux_ctl[1][col];
            adrmux_ctl.map_ram_wadr_mux_select = MapRam::Mux::IDLETIME;
            adrmux_ctl.map_ram_wadr_mux_enable = 1;
            adrmux_ctl.map_ram_radr_mux_select_smoflo = 1;
            adrmux_ctl.ram_ofo_stats_mux_select_statsmeter = 1;
            adrmux_ctl.ram_stats_meter_adr_mux_select_idlet = 1;
            setup_muxctl(adrmux.idletime_logical_to_physical_sweep_grant_ctl[col], logical_id);
            setup_muxctl(adrmux.idletime_physical_to_logical_req_inc_ctl[col], logical_id);
            unsigned clear_val = ~(~0U << precision);
            if (per_flow_enable || precision == 1) clear_val &= ~1U;
            for (unsigned i = 0; i < 8U / precision; i++)
                adrmux.idletime_cfg_rd_clear_val[col].set_subfield(clear_val, i * precision,
                                                                   precision);
            if (gress)
                regs.cfg_regs.mau_cfg_mram_thread[col / 3U] |= 1U << (col % 3U * 8U + row.row);
        }
        adrdist.adr_dist_idletime_adr_oxbar_ctl[idle_bus / 4].set_subfield(logical_id | 0x10,
                                                                           5 * (idle_bus % 4), 5);
    }
    // don't enable initially -- runtime will enable
    // adrdist.idletime_sweep_ctl[logical_id].idletime_en = 1;
    adrdist.idletime_sweep_ctl[logical_id].idletime_sweep_offset = minvpn;
    adrdist.idletime_sweep_ctl[logical_id].idletime_sweep_size = layout_size() - 1;
    adrdist.idletime_sweep_ctl[logical_id].idletime_sweep_remove_hole_pos = 0;  // TODO
    adrdist.idletime_sweep_ctl[logical_id].idletime_sweep_remove_hole_en = 0;   // TODO
    adrdist.idletime_sweep_ctl[logical_id].idletime_sweep_interval = sweep_interval;
    auto &idle_dump_ctl = regs.cfg_regs.idle_dump_ctl[logical_id];
    idle_dump_ctl.idletime_dump_offset = minvpn;
    idle_dump_ctl.idletime_dump_size = maxvpn;
    idle_dump_ctl.idletime_dump_remove_hole_pos = 0;  // TODO
    idle_dump_ctl.idletime_dump_remove_hole_en = 0;   // TODO
    adrdist.movereg_idle_ctl[logical_id].movereg_idle_ctl_size = precision_bits[precision];
    adrdist.movereg_idle_ctl[logical_id].movereg_idle_ctl_direct = 1;
    adrdist.movereg_ad_direct[MoveReg::IDLE] |= 1 << logical_id;
    adrdist.idle_bubble_req[timing_thread(gress)].bubble_req_1x_class_en |= 1 << logical_id;
}
FOR_ALL_REGISTER_SETS(TARGET_OVERLOAD, void IdletimeTable::write_regs, (mau_regs & regs),
                      { write_regs_vt(regs); })

void IdletimeTable::gen_stage_tbl_cfg(json::map &out) const {
    unsigned number_entries = layout_size() * (8U / precision) * SRAM_DEPTH;
    json::map &tbl = out["idletime_stage_table"] = json::map();
    tbl["stage_number"] = stage->stageno;
    tbl["size"] = number_entries;
    tbl["stage_table_type"] = "idletime";
    tbl["precision"] = precision;
    tbl["disable_notification"] = disable_notification;
    tbl["two_way_notification"] = two_way_notification;
    // ??
    tbl["logical_table_id"] = match_table->logical_id;
    tbl["enable_pfe"] = per_flow_enable;
    add_pack_format(tbl, 11, 1, 8U / precision);
    tbl["memory_resource_allocation"] = gen_memory_resource_allocation_tbl_cfg("map_ram", layout);
}
