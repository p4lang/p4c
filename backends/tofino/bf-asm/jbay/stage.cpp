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

/* mau stage template specializations for jbay -- #included directly in top-level stage.cpp */

template <>
void Stage::gen_configuration_cache(Target::JBay::mau_regs &regs, json::vector &cfg_cache) {
    Stage::gen_configuration_cache_common(regs, cfg_cache);

    static unsigned i_pdddelay;
    static unsigned e_pdddelay;
    unsigned reg_width = 8;  // this means number of hex characters
    std::string i_reg_value_str;
    std::string e_reg_value_str;
    std::string reg_fqname;
    std::string reg_name;
    unsigned reg_value;
    std::string reg_value_str;

    if (stageno != 0) {
        if (i_pdddelay > regs.cfg_regs.amod_pre_drain_delay[INGRESS])
            i_pdddelay = regs.cfg_regs.amod_pre_drain_delay[INGRESS];
        if (e_pdddelay > regs.cfg_regs.amod_pre_drain_delay[EGRESS])
            e_pdddelay = regs.cfg_regs.amod_pre_drain_delay[EGRESS];

        if (stageno == AsmStage::numstages() - 1) {
            // 64 is due to number of CSR's
            i_pdddelay += (7 + 64);
            i_reg_value_str = int_to_hex_string(i_pdddelay, reg_width);
            e_pdddelay += (7 + 64);
            e_reg_value_str = int_to_hex_string(e_pdddelay, reg_width);

            add_cfg_reg(cfg_cache, "pardereg.pgstnreg.parbreg.left.i_wb_ctrl", "left_i_wb_ctrl",
                        i_reg_value_str);
            add_cfg_reg(cfg_cache, "pardereg.pgstnreg.parbreg.right.e_wb_ctrl", "right_e_wb_ctrl",
                        e_reg_value_str);
        }
    }

    // meter_ctl
    auto &meter_ctl = regs.rams.map_alu.meter_group;
    for (int i = 0; i < 4; i++) {
        reg_fqname = "mau[" + std::to_string(stageno) + "].rams.map_alu.meter_group[" +
                     std::to_string(i) + "]" + ".meter.meter_ctl";
        reg_name = "stage_" + std::to_string(stageno) + "_meter_ctl_" + std::to_string(i);
        reg_value = meter_ctl[i].meter.meter_ctl;
        if ((reg_value != 0) || (options.match_compiler)) {
            reg_value_str = int_to_hex_string(reg_value, reg_width);
            add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
        }
    }
}

static void addvec(json::vector &vec, ubits_base &val, uint32_t extra = 0) {
    vec.push_back(val | extra);
}
static void addvec(json::vector &vec, uint32_t val, uint32_t extra = 0) {
    vec.push_back(val | extra);
}

template <class T>
static void addvec(json::vector &vec, checked_array_base<T> &array, uint32_t extra = 0) {
    for (auto &el : array) addvec(vec, el, extra);
}

template <class REGS, class REG>
static json::map make_reg_vec(REGS &regs, REG &reg, const char *name, uint32_t mask0,
                              uint32_t mask1, uint32_t mask2, uint32_t extra = 0) {
    json::map rv;
    rv["name"] = name;
    rv["offset"] = regs.binary_offset(&reg);
    addvec(rv["value"], reg, extra);
    rv["mask"] = json::vector{json::number(mask0), json::number(mask1), json::number(mask2)};
    return rv;
}

template <class REGS>
void Stage::gen_mau_stage_extension(REGS &regs, json::map &extend) {
    extend["last_programmed_stage"] = Target::NUM_MAU_STAGES() - 1;
    json::vector &registers = extend["registers"] = json::vector();
    registers.push_back(make_reg_vec(regs, regs.dp.phv_ingress_thread, "regs.dp.phv_ingress_thread",
                                     0, 0x3ff, 0x3ff));
    registers.push_back(make_reg_vec(regs, regs.dp.phv_ingress_thread_imem,
                                     "regs.dp.phv_ingress_thread_imem", 0, 0x3ff, 0x3ff));
    registers.push_back(make_reg_vec(regs, regs.dp.phv_egress_thread, "regs.dp.phv_egress_thread",
                                     0, 0x3ff, 0x3ff));
    registers.push_back(make_reg_vec(regs, regs.dp.phv_egress_thread_imem,
                                     "regs.dp.phv_egress_thread_imem", 0, 0x3ff, 0x3ff));
    registers.push_back(make_reg_vec(regs, regs.rams.match.adrdist.adr_dist_pipe_delay,
                                     "regs.rams.match.adrdist.adr_dist_pipe_delay", 0, 0xf, 0xf));
    typename std::remove_reference<
        decltype(regs.rams.match.adrdist.deferred_eop_bus_delay[0])>::type mask0,
        mask1;
    mask0.eop_delay_fifo_en = mask1.eop_delay_fifo_en = 1;
    mask0.eop_internal_delay_fifo = mask1.eop_internal_delay_fifo = 0x1f;
    mask0.eop_output_delay_fifo = 0x1;
    mask1.eop_output_delay_fifo = 0x1f;
    BUG_CHECK(regs.rams.match.adrdist.deferred_eop_bus_delay[0].eop_output_delay_fifo &
              regs.rams.match.adrdist.deferred_eop_bus_delay[1].eop_output_delay_fifo & 1);
    registers.push_back(make_reg_vec(regs, regs.rams.match.adrdist.deferred_eop_bus_delay,
                                     "regs.rams.match.adrdist.deferred_eop_bus_delay", mask0, mask0,
                                     mask1));
    registers.push_back(make_reg_vec(regs, regs.dp.cur_stage_dependency_on_prev,
                                     "regs.dp.cur_stage_dependency_on_prev", 0, 0x3, 0x3, 0x1));
    registers.push_back(make_reg_vec(regs, regs.dp.next_stage_dependency_on_cur,
                                     "regs.dp.next_stage_dependency_on_cur", 0x3, 0x3, 0, 0x1));
    registers.push_back(make_reg_vec(regs, regs.rams.match.merge.mpr_bus_dep,
                                     "regs.rams.match.merge.mpr_bus_dep", 0x3, 0x3, 0, 0x3));
    registers.push_back(make_reg_vec(regs, regs.dp.pipelength_added_stages,
                                     "regs.dp.pipelength_added_stages", 0, 0xf, 0xf));
    registers.push_back(make_reg_vec(regs, regs.rams.match.merge.exact_match_delay_thread,
                                     "regs.rams.match.merge.exact_match_delay_thread", 0, 0x3,
                                     0x3));
    BUG_CHECK((regs.rams.match.merge.mpr_thread_delay[0] & 1) == 0);
    BUG_CHECK((regs.rams.match.merge.mpr_thread_delay[1] & 1) == 0);
    registers.push_back(make_reg_vec(regs, regs.rams.match.merge.mpr_thread_delay,
                                     "regs.rams.match.merge.mpr_thread_delay", 1, 1, 0x1f));
}

/* disable power gating configuration for specific MAU regs to weedout delay programming
 * issues. We dont expect to call this function in the normal usage of JBay - this is
 * only for emulator debug/bringup
 */
template <class REGS>
static void disable_jbay_power_gating(REGS &regs) {
    for (gress_t gress : Range(INGRESS, EGRESS)) {
        regs.dp.mau_match_input_xbar_exact_match_enable[gress] |= 0x1;
        regs.dp.xbar_hash.xbar.mau_match_input_xbar_ternary_match_enable[gress] |= 0x1;
    }

    auto &xbar_power_ctl = regs.dp.match_input_xbar_din_power_ctl;
    auto &actionmux_power_ctl = regs.dp.actionmux_din_power_ctl;
    for (int side = 0; side < 2; side++) {
        for (int reg = 0; reg < 16; reg++) {
            xbar_power_ctl[side][reg] |= 0x3FF;
            actionmux_power_ctl[side][reg] |= 0x3FF;
        }
    }
}

template <>
void Stage::write_regs(Target::JBay::mau_regs &regs, bool) {
    write_common_regs<Target::JBay>(regs);
    auto &merge = regs.rams.match.merge;
    for (gress_t gress : Range(INGRESS, EGRESS)) {
        if (stageno == 0) {
            merge.predication_ctl[gress].start_table_fifo_delay0 = pred_cycle(gress) - 2;
            merge.predication_ctl[gress].start_table_fifo_enable = 1;
        } else if (stage_dep[gress] == MATCH_DEP) {
            merge.predication_ctl[gress].start_table_fifo_delay0 =
                this[-1].pipelength(gress) - this[-1].pred_cycle(gress) + pred_cycle(gress) - 3;
            merge.predication_ctl[gress].start_table_fifo_enable = 1;
        } else {
            BUG_CHECK(stage_dep[gress] == ACTION_DEP);
            merge.predication_ctl[gress].start_table_fifo_delay0 = 0;
            merge.predication_ctl[gress].start_table_fifo_enable = 0;
        }

        if (stageno != 0)
            regs.dp.cur_stage_dependency_on_prev[gress] = stage_dep[gress] != MATCH_DEP;

        /* set stage0 dependency if explicitly set by the commandline option */
        if (stageno == 0 && !options.stage_dependency_pattern.empty())
            regs.dp.cur_stage_dependency_on_prev[gress] = stage_dep[gress] != MATCH_DEP;

        if (stageno != AsmStage::numstages() - 1)
            regs.dp.next_stage_dependency_on_cur[gress] = this[1].stage_dep[gress] != MATCH_DEP;
        else if (AsmStage::numstages() < Target::NUM_MAU_STAGES())
            regs.dp.next_stage_dependency_on_cur[gress] = 1;
        auto &deferred_eop_bus_delay = regs.rams.match.adrdist.deferred_eop_bus_delay[gress];
        deferred_eop_bus_delay.eop_internal_delay_fifo = pred_cycle(gress) + 2;
        /* FIXME -- making this depend on the dependency of the next stage seems wrong */
        if (stageno == AsmStage::numstages() - 1) {
            if (AsmStage::numstages() < Target::NUM_MAU_STAGES())
                deferred_eop_bus_delay.eop_output_delay_fifo = 1;
            else
                deferred_eop_bus_delay.eop_output_delay_fifo = pipelength(gress) - 2;
        } else if (this[1].stage_dep[gress] == MATCH_DEP) {
            deferred_eop_bus_delay.eop_output_delay_fifo = pipelength(gress) - 2;
        } else {
            deferred_eop_bus_delay.eop_output_delay_fifo = 1;
        }
        deferred_eop_bus_delay.eop_delay_fifo_en = 1;
        if (stageno != AsmStage::numstages() - 1 && this[1].stage_dep[gress] == MATCH_DEP) {
            merge.mpr_thread_delay[gress] = pipelength(gress) - pred_cycle(gress) - 4;
        } else {
            /* last stage in JBay must be always set as match-dependent on deparser */
            if (stageno == AsmStage::numstages() - 1) {
                merge.mpr_thread_delay[gress] = pipelength(gress) - pred_cycle(gress) - 4;
            } else {
                merge.mpr_thread_delay[gress] = 0;
            }
        }
    }

    for (gress_t gress : Range(INGRESS, EGRESS))
        if (table_use[gress] & USE_TCAM)
            regs.tcams.tcam_piped |= options.match_compiler ? 3 : 1 << gress;

    for (gress_t gress : Range(INGRESS, EGRESS)) {
        regs.cfg_regs.amod_pre_drain_delay[gress] = pipelength(gress) - 9;
        if (this[1].stage_dep[gress] == MATCH_DEP)
            regs.cfg_regs.amod_wide_bubble_rsp_delay[gress] = pipelength(gress) - 3;
        else
            regs.cfg_regs.amod_wide_bubble_rsp_delay[gress] = 0;
    }
    /* Max re-request limit with a long interval.  Parb is going to have a large
     * gap configured to minimize traffic hits during configuration this means
     * that individual stages may not get their bubbles and will need to retry. */
    regs.cfg_regs.amod_req_interval = 6732;
    regs.cfg_regs.amod_req_limit = 15;

    if (stageno == 0) {
        /* MFerrera: "After some debug on the emulator, we've found a programming issue due to
         * incorrect documentation and CSR description of match_ie_input_mux_sel in JBAY"
         * MAU Stage 0 must always be configured to source iPHV from Parser-Arbiter
         * Otherwise, MAU stage 0 is configured as match-dependent on Parser-Arbiter */
        regs.dp.match_ie_input_mux_sel |= 3;
    }

    merge.pred_stage_id = stageno;
    if (long_branch_terminate) merge.pred_long_brch_terminate = long_branch_terminate;
    for (gress_t gress : Range(INGRESS, GHOST)) {
        if (long_branch_thread[gress])
            merge.pred_long_brch_thread[gress] = long_branch_thread[gress];
    }

    for (gress_t gress : Range(INGRESS, GHOST)) {
        merge.mpr_stage_id[gress] = mpr_stage_id[gress];
        for (int id = 0; id < LOGICAL_TABLES_PER_STAGE; ++id) {
            merge.mpr_next_table_lut[gress][id] = mpr_next_table_lut[gress][id];
        }
    }
    for (int id = 0; id < LOGICAL_TABLES_PER_STAGE; ++id) {
        merge.mpr_glob_exec_lut[id] = mpr_glob_exec_lut[id];
    }
    for (int id = 0; id < MAX_LONGBRANCH_TAGS; ++id) {
        merge.mpr_long_brch_lut[id] = mpr_long_brch_lut[id];
    }
    merge.mpr_always_run = mpr_always_run;

    if (stageno != AsmStage::numstages() - 1) {
        merge.mpr_bus_dep.mpr_bus_dep_ingress = this[1].stage_dep[INGRESS] != MATCH_DEP;
        merge.mpr_bus_dep.mpr_bus_dep_egress = this[1].stage_dep[EGRESS] != MATCH_DEP;
    }

    merge.mpr_bus_dep.mpr_bus_dep_glob_exec = mpr_bus_dep_glob_exec[INGRESS] |
                                              mpr_bus_dep_glob_exec[EGRESS] |
                                              mpr_bus_dep_glob_exec[GHOST];
    merge.mpr_bus_dep.mpr_bus_dep_long_brch = mpr_bus_dep_long_branch[INGRESS] |
                                              mpr_bus_dep_long_branch[EGRESS] |
                                              mpr_bus_dep_long_branch[GHOST];

    merge.mpr_long_brch_thread = long_branch_thread[EGRESS];
    if (auto conflict = (long_branch_thread[INGRESS] | long_branch_thread[GHOST]) &
                        long_branch_thread[EGRESS]) {
        // Should probably check this earlier, but there's not a good place to do it.
        for (auto tag : bitvec(conflict)) {
            error(long_branch_use[tag]->lineno,
                  "Need one-stage turnaround before reusing "
                  "long_branch tag %d in a different thread",
                  tag);
        }
    }

    bitvec in_use = match_use[INGRESS] | action_use[INGRESS] | action_set[INGRESS];
    bitvec eg_use = match_use[EGRESS] | action_use[EGRESS] | action_set[EGRESS];
    /* FIXME -- if the regs are live across a stage (even if not used in that stage) they
     * need to be set in the thread registers.  For now we just assume if they are used
     * anywhere, they need to be marked as live */
    in_use |= Phv::use(INGRESS);
    eg_use |= Phv::use(EGRESS);
    static const int phv_use_transpose[2][14] = {
        {0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 20, 21},
        {4, 5, 6, 7, 12, 13, 14, 15, 22, 23, 24, 25, 26, 27}};
    // FIXME -- this code depends on the Phv::Register uids matching the
    // FIXME -- mau encoding of phv containers. (FIXME-PHV)
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 14; j++) {
            regs.dp.phv_ingress_thread[i][j] = regs.dp.phv_ingress_thread_imem[i][j] =
                in_use.getrange(10 * phv_use_transpose[i][j], 10);
            regs.dp.phv_egress_thread[i][j] = regs.dp.phv_egress_thread_imem[i][j] =
                eg_use.getrange(10 * phv_use_transpose[i][j], 10);
        }
    }

    /* Things following are for debug/bringup only : not for normal flows  */

    if (options.disable_power_gating) {
        disable_jbay_power_gating(regs);
    }

    write_teop_regs(regs);
}

void AlwaysRunTable::write_regs(Target::JBay::mau_regs &regs) {
    if (gress == EGRESS)
        regs.dp.imem_word_read_override.imem_word_read_override_egress = 1;
    else
        regs.dp.imem_word_read_override.imem_word_read_override_ingress = 1;
    actions->write_regs(regs, this);
}
