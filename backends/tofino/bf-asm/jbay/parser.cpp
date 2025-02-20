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

#include "backends/tofino/bf-asm/parser-tofino-jbay.h"
#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/top_level.h"

template <>
void Parser::Checksum::write_config(Target::JBay::parser_regs &regs, Parser *parser) {
    if (unit == 0)
        write_row_config(regs.memory[gress].po_csum_ctrl_0_row[addr]);
    else if (unit == 1)
        write_row_config(regs.memory[gress].po_csum_ctrl_1_row[addr]);
    else if (unit == 2)
        write_row_config(regs.memory[gress].po_csum_ctrl_2_row[addr]);
    else if (unit == 3)
        write_row_config(regs.memory[gress].po_csum_ctrl_3_row[addr]);
    else if (unit == 4)
        write_row_config(regs.memory[gress].po_csum_ctrl_4_row[addr]);
    else
        error(lineno, "invalid unit for parser checksum");
}

template <>
void Parser::Checksum::write_output_config(Target::JBay::parser_regs &regs, Parser *pa,
                                           State::Match * /*ma*/, void *_row,
                                           unsigned &used) const {
    if (type != 0 || !dest) return;

    Target::JBay::parser_regs::_memory::_po_action_row *row =
        (Target::JBay::parser_regs::_memory::_po_action_row *)_row;

    // checksum verification outputs "steal" extractors, see parser uArch (6.3.6)

    for (int i = 0; i < 20; ++i) {
        if (used & (1 << i)) continue;
        used |= 1 << i;
        row->phv_dst[i] = dest->reg.parser_id();
        row->extract_type[i] = 3;
        return;
    }
    error(lineno, "Ran out of phv output extractor slots");
}

template <>
void Parser::CounterInit::write_config(Target::JBay::parser_regs &regs, gress_t gress, int idx) {
    auto &ctr_init_ram = regs.memory[gress].ml_ctr_init_ram[idx];
    ctr_init_ram.add = add;
    ctr_init_ram.mask_8 = mask;
    ctr_init_ram.rotate = rot;
    ctr_init_ram.max = max;
    ctr_init_ram.src = src;
}

template <>
void Parser::State::Match::write_lookup_config(Target::JBay::parser_regs &regs, State *state,
                                               int r) const {
    auto &row = regs.memory[state->gress].ml_tcam_row[r];
    match_t lookup = {0, 0};
    unsigned dont_care = 0;
    for (int i = 0; i < 4; i++) {
        lookup.word0 <<= 8;
        lookup.word1 <<= 8;
        dont_care <<= 8;
        if (state->key.data[i].bit >= 0) {
            lookup.word0 |= ((match.word0 >> state->key.data[i].bit) & 0xff);
            lookup.word1 |= ((match.word1 >> state->key.data[i].bit) & 0xff);
        } else {
            dont_care |= 0xff;
        }
    }
    lookup.word0 |= dont_care;
    lookup.word1 |= dont_care;
    for (int i = 3; i >= 0; i--) {
        row.w0_lookup_8[i] = lookup.word0 & 0xff;
        row.w1_lookup_8[i] = lookup.word1 & 0xff;
        lookup.word0 >>= 8;
        lookup.word1 >>= 8;
    }
    row.w0_curr_state = state->stateno.word0;
    row.w1_curr_state = state->stateno.word1;
    if (state->key.ctr_zero >= 0) {
        row.w0_ctr_zero = (match.word0 >> state->key.ctr_zero) & 1;
        row.w1_ctr_zero = (match.word1 >> state->key.ctr_zero) & 1;
    } else {
        row.w0_ctr_zero = row.w1_ctr_zero = 1;
    }
    if (state->key.ctr_neg >= 0) {
        row.w0_ctr_neg = (match.word0 >> state->key.ctr_neg) & 1;
        row.w1_ctr_neg = (match.word1 >> state->key.ctr_neg) & 1;
    } else {
        row.w0_ctr_neg = row.w1_ctr_neg = 1;
    }
    row.w0_ver_0 = row.w1_ver_0 = 1;
    row.w0_ver_1 = row.w1_ver_1 = 1;
}

/* FIXME -- combine these next two methods into a single method on MatchKey */
/* FIXME -- factor Tofino/JBay variation better (most is common) */
template <>
int Parser::State::write_lookup_config(Target::JBay::parser_regs &regs, Parser *pa, State *state,
                                       int row, const std::vector<State *> &prev) {
    LOG2("-- checking match from state " << name << " (" << stateno << ')');
    auto &ea_row = regs.memory[gress].ml_ea_row[row];
    int max_off = -1;
    for (int i = 0; i < 4; i++) {
        if (key.data[i].bit < 0) continue;
        bool set = true;
        for (State *p : prev) {
            if (p->key.data[i].bit >= 0) {
                set = false;
                if (p->key.data[i].byte != key.data[i].byte)
                    error(p->lineno,
                          "Incompatible match fields between states "
                          "%s and %s, triggered from state %s",
                          name.c_str(), p->name.c_str(), state->name.c_str());
            }
        }
        if (set && key.data[i].byte != MatchKey::USE_SAVED) {
            int off = key.data[i].byte + ea_row.shift_amt;
            // Valid offset ranges:
            //   0..31  : Input packet
            //   60..63 : Scratch registers
            if ((off < 0) || ((off > 31) && (off < 60)) || (off > 63)) {
                error(key.lineno,
                      "Match offset of %d in state %s out of range "
                      "for previous state %s",
                      key.data[i].byte, name.c_str(), state->name.c_str());
            }
            ea_row.lookup_offset_8[i] = off;
            ea_row.ld_lookup_8[i] = 1;
            max_off = std::max(max_off, off);
        }
    }
    return max_off;
}

template <>
int Parser::State::Match::write_load_config(Target::JBay::parser_regs &regs, Parser *pa,
                                            State *state, int row) const {
    auto &ea_row = regs.memory[state->gress].ml_ea_row[row];
    int max_off = -1;
    for (int i = 0; i < 4; i++) {
        if (load.data[i].bit < 0) continue;
        if (load.data[i].byte != MatchKey::USE_SAVED) {
            int off = load.data[i].byte;
            // Valid offset ranges:
            //   0..31  : Input packet
            //   60..63 : Scratch registers
            if ((off < 0) || ((off > 31) && (off < 60)) || (off > 63)) {
                error(load.lineno, "Load offset of %d in state %s out of range", load.data[i].byte,
                      state->name.c_str());
            }
            ea_row.lookup_offset_8[i] = off;
            ea_row.ld_lookup_8[i] = 1;
            max_off = std::max(max_off, off);
        }
        ea_row.sv_lookup_8[i] = (load.save >> i) & 1;
    }

    return max_off;
}

static void write_output_slot(int lineno, Target::JBay::parser_regs::_memory::_po_action_row *row,
                              unsigned &used, int src, int dest, int bytemask, bool offset) {
    BUG_CHECK(bytemask > 0 && bytemask < 4);
    for (int i = 0; i < 20; ++i) {
        if (used & (1 << i)) continue;
        row->phv_dst[i] = dest;
        row->phv_src[i] = src;
        if (offset) row->phv_offset_add_dst[i] = 1;
        row->extract_type[i] = bytemask;
        used |= 1 << i;
        return;
    }
    error(lineno, "Ran out of phv output slots");
}

template <>
void Parser::State::Match::write_row_config(Target::JBay::parser_regs &regs, Parser *pa,
                                            State *state, int row, Match *def,
                                            json::map &ctxt_json) {
    write_common_row_config(regs, pa, state, row, def, ctxt_json);
    auto &action_row = regs.memory[state->gress].po_action_row[row];

    if (disable_partial_hdr_err > 0) action_row.disable_partial_hdr_err = 1;
}

template <>
int Parser::State::Match::Save::write_output_config(Target::JBay::parser_regs &regs, void *_row,
                                                    unsigned &used, int, int) const {
    Target::JBay::parser_regs::_memory::_po_action_row *row =
        (Target::JBay::parser_regs::_memory::_po_action_row *)_row;
    int dest = where->reg.parser_id();
    int mask = (1 << (1 + where->hi / 8U)) - (1 << (where->lo / 8U));
    int lo = this->lo;
    // 8b containers are paired in 16b chunks in the parser
    // If we're extracting to the upper half of a chunk (the odd 8b register) then
    // adjust the extract type to be to the upper half
    if (where->reg.size == 8 && mask == 1) {
        if (where->reg.index & 1) {
            mask <<= 1;
        }
    }
    if (flags & ROTATE) error(where.lineno, "no rotate support in Tofino2");

    // All containers are 16b in the parser. 32b container extracts are implemented as
    // a pair of 16b extracts.
    int bytemask = (mask >> 2) & 3;
    if (bytemask) {
        write_output_slot(where.lineno, row, used, lo, dest + 1, bytemask, flags & OFFSET);
        lo += bitcount(mask & 0xc);
    }

    bytemask = mask & 3;
    if (bytemask) write_output_slot(where.lineno, row, used, lo, dest, bytemask, flags & OFFSET);
    return hi;
}

#define SAVE_ONLY_USED_SLOTS 0xffc00
static void write_output_const_slot(int lineno,
                                    Target::JBay::parser_regs::_memory::_po_action_row *row,
                                    unsigned &used, unsigned src, int dest, int bytemask,
                                    int flags) {
    // use bits 24..27 of 'used' to track the two constant slots
    BUG_CHECK(bytemask > 0 && bytemask < 4);
    BUG_CHECK((src & ~((0xffff00ff >> (8 * (bytemask - 1))) & 0xffff)) == 0);
    // FIXME -- should be able to treat this as 4x8-bit rather than 2x16-bit slots, as long
    // as the ROTATE flag is consistent for each half.
    int cslot = -1;
    // see if const already allocated and reuse
    for (cslot = 0; cslot < 2; cslot++)
        if (row->val_const[cslot] == src && (used & (bytemask << (2 * cslot + 24)))) break;
    if (cslot >= 2) {
        for (cslot = 0; cslot < 2; cslot++)
            if (0 == (used & (bytemask << (2 * cslot + 24)))) break;
    }
    if (cslot >= 2) {
        error(lineno, "Ran out of constant output slots");
        return;
    }
    row->val_const[cslot] |= src;
    if (flags & 2 /*ROTATE*/) row->val_const_rot[cslot] = 1;
    used |= bytemask << (2 * cslot + 24);
    unsigned tmpused = used | SAVE_ONLY_USED_SLOTS;
    write_output_slot(lineno, row, tmpused, 62 - 2 * cslot + (bytemask == 1), dest, bytemask,
                      flags & 1 /*OFFSET*/);
    used |= tmpused & ~SAVE_ONLY_USED_SLOTS;
}

template <>
void Parser::State::Match::Set::write_output_config(Target::JBay::parser_regs &regs, void *_row,
                                                    unsigned &used, int, int) const {
    Target::JBay::parser_regs::_memory::_po_action_row *row =
        (Target::JBay::parser_regs::_memory::_po_action_row *)_row;
    int dest = where->reg.parser_id();
    int mask = (1 << (1 + where->hi / 8U)) - (1 << (where->lo / 8U));
    unsigned what = this->what << where->lo;
    // Trim the bytes to be written, unless the value is being rotated
    if (what && !(flags & ROTATE)) {
        for (unsigned i = 0; i < 4; ++i)
            if (((what >> (8 * i)) & 0xff) == 0) mask &= ~(1 << i);
    }
    if (where->reg.size == 8) {
        BUG_CHECK((mask & ~1) == 0);
        if (where->reg.index & 1) {
            mask <<= 1;
            what <<= 8;
        }
    }
    if (mask & 3)
        write_output_const_slot(where.lineno, row, used, what & 0xffff, dest, mask & 3, flags);
    if (mask & 0xc) {
        write_output_const_slot(where.lineno, row, used, (what >> 16) & 0xffff, dest + 1,
                                (mask >> 2) & 3, flags);
        if ((mask & 3) && (flags & ROTATE)) row->val_const_32b_bond = 1;
    }
}

/* Tofino2 has a simple uniform array of 20 extractors, so doesn't really need an output
 * map to track them.  Constants 'sets' are handled by having 4 bytes of data that is set
 * per row and extrated from the input buffer like a 'save', except only the first 10
 * extractors can access them.  So `output_map` ends up being just a pointer to the
 * register object for the row, and `used` is a 24-bit bitmap, tracking the 20 extractors
 * and the 4 constant bytes.
 */
template <>
void *Parser::setup_phv_output_map(Target::JBay::parser_regs &regs, gress_t gress, int row) {
    return &regs.memory[gress].po_action_row[row];
}
template <>
void Parser::mark_unused_output_map(Target::JBay::parser_regs & /*regs*/, void * /*map*/,
                                    unsigned /*used*/) {
    // unneeded on jbay
}

template <>
void Parser::State::Match::HdrLenIncStop::write_config(
    JBay::memories_parser_::_po_action_row &po_row) const {
    po_row.hdr_len_inc_stop = 1;
    po_row.hdr_len_inc_final_amt = final_amt;
}

template <>
void Parser::State::Match::Clot::write_config(JBay::memories_parser_::_po_action_row &po_row,
                                              int idx, bool offset_add) const {
    po_row.clot_tag[idx] = tag;
    po_row.clot_offset[idx] = start;
    if (load_length) {
        po_row.clot_type[idx] = 1;
        po_row.clot_len_src[idx] = length;
        po_row.clot_en_len_shr[idx] = length_shift;
        // po_row.clot_len_mask[idx] = length_mask; -- FIXME -- CSR reg commented out
    } else {
        po_row.clot_len_src[idx] = length - 1;
        po_row.clot_type[idx] = 0;
        po_row.clot_en_len_shr[idx] = 1;
    }
    po_row.clot_has_csum[idx] = csum_unit > 0;
    po_row.clot_tag_offset_add[idx] = offset_add;
}

template <>
void Parser::State::Match::write_counter_config(
    Target::JBay::parser_regs::_memory::_ml_ea_row &ea_row) const {
    if (ctr_load) {
        switch (ctr_ld_src) {
            case 0:
                ea_row.ctr_op = 2;
                break;
            case 1:
                ea_row.ctr_op = 3;
                break;
            default:
                error(lineno, "Unsupported parser counter load instruction (JBay)");
        }
    } else if (ctr_stack_pop) {
        ea_row.ctr_op = 1;
    } else {  // add
        ea_row.ctr_op = 0;
    }

    ea_row.ctr_amt_idx = ctr_instr ? ctr_instr->addr : ctr_imm_amt;
    ea_row.ctr_stack_push = ctr_stack_push;
    ea_row.ctr_stack_upd_w_top = ctr_stack_upd_w_top;
}

// Workaround for JBAY-2717: parser counter adds RAM index or immediate value
// to the pushed value when doing push + update_w_top. To cancel this offset,
// we subtract the amount on pop.
void jbay2717_workaround(Parser *parser, Target::JBay::parser_regs &regs) {
    for (auto &kv : parser->match_to_row) {
        if (kv.first->ctr_stack_pop) {
            for (auto p : kv.first->get_all_preds()) {
                if (p->ctr_stack_push && p->ctr_stack_upd_w_top) {
                    auto &ea_row = regs.memory[parser->gress].ml_ea_row[kv.second];
                    auto adjust = p->ctr_instr ? p->ctr_instr->addr : p->ctr_imm_amt;
                    ea_row.ctr_amt_idx = ea_row.ctr_amt_idx.value - adjust;
                    break;
                }
            }
        }
    }
}

template <>
void Parser::write_config(Target::JBay::parser_regs &regs, json::map &ctxt_json,
                          bool single_parser) {
    if (single_parser) {
        for (auto st : all)
            st->write_config(regs, this, ctxt_json[st->gress == EGRESS ? "egress" : "ingress"]);
    } else {
        ctxt_json["states"] = json::vector();
        for (auto st : all) st->write_config(regs, this, ctxt_json["states"]);
    }
    if (error_count > 0) return;

    jbay2717_workaround(this, regs);

    int i = 0;
    for (auto ctr : counter_init) {
        if (ctr) ctr->write_config(regs, gress, i);
        ++i;
    }

    for (i = 0; i < checksum_use.size(); i++) {
        for (auto csum : checksum_use[i]) {
            if (csum) {
                csum->write_config(regs, this);
                if (csum->dest) phv_use[csum->gress].setbit(csum->dest->reg.uid);
            }
        }
    }

    if (gress == EGRESS) {
        regs.egress.epbreg.chan0_group.chnl_ctrl.meta_opt = meta_opt;
        regs.egress.epbreg.chan1_group.chnl_ctrl.meta_opt = meta_opt;
        regs.egress.epbreg.chan2_group.chnl_ctrl.meta_opt = meta_opt;
        regs.egress.epbreg.chan3_group.chnl_ctrl.meta_opt = meta_opt;
        regs.egress.epbreg.chan4_group.chnl_ctrl.meta_opt = meta_opt;
        regs.egress.epbreg.chan5_group.chnl_ctrl.meta_opt = meta_opt;
        regs.egress.epbreg.chan6_group.chnl_ctrl.meta_opt = meta_opt;
        regs.egress.epbreg.chan7_group.chnl_ctrl.meta_opt = meta_opt;
    }

    // All phvs used globaly by egress and not by ingress parser should be owned by
    // egress parser so they get zeroed properly in the parser
    phv_use[EGRESS] |= remove_nonparser(Phv::use(EGRESS)) - expand_parser_groups(phv_use[INGRESS]);

    setup_jbay_ownership(phv_use, regs.merge.ul.phv_owner_127_0.owner,
                         regs.merge.ur.phv_owner_255_128.owner, regs.main[INGRESS].phv_owner.owner,
                         regs.main[EGRESS].phv_owner.owner);

    setup_jbay_clear_on_write(phv_allow_clear_on_write, regs.merge.ul.phv_clr_on_wr_127_0.clr,
                              regs.merge.ur.phv_clr_on_wr_255_128.clr,
                              regs.main[INGRESS].phv_clr_on_wr.clr,
                              regs.main[EGRESS].phv_clr_on_wr.clr);

    setup_jbay_no_multi_write(phv_allow_bitwise_or, phv_allow_clear_on_write,
                              regs.main[INGRESS].no_multi_wr.nmw,
                              regs.main[EGRESS].no_multi_wr.nmw);

    regs.main[gress].hdr_len_adj.amt = hdr_len_adj;

    if (parser_error.lineno >= 0) {
        for (auto i : {0, 1}) {
            regs.main[gress].err_phv_cfg[i].en = 1;
            regs.main[gress].err_phv_cfg[i].dst = parser_error->reg.parser_id();
            regs.main[gress].err_phv_cfg[i].no_tcam_match_err_en = 1;
            regs.main[gress].err_phv_cfg[i].partial_hdr_err_en = 1;
            regs.main[gress].err_phv_cfg[i].ctr_range_err_en = 1;
            regs.main[gress].err_phv_cfg[i].timeout_iter_err_en = 1;
            regs.main[gress].err_phv_cfg[i].timeout_cycle_err_en = 1;
            regs.main[gress].err_phv_cfg[i].src_ext_err_en = 1;
            regs.main[gress].err_phv_cfg[i].phv_owner_err_en = 1;
            regs.main[gress].err_phv_cfg[i].multi_wr_err_en = 1;
            regs.main[gress].err_phv_cfg[i].aram_mbe_en = 1;
            regs.main[gress].err_phv_cfg[i].fcs_err_en = 1;
            regs.main[gress].err_phv_cfg[i].csum_mbe_en = 1;
        }
    } else {
        // en has a reset value of 1 and that is why we have to explicitly disable it
        // otherwise dst will assume default value of 0
        for (auto i : {0, 1}) regs.main[gress].err_phv_cfg[i].en = 0;
    }

    int i_start = Stage::first_table(INGRESS) & 0x1ff;
    for (auto &reg : regs.merge.ll1.i_start_table) reg.table = i_start;

    int e_start = Stage::first_table(EGRESS) & 0x1ff;
    for (auto &reg : regs.merge.lr1.e_start_table) reg.table = e_start;

    regs.merge.lr1.g_start_table.table = Stage::first_table(GHOST) & 0x1ff;
    if (ghost_parser.size()) {
        // tm_status_phv sets the location for ghost intrinsic metadata in the
        // parser
        // The parser carves up the 4k of PHV it sees into 256 x 16b containers.
        // (32b MAU containers map to a pair of parser 16b containers, and 2 x
        // 8b MAU containers map to a single parser 16b container.) PHV
        // specified here will take the two containers at address
        // { PHV & 0xfe, PHV & 0xfe + 1 }.
        // Hence, ghost intrinsic metadata is assumed to be allocated in a
        // contiguous 32b location.
        regs.merge.lr1.tm_status_phv.en = 1;
        regs.merge.lr1.tm_status_phv.phv = ghost_parser[0]->reg.parser_id();
        if (ghost_pipe_mask != 0xf)  // if not default set given value
            regs.merge.lr1.tm_status_phv.pipe_mask = ghost_pipe_mask;
    }

    if (gress == INGRESS) {
        for (auto &ref : regs.ingress.prsr)
            ref.set("regs.parser.main.ingress", &regs.main[INGRESS]);
    }
    if (gress == EGRESS) {
        for (auto &ref : regs.egress.prsr) ref.set("regs.parser.main.egress", &regs.main[EGRESS]);
    }
    if (error_count == 0) {
        if (options.condense_json) {
            // FIXME -- removing the uninitialized memory causes problems?
            // FIXME -- walle gets the addresses wrong.  Might also require explicit
            // FIXME -- zeroing in the driver on real hardware
            // regs.memory[INGRESS].disable_if_reset_value();
            // regs.memory[EGRESS].disable_if_reset_value();
            // regs.ingress.disable_if_reset_value();
            // regs.egress.disable_if_reset_value();
            regs.main[INGRESS].disable_if_reset_value();
            regs.main[EGRESS].disable_if_reset_value();
            regs.merge.disable_if_reset_value();
        }
        if (options.gen_json) {
            if (single_parser) {
                regs.memory[INGRESS].emit_json(*open_output("memories.parser.ingress.cfg.json"),
                                               "ingress");
                regs.memory[EGRESS].emit_json(*open_output("memories.parser.egress.cfg.json"),
                                              "egress");
                regs.ingress.emit_json(*open_output("regs.parser.ingress.cfg.json"));
                regs.egress.emit_json(*open_output("regs.parser.egress.cfg.json"));
                regs.main[INGRESS].emit_json(*open_output("regs.parser.main.ingress.cfg.json"),
                                             "ingress");
                regs.main[EGRESS].emit_json(*open_output("regs.parser.main.egress.cfg.json"),
                                            "egress");
                regs.merge.emit_json(*open_output("regs.parse_merge.cfg.json"));
            } else {
                regs.memory[INGRESS].emit_json(
                    *open_output("memories.parser.ingress.%02x.cfg.json", parser_no), "ingress");
                regs.memory[EGRESS].emit_json(
                    *open_output("memories.parser.egress.%02x.cfg.json", parser_no), "egress");
                regs.ingress.emit_json(
                    *open_output("regs.parser.ingress.%02x.cfg.json", parser_no));
                regs.egress.emit_json(*open_output("regs.parser.egress.%02x.cfg.json", parser_no));
                regs.main[INGRESS].emit_json(*open_output("regs.parser.main.ingress.cfg.json"),
                                             "ingress");
                regs.main[EGRESS].emit_json(*open_output("regs.parser.main.egress.cfg.json"),
                                            "egress");
                regs.merge.emit_json(*open_output("regs.parse_merge.cfg.json"));
            }
        }
    }

    /* multiple JBay parser mem blocks can respond to same address range to allow programming
     * the device with a single write operation. See: pardereg.pgstnreg.ibprsr4reg.prsr.mem_ctrl */
    if (gress == INGRESS) {
        for (unsigned i = 0; i < TopLevel::regs<Target::JBay>()->mem_pipe.parde.i_prsr_mem.size();
             options.singlewrite ? i += 4 : i += 1) {
            TopLevel::regs<Target::JBay>()->mem_pipe.parde.i_prsr_mem[i].set(
                "memories.parser.ingress", &regs.memory[INGRESS]);
        }
    }

    if (gress == EGRESS) {
        for (unsigned i = 0; i < TopLevel::regs<Target::JBay>()->mem_pipe.parde.e_prsr_mem.size();
             options.singlewrite ? i += 4 : i += 1) {
            TopLevel::regs<Target::JBay>()->mem_pipe.parde.e_prsr_mem[i].set(
                "memories.parser.egress", &regs.memory[EGRESS]);
        }
    }

    if (gress == INGRESS) {
        for (auto &ref : TopLevel::regs<Target::JBay>()->reg_pipe.pardereg.pgstnreg.ipbprsr4reg)
            ref.set("regs.parser.ingress", &regs.ingress);
    }

    if (gress == EGRESS) {
        for (auto &ref : TopLevel::regs<Target::JBay>()->reg_pipe.pardereg.pgstnreg.epbprsr4reg)
            ref.set("regs.parser.egress", &regs.egress);
    }
    TopLevel::regs<Target::JBay>()->reg_pipe.pardereg.pgstnreg.pmergereg.set("regs.parse_merge",
                                                                             &regs.merge);
}

template <>
void Parser::gen_configuration_cache(Target::JBay::parser_regs &regs, json::vector &cfg_cache) {
    std::string reg_fqname;
    std::string reg_name;
    unsigned reg_value;
    std::string reg_value_str;
    unsigned reg_width = 13;

    /* Publishing meta_opt field for chnl_ctrl register */
    /* Are ovr_pipeid, chnl_clean, init_dprsr_credit, init_ebuf_credit always handled by the
     * driver?
     */
    for (int i = 0; i < 9; i++) {
        reg_fqname = "pardereg.pgstnreg.epbprsr4reg[" + std::to_string(i) +
                     "].epbreg.chan0_group.chnl_ctrl.meta_opt";
        reg_name = "epb" + std::to_string(i) + "parser0_chnl_ctrl_0";
        reg_value = meta_opt;
        reg_value_str = int_to_hex_string(meta_opt, reg_width);
        add_cfg_reg(cfg_cache, reg_fqname, reg_name, reg_value_str);
    }
}
