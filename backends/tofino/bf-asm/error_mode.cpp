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

#include "error_mode.h"

#include "backends/tofino/bf-asm/stage.h"
#include "lib/exceptions.h"

DefaultErrorMode DefaultErrorMode::singleton;

ErrorMode::mode_t ErrorMode::str2mode(const value_t &v) {
    if (CHECKTYPE(v, tSTR)) {
        if (v == "propagate") return PROPAGATE;
        if (v == "map_to_immediate") return MAP_TO_IMMEDIATE;
        if (v == "disable") return DISABLE_ALL_TABLES;
        if (v == "propagate_and_map") return PROPAGATE_AND_MAP;
        if (v == "propagate_and_disable") return PROPAGATE_AND_DISABLE;
        if (v != "no_config") error(v.lineno, "Not a valid error mode: %s", v.s);
    }
    return NO_CONFIG;
}

const char *ErrorMode::mode2str(ErrorMode::mode_t m) {
    switch (m) {
        case NO_CONFIG:
            return "no_config";
        case PROPAGATE:
            return "propagate";
        case MAP_TO_IMMEDIATE:
            return "map_to_immediate";
        case DISABLE_ALL_TABLES:
            return "disable";
        case PROPAGATE_AND_MAP:
            return "propagate_and_map";
        case PROPAGATE_AND_DISABLE:
            return "propagate_and_disable";
        default:
            return "<invalid mode>";
    }
}

ErrorMode::type_t ErrorMode::str2type(const value_t &v) {
    if (CHECKTYPE(v, tSTR)) {
        if (v == "tcam_match") return TCAM_MATCH;
        if (v == "tind_ecc") return TIND_ECC;
        if (v == "gfm_parity") return GFM_PARITY;
        if (v == "emm_ecc") return EMM_ECC;
        if (v == "prev_err") return PREV_ERROR;
        if (v == "actiondata") return ACTIONDATA_ERROR;
        if (v == "imem_parity") return IMEM_PARITY_ERROR;
        error(v.lineno, "Not a valid error type: %s", v.s);
    }
    return TCAM_MATCH;  // avoid invalid type here, error message has been output already
}

const char *ErrorMode::type2str(ErrorMode::type_t t) {
    switch (t) {
        case TCAM_MATCH:
            return "tcam_match";
        case TIND_ECC:
            return "tind_ecc";
        case GFM_PARITY:
            return "gfm_parity";
        case EMM_ECC:
            return "emm_ecc";
        case PREV_ERROR:
            return "prev_err";
        case ACTIONDATA_ERROR:
            return "actiondata";
        case IMEM_PARITY_ERROR:
            return "imem_parity";
        default:
            return "<invalid type>";
    }
}

void ErrorMode::input(value_t data) {
    if (!CHECKTYPE2(data, tSTR, tMAP)) return;
    if (data.type == tSTR) {
        mode_t m = str2mode(data);
        for (int i = 0; i < NUM_TYPE_T; ++i) {
            if (i == LATE_ERROR && m != NO_CONFIG) m = PROPAGATE;
            mode[i] = m;
        }
    } else {
        for (auto &kv : MapIterChecked(data.map)) {
            type_t t = str2type(kv.key);
            mode_t m = str2mode(kv.value);
            if (t >= LATE_ERROR && m > PROPAGATE)
                error(kv.value.lineno, "%s error mode can only propagate, not %s", type2str(t),
                      mode2str(m));
            mode[t] = m;
        }
    }
}

template <class REGS>
void ErrorMode::write_regs(REGS &regs, const Stage *stage, gress_t gress) {
    auto &merge = regs.rams.match.merge;
    int tcam_err_delay = stage->tcam_delay(gress) ? 1 : 0;
    int fifo_err_delay =
        stage->pipelength(gress) - stage->pred_cycle(gress) - Target::MAU_ERROR_DELAY_ADJUST();
    bool map_to_immed = false;
    bool propagate = false;
#define YES(X) X
#define NO(X)
#define HANDLE_ERROR_CASES(REG, HAVE_O_ERR_EN)              \
    case NO_CONFIG:                                         \
        break;                                              \
    case PROPAGATE:                                         \
        HAVE_O_ERR_EN(merge.REG[gress].REG##_o_err_en = 1;) \
        propagate = true;                                   \
        break;                                              \
    case PROPAGATE_AND_MAP:                                 \
        HAVE_O_ERR_EN(merge.REG[gress].REG##_o_err_en = 1;) \
        propagate = true;                                   \
        /* fall through */                                  \
    case MAP_TO_IMMEDIATE:                                  \
        merge.REG[gress].REG##_idata_ovr = 1;               \
        map_to_immed = true;                                \
        break;                                              \
    case PROPAGATE_AND_DISABLE:                             \
        HAVE_O_ERR_EN(merge.REG[gress].REG##_o_err_en = 1;) \
        propagate = true;                                   \
        /* fall through */                                  \
    case DISABLE_ALL_TABLES:                                \
        merge.REG[gress].REG##_dis_pred = 1;                \
        break;                                              \
    default:                                                \
        BUG("unexpected error mode");

    switch (mode[PREV_ERROR]) { HANDLE_ERROR_CASES(prev_error_ctl, NO) }
    merge.prev_error_ctl[gress].prev_error_ctl_delay = tcam_err_delay;
    if (propagate) {
        switch (stage->stage_dep[gress]) {
            case Stage::CONCURRENT:
                merge.prev_error_ctl[gress].prev_error_ctl_conc = 1;
                break;
            case Stage::ACTION_DEP:
                merge.prev_error_ctl[gress].prev_error_ctl_action = 1;
                break;
            case Stage::NONE:
                if (stage->stageno == 0) {
                        // stage 0 does not have stage_dep set, but counts as if it was match
                        // dependent (on the parser).  FIXME -- should just always set stage_dep to
                        // MATCH_DEP for stage 0? fall through
                    case Stage::MATCH_DEP:
                        merge.prev_error_ctl[gress].prev_error_ctl_match = 1;
                        break;
                }
                [[fallthrough]];
            default:
                BUG("unexpected stage_dep: %d", stage->stage_dep[gress]);
        }
    }

    switch (mode[TCAM_MATCH]) { HANDLE_ERROR_CASES(tcam_match_error_ctl, YES) }
    switch (mode[TIND_ECC]) { HANDLE_ERROR_CASES(tind_ecc_error_ctl, YES) }
    switch (mode[GFM_PARITY]) { HANDLE_ERROR_CASES(gfm_parity_error_ctl, YES) }
    merge.gfm_parity_error_ctl[gress].gfm_parity_error_ctl_delay = tcam_err_delay;
    switch (mode[EMM_ECC]) { HANDLE_ERROR_CASES(emm_ecc_error_ctl, YES) }
    merge.emm_ecc_error_ctl[gress].emm_ecc_error_ctl_delay = tcam_err_delay;

    if (map_to_immed) {
        merge.err_idata_ovr_fifo_ctl[gress].err_idata_ovr_fifo_ctl_en = 1;
        merge.err_idata_ovr_fifo_ctl[gress].err_idata_ovr_fifo_ctl_delay = fifo_err_delay - 2;
    }
    if (propagate) {
        merge.o_error_fifo_ctl[gress].o_error_fifo_ctl_en = 1;
        merge.o_error_fifo_ctl[gress].o_error_fifo_ctl_delay = fifo_err_delay;
    }

    // action error sources can only propagate (too late for disable or map_to_immed
    if (mode[ACTIONDATA_ERROR]) merge.actiondata_error_ctl |= 1 << gress;
    if (mode[IMEM_PARITY_ERROR]) merge.imem_parity_error_ctl |= 1 << gress;

    /* TODO -- additional error cfg regs:
     * rams.match.merge.err_idata_ovr_ctl[gress]
     * rams.match.merge.s2p_meter_error_ctl[gress]
     * rams.match.merge.s2p_stats_error_ctl[gress]
     * rams.map_alu.stats_wrap[alu].stats.statistics.ctl.stats_alu_error_enable;
     * rams.map_alu.meter_alu_group_error_ctl[alu]
     * rams.array.row[r].actiondata_error_uram_ctl[gress]
     * rams.array.row[r].emm_ecc_error_uram_ctl[gress]
     * rams.array.row[r].tind_ecc_error_uram_ctl[gress]
     */
}
FOR_ALL_REGISTER_SETS(INSTANTIATE_TARGET_TEMPLATE, void ErrorMode::write_regs, mau_regs &,
                      const Stage *, gress_t);
