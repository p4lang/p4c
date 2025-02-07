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

#include "backends/tofino/bf-asm/tables.h"

#include <config.h>

#include <sstream>
#include <string>

#include "action_bus.h"
#include "backends/tofino/bf-asm/stage.h"
#include "input_xbar.h"
#include "instruction.h"
#include "lib/algorithm.h"
#include "misc.h"

// template specialization declarations

const char *MemUnit::desc() const {
    static char buffer[256], *p = buffer;
    char *end = buffer + sizeof(buffer), *rv;
    do {
        if (end - p < 7) p = buffer;
        rv = p;
        if (stage != INT_MIN)
            p += snprintf(p, end - p, "Mem %d,%d,%d", stage, row, col);
        else if (row >= 0)
            p += snprintf(p, end - p, "Mem %d,%d", row, col);
        else
            p += snprintf(p, end - p, "Mem %d", col);
    } while (p++ >= end);
    return rv;
}

bool Table::Layout::operator==(const Table::Layout &a) const {
    return row == a.row && bus == a.bus && word == a.word && memunits == a.memunits;
    // ignoring other fields as if the above are all the same, will use the same resources
}

unsigned StatefulTable::const_info_t::unique_register_param_handle = REGISTER_PARAM_HANDLE_START;

std::map<std::string, Table *> *Table::all;
std::vector<Table *> *Table::by_uid;
std::map<std::string, Table::Type *> *Table::Type::all;

Table::Table(int line, std::string &&n, gress_t gr, Stage *s, int lid)
    :  // NOLINT(whitespace/operators)
      name_(n),
      stage(s),
      gress(gr),
      lineno(line),
      logical_id(lid) {
    if (!all) all = new std::map<std::string, Table *>;
    if (!by_uid) by_uid = new std::vector<Table *>;
    uid = by_uid->size();
    by_uid->push_back(this);
    if (all->count(name_)) {
        error(lineno, "Duplicate table %s", name());
        error(all->at(name_)->lineno, "previously defined here");
    }
    all->emplace(name_, this);
    if (stage) stage->all_refs.insert(&stage);
}
Table::~Table() {
    BUG_CHECK(by_uid && uid >= 0 && uid < by_uid->size(), "invalid uid %d in table", uid);
    all->erase(name_);
    (*by_uid)[uid] = nullptr;
    if (stage) stage->all_refs.erase(&stage);
    if (all->empty()) {
        delete all;
        delete by_uid;
        all = nullptr;
        by_uid = nullptr;
    }
}

Table::Type::Type(std::string &&name) {  // NOLINT(whitespace/operators)
    if (!all) all = new std::map<std::string, Type *>();
    if (get(name)) {
        fprintf(stderr, "Duplicate table type %s\n", name.c_str());
        exit(1);
    }
    self = all->emplace(name, this).first;
}

Table::Type::~Type() {
    all->erase(self);
    if (all->empty()) {
        delete all;
        all = nullptr;
    }
}

Table::NextTables::NextTables(value_t &v) : lineno(v.lineno) {
    if (v.type == tVEC && (Target::LONG_BRANCH_TAGS() > 0 || v.vec.size == 0)) {
        for (auto &el : v.vec)
            if (CHECKTYPE(el, tSTR)) next.emplace(el);
    } else if (CHECKTYPE(v, tSTR)) {
        if (v != "END") next.emplace(v);
    }
}

bool Table::NextTables::can_use_lb(int stage, const NextTables &lbrch) {
    if (options.disable_long_branch) return false;
    if (!lbrch.subset_of(*this)) return false;
    return true;
}

void Table::NextTables::resolve_long_branch(const Table *tbl,
                                            const std::map<int, NextTables> &lbrch) {
    if (resolved) return;
    resolved = true;
    for (auto &lb : lbrch) {
        if (can_use_lb(tbl->stage->stageno, lb.second)) {
            lb_tags |= 1U << lb.first;
        }
    }
    for (auto &lb : tbl->long_branch) {
        if (can_use_lb(tbl->stage->stageno, lb.second)) {
            lb_tags |= 1U << lb.first;
        }
    }
    for (auto &n : next) {
        if (!n) continue;
        if (Target::LONG_BRANCH_TAGS() > 0 && !options.disable_long_branch) {
            if (n->stage->stageno <= tbl->stage->stageno + 1)  // local or global exec
                continue;
            auto lb_covers = [this, n](const std::pair<const int, NextTables> &lb) -> bool {
                return ((lb_tags >> lb.first) & 1) && lb.second.next.count(n);
            };
            if (std::any_of(lbrch.begin(), lbrch.end(), lb_covers)) continue;
            if (std::any_of(tbl->long_branch.begin(), tbl->long_branch.end(), lb_covers)) continue;
        }
        if (next_table_) {
            error(n.lineno, "Can't have multiple next tables for table %s", tbl->name());
            break;
        }
        next_table_ = n;
    continue2:;
    }
}

unsigned Table::NextTables::next_in_stage(int stage) const {
    unsigned rv = 0;
    for (auto &n : next)
        if (n->stage->stageno == stage) rv |= 1U << n->logical_id;
    return rv;
}

bool Table::NextTables::need_next_map_lut() const {
    BUG_CHECK(resolved);
    return next.size() > 1 || (next.size() == 1 && !next_table_);
}

void Table::NextTables::force_single_next_table() {
    BUG_CHECK(resolved);  // must be resolved already
    if (next.size() > 1)
        error(lineno,
              "Can't support multiple next tables; next is directly in overhead "
              "without using 8-entry lut");
    if (next.size() == 1) next_table_ = *next.begin();
}

int Table::table_id() const { return (stage->stageno << 4) + logical_id; }

void Table::Call::setup(const value_t &val, Table *tbl) {
    if (!CHECKTYPE2(val, tSTR, tCMD)) return;
    if (val.type == tSTR) {
        Ref::operator=(val);
        return;
    }
    Ref::operator=(val[0]);
    for (int i = 1; i < val.vec.size; i++) {
        int mode;
        if (val[i].type == tINT) {
            args.emplace_back(val[i].i);
        } else if (val[i].type == tCMD && val[i] == "hash_dist") {
            if (PCHECKTYPE(val[i].vec.size > 1, val[i][1], tINT)) {
                if (auto hd = tbl->find_hash_dist(val[i][1].i))
                    args.emplace_back(hd);
                else
                    error(val[i].lineno, "hash_dist %" PRId64 " not defined in table %s",
                          val[i][1].i, tbl->name());
            }
        } else if ((mode = StatefulTable::parse_counter_mode(val[i])) >= 0) {
            args.emplace_back(Arg::Counter, mode);
        } else if (!CHECKTYPE(val[i], tSTR)) {
            // syntax error message emit by CHEKCTYPE
        } else if (auto arg = tbl->lookup_field(val[i].s)) {
            if (arg->bits.size() != 1) error(val[i].lineno, "arg fields can't be split in format");
            args.emplace_back(arg);
        } else {
            args.emplace_back(val[i].s);
        }
    }
    lineno = val.lineno;
}

unsigned Table::Call::Arg::size() const {
    switch (type) {
        case Field:
            return fld ? fld->size : 0;
        case HashDist:
            return hd ? hd->expand >= 0 ? 23 : 16 : 0;
        case Counter:
            return 23;
        case Const:
        case Name:
            return 0;
        default:
            BUG();
    }
    return -1;
}

static void add_row(int lineno, std::vector<Table::Layout> &layout, int row) {
    layout.push_back(Table::Layout(lineno, row));
}

static int add_rows(std::vector<Table::Layout> &layout, const value_t &rows) {
    if (!CHECKTYPE2(rows, tINT, tRANGE)) return 1;
    if (rows.type == tINT) {
        add_row(rows.lineno, layout, rows.i);
    } else {
        int step = rows.lo > rows.hi ? -1 : 1;
        for (int i = rows.lo; i != rows.hi; i += step) add_row(rows.lineno, layout, i);
        add_row(rows.lineno, layout, rows.hi);
    }
    return 0;
}

static int add_col(int lineno, int stage, Table::Layout &row, int col) {
    for (auto &mu : row.memunits) {
        if (mu.stage == stage && mu.col == col) {
            error(lineno, "column %d duplicated", col);
            return 1;
        }
    }
    row.memunits.emplace_back(stage, row.row, col);
    return 0;
}

static int add_cols(int stage, Table::Layout &row, const value_t &cols) {
    int rv = 0;
    if (cols.type == tVEC) {
        if (cols.vec.size == 1) return add_cols(stage, row, cols.vec[0]);
        for (auto &col : cols.vec) {
            if (col.type == tVEC) {
                error(col.lineno, "Column shape doesn't match rows");
                rv |= 1;
            } else {
                rv |= add_cols(stage, row, col);
            }
        }
        return rv;
    }
    if (cols.type == tMAP && Target::SRAM_GLOBAL_ACCESS()) {
        bitvec stages_seen;
        for (auto &kv : cols.map) {
            if (kv.key == "stage" && kv.key.type == tCMD && kv.key[1].type == tINT)
                stage = kv.key[1].i;
            else {
                error(kv.key.lineno, "syntax error, expecting a stage number");
                continue;
            }
            if (stage < 0 || stage > Target::NUM_MAU_STAGES()) {
                error(kv.key.lineno, "stage %d out of range", stage);
            } else if (stages_seen[stage]) {
                error(kv.key.lineno, "duplicate stage %d", stage);
            } else {
                rv |= add_cols(stage, row, kv.value);
            }
        }
        return rv;
    }
    if (!CHECKTYPE2(cols, tINT, tRANGE)) return 1;
    if (cols.type == tINT) return add_col(cols.lineno, stage, row, cols.i);
    int step = cols.lo > cols.hi ? -1 : 1;
    for (int i = cols.lo; i != cols.hi; i += step) rv |= add_col(cols.lineno, stage, row, i);
    rv |= add_col(cols.lineno, stage, row, cols.hi);
    return rv;
}

static int add_stages(Table::Layout &row, const value_t &stages) {
    int rv = 0;
    if (stages.type == tVEC) {
        if (stages.vec.size == 1) return add_stages(row, stages.vec[0]);
        for (auto &stg : stages.vec) {
            if (stg.type == tVEC) {
                error(stg.lineno, "Stages shape doesn't match rows");
                rv |= 1;
            } else {
                rv |= add_stages(row, stg);
            }
        }
        return rv;
    }
    if (!CHECKTYPE2(stages, tINT, tRANGE)) return 1;
    if (stages.type == tINT) return add_col(stages.lineno, stages.i, row, 0);
    int step = stages.lo > stages.hi ? -1 : 1;
    for (int i = stages.lo; i != stages.hi; i += step) rv |= add_col(stages.lineno, i, row, 0);
    rv |= add_col(stages.lineno, stages.hi, row, 0);
    return rv;
}

std::ostream &operator<<(std::ostream &out, const Table::Layout::bus_type_t type) {
    switch (type) {
        case Table::Layout::SEARCH_BUS:
            return out << "search_bus";
        case Table::Layout::RESULT_BUS:
            return out << "result_bus";
        case Table::Layout::TIND_BUS:
            return out << "tind_bus";
        case Table::Layout::IDLE_BUS:
            return out << "idle_bus";
        case Table::Layout::L2R_BUS:
            return out << "l2r bus";
        case Table::Layout::R2L_BUS:
            return out << "r2l bus";
        default:
            return out << "[bus_t " << static_cast<int>(type) << "]";
    }
}

std::ostream &operator<<(std::ostream &out, const Table::Layout &l) {
    if (l.home_row) out << "home_";
    out << "row=" << l.row;
    for (auto [type, idx] : l.bus) out << " " << type << "=" << idx;
    if (l.word >= 0) out << " word=" << l.word;
    if (!l.memunits.empty()) {
        const char *sep = "";
        out << " [";
        for (auto &unit : l.memunits) {
            out << sep << unit;
            sep = ", ";
        }
        out << ']';
    }
    if (!l.vpns.empty()) {
        const char *sep = "";
        out << " vpns=[";
        for (auto vpn : l.vpns) {
            out << sep << vpn;
            sep = ", ";
        }
        out << ']';
    }
    if (!l.maprams.empty()) {
        const char *sep = "";
        out << " maprams=[";
        for (auto mr : l.maprams) {
            out << sep << mr;
            sep = ", ";
        }
        out << ']';
    }
    return out;
}

int Table::setup_layout_attrib(std::vector<Layout> &layout, const value_t &data, const char *what,
                               int Layout::*attr) {
    if (!CHECKTYPE2(data, tINT, tVEC)) {
        return 1;
    } else if (data.type == tVEC) {
        if (data.vec.size != static_cast<int>(layout.size())) {
            error(data.lineno, "%s shape doesn't match rows", what);
            return 1;
        } else {
            for (int i = 0; i < data.vec.size; i++) {
                if (CHECKTYPE(data.vec[i], tINT))
                    layout[i].*attr = data.vec[i].i;
                else
                    return 1;
            }
        }
    } else {
        for (auto &lrow : layout) lrow.*attr = data.i;
    }
    return 0;
}

int Table::setup_layout_bus_attrib(std::vector<Layout> &layout, const value_t &data,
                                   const char *what, Layout::bus_type_t type) {
    int limit = Target::NUM_BUS_OF_TYPE(type);
    int err = 0;
    if (limit <= 0) {
        error(data.lineno, "No %s on target %s", to_string(type).c_str(), Target::name());
        return 1;
    } else if (!CHECKTYPE2(data, tINT, tVEC)) {
        return 1;
    } else if (data.type == tVEC) {
        if (data.vec.size != static_cast<int>(layout.size())) {
            error(data.lineno, "%s shape doesn't match rows", what);
            return 1;
        } else {
            for (int i = 0; i < data.vec.size; i++) {
                if (!CHECKTYPE(data.vec[i], tINT)) return 1;
                if (data.vec[i].i >= limit) {
                    error(data.vec[i].lineno, "%" PRId64 " to large for %s", data.vec[i].i,
                          to_string(type).c_str());
                    err = 1;
                }
                if (data.vec[i].i >= 0) layout[i].bus[type] = data.vec[i].i;
            }
        }
    } else if (data.i < 0) {
        error(data.lineno, "%s value %" PRId64 " invalid", what, data.i);
        err = 1;
    } else if (data.i >= limit) {
        error(data.lineno, "%" PRId64 " to large for %s", data.i, to_string(type).c_str());
        err = 1;
    } else {
        for (auto &lrow : layout) lrow.bus[type] = data.i;
    }
    return err;
}

void Table::setup_layout(std::vector<Layout> &layout, const VECTOR(pair_t) & data,
                         const char *subname) {
    auto *row = get(data, "row");
    if (!row && this->to<AttachedTable>()) row = get(data, "logical_row");
    if (!row) {
        if (table_type() != TERNARY && Target::TABLES_REQUIRE_ROW())
            error(lineno, "No 'row' attribute in table %s%s", name(), subname);
        return;
    }
    int err = 0;
    if (row->type == tVEC)
        for (value_t &r : row->vec) err |= add_rows(layout, r);
    else
        err |= add_rows(layout, *row);
    if (err) return;
    bool global_access =
        (table_type() == TERNARY) ? Target::TCAM_GLOBAL_ACCESS() : Target::SRAM_GLOBAL_ACCESS();
    if (global_access && table_type() == TERNARY && Target::TCAM_UNITS_PER_ROW() == 1) {
        if (auto *stg = get(data, "stages")) {
            if (stg->type == tVEC && stg->vec.size == static_cast<int>(layout.size())) {
                for (int i = 0; i < stg->vec.size; i++) err |= add_stages(layout[i], stg->vec[i]);
            } else if (layout.size() == 1)
                err |= add_stages(layout[0], *stg);
        } else {
            for (auto &lrow : layout) err |= add_col(lineno, this->stage->stageno, lrow, 0);
        }
    } else if (auto *col = get(data, "column")) {
        int stage = global_access ? this->stage->stageno : INT_MIN;
        if (col->type == tMAP && global_access) {
            bitvec stages_seen;
            for (auto &kv : col->map) {
                if (kv.key.type == tINT)
                    stage = kv.key.i;
                else if (kv.key == "stage" && kv.key.type == tCMD && kv.key[1].type == tINT)
                    stage = kv.key[1].i;
                else {
                    error(kv.key.lineno, "syntax error, expecting a stage number");
                    continue;
                }
                if (stage < 0 || stage > Target::NUM_STAGES(gress)) {
                    error(kv.key.lineno, "stage %d out of range", stage);
                } else if (stages_seen[stage]) {
                    error(kv.key.lineno, "duplicate stage %d", stage);
                } else {
                    if (kv.value.type == tVEC && kv.value.vec.size + 0U == layout.size()) {
                        for (int i = 0; i < kv.value.vec.size; i++)
                            err |= add_cols(stage, layout[i], kv.value.vec[i]);
                    } else {
                        for (auto &lrow : layout)
                            if ((err |= add_cols(stage, lrow, kv.value))) break;
                    }
                }
            }
        } else if (col->type == tVEC && col->vec.size == static_cast<int>(layout.size())) {
            for (int i = 0; i < col->vec.size; i++) err |= add_cols(stage, layout[i], col->vec[i]);
        } else {
            for (auto &lrow : layout)
                if ((err |= add_cols(stage, lrow, *col))) break;
        }
    } else if (layout.size() > 1) {
        error(lineno, "No 'column' attribute in table %s%s", name(), subname);
        return;
    }
    if (auto *bus = get(data, "bus"))
        err |= Table::setup_layout_bus_attrib(layout, *bus, "Bus", default_bus_type());
    else if (auto *bus = get(data, "search_bus"))
        err |= Table::setup_layout_bus_attrib(layout, *bus, "Bus", Layout::SEARCH_BUS);
    if (auto *bus = get(data, "lhbus"))
        err |= Table::setup_layout_bus_attrib(layout, *bus, "R2L hbus", Layout::R2L_BUS);
    if (auto *bus = get(data, "rhbus"))
        err |= Table::setup_layout_bus_attrib(layout, *bus, "L2R hbus", Layout::L2R_BUS);
    if (auto *bus = get(data, "result_bus"))
        err |= Table::setup_layout_bus_attrib(layout, *bus, "Bus", Layout::RESULT_BUS);
    if (auto *word = get(data, "word"))
        err |= Table::setup_layout_attrib(layout, *word, "Word", &Layout::word);
    if (err) return;
    for (auto i = layout.begin(); i != layout.end(); i++)
        for (auto j = i + 1; j != layout.end(); j++)
            if (*i == *j) {
                std::stringstream bus;
                if (!i->bus.empty())
                    bus << " " << i->bus.begin()->first << " " << i->bus.begin()->second;
                error(i->lineno, "row %d%s duplicated in table %s%s", i->row, bus.str().c_str(),
                      name(), subname);
            }
}

void Table::setup_logical_id() {
    if (logical_id >= 0) {
        if (Table *old = stage->logical_id_use[logical_id]) {
            error(lineno, "table %s wants logical id %d:%d", name(), stage->stageno, logical_id);
            error(old->lineno, "already in use by %s", old->name());
        }
        stage->logical_id_use[logical_id] = this;
    }
}

void Table::setup_maprams(value_t &v) {
    if (!CHECKTYPE2(v, tINT, tVEC)) return;
    VECTOR(value_t) *rams = &v.vec, single_ram;
    if (v.type == tINT) {
        // treat as a vector of length 1
        rams = &single_ram;
        single_ram.size = single_ram.capacity = 1;
        single_ram.data = &v;
    }
    auto r = rams->begin();
    for (auto &row : layout) {
        if (r == rams->end()) {
            error(r->lineno, "Mapram layout doesn't match table layout");
            break;
        }
        auto &maprow = *r++;
        VECTOR(value_t) * maprow_rams, tmp;
        if (maprow.type == tINT) {
            if (layout.size() == 1) {
                maprow_rams = rams;
            } else {
                // treat as a vector of length 1
                maprow_rams = &tmp;
                tmp.size = tmp.capacity = 1;
                tmp.data = &maprow;
            }
        } else if (CHECKTYPE(maprow, tVEC)) {
            maprow_rams = &maprow.vec;
        } else {
            continue;
        }
        if (maprow_rams->size != static_cast<int>(row.memunits.size())) {
            error(r->lineno, "Mapram layout doesn't match table layout");
            continue;
        }
        for (auto mapcol : *maprow_rams)
            if (CHECKTYPE(mapcol, tINT)) {
                if (mapcol.i < 0 || mapcol.i >= MAPRAM_UNITS_PER_ROW)
                    error(mapcol.lineno, "Invalid mapram column %" PRId64 "", mapcol.i);
                else
                    row.maprams.push_back(mapcol.i);
            }
    }
}

/**
 * Guarantees that the instruction call provided to the table has valid entries, and that
 * if multiple choices are required, the compiler can make that choices.
 *
 * The instruction address is a two piece address.  The first argument is the address bits
 * location.  The second argument is a per flow enable bit location.  These are both required.
 * Additionally, the keyword $DEFAULT means that that particular portion of the address comes
 * from the default register.
 *
 * FIXME -- this code is a messy hack -- various target-specific special cases.  Should try
 * to figure out a better way to organize this.
 */
bool Table::validate_instruction(Table::Call &call) const {
    if (call.args.size() != 2) {
        error(call.lineno, "Instruction call has invalid number of arguments");
        return false;
    }

    bool field_address = false;

    if (call.args[0].name()) {
        if (Target::GATEWAY_INHIBIT_INDEX() && call.args[0] == "$GATEWAY_IDX") {
            field_address = true;
        } else if (call.args[0] != "$DEFAULT") {
            error(call.lineno, "Index %s for %s cannot be found", call.args[0].name(),
                  call->name());
            return false;
        }
    } else if (!call.args[0].field()) {
        error(call.lineno, "Index for %s cannot be understood", call->name());
        return false;
    } else {
        field_address = true;
    }

    if (call.args[1].name()) {
        if (call.args[1] != "$DEFAULT") {
            error(call.lineno, "Per flow enable %s for %s cannot be found", call.args[1].name(),
                  call->name());
            return false;
        }
    } else if (!call.args[1].field()) {
        error(call.lineno, "Per flow enable for %s cannot be understood", call->name());
        return false;
    }

    if (actions->hit_actions_count() > 1 && !field_address)
        error(lineno, "No field to select between multiple action in table %s format", name());

    return true;
}

static bool column_match(const std::vector<MemUnit> &a, const std::vector<MemUnit> &b) {
    auto it = b.begin();
    for (auto &u : a) {
        if (it == b.end()) return false;
        if (u.col != it->col) return false;
        ++it;
    }
    return it == b.end();
}

void Table::setup_vpns(std::vector<Layout> &layout, VECTOR(value_t) * vpn, bool allow_holes) {
    int period, width, depth;
    const char *period_name;
    vpn_params(width, depth, period, period_name);
    int word = width;
    Layout *firstrow = 0;
    auto vpniter = vpn ? vpn->begin() : 0;
    int *vpn_ctr = new int[period];
    std::fill_n(vpn_ctr, period, get_start_vpn());
    std::vector<bitvec> used_vpns(period);
    bool on_repeat = false;
    for (auto &row : layout) {
        if (++word < width) {
            BUG_CHECK(firstrow);
            if (!column_match(row.memunits, firstrow->memunits))
                error(row.lineno, "Columns across wide rows don't match in table %s", name());
            row.vpns = firstrow->vpns;
            continue;
        }
        word = 0;
        firstrow = &row;
        row.vpns.resize(row.memunits.size());
        value_t *vpncoliter = 0;
        for (int &el : row.vpns) {
            // If VPN's are provided by the compiler, they need to match each
            // element in the specified columns. Below code checks if all
            // elements are present and errors out if there is any discrepancy.
            if (vpniter) {
                if (vpniter == vpn->end()) {
                    on_repeat = true;
                    vpniter = vpn->begin();
                }
                if (CHECKTYPE2(*vpniter, tVEC, tINT)) {
                    if (vpniter->type == tVEC) {
                        if (!vpncoliter) {
                            if (static_cast<int>(row.vpns.size()) != vpniter->vec.size) {
                                error(vpniter->lineno,
                                      "Vpn entries for row %d is %d not equal to column "
                                      "entries %d",
                                      row.row, vpniter->vec.size,
                                      static_cast<int>(row.vpns.size()));
                                continue;
                            } else {
                                vpncoliter = vpniter->vec.begin();
                            }
                        }
                        el = vpncoliter->i;
                        if (++vpncoliter == &*vpniter->vec.end()) ++vpniter;
                        continue;
                    } else if (vpniter->type == tINT) {
                        el = vpniter->i;
                    }
                    ++vpniter;
                }
                // Error out if VPN's are repeated in a table. For wide words,
                // each individual word can have the same vpn
                if (!on_repeat && used_vpns[period - 1][el].set(true))
                    error(vpniter->lineno, "Vpn %d used twice in table %s", el, name());
            } else {
                // If there is no word information provided in assembly (Ternary
                // Indirect/Stats) tables, the allocation is always a single
                // word.
                // For SRamMatchTables, this should be handled by SRamMatchTable::alloc_vpns(),
                // so this code will never be hit
                // FIXME -- move this to Table::alloc_vpns and only call setup_vpns when
                // there's a vpn specified in the bfa?
                if (row.word < 0) row.word = word;
                el = vpn_ctr[row.word];
                if ((vpn_ctr[row.word] += period) == depth) vpn_ctr[row.word] = 0;
            }
        }
    }
    delete[] vpn_ctr;
}

void Table::common_init_setup(const VECTOR(pair_t) & data, bool, P4Table::type) {
    setup_layout(layout, data);
    if (auto *fmt = get(data, "format")) {
        if (CHECKTYPEPM(*fmt, tMAP, fmt->map.size > 0, "non-empty map"))
            format.reset(new Format(this, fmt->map));
    }
    if (auto *hd = get(data, "hash_dist")) HashDistribution::parse(hash_dist, *hd);
}

bool Table::common_setup(pair_t &kv, const VECTOR(pair_t) & data, P4Table::type p4type) {
    bool global_access =
        (table_type() == TERNARY) ? Target::TCAM_GLOBAL_ACCESS() : Target::SRAM_GLOBAL_ACCESS();
    if (kv.key == "format" || kv.key == "row" || kv.key == "column" || kv.key == "bus") {
        /* done in Table::common_init_setup */
    } else if (global_access && (kv.key == "stages" || kv.key == "lhbus" || kv.key == "rhbus")) {
        /* done in Table::common_init_setup */
    } else if (kv.key == "action") {
        action.setup(kv.value, this);
    } else if (kv.key == "instruction") {
        instruction.setup(kv.value, this);
    } else if (kv.key == "action_enable") {
        if (CHECKTYPE(kv.value, tINT)) action_enable = kv.value.i;
        if (get(data, "action")) enable_action_data_enable = true;
        enable_action_instruction_enable = true;
    } else if (kv.key == "enable_action_data_enable") {
        enable_action_data_enable = get_bool(kv.value);
    } else if (kv.key == "enable_action_instruction_enable") {
        enable_action_instruction_enable = get_bool(kv.value);
    } else if (kv.key == "actions") {
        if (CHECKTYPE(kv.value, tMAP)) actions.reset(new Actions(this, kv.value.map));
    } else if (kv.key == "action_bus") {
        if (CHECKTYPE(kv.value, tMAP)) action_bus = ActionBus::create(this, kv.value.map);
    } else if ((kv.key == "default_action") || (kv.key == "default_only_action")) {
        if (kv.key == "default_only_action") default_only_action = true;
        default_action_lineno = kv.value.lineno;
        if (CHECKTYPE2(kv.value, tSTR, tCMD))
            if (CHECKTYPE(kv.value, tSTR)) default_action = kv.value.s;
    } else if (kv.key == "default_action_parameters") {
        if (CHECKTYPE(kv.value, tMAP))
            for (auto &v : kv.value.map)
                if (CHECKTYPE(v.key, tSTR) && CHECKTYPE(v.value, tSTR))
                    default_action_parameters[v.key.s] = v.value.s;
    } else if (kv.key == "default_action_handle") {
        default_action_handle = kv.value.i;
    } else if (kv.key == "hit") {
        if (!hit_next.empty()) {
            error(kv.key.lineno, "Specifying both 'hit' and 'next' in table %s", name());
        } else if (kv.value.type == tVEC) {
            for (auto &v : kv.value.vec) hit_next.emplace_back(v);
        } else {
            hit_next.emplace_back(kv.value);
        }
    } else if (kv.key == "miss") {
        if (miss_next.set()) {
            error(kv.key.lineno, "Specifying both 'miss' and 'next' in table %s", name());
        } else {
            miss_next = kv.value;
        }
    } else if (kv.key == "next") {
        if (!hit_next.empty()) {
            error(kv.key.lineno, "Specifying both 'hit' and 'next' in table %s", name());
        } else if (miss_next.set()) {
            error(kv.key.lineno, "Specifying both 'miss' and 'next' in table %s", name());
        } else {
            miss_next = kv.value;
            hit_next.emplace_back(miss_next);
        }
    } else if (kv.key == "long_branch" && Target::LONG_BRANCH_TAGS() > 0) {
        if (options.disable_long_branch) error(kv.key.lineno, "long branches disabled");
        if (CHECKTYPE(kv.value, tMAP)) {
            for (auto &lb : kv.value.map) {
                if (lb.key.type != tINT || lb.key.i < 0 || lb.key.i >= Target::LONG_BRANCH_TAGS())
                    error(lb.key.lineno, "Invalid long branch tag %s", value_desc(lb.key));
                else if (long_branch.count(lb.key.i))
                    error(lb.key.lineno, "Duplicate long branch tag %" PRId64, lb.key.i);
                else
                    long_branch.emplace(lb.key.i, lb.value);
            }
        }
    } else if (kv.key == "vpns") {
        if (CHECKTYPESIZE(kv.value, tVEC)) setup_vpns(layout, &kv.value.vec);
    } else if (kv.key == "p4") {
        if (CHECKTYPE(kv.value, tMAP)) p4_table = P4Table::get(p4type, kv.value.map);
    } else if (kv.key == "p4_param_order") {
        if (CHECKTYPE(kv.value, tMAP)) {
            unsigned position = 0;
            for (auto &v : kv.value.map) {
                if ((CHECKTYPE(v.key, tSTR)) && (CHECKTYPE(v.value, tMAP))) {
                    p4_param p(v.key.s);
                    for (auto &w : v.value.map) {
                        if (!CHECKTYPE(w.key, tSTR)) continue;

                        if (w.key == "type" && CHECKTYPE(w.value, tSTR))
                            p.type = w.value.s;
                        else if (w.key == "size" && CHECKTYPE(w.value, tINT))
                            p.bit_width = w.value.i;
                        else if (w.key == "full_size" && CHECKTYPE(w.value, tINT))
                            p.bit_width_full = w.value.i;
                        else if (w.key == "mask")
                            p.mask = get_bitvec(w.value);
                        else if (w.key == "alias" && CHECKTYPE(w.value, tSTR))
                            p.alias = w.value.s;
                        else if (w.key == "key_name" && CHECKTYPE(w.value, tSTR))
                            p.key_name = w.value.s;
                        else if (w.key == "start_bit" && CHECKTYPE(w.value, tINT))
                            p.start_bit = w.value.i;
                        else if (w.key == "context_json" && CHECKTYPE(w.value, tMAP))
                            p.context_json = toJson(w.value.map);
                        else
                            error(lineno, "Incorrect param type %s in p4_param_order", w.key.s);
                    }
                    // Determine position in p4_param_order. Repeated fields get
                    // the same position which is set on first occurrence.
                    // Driver relies on position to order fields. The case when
                    // we have multiple slices of same field based on position
                    // only one location is assigned for the entire field.
                    // However if the field has a name annotation (key_name)
                    // this overrides the position even if the field belongs to
                    // the same slice.
                    bool ppFound = false;
                    for (auto &pp : p4_params_list) {
                        if ((pp.name == p.name) && (pp.key_name == p.key_name)) {
                            ppFound = true;
                            p.position = pp.position;
                            break;
                        }
                    }
                    if (!ppFound) p.position = position++;
                    p4_params_list.emplace_back(std::move(p));
                }
            }
        }
    } else if (kv.key == "context_json") {
        setup_context_json(kv.value);
    } else {
        return false;
    }
    return true;
}

void Table::setup_context_json(value_t &v) {
    if (!CHECKTYPE(v, tMAP)) return;

    auto map = toJson(v.map);
    if (context_json)
        context_json->merge(*map);
    else
        context_json = std::move(map);
}

/** check two tables to see if they can share ram.
 * FIXME -- for now we just allow a STATEFUL and a SELECTION to share -- we should
 * FIXME -- check to make sure they're mutually exclusive and use the memory in
 * FIXME -- a compatible way
 */
bool Table::allow_ram_sharing(const Table *t1, const Table *t2) {
    if (t1->table_type() == STATEFUL && t2->table_type() == SELECTION &&
        t1->to<StatefulTable>()->bound_selector == t2)
        return true;
    if (t2->table_type() == STATEFUL && t1->table_type() == SELECTION &&
        t2->to<StatefulTable>()->bound_selector == t1)
        return true;
    return false;
}

/** check two tables to see if they can share action bus
 * Two ATCAM tables or their action tables can occur in the same stage and share
 * bytes on the action bus which is valid as they are always mutually exclusive
 */
bool Table::allow_bus_sharing(Table *t1, Table *t2) {
    if (!t1 || !t2) return false;
    if ((t1->table_type() == ATCAM) && (t2->table_type() == ATCAM) &&
        (t1->p4_name() == t2->p4_name()))
        return true;
    if ((t1->table_type() == ACTION) && (t2->table_type() == ACTION) &&
        (t1->p4_name() == t2->p4_name())) {
        // Check if action tables are attached to atcam's
        auto *m1 = t1->to<ActionTable>()->get_match_table();
        auto *m2 = t2->to<ActionTable>()->get_match_table();
        if (m1 && m2) {
            if ((m1->table_type() == ATCAM) && (m2->table_type() == ATCAM)) return true;
        }
    }
    return false;
}

void Table::alloc_rams(bool logical, BFN::Alloc2Dbase<Table *> &use,
                       BFN::Alloc2Dbase<Table *> *bus_use, Layout::bus_type_t bus_type) {
    for (auto &row : layout) {
        for (auto &memunit : row.memunits) {
            BUG_CHECK(memunit.stage == INT_MIN && memunit.row == row.row, "memunit fail");
            int r = row.row, c = memunit.col;
            if (logical) {
                c += 6 * (r & 1);
                r >>= 1;
            }
            try {
                if (Table *old = use[r][c]) {
                    if (!allow_ram_sharing(this, old)) {
                        error(lineno,
                              "Table %s trying to use (%d,%d) which is already in use "
                              "by table %s",
                              name(), r, c, old->name());
                    }
                } else {
                    use[r][c] = this;
                }
            } catch (std::out_of_range) {
                error(lineno, "Table %s using out-of-bounds (%d,%d)", name(), r, c);
            }
        }
        if (bus_use && row.bus.count(bus_type)) {
            int bus = row.bus.at(bus_type);
            if (Table *old = (*bus_use)[row.row][bus]) {
                if (old != this && old->p4_name() != p4_name())
                    error(lineno,
                          "Table %s trying to use bus %d on row %d which is already in "
                          "use by table %s",
                          name(), bus, row.row, old->name());
            } else {
                (*bus_use)[row.row][bus] = this;
            }
        }
    }
}

void Table::alloc_global_busses() { BUG(); }
void Table::alloc_global_srams() { BUG(); }
void Table::alloc_global_tcams() { BUG(); }

void Table::alloc_busses(BFN::Alloc2Dbase<Table *> &bus_use, Layout::bus_type_t bus_type) {
    for (auto &row : layout) {
        // If row.memunits is empty, we don't really need a bus here (won't use it
        // for anything).
        // E.g. An exact match table with 4 or less static entries (JBay) or 1
        // static entry (Tofino)
        // In these examples compiler does gateway optimization where static
        // entries are encoded in the gateway and no RAM's are used. We skip bus
        // allocation in these cases.
        if (!row.bus.count(bus_type) && !row.memunits.empty()) {
            // FIXME -- iterate over bus_use[row.row] rather than assuming 2 rows
            if (bus_use[row.row][0] == this)
                row.bus[bus_type] = 0;
            else if (bus_use[row.row][1] == this)
                row.bus[bus_type] = 1;
            else if (!bus_use[row.row][0])
                bus_use[row.row][row.bus[bus_type] = 0] = this;
            else if (!bus_use[row.row][1])
                bus_use[row.row][row.bus[bus_type] = 1] = this;
            else
                error(lineno, "No bus available on row %d for table %s", row.row, name());
        }
    }
}

void Table::alloc_id(const char *idname, int &id, int &next_id, int max_id, bool order,
                     BFN::Alloc1Dbase<Table *> &use) {
    if (id >= 0) {
        next_id = id;
        return;
    }
    while (++next_id < max_id && use[next_id]) {
    }
    if (next_id >= max_id && !order) {
        next_id = -1;
        while (++next_id < max_id && use[next_id]) {
        }
    }
    if (next_id < max_id)
        use[id = next_id] = this;
    else
        error(lineno, "Can't pick %s id for table %s (ran out)", idname, name());
}

void Table::alloc_maprams() {
    if (!Target::SYNTH2PORT_NEED_MAPRAMS()) return;
    for (auto &row : layout) {
        int sram_row = row.row / 2;
        if ((row.row & 1) == 0) {
            error(row.lineno, "Can only use 2-port rams on right side srams (odd logical rows)");
            continue;
        }
        if (row.maprams.empty()) {
            int use = 0;
            for (unsigned i = 0; i < row.memunits.size(); i++) {
                while (use < MAPRAM_UNITS_PER_ROW && stage->mapram_use[sram_row][use]) use++;
                if (use >= MAPRAM_UNITS_PER_ROW) {
                    error(row.lineno, "Ran out of maprams on row %d in stage %d", sram_row,
                          stage->stageno);
                    break;
                }
                row.maprams.push_back(use);
                stage->mapram_use[sram_row][use++] = this;
            }
        } else {
            for (auto mapcol : row.maprams) {
                if (auto *old = stage->mapram_use[sram_row][mapcol]) {
                    if (!allow_ram_sharing(this, old))
                        error(lineno,
                              "Table %s trying to use mapram %d,%d which is use by "
                              "table %s",
                              name(), sram_row, mapcol, old->name());
                } else {
                    stage->mapram_use[sram_row][mapcol] = this;
                }
            }
        }
    }
}

void Table::alloc_vpns() {
    if (no_vpns || layout_size() == 0 || layout[0].vpns.size() > 0) return;
    setup_vpns(layout, 0);
}

void Table::check_next(const Table::Ref &n) {
    if (n.check()) {
        if (logical_id >= 0 && n->logical_id >= 0 ? table_id() > n->table_id()
                                                  : stage->stageno > n->stage->stageno)
            error(n.lineno, "Next table %s comes before %s", n->name(), name());
        if (gress != n->gress)
            error(n.lineno, "Next table %s in %s when %s is in %s", n->name(),
                  P4Table::direction_name(n->gress).c_str(), name(),
                  P4Table::direction_name(gress).c_str());
        // Need to add to the predication map
        Table *tbl = get_match_table();
        if (!tbl) tbl = this;  // standalone gateway
        if (tbl != n) {
            n->pred[tbl];  // ensure that its in the map, even as an empty set
        }
    }
}

void Table::for_all_next(std::function<void(const Ref &)> fn) {
    for (auto &n1 : hit_next)
        for (auto &n2 : n1) fn(n2);
    for (auto &n : miss_next) fn(n);
}

void Table::check_next(NextTables &next) {
    for (auto &n : next) check_next(n);
    Table *tbl = get_match_table();
    if (!tbl) tbl = this;
    next.resolve_long_branch(tbl, long_branch);
}

void Table::check_next() {
    for (auto &lb : long_branch) {
        for (auto &t : lb.second) {
            if (t.check()) {
                if (t->stage->stageno <= stage->stageno)
                    error(t.lineno, "Long branch table %s is not in a later stage than %s",
                          t->name(), name());
                else if (stage->stageno + 1 == t->stage->stageno)
                    warning(t.lineno, "Long branch table %s is the next stage after %s", t->name(),
                            name());
                if (gress != t->gress)
                    error(t.lineno, "Long branch table %s in %s when %s is in %s", t->name(),
                          P4Table::direction_name(t->gress).c_str(), name(),
                          P4Table::direction_name(gress).c_str());
            }
        }
    }
    for (auto &hn : hit_next) check_next(hn);
    for (auto &hn : extra_next_lut) check_next(hn);
    check_next(miss_next);
}

void Table::set_pred() {
    if (actions == nullptr) return;
    for (auto &act : *actions) {
        if (!act.default_only)
            for (auto &n : act.next_table_ref) n->pred[this].insert(&act);
        for (auto &n : act.next_table_miss_ref) n->pred[this].insert(&act);
    }
}

bool Table::choose_logical_id(const slist<Table *> *work) {
    if (logical_id >= 0) return true;
    if (work && find(*work, this) != work->end()) {
        error(lineno, "Logical table loop with table %s", name());
        for (auto *tbl : *work) {
            if (tbl == this) break;
            warning(tbl->lineno, "loop involves table %s", tbl->name());
        }
        return false;
    }
    slist<Table *> local(this, work);
    for (auto *p : Keys(pred))
        if (!p->choose_logical_id(&local)) return false;
    int min_id = 0, max_id = LOGICAL_TABLES_PER_STAGE - 1;
    for (auto *p : Keys(pred))
        if (p->stage->stageno == stage->stageno && p->logical_id >= min_id)
            min_id = p->logical_id + 1;
    for_all_next([&max_id, this](const Ref &n) {
        if (n && n->stage->stageno == stage->stageno && n->logical_id >= 0 &&
            n->logical_id <= max_id) {
            max_id = n->logical_id - 1;
        }
    });
    for (int id = min_id; id <= max_id; ++id) {
        if (!stage->logical_id_use[id]) {
            logical_id = id;
            stage->logical_id_use[id] = this;
            return true;
        }
    }
    error(lineno, "Can't find a logcial id for table %s", name());
    return false;
}

void Table::need_bus(int lineno, BFN::Alloc1Dbase<Table *> &use, int idx, const char *busname) {
    if (use[idx] && use[idx] != this) {
        error(lineno, "%s bus conflict on row %d between tables %s and %s", busname, idx, name(),
              use[idx]->name());
        error(use[idx]->lineno, "%s defined here", use[idx]->name());
    } else {
        use[idx] = this;
    }
}

bitvec Table::compute_reachable_tables() {
    reachable_tables_[uid] = 1;
    for_all_next([this](const Ref &t) {
        if (t) {
            reachable_tables_ |= t->reachable_tables();
        }
    });
    return reachable_tables_;
}

std::string Table::loc() const {
    std::stringstream ss;
    ss << "(" << gress << ", stage=" << stage->stageno << ")";
    return ss.str();
}

void Table::pass1() {
    alloc_vpns();
    check_next();
    if (auto att = get_attached()) att->pass1(get_match_table());
    if (action_bus) action_bus->pass1(this);

    if (actions) {
        if (instruction) {
            validate_instruction(instruction);
        } else {
            // Phase0 has empty actions which list param order
            if (table_type() != PHASE0) {
                error(lineno, "No instruction call provided, but actions provided");
            }
        }
        actions->pass1(this);
    }
    set_pred();

    if (action) {
        auto reqd_args = 2;
        action->validate_call(action, get_match_table(), reqd_args,
                              HashDistribution::ACTION_DATA_ADDRESS, action);
    }
    for (auto &lb : long_branch) {
        int last_stage = -1;
        for (auto &n : lb.second) {
            if (!n) continue;  // already output error about invalid table
            last_stage = std::max(last_stage, n->stage->stageno);
            if (n->long_branch_input >= 0 && n->long_branch_input != lb.first)
                error(lb.second.lineno, "Conflicting long branch input (%d and %d) for table %s",
                      lb.first, n->long_branch_input, n->name());
            n->long_branch_input = lb.first;
        }
        // we track the long branch as being 'live' from the stage it is set until the stage
        // before it is terminated; it can still be use to trigger a table in that stage, even
        // though it is not 'live' there.  It can also be reused (set) in that stage for use in
        // later stages.  This matches the range of stages we need to set timing regs for.
        for (int st = stage->stageno; st < last_stage; ++st) {
            auto stg = Stage::stage(gress, st);
            BUG_CHECK(stg);
            auto &prev = stg->long_branch_use[lb.first];
            if (prev && *prev != lb.second) {
                error(lb.second.lineno, "Conflicting use of long_branch tag %d", lb.first);
                error(prev->lineno, "previous use");
            } else {
                prev = &lb.second;
            }
            stg->long_branch_thread[gress] |= 1U << lb.first;
        }
        auto last_stg = Stage::stage(gress, last_stage);
        BUG_CHECK(last_stg);
        last_stg->long_branch_thread[gress] |= 1U << lb.first;
        last_stg->long_branch_terminate |= 1U << lb.first;
    }
}

static void overlap_test(int lineno, unsigned a_bit,
                         ordered_map<std::string, Table::Format::Field>::iterator a, unsigned b_bit,
                         ordered_map<std::string, Table::Format::Field>::iterator b) {
    if (b_bit <= a->second.hi(a_bit)) {
        if (a->second.group || b->second.group)
            error(lineno, "Field %s(%d) overlaps with %s(%d)", a->first.c_str(), a->second.group,
                  b->first.c_str(), b->second.group);
        else
            error(lineno, "Field %s overlaps with %s", a->first.c_str(), b->first.c_str());
    }
}

static void append_bits(std::vector<Table::Format::bitrange_t> &vec, int lo, int hi) {
    /* split any chunks that cross a word (128-bit) boundary */
    while (lo < hi && lo / 128U != hi / 128U) {
        vec.emplace_back(lo, lo | 127);
        lo = (lo | 127) + 1;
    }
    vec.emplace_back(lo, hi);
}

bool Table::Format::equiv(const ordered_map<std::string, Field> &a,
                          const ordered_map<std::string, Field> &b) {
    if (a.size() != b.size()) return false;
    for (auto &el : a)
        if (!b.count(el.first) || b.at(el.first) != el.second) return false;
    return true;
}

Table::Format::Format(Table *t, const VECTOR(pair_t) & data, bool may_overlap) : tbl(t) {
    unsigned nextbit = 0;
    fmt.resize(1);
    for (auto &kv : data) {
        if (lineno < 0) lineno = kv.key.lineno;
        if (!CHECKTYPE2M(kv.key, tSTR, tCMD, "expecting field desc")) continue;
        value_t &name = kv.key.type == tSTR ? kv.key : kv.key[0];
        unsigned idx = 0;
        if (kv.key.type == tCMD &&
            (kv.key.vec.size != 2 || !CHECKTYPE(kv.key[1], tINT) || (idx = kv.key[1].i) > 15)) {
            error(kv.key.lineno, "Invalid field group");
            continue;
        }
        if (kv.value.type != tVEC &&
            !(CHECKTYPE2(kv.value, tINT, tRANGE) && VALIDATE_RANGE(kv.value)))
            continue;
        if (idx >= fmt.size()) fmt.resize(idx + 1);
        if (fmt[idx].count(name.s) > 0) {
            if (kv.key.type == tCMD)
                error(name.lineno, "Duplicate key %s(%d) in format", name.s, idx);
            else
                error(name.lineno, "Duplicate key %s in format", name.s);
            continue;
        }
        Field *f = &fmt[idx].emplace(name.s, Field(this)).first->second;
        f->group = idx;
        if (kv.value.type == tINT) {
            if (kv.value.i <= 0)
                error(kv.value.lineno, "invalid size %" PRId64 " for format field %s", kv.value.i,
                      name.s);
            f->size = kv.value.i;
            append_bits(f->bits, nextbit, nextbit + f->size - 1);
        } else if (kv.value.type == tRANGE) {
            if (kv.value.lo > kv.value.hi)
                error(kv.value.lineno, "invalid range %d..%d", kv.value.lo, kv.value.hi);
            append_bits(f->bits, kv.value.lo, kv.value.hi);
            f->size = kv.value.hi - kv.value.lo + 1;
        } else if (kv.value.type == tVEC) {
            f->size = 0;
            for (auto &c : kv.value.vec)
                if (CHECKTYPE(c, tRANGE) && VALIDATE_RANGE(c)) {
                    append_bits(f->bits, c.lo, c.hi);
                    f->size += c.hi - c.lo + 1;
                    if ((size_t)c.hi + 1 > size) size = c.hi + 1;
                }
        }
        nextbit = f->bits.back().hi + 1;
        if (nextbit > size) size = nextbit;
    }
    if (!may_overlap) {
        for (auto &grp : fmt) {
            for (auto it = grp.begin(); it != grp.end(); ++it) {
                for (auto &piece : it->second.bits) {
                    auto p = byindex.upper_bound(piece.lo);
                    if (p != byindex.end()) overlap_test(lineno, piece.lo, it, p->first, p->second);
                    if (p != byindex.begin()) {
                        --p;
                        overlap_test(lineno, p->first, p->second, piece.lo, it);
                        if (p->first == piece.lo && piece.hi <= p->second->second.hi(piece.lo))
                            continue;
                    }
                    byindex[piece.lo] = it;
                }
            }
        }
    }
    for (size_t i = 1; i < fmt.size(); i++)
        if (!equiv(fmt[0], fmt[i]))
            error(data[0].key.lineno, "Format group %zu doesn't match group 0", i);
    for (log2size = 0; (1U << log2size) < size; log2size++) {
    }
    if (error_count > 0) return;
    for (auto &f : fmt[0]) {
        f.second.by_group = new Field *[fmt.size()];
        f.second.by_group[0] = &f.second;
    }
    for (size_t i = 1; i < fmt.size(); i++)
        for (auto &f : fmt[i]) {
            Field &f0 = fmt[0].at(f.first);
            f.second.by_group = f0.by_group;
            f.second.by_group[i] = &f.second;
        }
}

Table::Format::~Format() {
    for (auto &f : fmt[0]) delete[] f.second.by_group;
}

void Table::Format::pass1(Table *tbl) {
    std::map<unsigned, Field *> immed_fields;
    unsigned lo = INT_MAX, hi = 0;
    for (auto &f : fmt[0]) {
        if (!(f.second.flags & Field::USED_IMMED)) continue;
        if (f.second.bits.size() > 1)
            error(lineno, "Immmediate action data %s cannot be split", f.first.c_str());
        immed_fields[f.second.bits[0].lo] = &f.second;
        if (f.second.bits[0].lo < lo) {
            immed = &f.second;
            lo = immed->bits[0].lo;
        }
        if (f.second.bits[0].hi > hi) hi = f.second.bits[0].hi;
    }
    if (immed_fields.empty()) {
        LOG2("table " << tbl->name() << " has no immediate data");
    } else {
        LOG2("table " << tbl->name() << " has " << immed_fields.size()
                      << " immediate data fields "
                         "over "
                      << (hi + 1 - lo) << " bits");
        if (hi - lo >= Target::MAX_IMMED_ACTION_DATA()) {
            error(lineno, "Immediate data for table %s spread over more than %d bits", tbl->name(),
                  Target::MAX_IMMED_ACTION_DATA());
            return;
        }
        immed_size = hi + 1 - lo;
        for (unsigned i = 1; i < fmt.size(); i++) {
            int delta = static_cast<int>(immed->by_group[i]->bits[0].lo) -
                        static_cast<int>(immed->bits[0].lo);
            for (auto &f : fmt[0]) {
                if (!(f.second.flags & Field::USED_IMMED)) continue;
                if (delta != static_cast<int>(f.second.by_group[i]->bits[0].lo) -
                                 static_cast<int>(f.second.bits[0].lo)) {
                    error(lineno,
                          "Immediate data field %s for table %s does not match across "
                          "ways in a ram",
                          f.first.c_str(), tbl->name());
                    break;
                }
            }
        }
    }
    lo = INT_MAX, hi = 0;
    for (auto &[name, field] : fmt[0]) {
        // FIXME -- should use a flag rather than names here?  Someone would need to set the flag
        if (name == "match" || name == "version" || name == "valid") continue;
        lo = std::min(lo, field.bit(0));
        hi = std::max(hi, field.bit(field.size - 1));
    }
    overhead_size = hi > lo ? hi - lo + 1 : 0;
    overhead_start = hi > lo ? lo : 0;
}

void Table::Format::pass2(Table *tbl) {
    int byte[4] = {-1, -1, -1, -1};
    int half[2] = {-1, -1};
    int word = -1;
    bool err = false;
    for (auto &f : fmt[0]) {
        int byte_slot = tbl->find_on_actionbus(&f.second, 0, 8 * f.second.size - 1, f.second.size);
        if (byte_slot < 0) continue;
        int slot = Stage::action_bus_slot_map[byte_slot];
        unsigned off = f.second.immed_bit(0);
        switch (Stage::action_bus_slot_size[slot]) {
            case 8:
                for (unsigned b = off / 8; b <= (off + f.second.size - 1) / 8; b++) {
                    if (b >= 4 || (b & 3) != (slot & 3) || (byte[b] >= 0 && byte[b] != slot) ||
                        (byte[b ^ 1] >= 0 && byte[b ^ 1] != (slot ^ 1)) ||
                        Stage::action_bus_slot_size[slot] != 8) {
                        err = true;
                        break;
                    }
                    byte[b] = slot++;
                }
                break;
            case 16:
                for (unsigned w = off / 16; w <= (off + f.second.size - 1) / 16; w++) {
                    if (w >= 2 || (w & 1) != (slot & 1) || (half[w] >= 0 && half[w] != slot) ||
                        Stage::action_bus_slot_size[slot] != 16) {
                        err = true;
                        break;
                    }
                    half[w] = slot++;
                }
                break;
            case 32:
                if (word >= 0 && word != slot) err = true;
                word = slot;
                break;
            default:
                BUG();
        }
        if (err) error(lineno, "Immediate data misaligned for action bus byte %d", byte_slot);
    }
}

std::ostream &operator<<(std::ostream &out, const Table::Format::Field &f) {
    out << "(size = " << f.size << " ";
    for (auto b : f.bits) out << "[" << b.lo << ".." << b.hi << "]";
    out << ")";
    return out;
}

bool Table::Actions::Action::equiv(Action *a) {
    if (instr.size() != a->instr.size()) return false;
    for (unsigned i = 0; i < instr.size(); i++)
        if (!instr[i]->equiv(a->instr[i])) return false;
    if (attached.size() != a->attached.size()) return false;
    for (unsigned i = 0; i < attached.size(); i++)
        if (attached[i] != a->attached[i]) return false;
    return true;
}

bool Table::Actions::Action::equivVLIW(Action *a) {
    if (instr.size() != a->instr.size()) return false;
    for (unsigned i = 0; i < instr.size(); i++)
        if (!instr[i]->equiv(a->instr[i])) return false;
    return true;
}

std::map<std::string, std::vector<Table::Actions::Action::alias_value_t *>>
Table::Actions::Action::reverse_alias() const {
    std::map<std::string, std::vector<alias_value_t *>> rv;
    for (auto &a : alias) rv[a.second.name].push_back(&a);
    return rv;
}

std::string Table::Actions::Action::alias_lookup(int lineno, std::string name, int &lo,
                                                 int &hi) const {
    bool err = false;
    bool found = false;
    while (alias.count(name) && !found) {
        for (auto &a : ValuesForKey(alias, name)) {
            // FIXME -- need better handling of multiple aliases...
            if (lo >= 0 && a.name != "hash_dist") {
                if (a.lo >= 0) {
                    if (a.hi >= 0 && hi + a.lo > a.hi) {
                        err = true;
                        continue;
                    }
                    lo += a.lo;
                    hi += a.lo;
                    name = a.name;
                    found = true;
                }
            } else {
                lo = a.lo;
                hi = a.hi;
                name = (alias.count(a.name)) ? alias_lookup(lineno, a.name, lo, hi) : a.name;
            }
            lineno = a.lineno;
            err = false;
            break;
        }
        if (err) {
            error(lineno, "invalid bitslice of %s", name.c_str());
            break;
        }
    }
    return name;
}

Table::Actions::Action::alias_t::alias_t(value_t &data) {
    lineno = data.lineno;
    if (CHECKTYPE3(data, tSTR, tCMD, tINT)) {
        if (data.type == tSTR) {
            name = data.s;
            lo = 0;
            hi = -1;
        } else if (data.type == tCMD) {
            name = data.vec[0].s;
            if (CHECKTYPE2(data.vec[1], tINT, tRANGE)) {
                if (data.vec[1].type == tINT) {
                    lo = hi = data.vec[1].i;
                } else {
                    lo = data.vec[1].lo;
                    hi = data.vec[1].hi;
                }
            }
        } else {
            is_constant = true;
        }
        value = data.i;
    }
}

/**
 * Builds a map of conditional variable to which bits in the action data format that they
 * control.  Used for JSON later.
 *
 * @sa asm_output::EmitAction::mod_cond_value
 */
void Table::Actions::Action::setup_mod_cond_values(value_t &map) {
    for (auto &kv : map.map) {
        if (CHECKTYPE(kv.key, tSTR) && CHECKTYPE(kv.value, tVEC)) {
            mod_cond_values[kv.key.s].resize(2, bitvec());
            for (auto &v : kv.value.vec) {
                if (CHECKTYPEPM(v, tCMD, v.vec.size == 2, "action data or immediate slice")) {
                    int array_index = -1;
                    if (v[0] == "action_data_table") {
                        array_index = MC_ADT;
                    } else if (v[0] == "immediate") {
                        array_index = MC_IMMED;
                    } else {
                        error(map.lineno,
                              "A non action_data_table or immediate value in the "
                              "mod_con_value map: %s",
                              v[0].s);
                        continue;
                    }
                    int lo = -1;
                    int hi = -1;
                    if (v[1].type == tINT) {
                        lo = hi = v[1].i;
                    } else if (v[1].type == tRANGE) {
                        lo = v[1].lo;
                        hi = v[1].hi;
                    }
                    mod_cond_values.at(kv.key.s).at(array_index).setrange(lo, hi - lo + 1);
                }
            }
        }
    }
}

Table::Actions::Action::Action(Table *tbl, Actions *actions, pair_t &kv, int pos) {
    lineno = kv.key.lineno;
    position_in_assembly = pos;
    if (kv.key.type == tCMD) {
        name = kv.key[0].s;
        if (CHECKTYPE(kv.key[1], tINT)) code = kv.key[1].i;
        if (kv.key.vec.size > 2 && CHECKTYPE(kv.key[2], tINT)) {
            if ((addr = kv.key[2].i) < 0 || addr >= ACTION_IMEM_ADDR_MAX)
                error(kv.key[2].lineno, "Invalid instruction address %d", addr);
        }
    } else if (kv.key.type == tINT) {
        name = std::to_string((code = kv.key.i));
    } else {
        name = kv.key.s;
    }
    if (code >= 0) {
        if (actions->code_use[code]) {
            if (!equivVLIW(actions->by_code[code]))
                error(kv.key.lineno, "Duplicate action code %d", code);
        } else {
            actions->by_code[code] = this;
            actions->code_use[code] = true;
        }
    }
    for (auto &i : kv.value.vec) {
        if (i.type == tINT && instr.empty()) {
            if ((addr = i.i) < 0 || i.i >= ACTION_IMEM_ADDR_MAX)
                error(i.lineno, "Invalid instruction address %" PRId64 "", i.i);
        } else if (i.type == tMAP) {
            for (auto &a : i.map)
                if (CHECKTYPE(a.key, tSTR)) {
                    if (a.key == "p4_param_order") {
                        if (!CHECKTYPE(a.value, tMAP)) continue;

                        unsigned position = 0;
                        for (auto &v : a.value.map) {
                            if (!(CHECKTYPE(v.key, tSTR) && CHECKTYPE2(v.value, tINT, tMAP)))
                                continue;

                            if (v.value.type == tINT) {
                                p4_params_list.emplace_back(v.key.s, position++, v.value.i);
                            } else {
                                p4_param p(v.key.s, position++);
                                for (auto &w : v.value.map) {
                                    if (!CHECKTYPE(w.key, tSTR)) continue;
                                    if (w.key == "width" && CHECKTYPE(w.value, tINT))
                                        p.bit_width = w.value.i;
                                    else if (w.key == "context_json" && CHECKTYPE(w.value, tMAP))
                                        p.context_json = toJson(w.value.map);
                                    else
                                        error(lineno, "Incorrect param type %s in p4_param_order",
                                              w.key.s);
                                }

                                p4_params_list.emplace_back(std::move(p));
                            }
                        }
                    } else if (a.key == "hit_allowed") {
                        if CHECKTYPE (a.value, tMAP) {
                            for (auto &p : a.value.map) {
                                if (CHECKTYPE(p.key, tSTR) && CHECKTYPE(p.value, tSTR)) {
                                    if (p.key == "allowed")
                                        hit_allowed = get_bool(p.value);
                                    else if (p.key == "reason")
                                        hit_disallowed_reason = p.value.s;
                                }
                            }
                        }
                    } else if (a.key == "default_action" || a.key == "default_only_action") {
                        if CHECKTYPE (a.value, tMAP) {
                            for (auto &p : a.value.map) {
                                if (CHECKTYPE(p.key, tSTR) && CHECKTYPE(p.value, tSTR)) {
                                    if (p.key == "allowed")
                                        default_allowed = get_bool(p.value);
                                    else if (p.key == "is_constant")
                                        is_constant = get_bool(p.value);
                                    else if (p.key == "reason")
                                        default_disallowed_reason = p.value.s;
                                }
                            }
                        }
                        default_only = a.key == "default_only_action";
                    } else if (a.key == "handle") {
                        if CHECKTYPE (a.value, tINT) {
                            handle = a.value.i;
                        }
                    } else if (a.key == "next_table") {
                        if (a.value.type == tINT)
                            next_table_encode = a.value.i;
                        else
                            next_table_ref = a.value;
                    } else if (a.key == "next_table_miss") {
                        next_table_miss_ref = a.value;
                    } else if (a.key == "mod_cond_value") {
                        if (CHECKTYPE(a.value, tMAP)) {
                            setup_mod_cond_values(a.value);
                        }
                    } else if (a.key == "context_json") {
                        if (CHECKTYPE(a.value, tMAP)) {
                            context_json = toJson(a.value.map);
                        }
                    } else if (CHECKTYPE3(a.value, tSTR, tCMD, tINT)) {
                        if (a.value.type == tINT) {
                            auto k = alias.find(a.key.s);
                            if (k == alias.end()) {
                                alias.emplace(a.key.s, a.value);
                            } else {
                                k->second.is_constant = true;
                                k->second.value = a.value.i;
                            }
                        } else if (a.value.type == tSTR) {
                            auto k = alias.find(a.value.s);
                            if (k == alias.end()) {
                                alias.emplace(a.key.s, a.value);
                            } else {
                                auto alias_value = k->second;
                                alias.erase(k);
                                alias.emplace(a.key.s, alias_value);
                            }
                        } else {
                            alias.emplace(a.key.s, a.value);
                        }
                    }
                }

        } else if (CHECKTYPE2(i, tSTR, tCMD)) {
            VECTOR(value_t) tmp;
            if (i.type == tSTR) {
                if (!*i.s) continue;  // skip blank line
                VECTOR_init1(tmp, i);
            } else {
                VECTOR_initcopy(tmp, i.vec);
            }
            if (auto *p = Instruction::decode(tbl, this, tmp))
                instr.emplace_back(p);
            else if (tbl->to<MatchTable>() || tbl->to<TernaryIndirectTable>() ||
                     tbl->to<ActionTable>())
                attached.emplace_back(i, tbl);
            else
                error(i.lineno, "Unknown instruction %s", tmp[0].s);
            VECTOR_fini(tmp);
        }
    }
}

Table::Actions::Action::Action(const char *n, int l) : name(n), lineno(l) {}
Table::Actions::Action::~Action() {}

Table::Actions::Actions(Table *tbl, VECTOR(pair_t) & data) {
    table = tbl;
    int pos = 0;
    for (auto &kv : data) {
        if ((kv.key.type != tINT && !CHECKTYPE2M(kv.key, tSTR, tCMD, "action")) ||
            !CHECKTYPE(kv.value, tVEC))
            continue;
        std::string name = kv.key.type == tINT   ? std::to_string(kv.key.i)
                           : kv.key.type == tSTR ? kv.key.s
                                                 : kv.key[0].s;
        if (actions.count(name)) {
            error(kv.key.lineno, "Duplicate action %s", name.c_str());
            continue;
        }
        actions.emplace(name, tbl, this, kv, pos++);
    }
}

int Table::Actions::hit_actions_count() const {
    int cnt = 0;
    for (auto &a : actions) {
        if (a.second.hit_allowed) ++cnt;
    }
    return cnt;
}

int Table::Actions::default_actions_count() const {
    int cnt = 0;
    for (auto &a : actions) {
        if (a.second.default_allowed) ++cnt;
    }
    return cnt;
}

AlwaysRunTable::AlwaysRunTable(gress_t gress, Stage *stage, pair_t &init)
    : Table(init.key.lineno,
            "always run " + to_string(gress) + " stage " + to_string(stage->stageno), gress,
            stage) {
    VECTOR(pair_t) tmp = {1, 1, &init};
    actions.reset(new Actions(this, tmp));
    if (actions->count() == 1) {  // unless there was an error parsing the action...
        auto &act = *actions->begin();
        if (act.addr >= 0) error(act.lineno, "always run action address is fixed");
        act.addr = ACTION_ALWAYS_RUN_IMEM_ADDR;
    }
}

void Table::Actions::Action::check_next_ref(Table *tbl, const Table::Ref &ref) const {
    if (ref.check() && ref->table_id() >= 0 && ref->table_id() < tbl->table_id()) {
        error(lineno, "Next table %s for action %s before containing table %s", ref->name(),
              name.c_str(), tbl->name());
        return;
    }

    if (ref->table_id() > (1U << NEXT_TABLE_MAX_RAM_EXTRACT_BITS) - 1 &&
        tbl->get_hit_next().size() == 0) {
        error(lineno, "Next table cannot properly be saved on the RAM line for this action %s",
              name.c_str());
    }
}

/**
 * By the end of this function, both next_table and next_table_miss_ref will have been created
 * and validated.
 *
 * Each action must have at least next_table or a next_table_miss from the node.
 *     - next_table: The next table to run on hit
 *     - next_table_miss: The next table to run on miss
 *
 * The next_table_encode is the entry into the next_table_hitmap, if a next_table hit map is
 * provided.  If the next_table hit map is empty, then the next_table_encode won't have been
 * set.  If the action can be used on a hit, then either a next_table_ref/next_table_encode
 * would be provided.
 *
 * The next_table_ref could come from the next_table as an int value, which would be on offset
 * into the hit_map
 */
void Table::Actions::Action::check_next(Table *tbl) {
    if (next_table_encode >= 0) {
        int idx = next_table_encode;
        if (idx < tbl->get_hit_next().size()) {
            next_table_ref = tbl->get_hit_next().at(idx);
        } else if ((idx -= tbl->get_hit_next().size()) < tbl->extra_next_lut.size()) {
            next_table_ref = tbl->extra_next_lut.at(idx);
        } else {
            error(lineno,
                  "The encoding on action %s is outside the range of the hitmap in "
                  "table %s",
                  name.c_str(), tbl->name());
        }
    }

    if (!next_table_miss_ref.set() && !next_table_ref.set()) {
        if (tbl->get_hit_next().size() != 1) {
            error(lineno,
                  "Either next_table or next_table_miss must be required on action %s "
                  "if the next table cannot be determined",
                  name.c_str());
        } else {
            next_table_ref = tbl->get_hit_next()[0];
            next_table_miss_ref = next_table_ref;
            next_table_encode = 0;
        }
    } else if (!next_table_ref.set()) {
        if (!default_only) {
            error(lineno,
                  "Action %s on table %s that can be programmed on hit must have "
                  "a next_table encoding",
                  name.c_str(), tbl->name());
        }
        next_table_ref = next_table_miss_ref;
    } else if (!next_table_miss_ref.set()) {
        next_table_miss_ref = next_table_ref;
    }
    tbl->check_next(next_table_ref);
    tbl->check_next(next_table_miss_ref);
    if (next_table_encode < 0 && !default_only) next_table_ref.force_single_next_table();
    for (auto &n : next_table_ref) check_next_ref(tbl, n);
    for (auto &n : next_table_miss_ref) check_next_ref(tbl, n);
}

void Table::Actions::Action::pass1(Table *tbl) {
    // The compiler generates all action handles which must be specified in the
    // assembly, if not we throw an error.
    if ((handle == 0) && tbl->needs_handle()) {
        error(lineno, "No action handle specified for table - %s, action - %s", tbl->name(),
              name.c_str());
    }

    if (tbl->needs_next()) {
        check_next(tbl);
    }

    if (tbl->get_default_action() == name) {
        if (!tbl->default_action_handle) tbl->default_action_handle = handle;
        if (tbl->default_only_action) default_only = true;
    }
    /* SALU actions always have addr == -1 (so iaddr == -1) */
    int iaddr = -1;
    bool shared_VLIW = false;
    for (auto &inst : instr) {
        inst.reset(inst.release()->pass1(tbl, this));
        if (inst->slot >= 0) {
            if (slot_use[inst->slot])
                error(inst->lineno, "instruction slot %d used multiple times in action %s",
                      inst->slot, name.c_str());
            slot_use[inst->slot] = 1;
        }
    }
    if (addr >= 0) {
        if (auto old = tbl->stage->imem_addr_use[imem_thread(tbl->gress)][addr]) {
            if (equivVLIW(old)) {
                shared_VLIW = true;
            } else {
                error(lineno, "action instruction addr %d in use elsewhere", addr);
                warning(old->lineno, "also defined here");
            }
        }
        tbl->stage->imem_addr_use[imem_thread(tbl->gress)][addr] = this;
        iaddr = addr / ACTION_IMEM_COLORS;
    }
    if (!shared_VLIW) {
        for (auto &inst : instr) {
            if (inst->slot >= 0 && iaddr >= 0) {
                if (tbl->stage->imem_use[iaddr][inst->slot])
                    error(lineno, "action instruction slot %d.%d in use elsewhere", iaddr,
                          inst->slot);
                tbl->stage->imem_use[iaddr][inst->slot] = 1;
            }
        }
    }
    for (auto &a : alias) {
        while (alias.count(a.second.name) >= 1) {
            // the alias refers to something else in the alias list
            auto &rec = alias.find(a.second.name)->second;
            if (rec.name == a.first) {
                error(a.second.lineno, "recursive alias %s", a.first.c_str());
                break;
            }
            if (rec.lo > 0) {
                a.second.lo += rec.lo;
                if (a.second.hi >= 0) a.second.hi += rec.lo;
            }
            if (rec.hi > 0 && a.second.hi < 0) a.second.hi = rec.hi;
            if (a.second.lo < rec.lo || (rec.hi >= 0 && a.second.hi > rec.hi)) {
                error(a.second.lineno,
                      "alias for %s:%s(%d:%d) has out of range index from allowed %s:%s(%d:%d)",
                      a.first.c_str(), a.second.name.c_str(), a.second.lo, a.second.hi,
                      a.second.name.c_str(), rec.name.c_str(), rec.lo, rec.hi);
                break;
            }
            a.second.name = rec.name;
        }
        if (auto *f = tbl->lookup_field(a.second.name, name)) {
            if (a.second.hi < 0) a.second.hi = f->size - 1;
        } else if (a.second.name == "hash_dist" && a.second.lo >= 0) {
            // nothing to be done for now.  lo..hi is the hash dist index rather than
            // a bit index, which will cause problems if we want to later slice the alias
            // to access only some bits of it.
        } else {
            error(a.second.lineno, "No field %s in table %s", a.second.to_string().c_str(),
                  tbl->name());
        }
    }
    // Update default value for params if default action parameters present
    for (auto &p : p4_params_list) {
        if (auto def_act_params = tbl->get_default_action_parameters()) {
            if (def_act_params->count(p.name) > 0) {
                p.default_value = (*def_act_params)[p.name];
                p.defaulted = true;
            }
        }
    }
    for (auto &c : attached) {
        if (!c) {
            error(c.lineno, "Unknown instruction or table %s", c.name.c_str());
            continue;
        }
        if (c->table_type() != COUNTER && c->table_type() != METER && c->table_type() != STATEFUL) {
            error(c.lineno, "%s is not a counter, meter or stateful table", c.name.c_str());
            continue;
        }
    }
}

/**
 * Determines if the field, which has a particular range of bits in the format, is controlled
 * by a conditional variable.  This is required for context JSON information on parameters in
 * the action data table pack format, or in the immediate fields:
 *
 *     -is_mod_field_conditionally_value
 *     -mod_field_conditionally_mask_field_name
 *
 * @sa asm_output::EmitAction::mod_cond_value
 */
void Table::Actions::Action::check_conditional(Table::Format::Field &field) const {
    bool found = false;
    std::string condition;
    for (auto kv : mod_cond_values) {
        for (auto br : field.bits) {
            auto overlap = kv.second[MC_ADT].getslice(br.lo, br.size());
            if (overlap.empty()) {
                BUG_CHECK(!found || (found && condition != kv.first));
            } else if (overlap.popcount() == br.size()) {
                if (found) {
                    BUG_CHECK(condition == kv.first);
                } else {
                    found = true;
                    condition = kv.first;
                }
            } else {
                BUG();
            }
        }
    }
    if (found) {
        field.conditional_value = true;
        field.condition = condition;
    }
}

/**
 * @sa Table::Actions::Action::check_conditional
 */
bool Table::Actions::Action::immediate_conditional(int lo, int sz, std::string &condition) const {
    bool found = false;
    for (auto kv : mod_cond_values) {
        auto overlap = kv.second[MC_IMMED].getslice(lo, sz);
        if (overlap.empty()) {
            BUG_CHECK(!found || (found && condition != kv.first));
        } else {
            if (found) {
                BUG_CHECK(condition == kv.first);
            } else if (overlap.popcount() == sz) {
                found = true;
                condition = kv.first;
            } else {
                BUG();
            }
        }
    }
    return found;
}

void Table::Actions::pass1(Table *tbl) {
    for (auto &act : *this) {
        act.pass1(tbl);
        slot_use |= act.slot_use;
    }
}

std::map<Table *, std::set<Table::Actions::Action *>> Table::find_pred_in_stage(
    int stageno, const std::set<Actions::Action *> &acts) {
    std::map<Table *, std::set<Actions::Action *>> rv;
    if (stage->stageno < stageno) return rv;
    if (stage->stageno == stageno) {
        rv[this].insert(acts.begin(), acts.end());
    }
    for (auto &p : pred) {
        for (auto &kv : p.first->find_pred_in_stage(stageno, p.second)) {
            rv[kv.first].insert(kv.second.begin(), kv.second.end());
        }
    }
    for (auto *mt : get_match_tables()) {
        if (mt != this) {
            for (auto &kv : mt->find_pred_in_stage(stageno, acts)) {
                rv[kv.first].insert(kv.second.begin(), kv.second.end());
            }
        }
    }
    return rv;
}

void Table::Actions::pass2(Table *tbl) {
    /* We do NOT call this for SALU actions, so we can assume VLIW actions here */
    BUG_CHECK(tbl->table_type() != STATEFUL);
    int code = tbl->get_gateway() ? 1 : 0;  // if there's a gateway, reserve code 0 for a NOP
                                            // to run when the gateway inhibits the table

    /* figure out how many codes we can encode in the match table(s), and if we need a distinct
     * code for every action to handle next_table properly */
    int code_limit = 0x10000;
    bool use_code_for_next = false;  // true iff a table uses the action code for next table
                                     // selection in addition to using it for the action instruction

    for (auto match : tbl->get_match_tables()) {
        // action is currently a default keyword for the instruction address
        auto instruction = match->instruction_call();
        auto fld = instruction.args[0].field();
        if (fld) {
            code_limit = 1 << fld->size;
            if (match->hit_next_size() > 1 && !match->lookup_field("next"))
                use_code_for_next = true;
        } else {
            code_limit = code + 1;
        }
    }

    /* figure out if we need more codes than can fit in the action_instruction_adr_map.
     * use code = -1 to signal that condition. */
    int non_nop_actions = by_code.size();
    // Check if a nop action is defined. The action will be empty (no
    // instructions). By default we will use code '0' for nop action, unless
    // compiler has assigned a different value.
    int nop_code = 0;
    for (auto &bc : by_code) {
        if (bc.second->instr.empty()) nop_code = bc.first;
    }
    if (by_code.count(nop_code) && by_code.at(nop_code)->instr.empty()) {
        --non_nop_actions;  // don't count nop code action
        code = 1;
    }
    for (auto &act : *this) {
        if (act.default_only) continue;
        if (act.instr.empty() && !use_code_for_next)
            code = 1;  // nop action -- use code 0 unless it needs to be used as next
        else if (act.code < 0)
            ++non_nop_actions;
    }  // FIXME -- should combine identical actions?
    if (code + non_nop_actions > ACTION_INSTRUCTION_SUCCESSOR_TABLE_DEPTH) code = -1;
    bool code0_is_noop = (code != 0);

    for (auto &act : *this) {
        for (auto &inst : act.instr) inst->pass2(tbl, &act);
        if (act.addr < 0) {
            for (int i = 0; i < ACTION_IMEM_ADDR_MAX; i++) {
                if (auto old = tbl->stage->imem_addr_use[imem_thread(tbl->gress)][i]) {
                    if (act.equivVLIW(old)) {
                        act.addr = i;
                        break;
                    }
                    continue;
                }
                if (tbl->stage->imem_use[i / ACTION_IMEM_COLORS].intersects(act.slot_use)) continue;
                act.addr = i;
                tbl->stage->imem_use[i / ACTION_IMEM_COLORS] |= act.slot_use;
                tbl->stage->imem_addr_use[imem_thread(tbl->gress)][i] = &act;
                break;
            }
        }
        if (act.addr < 0) error(act.lineno, "Can't find an available instruction address");
        if (act.code < 0 && !act.default_only) {
            if (code < 0 && !code_use[act.addr]) {
                act.code = act.addr;
            } else if (act.instr.empty() && !use_code_for_next && code0_is_noop) {
                act.code = 0;
            } else {
                while (code >= 0 && code_use[code]) code++;
                act.code = code;
            }
        } else if (code < 0 && act.code != act.addr && !act.default_only) {
            error(act.lineno,
                  "Action code must be the same as action instruction address "
                  "when there are more than %d actions",
                  ACTION_INSTRUCTION_SUCCESSOR_TABLE_DEPTH);
            if (act.code < 0)
                warning(act.lineno, "Code %d is already in use by another action", act.addr);
        }
        if (act.code >= 0) {
            by_code[act.code] = &act;
            code_use[act.code] = true;
        }
        if (act.code >= code_limit)
            error(act.lineno,
                  "Action code %d for %s too large for action specifier in "
                  "table %s",
                  act.code, act.name.c_str(), tbl->name());
        if (act.code > max_code) max_code = act.code;
    }
    actions.sort([](const value_type &a, const value_type &b) -> bool {
        return a.second.code < b.second.code;
    });
    if (!tbl->default_action.empty()) {
        if (!exists(tbl->default_action)) {
            error(tbl->default_action_lineno, "no action %s in table %s",
                  tbl->default_action.c_str(), tbl->name());
        } else {
            auto &defact = actions.at(tbl->default_action);
            if (!defact.default_allowed) {
                // FIXME -- should be an error, but the compiler currently does this?
                // FIXME -- see p4_16_programs_tna_lpm_match
                warning(tbl->default_action_lineno,
                        "default action %s in table %s is not allowed "
                        "to be default?",
                        tbl->default_action.c_str(), tbl->name());
                defact.default_allowed = true;
            }
        }
    }
    auto pred = tbl->find_pred_in_stage(tbl->stage->stageno);
    for (auto &p : pred) {
        auto *actions = p.first->get_actions();
        if (!actions || actions == this) continue;
        if (!slot_use.intersects(actions->slot_use)) continue;
        for (auto &a1 : *this) {
            bool first = false;
            for (auto a2 : p.second) {
                if (a1.slot_use.intersects(a2->slot_use)) {
                    if (!first)
                        warning(a1.lineno,
                                "Conflicting instruction slot usage for non-exlusive "
                                "table %s action %s",
                                tbl->name(), a1.name.c_str());
                    first = true;
                    warning(a2->lineno, "and table %s action %s", p.first->name(),
                            a2->name.c_str());
                }
            }
        }
    }
}

void Table::Actions::stateful_pass2(Table *tbl) {
    BUG_CHECK(tbl->table_type() == STATEFUL);
    auto *stbl = tbl->to<StatefulTable>();
    for (auto &act : *this) {
        if (act.code >= 4) {
            error(act.lineno, "Only 4 actions in a stateful table");
        } else if (act.code >= 0) {
            if (code_use[act.code]) {
                error(act.lineno, "duplicate use of code %d in SALU", act.code);
                warning(by_code[act.code]->lineno, "previous use here");
            }
            by_code[act.code] = &act;
            code_use[act.code] = true;
        }
        if (act.code == 3 && stbl->clear_value)
            error(act.lineno, "Can't use SALU action 3 with a non-zero clear value");
        for (const auto &inst : act.instr) inst->pass2(tbl, &act);
    }
    if (stbl->clear_value) code_use[3] = true;
    for (auto &act : *this) {
        if (act.code < 0) {
            if ((act.code = code_use.ffz(0)) >= 4) {
                error(act.lineno, "Only 4 actions in a stateful table");
                break;
            }
            by_code[act.code] = &act;
            code_use[act.code] = true;
        }
    }
}

template <class REGS>
void Table::Actions::write_regs(REGS &regs, Table *tbl) {
    for (auto &act : *this) {
        LOG2("# action " << act.name << " code=" << act.code << " addr=" << act.addr);
        tbl->write_action_regs(regs, &act);
        for (const auto &inst : act.instr) inst->write_regs(regs, tbl, &act);
        if (options.fill_noop_slot) {
            for (auto slot : Phv::use(tbl->gress) - tbl->stage->imem_use_all()) {
                auto tmp = VLIW::genNoopFill(tbl, &act, options.fill_noop_slot, slot);
                tmp->pass1(tbl, &act);
                tmp->pass2(tbl, &act);
                tmp->write_regs(regs, tbl, &act);
            }
        }
    }
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void Table::Actions::write_regs, mau_regs &,
                      Table *)

/**
 * Indirect Counters, Meters, and Stateful Alus can be addressed in many different ways, e.g.
 * Hash Distribution, Overhead Index, Stateful Counter, Constant, etc.
 *
 * The indexing can be different per individual action.  Say one action always uses an indirect
 * address, while another one uses a constant.  The driver has to know where to put that
 * constant into the RAM line.
 *
 * Also, say an address is from hash, but can have multiple meter types.  By using the override
 * address of an action, when that action is programmed, the meter type written in overhead will
 * be determined by the overhead address.
 *
 * override_addr - a boolean of whether to use the override value for these parameters.
 *     This is enabled if the address does not come from overhead.
 *
 * Override_addr_pfe - Not actually useful, given the override_full_addr contains the per flow
 *     enable bit
 *
 * Override_full_addr - the constant value to be written directly into the corresponding bit
 *     positions in the RAM line
 */
static void gen_override(json::map &cfg, const Table::Call &att) {
    auto type = att->table_type();
    // Direct tables currently don't require overrides
    // FIXME: Corner cases where miss actions do not use the stateful object should have
    // an override of all 0
    if (att->to<AttachedTable>()->is_direct()) return;
    std::string base;
    bool override_addr = false;
    bool override_addr_pfe = false;
    unsigned override_full_addr = 0;
    switch (type) {
        case Table::COUNTER:
            base = "override_stat";
            break;
        case Table::METER:
            base = "override_meter";
            break;
        case Table::STATEFUL:
            base = "override_stateful";
            break;
        default:
            error(att.lineno, "unsupported table type in action call");
    }
    // Always true if the call is provided
    override_addr_pfe = true;
    override_full_addr |= 1U << (type == Table::COUNTER ? STATISTICS_PER_FLOW_ENABLE_START_BIT
                                                        : METER_PER_FLOW_ENABLE_START_BIT);
    int idx = -1;
    for (auto &arg : att.args) {
        ++idx;
        if (arg.type == Table::Call::Arg::Name) {
            if (strcmp(arg.name(), "$hash_dist") == 0 ||
                strcmp(arg.name(), "$stful_counter") == 0) {
                override_addr = true;
            } else if (auto *st = att->to<StatefulTable>()) {
                if (auto *act = st->actions->action(arg.name())) {
                    override_full_addr |= 1 << METER_TYPE_START_BIT;
                    override_full_addr |= act->code << (METER_TYPE_START_BIT + 1);
                }
            }
            // FIXME -- else assume its a reference to a format field, so doesn't need to
            // FIXME -- be in the override.  Should check that somewhere, but need access
            // FIXME -- to the match_table to do it here.
        } else if (arg.type == Table::Call::Arg::Const) {
            if (idx == 0 && att.args.size() > 1) {
                // The first argument for meters/stateful is the meter type
                override_full_addr |= arg.value() << METER_TYPE_START_BIT;
            } else {
                override_full_addr |= arg.value() << att->address_shift();
                override_addr = true;
            }
        } else if (arg.type == Table::Call::Arg::Counter) {
            // does not affect context json
        } else {
            error(att.lineno, "argument not a constant");
        }
    }
    cfg[base + "_addr"] = override_addr;
    cfg[base + "_addr_pfe"] = override_addr ? override_addr_pfe : false;
    cfg[base + "_full_addr"] = override_addr ? override_full_addr : 0;
}

bool Table::Actions::Action::is_color_aware() const {
    for (auto &att : attached) {
        if (att->table_type() != Table::METER) continue;
        if (att.args.size() < 2) continue;
        auto type_arg = att.args[0];
        if (type_arg.type == Table::Call::Arg::Const && type_arg.value() == METER_COLOR_AWARE)
            return true;
    }
    return false;
}

void Table::Actions::Action::check_and_add_resource(json::vector &resources,
                                                    json::map &resource) const {
    // Check if resource already exists in the json::vector. For tables
    // spanning multiple stages, the same resource gets added as an attached
    // resource for every stage. To avoid duplication only add when not
    // present in the resource array
    bool found = false;
    for (auto &r : resources) {
        if (resource == r->to<json::map>()) {
            found = true;
            break;
        }
    }
    if (!found) resources.push_back(std::move(resource));
}

void Table::Actions::Action::add_direct_resources(json::vector &direct_resources,
                                                  const Call &att) const {
    json::map direct_resource;
    direct_resource["resource_name"] = att->p4_name();
    direct_resource["handle"] = att->handle();
    check_and_add_resource(direct_resources, direct_resource);
}

void Table::Actions::Action::add_indirect_resources(json::vector &indirect_resources,
                                                    const Call &att) const {
    auto addr_arg = att.args.back();
    json::map indirect_resource;
    if (addr_arg.type == Table::Call::Arg::Name) {
        auto *p = has_param(addr_arg.name());
        if (p) {
            indirect_resource["access_mode"] = "index";
            indirect_resource["parameter_name"] = p->name;
            indirect_resource["parameter_index"] = p->position;
        } else {
            return;
        }
    } else if (addr_arg.type == Table::Call::Arg::Const) {
        indirect_resource["access_mode"] = "constant";
        indirect_resource["value"] = addr_arg.value();
    } else {
        return;
    }
    indirect_resource["resource_name"] = att->p4_name();
    indirect_resource["handle"] = att->handle();
    check_and_add_resource(indirect_resources, indirect_resource);
}

void Table::Actions::gen_tbl_cfg(json::vector &actions_cfg) const {
    for (auto &act : *this) {
        // Use action node if it already exists in json
        bool act_json_present = false;
        json::map *action_ptr = nullptr;
        for (auto &_action_o : actions_cfg) {
            auto &_action = _action_o->to<json::map>();
            if (_action["name"] == act.name) {
                action_ptr = &_action;
                act_json_present = true;
                break;
            }
        }
        if (!act_json_present) action_ptr = new json::map();
        json::map &action_cfg = *action_ptr;

        action_cfg["name"] = act.name;
        action_cfg["handle"] = act.handle;  // FIXME-JSON
        if (act.instr.empty() || action_cfg.count("primitives") == 0)
            action_cfg["primitives"] = json::vector();
        auto &direct_resources = action_cfg["direct_resources"] = json::vector();
        auto &indirect_resources = action_cfg["indirect_resources"] = json::vector();
        for (auto &att : act.attached) {
            if (att.is_direct_call())
                act.add_direct_resources(direct_resources, att);
            else
                act.add_indirect_resources(indirect_resources, att);
        }
        if (!act.hit_allowed && !act.default_allowed)
            error(act.lineno, "Action %s must be allowed to be hit and/or default action.",
                  act.name.c_str());
        action_cfg["allowed_as_hit_action"] = act.hit_allowed;
        // TODO: allowed_as_default_action info is directly passed through assembly
        // This will be 'false' for following conditions:
        // 1. Action requires hardware in hit path i.e. hash distribution or
        // random number generator
        // 2. There is a default action declared constant in program which
        // implies all other actions cannot be set to default
        action_cfg["allowed_as_default_action"] = act.default_allowed;
        // TODO: "disallowed_as_default_action" is not used by driver.
        // Keeping it here as debugging info. Will be set to "none",
        // "has_const_default", "has_hash_dist". Once rng support is added
        // to the compiler this must reflect "has_rng" or similar string.
        if (!act.default_allowed)
            action_cfg["disallowed_as_default_action_reason"] = act.default_disallowed_reason;
        // TODO: Need to be set through assembly
        action_cfg["is_compiler_added_action"] = false;
        action_cfg["constant_default_action"] = act.is_constant;

        // TODO: These will be set to 'true' & "" for a keyless table to
        // allow any action to be set as default by the control plane
        // Exception is TernaryIndirectTables which dont have params list as they are on the main
        // TernaryMatchTable, hence check for match_table to query params list
        if (table->get_match_table()->p4_params_list.empty()) {
            action_cfg["allowed_as_default_action"] = true;
            action_cfg["disallowed_as_default_action_reason"] = "";
        }

        json::vector &p4_params = action_cfg["p4_parameters"] = json::vector();
        act.add_p4_params(p4_params);
        action_cfg["override_meter_addr"] = false;
        action_cfg["override_meter_addr_pfe"] = false;
        action_cfg["override_meter_full_addr"] = 0;
        action_cfg["override_stat_addr"] = false;
        action_cfg["override_stat_addr_pfe"] = false;
        action_cfg["override_stat_full_addr"] = 0;
        action_cfg["override_stateful_addr"] = false;
        action_cfg["override_stateful_addr_pfe"] = false;
        action_cfg["override_stateful_full_addr"] = 0;
        for (auto &att : act.attached) gen_override(action_cfg, att);
        action_cfg["is_action_meter_color_aware"] = act.is_color_aware();
        if (act.context_json) action_cfg.merge(*act.context_json.get());
        if (!act_json_present) actions_cfg.push_back(std::move(action_cfg));
    }
}

/**
 * For action data tables, the entirety of the action configuration is not necessary, as the
 * information is per match table, not per action data table.  The only required parameters
 * are the name, handle, and p4_parameters
 *
 * Even at some point, even actions that have the different p4_parameters could even share a
 * member, if for example, one of the parameters is not stored in the action data table,
 * but rather as an index for a counter/meter etc.  The compiler/driver do not have support for
 * this yet.
 */
void Table::Actions::Action::gen_simple_tbl_cfg(json::vector &actions_cfg) const {
    json::map action_cfg;
    action_cfg["name"] = name;
    action_cfg["handle"] = handle;
    json::vector &p4_params = action_cfg["p4_parameters"] = json::vector();
    add_p4_params(p4_params, false);
    actions_cfg.push_back(std::move(action_cfg));
}

void Table::Actions::Action::add_p4_params(json::vector &cfg, bool include_default) const {
    unsigned start_bit = 0;
    for (auto &a : p4_params_list) {
        json::map param;
        param["name"] = a.name;
        param["start_bit"] = start_bit;
        param["position"] = a.position;
        if (include_default && a.defaulted) param["default_value"] = a.default_value;
        param["bit_width"] = a.bit_width;
        if (a.context_json) param.merge(*a.context_json.get());
        cfg.push_back(std::move(param));
        start_bit += a.bit_width;
    }
}

void Table::Actions::add_p4_params(const Action &act, json::vector &cfg) const {
    int index = 0;
    unsigned start_bit = 0;
    // Add p4 params if present. This will add params even if the action is
    // otherwise empty. Driver will always generate an action spec if p4_params
    // are present for an action
    for (auto &a : act.p4_params_list) {
        json::map param;
        param["name"] = a.name;
        param["start_bit"] = start_bit;
        param["position"] = a.position;
        if (a.defaulted) param["default_value"] = a.default_value;
        param["bit_width"] = a.bit_width;
        cfg.push_back(std::move(param));
        start_bit += a.bit_width;
    }
}

void Table::Actions::add_action_format(const Table *table, json::map &tbl) const {
    json::vector &action_format = tbl["action_format"] = json::vector();
    for (auto &act : *this) {
        json::map action_format_per_action;
        unsigned next_table = -1;

        std::string next_table_name = "--END_OF_PIPELINE--";
        if (!act.default_only) {
            if (act.next_table_encode >= 0) {
                next_table = static_cast<unsigned>(act.next_table_encode);
            } else {
                // The RAM value is only 8 bits, for JBay must be solved by table placement
                next_table = act.next_table_ref.next_table_id() & 0xff;
                next_table_name = act.next_table_ref.next_table_name();
                if (next_table_name == "END") next_table_name = "--END_OF_PIPELINE--";
            }
        }
        unsigned next_table_full = act.next_table_miss_ref.next_table_id();

        /**
         * This following few fields are required on a per stage table action basis.
         * The following information is:
         *
         * - next_table - The value that will be written into the next field RAM line on a hit,
         *       when the entry is specified with this action.  This is either an index into
         *       the next_table_map_en (if that map is enabled), or the 8 bit next table value.
         *
         * - next_table_full - The value that will be written into the miss register for next
         *       table (next_table_format_data.match_next_table_adr_miss_value), if this action
         *       is set as the default action.  This is the full 8 bit (9 bit for JBay) next
         *       table.
         *
         * - vliw_instruction - The value that will be written into the action instruction RAM
         *       entry when the entry is specified with this action.  This is either an index
         *       into into the 8 entry table mau_action_instruction_adr_map_data, if that is
         *       enabled, or the full word instruction
         *
         * - vliw_instruction_full - The value that will written into the miss register for
         *       action_instruction (mau_action_instruction_adr_miss_value), when this
         *       action is specified as the default action.  The full address with the PFE
         *       bit enabled.
         */
        action_format_per_action["action_name"] = act.name;
        action_format_per_action["action_handle"] = act.handle;
        action_format_per_action["table_name"] = next_table_name;
        action_format_per_action["next_table"] = next_table;
        action_format_per_action["next_table_full"] = next_table_full;
        if (Target::LONG_BRANCH_TAGS() > 0 && !options.disable_long_branch) {
            if (Target::NEXT_TABLE_EXEC_COMBINED()) {
                action_format_per_action["next_table_exec"] =
                    ((act.next_table_miss_ref.next_in_stage(table->stage->stageno) & 0xfffe)
                     << 15) +
                    (act.next_table_miss_ref.next_in_stage(table->stage->stageno + 1) & 0xffff);
            } else {
                action_format_per_action["next_table_local_exec"] =
                    act.next_table_miss_ref.next_in_stage(table->stage->stageno) >> 1;
                action_format_per_action["next_table_global_exec"] =
                    act.next_table_miss_ref.next_in_stage(table->stage->stageno + 1);
            }
            action_format_per_action["next_table_long_brch"] =
                act.next_table_miss_ref.long_branch_tags();
        }
        action_format_per_action["vliw_instruction"] = act.code;
        action_format_per_action["vliw_instruction_full"] =
            ACTION_INSTRUCTION_ADR_ENABLE | act.addr;

        json::vector &next_tables = action_format_per_action["next_tables"] = json::vector();
        for (auto n : act.next_table_ref) {
            auto nP4Name = n->p4_name();
            // Gateway next tables dont have a p4 Name
            if (nP4Name == nullptr) {
                nP4Name = n.name.c_str();
            }
            next_tables.push_back(
                json::map{{"next_table_name", json::string(nP4Name)},
                          {"next_table_logical_id", json::number(n->logical_id)},
                          {"next_table_stage_no", json::number(n->stage->stageno)}});
        }
        json::vector &action_format_per_action_imm_fields =
            action_format_per_action["immediate_fields"] = json::vector();
        for (auto &a : act.alias) {
            json::string name = a.first;
            int lo = remove_name_tail_range(name);
            json::string immed_name = a.second.name;
            if (immed_name != "immediate") continue;  // output only immediate fields
            if (!(act.has_param(name) || a.second.is_constant))
                continue;  // and fields that are parameters or constants
            json::map action_format_per_action_imm_field;
            action_format_per_action_imm_field["param_name"] = name;
            action_format_per_action_imm_field["param_type"] = "parameter";
            if (a.second.is_constant) {
                action_format_per_action_imm_field["param_type"] = "constant";
                action_format_per_action_imm_field["const_value"] = a.second.value;
                action_format_per_action_imm_field["param_name"] =
                    "constant_" + std::to_string(a.second.value);
            }
            action_format_per_action_imm_field["param_shift"] = lo;
            action_format_per_action_imm_field["dest_start"] = a.second.lo;
            action_format_per_action_imm_field["dest_width"] = a.second.size();
            std::string condition;
            if (act.immediate_conditional(a.second.lo, a.second.size(), condition)) {
                action_format_per_action_imm_field["is_mod_field_conditionally_value"] = true;
                action_format_per_action_imm_field["mod_field_conditionally_mask_field_name"] =
                    condition;
            }
            action_format_per_action_imm_fields.push_back(
                std::move(action_format_per_action_imm_field));
        }
        action_format.push_back(std::move(action_format_per_action));
    }
}

std::ostream &operator<<(std::ostream &out, const Table::Actions::Action::alias_t &a) {
    out << "(" << a.name << ", lineno = " << a.lineno << ", lo = " << a.lo << ", hi = " << a.hi
        << ", is_constant = " << a.is_constant << ", value = 0x" << std::hex << a.value << std::dec
        << ")";
    return out;
}

std::ostream &operator<<(std::ostream &out, const Table::Actions::Action &a) {
    out << a.name << "(";
    auto indent = a.name.length() + 10;
    for (auto &p : a.p4_params_list) out << p << std::endl << std::setw(indent);
    out << ")";
    return out;
}

std::ostream &operator<<(std::ostream &out, const Table::p4_param &p) {
    out << p.name << "[ w =" << p.bit_width << ", w_full =" << p.bit_width_full
        << ", start_bit =" << p.start_bit << ", mask = 0x" << p.mask << ", position =" << p.position
        << ", default_value =" << p.default_value << ", defaulted =" << p.defaulted
        << ", is_valid =" << p.is_valid << ", type =" << p.type << ", alias =" << p.alias
        << ", key_name =" << p.key_name << "]";
    return out;
}

void Table::Actions::add_immediate_mapping(json::map &tbl) {
    for (auto &act : *this) {
        if (act.alias.empty()) continue;
        json::vector &map = tbl["action_to_immediate_mapping"][act.name];
        for (auto &a : act.alias) {
            json::string name = a.first;
            json::string immed_name = a.second.name;
            if (immed_name == "immediate") immed_name = "--immediate--";
            int lo = remove_name_tail_range(name);
            map.push_back(json::vector{json::map{
                {"name", std::move(name)},
                {"parameter_least_significant_bit", json::number(lo)},
                {"parameter_most_significant_bit", json::number(lo + a.second.hi - a.second.lo)},
                {"immediate_least_significant_bit", json::number(a.second.lo)},
                {"immediate_most_significant_bit", json::number(a.second.hi)},
                {"field_called", std::move(immed_name)}}});
        }
    }
}

template <class REGS>
void Table::write_mapram_regs(REGS &regs, int row, int col, int vpn, int type) {
    auto &mapram_config = regs.rams.map_alu.row[row].adrmux.mapram_config[col];
    // auto &mapram_ctl = map_alu_row.adrmux.mapram_ctl[col];
    mapram_config.mapram_type = type;
    mapram_config.mapram_logical_table = logical_id;
    mapram_config.mapram_vpn_members = 0;
    if (!options.match_compiler)  // FIXME -- glass doesn't set this?
        mapram_config.mapram_vpn = vpn;
    if (gress == INGRESS)
        mapram_config.mapram_ingress = 1;
    else
        mapram_config.mapram_egress = 1;
    mapram_config.mapram_enable = 1;
    mapram_config.mapram_ecc_check = 1;
    mapram_config.mapram_ecc_generate = 1;
    if (gress) regs.cfg_regs.mau_cfg_mram_thread[col / 3U] |= 1U << (col % 3U * 8U + row);
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void Table::write_mapram_regs, mau_regs &, int,
                      int, int, int)

HashDistribution *Table::find_hash_dist(int unit) {
    for (auto &hd : hash_dist)
        if (hd.id == unit) return &hd;
    for (auto t : get_match_tables())
        for (auto &hd : t->hash_dist)
            if (hd.id == unit) return &hd;
    if (auto *a = get_attached())
        for (auto &call : a->meters)
            for (auto &hd : call->hash_dist)
                if (hd.id == unit) return &hd;
    return nullptr;
}

int Table::find_on_actionbus(const char *name, TableOutputModifier mod, int lo, int hi, int size,
                             int *len) {
    return action_bus ? action_bus->find(name, mod, lo, hi, size, len) : -1;
}

void Table::need_on_actionbus(Table *att, TableOutputModifier mod, int lo, int hi, int size) {
    if (!action_bus) action_bus = ActionBus::create();
    action_bus->need_alloc(this, att, mod, lo, hi, size);
}

int Table::find_on_actionbus(const ActionBusSource &src, int lo, int hi, int size, int pos) {
    return action_bus ? action_bus->find(src, lo, hi, size, pos) : -1;
}

void Table::need_on_actionbus(const ActionBusSource &src, int lo, int hi, int size) {
    if (!action_bus) action_bus = ActionBus::create();
    action_bus->need_alloc(this, src, lo, hi, size);
}

int Table::find_on_ixbar(Phv::Slice sl, InputXbar::Group group, InputXbar::Group *found) {
    for (auto &ixb : input_xbar) {
        if (auto *i = ixb->find(sl, group, found)) {
            unsigned bit = (i->lo + sl.lo - i->what->lo);
            BUG_CHECK(bit < 128);
            return bit / 8;
        }
    }
    if (group.index >= 0) {
        for (auto *in : stage->ixbar_use[group]) {
            if (auto *i = in->find(sl, group)) {
                unsigned bit = (i->lo + sl.lo - i->what->lo);
                BUG_CHECK(bit < 128);
                return bit / 8;
            }
        }
    } else {
        for (auto &g : Keys(stage->ixbar_use)) {
            if (g.type != group.type) continue;
            int t;
            if ((t = find_on_ixbar(sl, g)) >= 0) {
                if (found) *found = g;
                return t;
            }
        }
    }
    return -1;
}

int Table::json_memunit(const MemUnit &r) const {
    if (r.stage >= 0) {
        return r.stage * Target::SRAM_STRIDE_STAGE() + r.row * Target::SRAM_STRIDE_ROW() +
               r.col * Target::SRAM_STRIDE_COLUMN();
    } else if (r.row >= 0) {
        // per-stage physical sram
        return r.row * Target::SRAM_UNITS_PER_ROW() + r.col;
    } else {
        // lamb
        return r.col;
    }
}

std::unique_ptr<json::map> Table::gen_memory_resource_allocation_tbl_cfg(
    const char *type, const std::vector<Layout> &layout, bool skip_spare_bank) const {
    int width, depth, period;
    const char *period_name;
    // FIXME -- calling vpn_params here is only valid when layout == this->layout, but we also
    // FIXME -- get here for color_maprams.  It works out as we don't use depth or width, only
    // FIXME -- period, which will always be 1 for meter layout or color_maprams
    vpn_params(width, depth, period, period_name);
    json::map mra;
    mra["memory_type"] = type;
    std::vector<std::map<int, int>> mem_units;
    json::vector &mem_units_and_vpns = mra["memory_units_and_vpns"] = json::vector();
    int vpn_ctr = 0;
    bool no_vpns = false;
    int spare_vpn;
    std::vector<int> spare_mem;

    // Retrieve the Spare banks
    // skip_spare_bank is only false on tables don't have spare banks, or when building
    // memory_units json for map rams
    if (skip_spare_bank) {
        BUG_CHECK(&layout == &this->layout, "layout not matching");
        spare_mem = determine_spare_bank_memory_units();
        BUG_CHECK(!spare_mem.empty(), "No spare banks in %s?", name());
        // if all the mems are "spare" this is really a DDP table, so we want to
        // put the usits/vpns of the spares in the memory_units json
        if (spare_mem.size() == layout_size()) skip_spare_bank = false;
    } else if (&layout == &this->layout) {
        BUG_CHECK(determine_spare_bank_memory_units().empty(),
                  "%s has spare banks, but we're not skipping them?", name());
    }

    for (auto &row : layout) {
        int word = row.word >= 0 ? row.word : 0;
        auto vpn_itr = row.vpns.begin();
        for (auto &ram : row.memunits) {
            BUG_CHECK(ram.row == row.row, "bogus %s in row %d", ram.desc(), row.row);
            if (vpn_itr == row.vpns.end())
                no_vpns = true;
            else
                vpn_ctr = *vpn_itr++;
            if (size_t(vpn_ctr) >= mem_units.size()) mem_units.resize(vpn_ctr + 1);
            // Create a vector indexed by vpn no where each element is a map
            // having a RAM entry indexed by word number
            // VPN WORD RAM
            //  0 -> 0   90
            //       1   91
            //  1 -> 0   92
            //       1   93
            //  E.g. VPN 0 has Ram 90 with word 0 and Ram 91 with word 1
            int unit = json_memunit(ram);
            if (skip_spare_bank &&
                std::find(spare_mem.begin(), spare_mem.end(), unit) != spare_mem.end())
                continue;
            mem_units[vpn_ctr][word] = json_memunit(ram);
        }
    }
    if (mem_units.size() == 0) return nullptr;
    int vpn = 0;
    for (auto &mem_unit : mem_units) {
        json::vector mem;
        // Below for loop orders the mem unit as { .., word1, word0 } which is
        // assumed to be what driver expects.
        for (int word = mem_unit.size() - 1; word >= 0; word--) {
            for (auto m : mem_unit) {
                if (m.first == word) {
                    mem.push_back(m.second);
                    break;
                }
            }
        }
        if (mem.size() != 0) {
            json::map tmp;
            tmp["memory_units"] = std::move(mem);
            json::vector vpns;
            if (no_vpns)
                vpns.push_back(nullptr);
            else
                vpns.push_back(vpn);
            tmp["vpns"] = std::move(vpns);
            mem_units_and_vpns.push_back(std::move(tmp));
        }
        vpn++;
    }
    if (skip_spare_bank && spare_mem.size() != 0) {
        if (spare_mem.size() == 1) {
            mra["spare_bank_memory_unit"] = spare_mem[0];
        } else {
            json::vector &spare = mra["spare_bank_memory_unit"];
            for (auto u : spare_mem) spare.push_back(u);
        }
    }
    return json::mkuniq<json::map>(std::move(mra));
}

json::map *Table::base_tbl_cfg(json::vector &out, const char *type, int size) const {
    auto tbl = p4_table->base_tbl_cfg(out, size, this);
    if (context_json) add_json_node_to_table(*tbl, "user_annotations");
    return tbl;
}

json::map *Table::add_stage_tbl_cfg(json::map &tbl, const char *type, int size) const {
    json::vector &stage_tables = tbl["stage_tables"];
    json::map stage_tbl;
    stage_tbl["stage_number"] = stage->stageno;
    stage_tbl["size"] = size;
    stage_tbl["stage_table_type"] = type;
    stage_tbl["logical_table_id"] = logical_id;
    if (physical_ids) {
        // this is only used by the driver to set miss entry imem/iad/next, so it should
        // not matter which physical table it is set on if there are multiple
        stage_tbl["physical_table_id"] = *physical_ids.begin();
    }

    if (this->to<MatchTable>()) {
        stage_tbl["has_attached_gateway"] = false;
        if (get_gateway()) stage_tbl["has_attached_gateway"] = true;
    }
    if (!strcmp(type, "selection") && get_stateful())
        tbl["bound_to_stateful_table_handle"] = get_stateful()->handle();
    if (Target::SUPPORT_ALWAYS_RUN() && (this->to<MatchTable>() || this->to<GatewayTable>()))
        stage_tbl["always_run"] = is_always_run();

    stage_tables.push_back(std::move(stage_tbl));
    return &(stage_tables.back()->to<json::map>());
}

/**
 * One can no longer use whether the table is directly or indirectly addressed on whether
 * a table is referenced that way.  This is due to the corner case on hash action tables
 * For a hash action table, an attached table that was previously directly addressed is now
 * addressed by hash.  However, for the driver, the driver must know which tables used to be
 * directly addressed vs. an attached table that is addressed by a hash based index.
 *
 * Thus, for those corner cases, a how_referenced in the p4 tag of the attached table is
 * currently provided.  Really for an attached table in hardware, it has no sense of how the
 * table is addressed, as it only receives an address, so if somehow two tables, where one was
 * direct while another was indirect (which is theoretically supportable if a hash action direct
 * counter is shared), would break this parameter.
 *
 * However, for the moment, there are no realistic attached table not either directly or indirectly
 * referenced
 *
 * If we need to change this, this was the delineation for how this was determined in match tables:
 *
 * In the call for both of these examples, the address field is a hash_dist object, as this is
 * necessary for the set up of the address.  This call, unlike every other type table, cannot
 * be the place where the address is determined.
 *
 * Instead, the attached calls in the action is how the assembler can delineate whether the
 * reference table is direct or indirect.  If the address argument is $DIRECT, then the direct
 * table has been converted to a hash, however if the argument is $hash_dist, then the original
 * call was from a hash-based index, and is indirect
 */
void Table::add_reference_table(json::vector &table_refs, const Table::Call &c) const {
    if (c) {
        auto t_name = c->name();
        if (c->p4_table) {
            t_name = c->p4_table->p4_name();
            if (!t_name) {
                error(-1, "No p4 table name found for table : %s", c->name());
                return;
            }
        }
        // Dont add ref table if already present in table_refs vector
        for (auto &tref : table_refs) {
            auto tref_name = tref->to<json::map>()["name"];
            if (!strcmp(tref_name->as_string()->c_str(), t_name)) return;
        }
        json::map table_ref;
        std::string hr = c->to<AttachedTable>()->how_referenced();
        if (hr.empty()) hr = c->to<AttachedTable>()->is_direct() ? "direct" : "indirect";
        table_ref["how_referenced"] = hr;
        table_ref["handle"] = c->handle();
        table_ref["name"] = t_name;
        auto mtr = c->to<MeterTable>();
        if (mtr && mtr->uses_colormaprams()) {
            BUG_CHECK(mtr->color_mapram_addr != MeterTable::NO_COLOR_MAP,
                      "inconsistent color mapram address bus for %s", mtr->name());
            table_ref["color_mapram_addr_type"] =
                mtr->color_mapram_addr == MeterTable::IDLE_MAP_ADDR ? "idle" : "stats";
        }

        table_refs.push_back(std::move(table_ref));
    }
}

bool Table::is_directly_referenced(const Table::Call &c) const {
    if (c) {
        std::string hr = c->to<AttachedTable>()->how_referenced();
        if (hr.empty()) {
            if (c->to<AttachedTable>()->is_direct()) return true;
        }
    }
    return false;
}

json::map &Table::add_pack_format(json::map &stage_tbl, int memword, int words, int entries) const {
    json::map pack_fmt;
    pack_fmt["table_word_width"] = memword * words;
    pack_fmt["memory_word_width"] = memword;
    if (entries >= 0) pack_fmt["entries_per_table_word"] = entries;
    pack_fmt["number_memory_units_per_table_word"] = words;
    json::vector &pack_format = stage_tbl["pack_format"];
    pack_format.push_back(std::move(pack_fmt));
    return pack_format.back()->to<json::map>();
}

void Table::canon_field_list(json::vector &field_list) const {
    for (auto &field_ : field_list) {
        auto &field = field_->to<json::map>();
        auto &name = field["field_name"]->to<json::string>();
        if (int lo = remove_name_tail_range(name)) field["start_bit"]->to<json::number>().val += lo;
    }
}

std::vector<Table::Call> Table::get_calls() const {
    std::vector<Call> rv;
    if (action) rv.emplace_back(action);
    if (instruction) rv.emplace_back(instruction);
    return rv;
}

/**
 * Determines both the start bit and the source name in the context JSON node for a particular
 * field.  Do not like string matching, and this should potentially be determined by looking
 * through a list of fields, but this will work in the short term
 */
void Table::get_cjson_source(const std::string &field_name, std::string &source,
                             int &start_bit) const {
    source = "spec";
    if (field_name == "hash_group") {
        source = "proxy_hash";
    } else if (field_name == "version") {
        source = "version";
    } else if (field_name == "immediate") {
        source = "immediate";
    } else if (field_name == "action") {
        source = "instr";
    } else if (field_name == "next") {
        source = "next_table";
    } else if (field_name == "action_addr") {
        source = "adt_ptr";
        if (auto adt = action->to<ActionTable>()) start_bit = std::min(5U, adt->get_log2size() - 2);
    } else if (field_name == "counter_addr") {
        source = "stats_ptr";
        auto a = get_attached();
        if (a && a->stats.size() > 0) {
            auto s = a->stats[0];
            start_bit = s->address_shift();
        }
    } else if (field_name == "counter_pfe") {
        source = "stats_ptr";
        start_bit = STATISTICS_PER_FLOW_ENABLE_START_BIT;
    } else if (field_name == "meter_addr") {
        if (auto m = get_meter()) {
            source = "meter_ptr";
            start_bit = m->address_shift();
        } else if (auto s = get_selector()) {
            source = "sel_ptr";
            start_bit = s->address_shift();
        } else if (auto s = get_stateful()) {
            source = "stful_ptr";
            start_bit = s->address_shift();
        } else {
            error(lineno, "Table %s has a meter_addr but no attached meter", name());
        }
    } else if (field_name == "meter_pfe") {
        if (get_meter()) {
            source = "meter_ptr";
        } else if (get_selector()) {
            source = "sel_ptr";
        } else if (get_stateful()) {
            source = "stful_ptr";
        } else {
            error(lineno, "Table %s has a meter_pfe but no attached meter", name());
        }
        start_bit = METER_PER_FLOW_ENABLE_START_BIT;
    } else if (field_name == "meter_type") {
        if (get_meter())
            source = "meter_ptr";
        else if (get_selector())
            source = "sel_ptr";
        else if (get_stateful())
            source = "stful_ptr";
        else
            error(lineno, "Table %s has a meter_type but no attached meter", name());
        start_bit = METER_TYPE_START_BIT;
    } else if (field_name == "sel_len_mod") {
        source = "selection_length";
    } else if (field_name == "sel_len_shift") {
        source = "selection_length_shift";
    } else if (field_name == "valid") {
        source = "valid";
    }
}

/**
 * Adds a field into the format of either a match or action table.  Honestly, this is used
 * for both action data tables and match tables, and this should be split up into two
 * separate functions, as the corner casing for these different cases can be quite different
 * and lead to some significant confusion
 */
void Table::add_field_to_pack_format(json::vector &field_list, int basebit, std::string name,
                                     const Table::Format::Field &field,
                                     const Table::Actions::Action *act) const {
    decltype(act->reverse_alias()) aliases;
    if (act) aliases = act->reverse_alias();
    auto alias = get(aliases, name);

    // we need to add only those aliases that are parameters, and there can be multiple
    // such fields that contain slices of one or more other aliases
    // FIXME: why aren't we de-aliasing in setup?
    for (auto a : alias) {
        json::string param_name = a->first;
        int lo = remove_name_tail_range(param_name);
        if (act->has_param(param_name) || a->second.is_constant) {
            auto newField = field;
            if (a->second.hi != -1) {
                unsigned fieldSize = a->second.hi - a->second.lo + 1;
                if (field.bits.size() > 1) warning(0, "multiple bit ranges for %s", name.c_str());
                newField =
                    Table::Format::Field(field.fmt, fieldSize, a->second.lo + field.bits[0].lo,
                                         static_cast<Format::Field::flags_t>(field.flags));
            }
            act->check_conditional(newField);

            if (a->second.is_constant)
                output_field_to_pack_format(field_list, basebit, a->first, "constant", 0, newField,
                                            a->second.value);
            else
                output_field_to_pack_format(field_list, basebit, a->first, "spec", 0, newField);
        }
    }

    // Determine the source of the field. If called recursively for an alias,
    // act will be a nullptr
    std::string source = "";
    int start_bit = 0;
    if (!act) get_cjson_source(name, source, start_bit);

    if (field.flags == Format::Field::ZERO) source = "zero";

    if (source != "")
        output_field_to_pack_format(field_list, basebit, name, source, start_bit, field);

    // Convert fields with slices embedded in the name, eg. "foo.bar[4:0]", to
    // slice-free field names with the start_bit incremented by the low bit of
    // the slice.
    canon_field_list(field_list);
}

void Table::output_field_to_pack_format(json::vector &field_list, int basebit, std::string name,
                                        std::string source, int start_bit,
                                        const Table::Format::Field &field, unsigned value) const {
    unsigned add_width = 0;
    bool pfe_enable = false;
    unsigned indirect_addr_start_bit = 0;
    int lobit = 0;
    for (auto &bits : field.bits) {
        json::map field_entry;
        field_entry["start_bit"] = lobit + start_bit;
        field_entry["field_width"] = bits.size() + add_width;
        field_entry["lsb_mem_word_idx"] = bits.lo / MEM_WORD_WIDTH;
        field_entry["msb_mem_word_idx"] = bits.hi / MEM_WORD_WIDTH;
        field_entry["source"] = json::string(source);
        field_entry["enable_pfe"] = false;
        if (source == "constant") {
            field_entry["const_tuples"] =
                json::vector{json::map{{"dest_start", json::number(0)},
                                       {"value", json::number(value)},
                                       {"dest_width", json::number(bits.size())}}};
        }
        field_entry["lsb_mem_word_offset"] = basebit + (bits.lo % MEM_WORD_WIDTH);
        field_entry["field_name"] = json::string(name);
        field_entry["global_name"] = json::string("");

        if (field.conditional_value) {
            field_entry["is_mod_field_conditionally_value"] = true;
            field_entry["mod_field_conditionally_mask_field_name"] = json::string(field.condition);
        }
        // field_entry["immediate_name"] = json::string(immediate_name);
        // if (this->to<ExactMatchTable>())
        if (this->to<SRamMatchTable>()) {
            // FIXME-JSON : match_mode only matters for ATCAM's not clear if
            // 'unused' or 'exact' is used by driver
            std::string match_mode = "unused";
            // For version bits field match mode is set to "s1q0" (to match
            // glass)
            if (name == "version") match_mode = "s1q0";
            field_entry["match_mode"] = match_mode;
        }
        field_list.push_back(std::move(field_entry));
        lobit += bits.size();
    }
}

void Table::add_zero_padding_fields(Table::Format *format, Table::Actions::Action *act,
                                    unsigned format_width) const {
    if (!format) return;
    // For an action with no format pad zeros for action table size
    unsigned pad_count = 0;
    if (format->log2size == 0) {
        if (auto at = this->to<ActionTable>()) {
            format->size = at->get_size();
            BUG_CHECK(format->size);
            format->log2size = at->get_log2size();
            // For wide action formats, entries per word is 1, so plug in a
            // single pad field of 256 bits
            unsigned action_entries_per_word = std::max(1U, 128U / format->size);
            // Add a flag type to specify padding?
            Format::Field f(format, format->size, 0, Format::Field::ZERO);
            for (unsigned i = 0; i < action_entries_per_word; i++)
                format->add_field(f, "--padding--");
        } else {
            error(lineno,
                  "Adding zero padding to a non action table "
                  "which has no action entries in format");
        }
        return;
    }
    decltype(act->reverse_alias()) alias;
    if (act) alias = act->reverse_alias();

    // Determine the zero padding necessary by creating a bitvector that has all
    // bits cleared, and then iterate through parameters and immediates and set the
    // bits that are used. Create padding for the remaining bit ranges.
    bitvec padbits;
    padbits.clrrange(0, format_width - 1);
    for (int entry = 0; entry < format->groups(); ++entry) {
        for (auto &field : format->group(entry)) {
            auto aliases = get(alias, field.first);
            for (auto a : aliases) {
                auto newField = field.second;
                json::string param_name = a->first;
                int lo = remove_name_tail_range(param_name);
                if (act->has_param(param_name) || a->second.is_constant) {
                    auto newField = Table::Format::Field(
                        field.second.fmt, a->second.size(), a->second.lo + field.second.bits[0].lo,
                        static_cast<Format::Field::flags_t>(field.second.flags));
                    newField.set_field_bits(padbits);
                }
            }
            if (aliases.size() == 0) field.second.set_field_bits(padbits);
        }
    }

    int idx_lo = 0;
    for (auto p : padbits) {
        if (p > idx_lo) {
            Format::Field f(format, p - idx_lo, idx_lo, Format::Field::ZERO);
            std::string pad_name =
                "--padding_" + std::to_string(idx_lo) + "_" + std::to_string(p - 1) + "--";
            format->add_field(f, pad_name);
        }
        idx_lo = p + 1;
    }
    if (idx_lo < int(format_width)) {
        Format::Field f(format, format_width - idx_lo, idx_lo, Format::Field::ZERO);
        std::string pad_name =
            "--padding_" + std::to_string(idx_lo) + "_" + std::to_string(format_width - 1) + "--";
        format->add_field(f, pad_name);
    }
}

json::map &Table::add_pack_format(json::map &stage_tbl, Table::Format *format, bool pad_zeros,
                                  bool print_fields, Table::Actions::Action *act) const {
    // Add zero padding fields to format
    // FIXME: Can this be moved to a format pass?
    if (pad_zeros)
        add_zero_padding_fields(format, act, format ? format->get_padding_format_width() : -1);
    json::map pack_fmt;
    auto mem_word_width = ram_word_width();
    pack_fmt["memory_word_width"] = mem_word_width;
    auto table_word_width = format ? format->get_table_word_width() : ram_word_width();
    pack_fmt["table_word_width"] = table_word_width;
    pack_fmt["entries_per_table_word"] = format ? format->get_entries_per_table_word() : 1;
    pack_fmt["number_memory_units_per_table_word"] =
        format ? format->get_mem_units_per_table_word() : 1;

    /**
     * Entry number has to be unique for all tables.  However, for ATCAM tables specifically,
     * the entry with the highest priority starts at entry number 0.  The priority decreases
     * as the entry number increases.
     *
     * This is actually reversed in the hardware.  The compiler format entries are in priority
     * order in the hardware, and have been validated in validate_format.  Thus, the context
     * JSON is reversed.
     */
    if (print_fields) {
        BUG_CHECK(format);
        int basebit = std::max(0, mem_word_width - (1 << format->log2size));
        json::vector &entry_list = pack_fmt["entries"];
        if (format->is_wide_format()) {
            for (int i = format->groups() - 1; i >= 0; --i) {
                int entry_number = i;
                if (table_type() == ATCAM) entry_number = format->groups() - 1 - i;
                json::vector field_list;
                for (auto it = format->begin(i); it != format->end(i); ++it)
                    add_field_to_pack_format(field_list, basebit, it->first, it->second, act);
                entry_list.push_back(json::map{{"entry_number", json::number(entry_number)},
                                               {"fields", std::move(field_list)}});
            }
        } else {
            for (int i = format->get_entries_per_table_word() - 1; i >= 0; --i) {
                int entry_number = i;
                if (table_type() == ATCAM)
                    entry_number = format->get_entries_per_table_word() - 1 - i;
                json::vector field_list;
                for (auto &field : *format)
                    add_field_to_pack_format(field_list, basebit, field.first, field.second, act);
                entry_list.push_back(json::map{{"entry_number", json::number(entry_number)},
                                               {"fields", std::move(field_list)}});
                basebit -= 1 << format->log2size;
            }
        }
    }
    if (act) pack_fmt["action_handle"] = act->handle;
    json::vector &pack_format = stage_tbl["pack_format"];
    pack_format.push_back(std::move(pack_fmt));
    return pack_format.back()->to<json::map>();
}

// Check if node exists in context_json entry in bfa, add entry to the input
// json node and remove the entry from context_json.
//
// Set parameter "append" to true in order to append to existing entries in
// specified section of context_json.  Set to false to overwrite.  Applies
// only to json::vector containers.
bool Table::add_json_node_to_table(json::map &tbl, const char *name, bool append) const {
    if (context_json) {
        if (context_json->count(name)) {
            std::unique_ptr<json::obj> new_obj = context_json->remove(name);
            json::vector *add_vect = nullptr;
            if (append && (add_vect = dynamic_cast<json::vector *>(new_obj.get()))) {
                json::vector &new_vect = tbl[name];
                std::move(add_vect->begin(), add_vect->end(), std::back_inserter(new_vect));
            } else
                tbl[name] = std::move(new_obj);
            return true;
        }
    }
    return false;
}

void Table::add_match_key_cfg(json::map &tbl) const {
    json::vector &params = tbl["match_key_fields"];
    if ((!p4_params_list.empty()) && this->to<MatchTable>()) {
        // If a table is splitted to different stages in backend, the
        // match_key_fields section will be populated every time the splitted
        // tables are emitted. Therefore, we clear the vector before populating
        // it again to avoid duplicated keys.
        params.clear();
        for (auto &p : p4_params_list) {
            json::map param;
            std::string name = p.name;
            std::string global_name = "";
            if (p.key_name.empty()) {
                param["name"] = name;
            } else {
                // Presence of key name indicates the field has a name
                // annotation. If the name annotation is on a field slice, then
                // the slice is treated as a field with the key_name as its
                // "name". The field output will have the same bit_width and
                // bit_width_full indicating its not treated as a slice.  We
                // also provide the original p4 name as the "global_name" to
                // allow driver to use it as a lookup up against the snapshot
                // fields published in context.json. These fields will all have
                // original p4 field names.
                param["name"] = p.key_name;
                param["global_name"] = p.name;
            }
            param["start_bit"] = p.start_bit;
            param["bit_width"] = p.bit_width;
            param["bit_width_full"] = p.bit_width_full;
            if (!p.mask.empty()) {
                std::stringstream ss;
                ss << "0x" << p.mask;
                param["mask"] = ss.str();
            }
            param["position"] = p.position;
            param["match_type"] = p.type;
            param["is_valid"] = p.is_valid;
            std::string fieldname, instname;
            gen_instfield_name(name, instname, fieldname);
            param["instance_name"] = instname;
            param["field_name"] = fieldname;
            if (!p.alias.empty()) param["alias"] = p.alias;
            if (p.context_json) param.merge(*p.context_json.get());
            params.push_back(std::move(param));
            if (p.type == "range") tbl["uses_range"] = true;
        }
    }
}

template <typename T>
void Table::init_json_node(json::map &tbl, const char *name) const {
    if (tbl.count(name)) return;
    tbl[name] = T();
}

void Table::common_tbl_cfg(json::map &tbl) const {
    tbl["default_action_handle"] = get_default_action_handle();
    tbl["action_profile"] = action_profile();
    // FIXME -- setting next_table_mask unconditionally only works because we process the
    // stage table in stage order (so we'll end up with the value from the last stage table,
    // which is what we want.)  Should we check in case the ordering ever changes?
    tbl["default_next_table_mask"] = next_table_adr_mask;
    // FIXME -- the driver currently always assumes this is 0, so we arrange for it to be
    // when choosing the action encoding.  But we should be able to choose something else
    tbl["default_next_table_default"] = 0;
    // FIXME-JSON: PD related, check glass examples for false (ALPM)
    tbl["is_resource_controllable"] = true;
    tbl["uses_range"] = false;
    if (p4_table && p4_table->disable_atomic_modify) tbl["disable_atomic_modify"] = true;
    add_match_key_cfg(tbl);
    init_json_node<json::vector>(tbl, "ap_bind_indirect_res_to_match");
    init_json_node<json::vector>(tbl, "static_entries");
    if (context_json) {
        add_json_node_to_table(tbl, "ap_bind_indirect_res_to_match");
    }
}

void Table::add_result_physical_buses(json::map &stage_tbl) const {
    json::vector &result_physical_buses = stage_tbl["result_physical_buses"] = json::vector();
    for (auto l : layout) {
        if (l.bus.count(Layout::RESULT_BUS))
            result_physical_buses.push_back(l.row * 2 + l.bus.at(Layout::RESULT_BUS));
    }
}

void Table::merge_context_json(json::map &tbl, json::map &stage_tbl) const {
    if (context_json) {
        add_json_node_to_table(tbl, "static_entries", true);
        stage_tbl.merge(*context_json);
    }
}
