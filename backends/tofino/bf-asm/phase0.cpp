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
#include "parser-tofino-jbay.h"

DEFINE_TABLE_TYPE(Phase0MatchTable)

void Phase0MatchTable::setup(VECTOR(pair_t) & data) {
    for (auto &kv : MapIterChecked(data)) {
        if (common_setup(kv, data, P4Table::MatchEntry)) {
        } else if (auto *fmt = get(data, "format")) {
            if (CHECKTYPEPM(*fmt, tMAP, fmt->map.size > 0, "non-empty map"))
                format.reset(new Format(this, fmt->map));
        } else if (kv.key == "size") {
            if (CHECKTYPE(kv.value, tINT)) size = kv.value.i;
        } else if (kv.key == "constant_value") {
            if (CHECKTYPE(kv.value, tINT)) constant_value = kv.value.i;
        } else {
            warning(kv.key.lineno, "ignoring unknown item %s in table %s", value_desc(kv.key),
                    name());
        }
    }
    if (gress != INGRESS || stage->stageno != 0)
        error(lineno, "Phase 0 match table can only be in stage 0 ingress");
}

void Phase0MatchTable::pass1() {
    LOG1("### Phase 0 match table " << name() << " pass1 " << loc());
    MatchTable::pass1();
    if (actions) actions->pass1(this);
}

void Phase0MatchTable::pass2() { LOG1("### Phase 0 match table " << name() << " pass2 " << loc()); }

void Phase0MatchTable::pass3() { LOG1("### Phase 0 match table " << name() << " pass3 " << loc()); }

template <class REGS>
void Phase0MatchTable::write_regs_vt(REGS &) {
    LOG1("### Phase 0 match table " << name() << " write_regs " << loc());
}

void Phase0MatchTable::gen_tbl_cfg(json::vector &out) const {
    json::map &tbl = *base_tbl_cfg(out, "match_entry", p4_table ? p4_table->size : size);
    common_tbl_cfg(tbl);
    tbl["statistics_table_refs"] = json::vector();
    tbl["meter_table_refs"] = json::vector();
    tbl["selection_table_refs"] = json::vector();
    tbl["stateful_table_refs"] = json::vector();
    tbl["action_data_table_refs"] = json::vector();
    json::map &match_attributes = tbl["match_attributes"] = json::map();
    json::map &stage_tbl = *add_stage_tbl_cfg(match_attributes, "phase_0_match", size);
    match_attributes["match_type"] = "phase_0_match";
    stage_tbl["stage_number"] = -1;
    // Associate the phase0 table with corresponding parser. This is used in a
    // multi parser scenario which has multiple phase0 tables
    // and the handle is used by the driver to link the phase0 table to the
    // parser.
    auto parser_handle = Parser::get_parser_handle(name());
    if (parser_handle > 0) stage_tbl["parser_handle"] = parser_handle;
    stage_tbl.erase("logical_table_id");
    stage_tbl.erase("default_next_table");
    stage_tbl.erase("has_attached_gateway");
    auto &mra = stage_tbl["memory_resource_allocation"] = json::map();
    mra["memory_type"] = "ingress_buffer";
    json::map tmp;
    (tmp["vpns"] = json::vector()).push_back(INT64_C(0));
    (tmp["memory_units"] = json::vector()).push_back(INT64_C(0));
    (mra["memory_units_and_vpns"] = json::vector()).push_back(std::move(tmp));
    // Driver looks at the pack format to determine the fields and their
    // positions. Since phase0 is only mimicking a table, the driver expects to
    // have a single entry within the pack format.
    bool pad_zeros = false;
    bool print_fields = true;
    add_pack_format(stage_tbl, format.get(), pad_zeros, print_fields);
    if (actions) actions->gen_tbl_cfg(tbl["actions"]);
    if (context_json) stage_tbl.merge(*context_json);
}
