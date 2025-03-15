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

/* JBay overloads for instructions #included in instruction.cpp
 * WARNING -- this is included in an anonymous namespace, as VLIWInstruction is
 * in that anonymous namespace */

template <typename REGS>
void VLIWInstruction::write_regs_2(REGS &regs, Table *tbl, Table::Actions::Action *act) {
    if (act != tbl->stage->imem_addr_use[imem_thread(tbl->gress)][act->addr]) {
        LOG3("skipping " << tbl->name() << '.' << act->name << " as its imem is used by "
                         << tbl->stage->imem_addr_use[imem_thread(tbl->gress)][act->addr]->name);
        return;
    }
    LOG2(this);
    auto &imem = regs.dp.imem;
    int iaddr = act->addr / ACTION_IMEM_COLORS;
    int color = act->addr % ACTION_IMEM_COLORS;
    unsigned bits = encode();
    BUG_CHECK(slot >= 0);
    unsigned off = slot % Phv::mau_groupsize();
    unsigned side = 0, group = 0;
    switch (slot / Phv::mau_groupsize()) {
        case 0:
            side = 0;
            group = 0;
            break;
        case 1:
            side = 0;
            group = 1;
            break;
        case 2:
            side = 1;
            group = 0;
            break;
        case 3:
            side = 1;
            group = 1;
            break;
        case 4:
            side = 0;
            group = 0;
            break;
        case 5:
            side = 0;
            group = 1;
            break;
        case 6:
            side = 1;
            group = 0;
            break;
        case 7:
            side = 1;
            group = 1;
            break;
        case 8:
            side = 0;
            group = 0;
            break;
        case 9:
            side = 0;
            group = 1;
            break;
        case 10:
            side = 0;
            group = 2;
            break;
        case 11:
            side = 1;
            group = 0;
            break;
        case 12:
            side = 1;
            group = 1;
            break;
        case 13:
            side = 1;
            group = 2;
            break;
        default:
            BUG();
    }

    switch (Phv::reg(slot)->type) {
        case Phv::Register::NORMAL:
            switch (Phv::reg(slot)->size) {
                case 8:
                    BUG_CHECK(group == 0 || group == 1);
                    imem.imem_subword8[side][group][off][iaddr].imem_subword8_instr = bits;
                    imem.imem_subword8[side][group][off][iaddr].imem_subword8_color = color;
                    imem.imem_subword8[side][group][off][iaddr].imem_subword8_parity =
                        parity(bits) ^ color;
                    break;
                case 16:
                    imem.imem_subword16[side][group][off][iaddr].imem_subword16_instr = bits;
                    imem.imem_subword16[side][group][off][iaddr].imem_subword16_color = color;
                    imem.imem_subword16[side][group][off][iaddr].imem_subword16_parity =
                        parity(bits) ^ color;
                    break;
                case 32:
                    BUG_CHECK(group == 0 || group == 1);
                    imem.imem_subword32[side][group][off][iaddr].imem_subword32_instr = bits;
                    imem.imem_subword32[side][group][off][iaddr].imem_subword32_color = color;
                    imem.imem_subword32[side][group][off][iaddr].imem_subword32_parity =
                        parity(bits) ^ color;
                    break;
                default:
                    BUG();
            }
            break;
        case Phv::Register::MOCHA:
            switch (Phv::reg(slot)->size) {
                case 8:
                    BUG_CHECK(group == 0 || group == 1);
                    imem.imem_mocha_subword8[side][group][off - 12][iaddr]
                        .imem_mocha_subword_instr = bits;
                    imem.imem_mocha_subword8[side][group][off - 12][iaddr]
                        .imem_mocha_subword_color = color;
                    imem.imem_mocha_subword8[side][group][off - 12][iaddr]
                        .imem_mocha_subword_parity = parity(bits) ^ color;
                    break;
                case 16:
                    imem.imem_mocha_subword16[side][group][off - 12][iaddr]
                        .imem_mocha_subword_instr = bits;
                    imem.imem_mocha_subword16[side][group][off - 12][iaddr]
                        .imem_mocha_subword_color = color;
                    imem.imem_mocha_subword16[side][group][off - 12][iaddr]
                        .imem_mocha_subword_parity = parity(bits) ^ color;
                    break;
                case 32:
                    BUG_CHECK(group == 0 || group == 1);
                    imem.imem_mocha_subword32[side][group][off - 12][iaddr]
                        .imem_mocha_subword_instr = bits;
                    imem.imem_mocha_subword32[side][group][off - 12][iaddr]
                        .imem_mocha_subword_color = color;
                    imem.imem_mocha_subword32[side][group][off - 12][iaddr]
                        .imem_mocha_subword_parity = parity(bits) ^ color;
                    break;
                default:
                    BUG();
            }
            break;
        case Phv::Register::DARK:
            switch (Phv::reg(slot)->size) {
                case 8:
                    BUG_CHECK(group == 0 || group == 1);
                    imem.imem_dark_subword8[side][group][off - 16][iaddr].imem_dark_subword_instr =
                        bits;
                    imem.imem_dark_subword8[side][group][off - 16][iaddr].imem_dark_subword_color =
                        color;
                    imem.imem_dark_subword8[side][group][off - 16][iaddr].imem_dark_subword_parity =
                        parity(bits) ^ color;
                    break;
                case 16:
                    imem.imem_dark_subword16[side][group][off - 16][iaddr].imem_dark_subword_instr =
                        bits;
                    imem.imem_dark_subword16[side][group][off - 16][iaddr].imem_dark_subword_color =
                        color;
                    imem.imem_dark_subword16[side][group][off - 16][iaddr]
                        .imem_dark_subword_parity = parity(bits) ^ color;
                    break;
                case 32:
                    BUG_CHECK(group == 0 || group == 1);
                    imem.imem_dark_subword32[side][group][off - 16][iaddr].imem_dark_subword_instr =
                        bits;
                    imem.imem_dark_subword32[side][group][off - 16][iaddr].imem_dark_subword_color =
                        color;
                    imem.imem_dark_subword32[side][group][off - 16][iaddr]
                        .imem_dark_subword_parity = parity(bits) ^ color;
                    break;
                default:
                    BUG();
            }
            break;
        default:
            BUG();
    }

    auto &power_ctl = regs.dp.actionmux_din_power_ctl;
    phvRead([&](const Phv::Slice &sl) { set_power_ctl_reg(power_ctl, sl.reg.mau_id()); });
}

void VLIWInstruction::write_regs(Target::JBay::mau_regs &regs, Table *tbl,
                                 Table::Actions::Action *act) {
    write_regs_2<Target::JBay::mau_regs>(regs, tbl, act);
}
