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

#ifndef BF_ASM_STAGE_H_
#define BF_ASM_STAGE_H_

#include <fstream>
#include <vector>

#include "alloc.h"
#include "bitvec.h"
#include "error_mode.h"
#include "input_xbar.h"
#include "tables.h"

class Stage_data {
    /* we encapsulate all the Stage non-static fields in a base class to automate the
     * generation of the move construtor properly */
 public:
    int stageno;
    std::vector<Table *> tables;
    std::set<Stage **> all_refs;
    BFN::Alloc2Dbase<Table *> sram_use;
    BFN::Alloc2D<Table *, SRAM_ROWS, 2> sram_search_bus_use;
    BFN::Alloc3Dbase<Table *> stm_hbus_use;
    BFN::Alloc2D<Table *, SRAM_ROWS, 2> match_result_bus_use;
    BFN::Alloc2D<Table *, SRAM_ROWS, MAPRAM_UNITS_PER_ROW> mapram_use;
    BFN::Alloc2Dbase<Table *> tcam_use;
    BFN::Alloc2Dbase<Table *> tcam_match_bus_use;
    BFN::Alloc2D<std::pair<Table *, int>, TCAM_ROWS, 2> tcam_byte_group_use;
    BFN::Alloc1Dbase<Table *> local_tind_use;
    BFN::Alloc2D<Table *, SRAM_ROWS, 2> tcam_indirect_bus_use;
    BFN::Alloc2D<GatewayTable *, SRAM_ROWS, 2> gw_unit_use;
    BFN::Alloc2D<GatewayTable *, SRAM_ROWS, 2> gw_payload_use;
    BFN::Alloc1D<Table *, LOGICAL_TABLES_PER_STAGE> logical_id_use;
    BFN::Alloc1D<Table *, PHYSICAL_TABLES_PER_STAGE> physical_id_use;
    BFN::Alloc1D<Table *, TCAM_TABLES_PER_STAGE> tcam_id_use;
    ordered_map<InputXbar::Group, std::vector<InputXbar *>> ixbar_use;
    BFN::Alloc1D<Table *, TCAM_XBAR_INPUT_BYTES> tcam_ixbar_input;
    BFN::Alloc1Dbase<std::vector<InputXbar *>> hash_table_use;
    BFN::Alloc1Dbase<std::vector<InputXbar *>> hash_group_use;
    BFN::Alloc1D<std::vector<HashDistribution *>, 6> hash_dist_use;
    BFN::Alloc1Dbase<ActionTable *> action_unit_use;
    BFN::Alloc1Dbase<Synth2Port *> dp_unit_use;
    BFN::Alloc1D<Table *, ACTION_DATA_BUS_SLOTS> action_bus_use;
    BFN::Alloc1D<Table *, LOGICAL_SRAM_ROWS> action_data_use, meter_bus_use, stats_bus_use,
        selector_adr_bus_use, overflow_bus_use;
    BFN::Alloc1D<Table *, IDLETIME_BUSSES> idletime_bus_use;
    bitvec action_bus_use_bit_mask;
    BFN::Alloc2D<Table::Actions::Action *, 2, ACTION_IMEM_ADDR_MAX> imem_addr_use;
    bitvec imem_use[ACTION_IMEM_SLOTS];
    BFN::Alloc1D<Table::NextTables *, MAX_LONGBRANCH_TAGS> long_branch_use;
    unsigned long_branch_thread[3] = {0};
    unsigned long_branch_terminate = 0;

    // for timing, ghost thread is tied to ingress, so we track ghost as ingress here
    enum {
        USE_TCAM = 1,
        USE_STATEFUL = 4,
        USE_METER = 8,
        USE_METER_LPF_RED = 16,
        USE_SELECTOR = 32,
        USE_WIDE_SELECTOR = 64,
        USE_STATEFUL_DIVIDE = 128
    };
    int /* enum */ table_use[2], group_table_use[2];

    enum { NONE = 0, CONCURRENT = 1, ACTION_DEP = 2, MATCH_DEP = 3 } stage_dep[2];
    bitvec match_use[3], action_use[3], action_set[3];

    // there's no error mode registers for ghost thread, so we don't allow it to be set
    ErrorMode error_mode[2];

    // MPR stage config
    int mpr_stage_id[3] = {0};  // per-gress
    int mpr_always_run = 0;
    int mpr_bus_dep_glob_exec[3] = {0};
    int mpr_bus_dep_long_branch[3] = {0};
    // per gress, per logical table
    BFN::Alloc2D<int, 3, LOGICAL_TABLES_PER_STAGE> mpr_next_table_lut;
    // per global execute bit
    BFN::Alloc1D<int, LOGICAL_TABLES_PER_STAGE> mpr_glob_exec_lut;
    // per long branch tag
    BFN::Alloc1D<int, MAX_LONGBRANCH_TAGS> mpr_long_brch_lut;

    int pass1_logical_id = -1, pass1_tcam_id = -1;

    // True egress accounting (4 buses) Tofino2
    static std::map<int, std::pair<bool, int>> teop;

 protected:
    Stage_data(int stage, bool egress_only)
        : stageno(stage),
          sram_use(Target::SRAM_ROWS(egress_only ? EGRESS : INGRESS), Target::SRAM_UNITS_PER_ROW()),
          stm_hbus_use(Target::SRAM_ROWS(egress_only ? EGRESS : INGRESS),
                       Target::SRAM_HBUS_SECTIONS_PER_STAGE(), Target::SRAM_HBUSSES_PER_ROW()),
          tcam_use(Target::TCAM_ROWS(), Target::TCAM_UNITS_PER_ROW()),
          tcam_match_bus_use(Target::TCAM_ROWS(), Target::TCAM_MATCH_BUSSES()),
          local_tind_use(Target::LOCAL_TIND_UNITS()),
          hash_table_use(Target::EXACT_HASH_TABLES()),
          hash_group_use(Target::EXACT_HASH_GROUPS()),
          action_unit_use(Target::ARAM_UNITS_PER_STAGE()),
          dp_unit_use(Target::DP_UNITS_PER_STAGE()) {}
    Stage_data(const Stage_data &) = delete;
    Stage_data(Stage_data &&) = default;
    ~Stage_data() {}
};

class Stage : public Stage_data {
 public:
    static unsigned char action_bus_slot_map[ACTION_DATA_BUS_BYTES];
    static unsigned char action_bus_slot_size[ACTION_DATA_BUS_SLOTS];  // size in bits

    explicit Stage(int stageno, bool egress_only);
    Stage(const Stage &) = delete;
    Stage(Stage &&);
    ~Stage();
    template <class TARGET>
    void output(json::map &ctxt_json, bool egress_only = false);
    template <class REGS>
    void fixup_regs(REGS &regs);
    template <class REGS>
    void gen_configuration_cache_common(REGS &regs, json::vector &cfg_cache);
    template <class REGS>
    void gen_configuration_cache(REGS &regs, json::vector &cfg_cache);
    template <class REGS>
    void gen_gfm_json_info(REGS &regs, std::ostream &out);
    template <class REGS>
    void gen_mau_stage_characteristics(REGS &regs, json::vector &stg_characteristics);
    template <class REGS>
    void gen_mau_stage_extension(REGS &regs, json::map &extend);
    template <class REGS>
    void write_regs(REGS &regs, bool egress_only);
    template <class TARGET>
    void write_common_regs(typename TARGET::mau_regs &regs);
    template <class REGS>
    void write_teop_regs(REGS &regs);
    int adr_dist_delay(gress_t gress) const;
    int meter_alu_delay(gress_t gress, bool uses_divmod) const;
    int pipelength(gress_t gress) const;
    int pred_cycle(gress_t gress) const;
    int tcam_delay(gress_t gress) const;
    int cycles_contribute_to_latency(gress_t gress);
    void verify_have_mpr(std::string key, int line_number);
    static int first_table(gress_t gress);
    static unsigned end_of_pipe() { return Target::END_OF_PIPE(); }
    static Stage *stage(gress_t gress, int stageno);
    void log_hashes(std::ofstream &out) const;
    bitvec imem_use_all() const;
};

class AsmStage : public Section {
    void start(int lineno, VECTOR(value_t) args);
    void input(VECTOR(value_t) args, value_t data);
    void output(json::map &);

    /// Propagates group_table_use to adjacent stages that are not match-dependent.
    void propagate_group_table_use();

    unsigned compute_latency(gress_t gress);
    AsmStage();
    ~AsmStage() {}
    std::vector<Stage> pipe;
    static AsmStage singleton_object;
    bitvec stages_seen[NUM_GRESS_T];

 public:
    void process();
    static int numstages() { return singleton_object.pipe.size(); }
    static std::vector<Stage> &stages(gress_t gress) { return singleton_object.pipe; }

    // for gtest
    void reset_stage(Stage &stage) {
        for (auto &tbl : stage.tables) tbl->all->clear();
        stage.tables.clear();
        stage.all_refs.clear();
        stage.sram_use.clear();
        stage.sram_search_bus_use.clear();
        stage.stm_hbus_use.clear();
        stage.match_result_bus_use.clear();
        stage.mapram_use.clear();
        stage.tcam_use.clear();
        stage.tcam_match_bus_use.clear();
        stage.tcam_byte_group_use.clear();
        stage.gw_unit_use.clear();
        stage.gw_payload_use.clear();
        stage.logical_id_use.clear();
        stage.physical_id_use.clear();
        stage.tcam_id_use.clear();
        stage.ixbar_use.clear();
        stage.tcam_ixbar_input.clear();
        stage.hash_table_use.clear();
        stage.hash_group_use.clear();
        stage.hash_dist_use.clear();
        stage.action_bus_use.clear();
        stage.action_data_use.clear();
        stage.meter_bus_use.clear();
        stage.stats_bus_use.clear();
        stage.selector_adr_bus_use.clear();
        stage.overflow_bus_use.clear();
        stage.idletime_bus_use.clear();
        stage.imem_addr_use.clear();
        stage.long_branch_use.clear();
    }

    void reset() {
        stages_seen[INGRESS].clear();
        stages_seen[EGRESS].clear();
        for (auto &stage : pipe) reset_stage(stage);
    }
};

#endif /* BF_ASM_STAGE_H_ */
