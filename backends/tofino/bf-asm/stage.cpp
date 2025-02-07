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

#include <config.h>
#include <time.h>

#include <fstream>

#include "deparser.h"
#include "input_xbar.h"
#include "misc.h"
#include "parser.h"
#include "phv.h"
#include "lib/range.h"
#include "sections.h"
#include "backends/tofino/bf-asm/target.h"
#include "top_level.h"

extern std::string asmfile_name;

unsigned char Stage::action_bus_slot_map[ACTION_DATA_BUS_BYTES];
unsigned char Stage::action_bus_slot_size[ACTION_DATA_BUS_SLOTS];

AsmStage AsmStage::singleton_object;

#include "jbay/stage.cpp"    // NOLINT(build/include)
#include "tofino/stage.cpp"  // NOLINT(build/include)

AsmStage::AsmStage() : Section("stage") {
    int slot = 0, byte = 0;
    for (int i = 0; i < ACTION_DATA_8B_SLOTS; i++) {
        Stage::action_bus_slot_map[byte++] = slot;
        Stage::action_bus_slot_size[slot++] = 8;
    }
    for (int i = 0; i < ACTION_DATA_16B_SLOTS; i++) {
        Stage::action_bus_slot_map[byte++] = slot;
        Stage::action_bus_slot_map[byte++] = slot;
        Stage::action_bus_slot_size[slot++] = 16;
    }
    for (int i = 0; i < ACTION_DATA_32B_SLOTS; i++) {
        Stage::action_bus_slot_map[byte++] = slot;
        Stage::action_bus_slot_map[byte++] = slot;
        Stage::action_bus_slot_map[byte++] = slot;
        Stage::action_bus_slot_map[byte++] = slot;
        Stage::action_bus_slot_size[slot++] = 32;
    }
    BUG_CHECK(byte == ACTION_DATA_BUS_BYTES);
    BUG_CHECK(slot == ACTION_DATA_BUS_SLOTS);
}

void AsmStage::start(int lineno, VECTOR(value_t) args) {
    while (int(pipe.size()) < Target::NUM_MAU_STAGES()) pipe.emplace_back(pipe.size(), false);
    if (args.size != 2 || args[0].type != tINT ||
        (args[1] != "ingress" && args[1] != "egress" &&
         (args[1] != "ghost" || options.target < JBAY))) {
        error(lineno, "stage must specify number and ingress%s or egress",
              options.target >= JBAY ? ", ghost" : "");
    } else if (args[0].i < 0) {
        error(lineno, "invalid stage number");
    } else if ((unsigned)args[0].i >= pipe.size()) {
        while ((unsigned)args[0].i >= pipe.size()) pipe.emplace_back(pipe.size(), false);
    }
}

void AsmStage::input(VECTOR(value_t) args, value_t data) {
    if (!CHECKTYPE(data, tMAP)) return;
    int stageno = args[0].i;
    gress_t gress =
        args[1] == "ingress"  ? INGRESS
        : args[1] == "egress" ? EGRESS
        : args[1] == "ghost" && options.target >= JBAY
            ? GHOST
            : (error(args[1].lineno, "Invalid thread %s", value_desc(args[1])), INGRESS);
    auto &stage = stages(gress);
    BUG_CHECK(stageno >= 0 && (unsigned)stageno < stage.size());
    if (stages_seen[gress][stageno])
        error(args[0].lineno, "Duplicate stage %d %s", stageno, to_string(gress).c_str());
    stages_seen[gress][stageno] = 1;
    for (auto &kv : MapIterChecked(data.map, true)) {
        if (kv.key == "dependency") {
            if (stageno == 0) warning(kv.key.lineno, "Stage dependency in stage 0 will be ignored");
            if (gress == GHOST) {
                error(kv.key.lineno,
                      "Can't specify dependency in ghost thread; it is "
                      "locked to ingress");
            } else if (kv.value == "concurrent") {
                stage[stageno].stage_dep[gress] = Stage::CONCURRENT;
                if (stageno == Target::NUM_MAU_STAGES() / 2 && options.target == TOFINO)
                    error(kv.value.lineno, "stage %d must be match dependent", stageno);
                else if (!Target::SUPPORT_CONCURRENT_STAGE_DEP())
                    error(kv.value.lineno, "no concurrent execution on %s", Target::name());
            } else if (kv.value == "action") {
                stage[stageno].stage_dep[gress] = Stage::ACTION_DEP;
                if (stageno == Target::NUM_MAU_STAGES() / 2 && options.target == TOFINO)
                    error(kv.value.lineno, "stage %d must be match dependent", stageno);
            } else if (kv.value == "match") {
                stage[stageno].stage_dep[gress] = Stage::MATCH_DEP;
            } else {
                error(kv.value.lineno, "Invalid stage dependency %s", value_desc(kv.value));
            }
            continue;

        } else if (kv.key == "mpr_stage_id") {
            stage[stageno].verify_have_mpr(kv.key.s, kv.key.lineno);
            if CHECKTYPE (kv.value, tINT) {
                if (kv.value.i > stageno)
                    error(kv.value.lineno,
                          "mpr_stage_id value cannot be greater than current stage.");
                stage[stageno].mpr_stage_id[gress] = kv.value.i;

                /* Intermediate stage must carry the mpr glob_exec and long_branch bitmap.
                 * If they have been left off by the compiler, we need to propagate the bits;
                 * if the compiler has provided them, we assume it did so correctly
                 * DANGER -- this assumes the stages appear in the .bfa file in order (at
                 * least for each gress)
                 */
                if (kv.value.i != stageno) {
                    for (int inter_stage = kv.value.i + 1; inter_stage < stageno; inter_stage++) {
                        if (!stages_seen[gress][inter_stage]) {
                            stage[inter_stage].mpr_bus_dep_glob_exec[gress] |=
                                stage[kv.value.i].mpr_bus_dep_glob_exec[gress];
                            stage[inter_stage].mpr_bus_dep_long_branch[gress] |=
                                stage[kv.value.i].mpr_bus_dep_long_branch[gress];
                        }
                    }
                }
            }
            continue;
        } else if (kv.key == "mpr_always_run") {
            stage[stageno].verify_have_mpr(kv.key.s, kv.key.lineno);
            if CHECKTYPE (kv.value, tINT) {
                stage[stageno].mpr_always_run |= kv.value.i;
            }
            continue;
        } else if (kv.key == "mpr_bus_dep_glob_exec") {
            stage[stageno].verify_have_mpr(kv.key.s, kv.key.lineno);
            if CHECKTYPE (kv.value, tINT) {
                stage[stageno].mpr_bus_dep_glob_exec[gress] = kv.value.i;
            }
            continue;
        } else if (kv.key == "mpr_bus_dep_long_brch") {
            stage[stageno].verify_have_mpr(kv.key.s, kv.key.lineno);
            if CHECKTYPE (kv.value, tINT) {
                stage[stageno].mpr_bus_dep_long_branch[gress] = kv.value.i;
            }
            continue;
        } else if (kv.key == "mpr_next_table_lut") {
            stage[stageno].verify_have_mpr(kv.key.s, kv.key.lineno);
            if (CHECKTYPE(kv.value, tMAP)) {
                for (auto &lut : kv.value.map) {
                    if (!CHECKTYPE(lut.key, tINT) || lut.key.i >= LOGICAL_TABLES_PER_STAGE)
                        error(lut.key.lineno, "Invalid mpr_next_table_lut key.");
                    if (!CHECKTYPE(lut.value, tINT) ||
                        lut.value.i >= (1 << LOGICAL_TABLES_PER_STAGE))
                        error(lut.value.lineno, "Invalid mpr_next_table_lut value.");
                    stage[stageno].mpr_next_table_lut[gress][lut.key.i] = lut.value.i;
                }
            }
            continue;
        } else if (kv.key == "mpr_glob_exec_lut") {
            stage[stageno].verify_have_mpr(kv.key.s, kv.key.lineno);
            if (CHECKTYPE(kv.value, tMAP)) {
                for (auto &lut : kv.value.map) {
                    if (!CHECKTYPE(lut.key, tINT) || lut.key.i >= LOGICAL_TABLES_PER_STAGE)
                        error(lut.key.lineno, "Invalid mpr_glob_exec_lut key.");
                    if (!CHECKTYPE(lut.value, tINT) ||
                        lut.value.i >= (1 << LOGICAL_TABLES_PER_STAGE))
                        error(lut.value.lineno, "Invalid mpr_glob_exec_lut value.");
                    stage[stageno].mpr_glob_exec_lut[lut.key.i] |= lut.value.i;
                }
            }
            continue;
        } else if (kv.key == "mpr_long_brch_lut") {
            stage[stageno].verify_have_mpr(kv.key.s, kv.key.lineno);
            if (CHECKTYPE(kv.value, tMAP)) {
                for (auto &lut : kv.value.map) {
                    if (!CHECKTYPE(lut.key, tINT) || lut.key.i >= MAX_LONGBRANCH_TAGS)
                        error(lut.key.lineno, "Invalid mpr_long_brch_lut key.");
                    if (!CHECKTYPE(lut.value, tINT) ||
                        lut.value.i >= (1 << LOGICAL_TABLES_PER_STAGE))
                        error(lut.value.lineno, "Invalid mpr_long_brch_lut value.");
                    stage[stageno].mpr_long_brch_lut[lut.key.i] |= lut.value.i;
                }
            }
            continue;
        } else if (kv.key == "error_mode") {
            if (gress == GHOST)
                error(kv.key.lineno, "Can't specify error mode in ghost thread");
            else
                stage[stageno].error_mode[gress].input(kv.value);
            continue;
        } else if (Target::SUPPORT_ALWAYS_RUN() && kv.key == "always_run_action") {
            if (gress == GHOST)
                error(kv.key.lineno, "No always run action for ghost thread, must use ingress");
            else
                stage[stageno].tables.push_back(new AlwaysRunTable(gress, &stage[stageno], kv));
            continue;
        }
        if (!CHECKTYPEM(kv.key, tCMD, "table declaration")) continue;
        if (!CHECKTYPE(kv.value, tMAP)) continue;
        auto tt = Table::Type::get(kv.key[0].s);
        if (!tt) {
            error(kv.key[0].lineno, "Unknown table type '%s'", kv.key[0].s);
            continue;
        }
        if (kv.key.vec.size < 2) {
            error(kv.key.lineno, "Need table name");
            continue;
        }
        if (!CHECKTYPE(kv.key[1], tSTR)) continue;
        if (kv.key.vec.size > 2 && !CHECKTYPE(kv.key[2], tINT)) continue;
        if (kv.key.vec.size > 3) warning(kv.key[3].lineno, "Ignoring extra stuff after table");
        if (auto old = ::get(Table::all, kv.key[1].s)) {
            error(kv.key[1].lineno, "Table %s already defined", kv.key[1].s);
            warning(old->lineno, "previously defined here");
            continue;
        }
        if (Table *table = tt->create(kv.key.lineno, kv.key[1].s, gress, &stage[stageno],
                                      kv.key.vec.size > 2 ? kv.key[2].i : -1, kv.value.map)) {
            stage[stageno].tables.push_back(table);
        }
    }
}

void AsmStage::process() {
    for (auto &stage : pipe) {
        stage.pass1_logical_id = stage.pass1_tcam_id = -1;
        for (auto table : stage.tables) table->pass0();
    }
    for (auto &stage : pipe) {
        for (auto table : stage.tables) table->pass1();
        if (options.target == TOFINO) {
            if (&stage - &pipe[0] == Target::NUM_MAU_STAGES() / 2) {
                /* to turn the corner, the middle stage must always be match dependent */
                for (gress_t gress : Range(INGRESS, EGRESS))
                    stage.stage_dep[gress] = Stage::MATCH_DEP;
            }
        }
        if (options.match_compiler || 1) {
            /* FIXME -- do we really want to do this?  In theory different stages could
             * FIXME -- use the same PHV slots differently, but the compiler always uses them
             * FIXME -- consistently, so we need this to get bit-identical results
             * FIXME -- we also don't correctly determine liveness, so need this */
            for (gress_t gress : Range(INGRESS, GHOST)) {
                Phv::setuse(gress, stage.match_use[gress]);
                Phv::setuse(gress, stage.action_use[gress]);
                Phv::setuse(gress, stage.action_set[gress]);
            }
        }
    }
    for (auto &stage : pipe) {
        for (auto table : stage.tables) table->pass2();
        std::sort(stage.tables.begin(), stage.tables.end(),
                  [](Table *a, Table *b) { return a->logical_id < b->logical_id; });
    }
    for (auto &stage : pipe) {
        for (auto table : stage.tables) table->pass3();
    }
}

void AsmStage::output(json::map &ctxt_json) {
    if (int(pipe.size()) > Target::NUM_MAU_STAGES()) {
        auto lineno = pipe.back().tables.empty() ? 0 : pipe.back().tables[0]->lineno;
        error(lineno, "%s supports up to %d stages, using %zd", Target::name(),
              Target::NUM_MAU_STAGES(), pipe.size());
    }

    // If we encounter errors, no binary is generated, however we still proceed
    // to generate the context.json with whatever info is provided in the .bfa.
    // This can be inspected in p4i for debugging.
    if (error_count > 0) {
        options.binary = NO_BINARY;
        error(0, "Due to errors, no binary will be generated");
    }
    if (pipe.empty()) return;

    /* Allow to set any stage as match dependent based on a pattern - Should never be used for
     * normal compilation */
    if (options.target != TOFINO && !options.stage_dependency_pattern.empty()) {
        for (gress_t gress : Range(INGRESS, EGRESS)) {
            auto &stage = stages(gress);
            unsigned i = 0;
            for (auto ch : options.stage_dependency_pattern) {
                if (ch == '1') {
                    LOG1("explicitly setting stage " << i << " " << gress
                                                     << " as match dependent on previous stage");
                    stage[i].stage_dep[gress] = Stage::MATCH_DEP;
                }
                if (++i >= stage.size()) break;
            }
        }
    }

    for (gress_t gress : Range(INGRESS, EGRESS)) {
        auto &stage = stages(gress);
        bitvec set_regs = stage[0].action_set[gress];
        for (unsigned i = 1; i < stage.size(); i++) {
            if (!stage[i].stage_dep[gress]) {
                if (stage[i].match_use[gress].intersects(set_regs)) {
                    LOG1("stage " << i << " " << gress << " is match dependent on previous stage");
                    stage[i].stage_dep[gress] = Stage::MATCH_DEP;
                } else if (stage[i].action_use[gress].intersects(set_regs)) {
                    LOG1("stage " << i << " " << gress << " is action dependent on previous stage");
                    stage[i].stage_dep[gress] = Stage::ACTION_DEP;
                } else {
                    LOG1("stage " << i << " " << gress << " is concurrent with previous stage");
                    if (!Target::SUPPORT_CONCURRENT_STAGE_DEP())
                        stage[i].stage_dep[gress] = Stage::ACTION_DEP;
                    else
                        stage[i].stage_dep[gress] = Stage::CONCURRENT;
                }
            }
            if (stage[i].stage_dep[gress] == Stage::MATCH_DEP)
                set_regs = stage[i].action_set[gress];
            else
                set_regs |= stage[i].action_set[gress];
        }
    }

    // Propagate group_table_use so we can estimate latencies.
    propagate_group_table_use();

    // In Tofino, add match-dependent stages if latency is not the minimum
    // egress latency. There is no such requirement for JBAY - COMPILER-757
    if (options.target == TOFINO) {
        // Compute Egress Latency
        auto total_cycles = compute_latency(EGRESS);
        if (!options.disable_egress_latency_padding) {
            // Get non match dependent stages
            bitvec non_match_dep;
            for (unsigned i = 1; i < pipe.size(); i++) {
                auto stage_dep = pipe[i].stage_dep[EGRESS];
                if (stage_dep != Stage::MATCH_DEP) non_match_dep.setbit(i);
            }
            // Add match-dependent stages and re-evaluate latency
            while (total_cycles < Target::Tofino::MINIMUM_REQUIRED_EGRESS_PIPELINE_LATENCY) {
                if (non_match_dep == bitvec(0)) break;
                auto non_match_dep_stage = non_match_dep.min().index();
                pipe[non_match_dep_stage].stage_dep[EGRESS] = Stage::MATCH_DEP;
                LOG3("Converting egress stage "
                     << non_match_dep_stage
                     << " to match dependent to meet minimum egress pipeline latency requirement");
                non_match_dep.clrbit(non_match_dep_stage);
                total_cycles = compute_latency(EGRESS);
            }
        } else {
            if (total_cycles < Target::Tofino::MINIMUM_REQUIRED_EGRESS_PIPELINE_LATENCY) {
                warning(0,
                        "User disabled adding latency to the egress MAU pipeline "
                        "to meet its minimum requirements. This may result in under "
                        "run in certain port speed configurations.");
            }
        }
    }

    // Re-propagate group_table_use to account for any stages that may now be match dependent.
    propagate_group_table_use();

    for (auto &stage : pipe) SWITCH_FOREACH_TARGET(options.target, stage.output<TARGET>(ctxt_json);)

    if (options.log_hashes) {
        std::ofstream hash_out;
        std::string fname = options.output_dir + "/logs/mau.hashes.log";
        hash_out.open(fname.c_str());
        if (hash_out) {
            for (auto &stage : pipe) stage.log_hashes(hash_out);
            hash_out.close();
        }
    }
}

void AsmStage::propagate_group_table_use() {
    for (gress_t gress : Range(INGRESS, EGRESS)) {
        auto &stage = stages(gress);
        stage[0].group_table_use[gress] = stage[0].table_use[gress];
        for (unsigned i = 1; i < stage.size(); i++) {
            stage[i].group_table_use[gress] = stage[i].table_use[gress];
            if (stage[i].stage_dep[gress] != Stage::MATCH_DEP)
                stage[i].group_table_use[gress] |= stage[i - 1].group_table_use[gress];
        }
        for (int i = stage.size() - 1; i > 0; i--)
            if (stage[i].stage_dep[gress] != Stage::MATCH_DEP)
                stage[i - 1].group_table_use[gress] |= stage[i].group_table_use[gress];
    }
}

unsigned AsmStage::compute_latency(gress_t gress) {
    // FIXME -- this is Tofino1 only, so should be in target specific code somewhere
    auto total_cycles = 4;  // There are 4 extra cycles between stages 5 & 6 of the MAU
    for (unsigned i = 1; i < pipe.size(); i++) {
        auto stage_dep = pipe[i].stage_dep[gress];
        auto contribute = 0;
        if (stage_dep == Stage::MATCH_DEP) {
            contribute = pipe[i].pipelength(gress);
        } else if (stage_dep == Stage::ACTION_DEP) {
            contribute = 2;
        } else if (stage_dep == Stage::CONCURRENT) {
            contribute = 1;
        }
        total_cycles += contribute;
    }
    return total_cycles;
}

static FakeTable invalid_rams("RAMS NOT PRESENT");

std::map<int, std::pair<bool, int>> Stage_data::teop = {
    {0, {false, INT_MAX}}, {1, {false, INT_MAX}}, {2, {false, INT_MAX}}, {3, {false, INT_MAX}}};

Stage::Stage(int stage, bool egress_only) : Stage_data(stage, egress_only) {
    static_assert(sizeof(Stage_data) == sizeof(Stage),
                  "All non-static Stage fields must be in Stage_data");
    table_use[0] = table_use[1] = NONE;
    stage_dep[0] = stage_dep[1] = NONE;
    error_mode[0] = error_mode[1] = DefaultErrorMode::get();
    for (int i = 0; i < Target::SRAM_ROWS(egress_only ? EGRESS : INGRESS); i++)
        for (int j = 0; j < Target::SRAM_REMOVED_COLUMNS(); j++) sram_use[i][j] = &invalid_rams;
}

Stage::~Stage() {
    for (auto *ref : all_refs) *ref = nullptr;
}

int Stage::first_table(gress_t gress) {
    for (auto &st : AsmStage::stages(gress)) {
        int min_logical_id = INT_MAX;
        for (auto tbl : st.tables) {
            if (tbl->gress != gress) continue;
            if (tbl->logical_id < 0) continue;  // ignore phase 0
            if (tbl->logical_id < min_logical_id) min_logical_id = tbl->logical_id;
        }
        if (min_logical_id != INT_MAX) {
            BUG_CHECK((min_logical_id & ~0xf) == 0);
            return (st.stageno << 4) + min_logical_id;
        }
    }
    return -1;
}

Stage *Stage::stage(gress_t gress, int stageno) {
    if (stageno < 0 || stageno >= AsmStage::stages(gress).size()) return nullptr;
    return &AsmStage::stages(gress).at(stageno);
}

Stage::Stage(Stage &&a) : Stage_data(std::move(a)) {
    for (auto *ref : all_refs) *ref = this;
}

bitvec Stage::imem_use_all() const {
    bitvec rv;
    for (auto &u : imem_use) rv |= u;
    return rv;
}

int Stage::tcam_delay(gress_t gress) const {
    if (group_table_use[timing_thread(gress)] & Stage::USE_TCAM) return 2;
    if (group_table_use[timing_thread(gress)] & Stage::USE_WIDE_SELECTOR) return 2;
    return 0;
}

int Stage::adr_dist_delay(gress_t gress) const {
    if (group_table_use[timing_thread(gress)] & Stage::USE_SELECTOR)
        return 8;
    else if (group_table_use[timing_thread(gress)] & Stage::USE_STATEFUL_DIVIDE)
        return 6;
    else if (group_table_use[timing_thread(gress)] & Stage::USE_STATEFUL)
        return 4;
    else if (group_table_use[timing_thread(gress)] & Stage::USE_METER_LPF_RED)
        return 4;
    else
        return 0;
}

/* Calculate the meter_alu delay for a meter/stateful ALU based on both things
 * used globally in the current stage group, and whether this ALU uses a divmod
 * (in which case it will already have an extra 2-cycle delay */
int Stage::meter_alu_delay(gress_t gress, bool uses_divmod) const {
    if (group_table_use[timing_thread(gress)] & Stage::USE_SELECTOR)
        return uses_divmod ? 2 : 4;
    else if (group_table_use[timing_thread(gress)] & Stage::USE_STATEFUL_DIVIDE)
        return uses_divmod ? 0 : 2;
    else
        return 0;
}

int Stage::cycles_contribute_to_latency(gress_t gress) {
    if (stage_dep[gress] == MATCH_DEP || stageno == 0)
        return pipelength(gress);
    else if (stage_dep[gress] == CONCURRENT && options.target == TOFINO)
        return 1;
    else
        return 2;  // action dependency
}

int Stage::pipelength(gress_t gress) const {
    return Target::MAU_BASE_DELAY() + tcam_delay(gress) + adr_dist_delay(gress);
}

int Stage::pred_cycle(gress_t gress) const {
    return Target::MAU_BASE_PREDICATION_DELAY() + tcam_delay(gress);
}

void Stage::verify_have_mpr(std::string key, int line_number) {
    if (!Target::HAS_MPR())
        error(line_number, "%s is not available on target %s.", key.c_str(), Target::name());
}

template <class TARGET>
void Stage::write_common_regs(typename TARGET::mau_regs &regs) {
    /* FIXME -- most of the values set here are 'placeholder' constants copied
     * from build_pipeline_output_2.py in the compiler */
    auto &merge = regs.rams.match.merge;
    auto &adrdist = regs.rams.match.adrdist;
    // merge.exact_match_delay_config.exact_match_delay_ingress = tcam_delay(INGRESS);
    // merge.exact_match_delay_config.exact_match_delay_egress = tcam_delay(EGRESS);
    for (gress_t gress : Range(INGRESS, EGRESS)) {
        if (tcam_delay(gress) > 0) {
            merge.exact_match_delay_thread[0] |= 1U << gress;
            merge.exact_match_delay_thread[1] |= 1U << gress;
            merge.exact_match_delay_thread[2] |= 1U << gress;
        }
        regs.rams.match.adrdist.adr_dist_pipe_delay[gress][0] =
            regs.rams.match.adrdist.adr_dist_pipe_delay[gress][1] = adr_dist_delay(gress);
        regs.dp.action_output_delay[gress] = pipelength(gress) - 3;
        regs.dp.pipelength_added_stages[gress] = pipelength(gress) - TARGET::MAU_BASE_DELAY;
        if (stageno > 0 && stage_dep[gress] == MATCH_DEP)
            regs.dp.match_ie_input_mux_sel |= 1 << gress;
    }

    for (gress_t gress : Range(INGRESS, EGRESS)) {
        if (stageno == 0) {
            /* Credit is set to 2 - Every 512 cycles the credit is reset and every
             * bubble request decrements this credit. Acts like a filter to cap bubble
             * requests */
            adrdist.bubble_req_ctl[gress].bubble_req_fltr_crd = 0x2;
            adrdist.bubble_req_ctl[gress].bubble_req_fltr_en = 0x1;
        }
        adrdist.bubble_req_ctl[gress].bubble_req_interval = 0x100;
        adrdist.bubble_req_ctl[gress].bubble_req_en = 0x1;
        adrdist.bubble_req_ctl[gress].bubble_req_interval_eop = 0x100;
        adrdist.bubble_req_ctl[gress].bubble_req_en_eop = 0x1;
        adrdist.bubble_req_ctl[gress].bubble_req_ext_fltr_en = 0x1;
    }

    regs.dp.phv_fifo_enable.phv_fifo_ingress_action_output_enable =
        stage_dep[INGRESS] != ACTION_DEP;
    regs.dp.phv_fifo_enable.phv_fifo_egress_action_output_enable = stage_dep[EGRESS] != ACTION_DEP;
    if (stageno != AsmStage::numstages() - 1) {
        regs.dp.phv_fifo_enable.phv_fifo_ingress_final_output_enable =
            this[1].stage_dep[INGRESS] == ACTION_DEP;
        regs.dp.phv_fifo_enable.phv_fifo_egress_final_output_enable =
            this[1].stage_dep[EGRESS] == ACTION_DEP;
    }

    /* Error handling related */
    for (gress_t gress : Range(INGRESS, EGRESS)) error_mode[gress].write_regs(regs, this, gress);

    /*--------------------
     * Since a stats ALU enable bit is missing from mau_cfg_stats_alu_lt, need to make sure that for
     * unused stats ALUs, they are programmed to point to a logical table that is either unused or
     * to one that does not use a stats table. */

    bool unused_stats_alus = false;
    for (auto &salu : regs.cfg_regs.mau_cfg_stats_alu_lt)
        if (!salu.modified()) unused_stats_alus = true;
    if (unused_stats_alus) {
        unsigned avail = 0xffff;
        int no_stats = -1;
        /* odd pattern of tests to replicate what the old compiler does */
        for (auto tbl : tables) {
            avail &= ~(1U << tbl->logical_id);
            if (no_stats < 0 && (!tbl->get_attached() || tbl->get_attached()->stats.empty()))
                no_stats = tbl->logical_id;
        }
        if (avail) {
            for (int i = 15; i >= 0; --i)
                if ((avail >> i) & 1) {
                    no_stats = i;
                    break;
                }
        }
        for (auto &salu : regs.cfg_regs.mau_cfg_stats_alu_lt)
            if (!salu.modified()) salu = no_stats;
    }
}

void Stage::log_hashes(std::ofstream &out) const {
    out << "+-----------------------------------------------------------+" << std::endl;
    out << "   Stage " << stageno << std::endl;
    out << "+-----------------------------------------------------------+" << std::endl;
    bool logged = false;
    for (auto xbar : ixbar_use) {
        if (xbar.first.type == InputXbar::Group::EXACT) {
            for (auto use : xbar.second) {
                if (use) logged |= use->log_hashes(out);
            }
        }
    }
    if (!logged) {
        out << "  Unused" << std::endl;
    }
    // Need to use other variables?
    out << std::endl;
}

template <class REGS>
void Stage::gen_gfm_json_info(REGS &regs, std::ostream &out) {
    auto &hash = regs.dp.xbar_hash.hash;
    auto &gfm = hash.galois_field_matrix;
    out << &gfm << "\n";
    out << "Col  :    ";
    for (auto c = 0; c < GALOIS_FIELD_MATRIX_COLUMNS; c++) {
        out << std::setw(3) << c;
    }
    out << " | Row Parity \n";
    for (auto r = 0; r < gfm.size(); r++) {
        out << "Row " << std::dec << r << ": \n";
        out << "  Byte 0 :";
        unsigned byte0_parity = 0;
        unsigned byte1_parity = 0;
        for (auto c = 0; c < GALOIS_FIELD_MATRIX_COLUMNS; c++) {
            out << std::setw(3) << std::hex << gfm[r][c].byte0;
            byte0_parity ^= gfm[r][c].byte0;
        }
        out << " | " << std::setw(3) << parity(byte0_parity) << "\n";
        out << "  Byte 1 :";
        for (auto c = 0; c < GALOIS_FIELD_MATRIX_COLUMNS; c++) {
            out << std::setw(3) << std::hex << gfm[r][c].byte1;
            byte1_parity ^= gfm[r][c].byte1;
        }
        out << " | " << std::setw(3) << parity(byte1_parity) << "\n";
    }

    out << "\n";
    auto &grp_enable = regs.dp.hashout_ctl.hash_parity_check_enable;
    for (int grp = 0; grp < 8; grp++) {
        out << "Hash Group : " << grp << "\n";
        out << "Hash Seed : ";
        int seed_parity = 0;
        bitvec hash_seed;
        for (int bit = 51; bit >= 0; bit--) {
            auto seed_bit = (hash.hash_seed[bit] >> grp) & 0x1;
            hash_seed[bit] = seed_bit;
            out << seed_bit;
            seed_parity ^= seed_bit;
        }
        out << " (" << hash_seed << ")";
        out << "\n";
        auto seed_parity_enable = ((grp_enable >> grp) & 0x1) ? "True" : "False";
        out << "Hash Seed Parity Enable : " << seed_parity_enable;
        out << "\n";
        out << "Hash Seed Parity : " << (seed_parity ? "Odd" : "Even");
        out << "\n";
        out << "\n";
    }
}

template <class REGS>
void Stage::fixup_regs(REGS &regs) {
    if (options.condense_json) {
        // if any part of the gf matrix is enabled, we can't elide any part of it when
        // generating .cfg.json, as otherwise walle will generate an invalid block write
        if (options.gen_json && !regs.dp.xbar_hash.hash.galois_field_matrix.disabled())
            regs.dp.xbar_hash.hash.galois_field_matrix.enable();
    }
    // Enable mapram_config and imem regs -
    // These are cached by the driver, so if they are disabled they wont go
    // into tofino.bin as dma block writes and driver will complain
    // The driver needs the regs to do parity error correction at runtime and it
    // checks for the base address of the register blocks to do a block DMA
    // during tofino.bin download
    regs.dp.imem.enable();
    for (int row = 0; row < SRAM_ROWS; row++)
        for (int col = 0; col < MAPRAM_UNITS_PER_ROW; col++)
            regs.rams.map_alu.row[row].adrmux.mapram_config[col].enable();
}

template <class TARGET>
void Stage::output(json::map &ctxt_json, bool egress_only) {
    auto *regs = new typename TARGET::mau_regs();
    declare_registers(regs, egress_only, stageno);
    json::vector &ctxt_tables = ctxt_json["tables"];
    for (auto table : tables) {
        table->write_regs(*regs);
        table->gen_tbl_cfg(ctxt_tables);
        if (auto gw = table->get_gateway()) gw->gen_tbl_cfg(ctxt_tables);
    }
    write_regs(*regs, egress_only);

    // Output GFM
    if (gfm_out) gen_gfm_json_info(*regs, *gfm_out);

    if (options.condense_json) regs->disable_if_reset_value();

    fixup_regs(*regs);
    char buf[64];
    snprintf(buf, sizeof(buf), "regs.match_action_stage%s.%02x", egress_only ? ".egress" : "",
             stageno);
    if (error_count == 0 && options.gen_json)
        regs->emit_json(*open_output("%s.cfg.json", buf), stageno);
    auto NUM_STAGES = egress_only ? Target::NUM_EGRESS_STAGES() : Target::NUM_MAU_STAGES();
    if (stageno < NUM_STAGES) TopLevel::all->set_mau_stage(stageno, buf, regs, egress_only);
    gen_mau_stage_characteristics(*regs, ctxt_json["mau_stage_characteristics"]);
    gen_configuration_cache(*regs, ctxt_json["configuration_cache"]);
    if (stageno == NUM_STAGES - 1 && Target::OUTPUT_STAGE_EXTENSION())
        gen_mau_stage_extension(*regs, ctxt_json["mau_stage_extension"]);
}

template <class REGS>
void Stage::gen_mau_stage_characteristics(REGS &regs, json::vector &stg_characteristics) {
    for (gress_t gress : Range(INGRESS, EGRESS)) {
        json::map anon;
        anon["stage"] = stageno;
        anon["gress"] = P4Table::direction_name(gress);
        anon["match_dependent"] = (regs.dp.cur_stage_dependency_on_prev[gress] == 0) ? true : false;
        anon["clock_cycles"] = pipelength(gress);
        anon["predication_cycle"] = pred_cycle(gress);
        anon["cycles_contribute_to_latency"] = cycles_contribute_to_latency(gress);
        stg_characteristics.push_back(std::move(anon));
    }
}

template <class REGS>
void Stage::gen_configuration_cache(REGS &regs, json::vector &cfg_cache) {
    BUG();  // Must be specialized for target -- no generic implementation
}

template <class REGS>
void Stage::gen_configuration_cache_common(REGS &regs, json::vector &cfg_cache) {
    std::string reg_fqname;
    std::string reg_name;
    unsigned reg_value;
    std::string reg_value_str;
    unsigned reg_width = 8;  // this means number of hex characters

    // meter_sweep_ctl
    auto &meter_sweep_ctl = regs.rams.match.adrdist.meter_sweep_ctl;
    for (int i = 0; i < 4; i++) {
        reg_fqname = "mau[" + std::to_string(stageno) + "].rams.match.adrdist.meter_sweep_ctl[" +
                     std::to_string(i) + "]";
        if (options.match_compiler) {  // FIXME: Temp fix to match glass typo
            reg_fqname = "mau[" + std::to_string(stageno) +
                         "].rams.match.adrdist.meter_sweep_ctl.meter_sweep_ctl[" +
                         std::to_string(i) + "]";
        }
        reg_name = "stage_" + std::to_string(stageno) + "_meter_sweep_ctl_" + std::to_string(i);
        reg_value = (meter_sweep_ctl[i].meter_sweep_en & 0x00000001) |
                    ((meter_sweep_ctl[i].meter_sweep_offset & 0x0000003F) << 1) |
                    ((meter_sweep_ctl[i].meter_sweep_size & 0x0000003F) << 7) |
                    ((meter_sweep_ctl[i].meter_sweep_remove_hole_pos & 0x00000003) << 13) |
                    ((meter_sweep_ctl[i].meter_sweep_remove_hole_en & 0x00000001) << 16) |
                    ((meter_sweep_ctl[i].meter_sweep_interval & 0x0000001F) << 17);
        if ((reg_value != 0) || (options.match_compiler)) {
            reg_value_str = int_to_hex_string(reg_value, reg_width);
            add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
        }
    }

    // meter_ctl is different for Tofino and Tofino2, so it is added in
    // specialized functions.

    // statistics_ctl
    auto &statistics_ctl = regs.rams.map_alu.stats_wrap;
    for (int i = 0; i < 4; i++) {
        reg_fqname = "mau[" + std::to_string(stageno) + "].rams.map_alu.stats_wrap[" +
                     std::to_string(i) + "]" + ".stats.statistics_ctl";
        reg_name = "stage_" + std::to_string(stageno) + "_statistics_ctl_" + std::to_string(i);
        reg_value =
            (statistics_ctl[i].stats.statistics_ctl.stats_entries_per_word & 0x00000007) |
            ((statistics_ctl[i].stats.statistics_ctl.stats_process_bytes & 0x00000001) << 3) |
            ((statistics_ctl[i].stats.statistics_ctl.stats_process_packets & 0x00000001) << 4) |
            ((statistics_ctl[i].stats.statistics_ctl.lrt_enable & 0x00000001) << 5) |
            ((statistics_ctl[i].stats.statistics_ctl.stats_alu_egress & 0x00000001) << 6) |
            ((statistics_ctl[i].stats.statistics_ctl.stats_bytecount_adjust & 0x00003FFF) << 7) |
            ((statistics_ctl[i].stats.statistics_ctl.stats_alu_error_enable & 0x00000001) << 21);
        if ((reg_value != 0) || (options.match_compiler)) {
            reg_value_str = int_to_hex_string(reg_value, reg_width);
            add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
        }
    }

    // match_input_xbar_din_power_ctl
    auto &mixdpctl = regs.dp.match_input_xbar_din_power_ctl;
    reg_value_str = "";
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 16; j++) {
            reg_value = mixdpctl[i][j];
            reg_value_str = reg_value_str + int_to_hex_string(reg_value, reg_width);
        }
    }
    if (!check_zero_string(reg_value_str) || options.match_compiler) {
        reg_fqname = "mau[" + std::to_string(stageno) + "].dp.match_input_xbar_din_power_ctl";
        reg_name = "stage_" + std::to_string(stageno) + "_match_input_xbar_din_power_ctl";
        add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
    }

    // hash_seed
    auto &hash_seed = regs.dp.xbar_hash.hash.hash_seed;
    reg_value_str = "";
    for (int i = 0; i < 52; i++) {
        reg_value = hash_seed[i];
        reg_value_str = reg_value_str + int_to_hex_string(reg_value, reg_width);
    }
    if (!check_zero_string(reg_value_str) || options.match_compiler) {
        reg_fqname = "mau[" + std::to_string(stageno) + "].dp.xbar_hash.hash.hash_seed";
        reg_name = "stage_" + std::to_string(stageno) + "_hash_seed";
        add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
    }

    // parity_group_mask
    auto &parity_group_mask = regs.dp.xbar_hash.hash.parity_group_mask;
    reg_value_str = "";
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 2; j++) {
            reg_value = parity_group_mask[i][j];
            reg_value_str = reg_value_str + int_to_hex_string(reg_value, reg_width);
        }
    }
    if (!check_zero_string(reg_value_str) || options.match_compiler) {
        reg_fqname = "mau[" + std::to_string(stageno) + "].dp.xbar_hash.hash.parity_group_mask";
        reg_name = "stage_" + std::to_string(stageno) + "_parity_group_mask";
        add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
    }
}

template <class REGS>
void Stage::write_teop_regs(REGS &regs) {
    BUG_CHECK(Target::SUPPORT_TRUE_EOP(), "teop not supported on target");
    // Set teop bus delay regs on current stage if previous stage is driving teop
    for (auto t : teop) {
        if (t.second.first && t.second.second < stageno) {
            auto delay_en = (stage_dep[EGRESS] != Stage::ACTION_DEP);
            if (delay_en) {
                auto delay = pipelength(EGRESS) - 4;
                auto &adrdist = regs.rams.match.adrdist;
                adrdist.teop_bus_ctl[t.first].teop_bus_ctl_delay = delay;
                adrdist.teop_bus_ctl[t.first].teop_bus_ctl_delay_en = delay_en;
            }
        }
    }
}
