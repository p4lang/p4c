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

/* deparser template specializations for tofino -- #included directly in top-level deparser.cpp */

#define YES(X) X
#define NO(X)

#define SIMPLE_INTRINSIC(GR, PFX, NAME, IF_SHIFT)                                        \
    DEPARSER_INTRINSIC(Tofino, GR, NAME, 1) {                                            \
        PFX.NAME.phv = intrin.vals[0].val->reg.deparser_id();                            \
        IF_SHIFT(PFX.NAME.shft = intrin.vals[0].val->lo;)                                \
        if (!intrin.vals[0].pov.empty())                                                 \
            error(intrin.vals[0].pov.front().lineno, "No POV support in tofino " #NAME); \
        PFX.NAME.valid = 1;                                                              \
    }
#define SIMPLE_INTRINSIC_RENAME(GR, PFX, NAME, REGNAME, IF_SHIFT) \
    DEPARSER_INTRINSIC(Tofino, GR, NAME, 1) {                     \
        PFX.REGNAME.phv = intrin.vals[0].val->reg.deparser_id();  \
        IF_SHIFT(PFX.REGNAME.shft = intrin.vals[0].val->lo;)      \
        PFX.REGNAME.valid = 1;                                    \
    }
#define IIR_MAIN_INTRINSIC(NAME, SHFT) SIMPLE_INTRINSIC(INGRESS, regs.input.iir.main_i, NAME, SHFT)
#define IIR_INTRINSIC(NAME, SHFT) SIMPLE_INTRINSIC(INGRESS, regs.input.iir.ingr, NAME, SHFT)
#define HIR_INTRINSIC(NAME, SHFT) SIMPLE_INTRINSIC(INGRESS, regs.header.hir.ingr, NAME, SHFT)
#define HIR_INTRINSIC_RENAME(NAME, REGNAME, SHFT) \
    SIMPLE_INTRINSIC_RENAME(INGRESS, regs.header.hir.ingr, NAME, REGNAME, SHFT)
#define IER_MAIN_INTRINSIC(NAME, SHFT) SIMPLE_INTRINSIC(EGRESS, regs.input.ier.main_e, NAME, SHFT)
#define HER_INTRINSIC(NAME, SHFT) SIMPLE_INTRINSIC(EGRESS, regs.header.her.egr, NAME, SHFT)

IIR_MAIN_INTRINSIC(egress_unicast_port, NO)
IIR_MAIN_INTRINSIC(drop_ctl, YES)
IIR_INTRINSIC(copy_to_cpu, YES)
HIR_INTRINSIC_RENAME(egress_multicast_group_0, egress_multicast_group[0], NO)
HIR_INTRINSIC_RENAME(egress_multicast_group_1, egress_multicast_group[1], NO)
HIR_INTRINSIC_RENAME(hash_lag_ecmp_mcast_0, hash_lag_ecmp_mcast[0], NO)
HIR_INTRINSIC_RENAME(hash_lag_ecmp_mcast_1, hash_lag_ecmp_mcast[1], NO)
HIR_INTRINSIC(copy_to_cpu_cos, YES)
DEPARSER_INTRINSIC(Tofino, INGRESS, ingress_port_source, 1) {
    regs.header.hir.ingr.ingress_port.phv = intrin.vals[0].val->reg.deparser_id();
    regs.header.hir.ingr.ingress_port.sel = 0;
}
HIR_INTRINSIC(deflect_on_drop, YES)
HIR_INTRINSIC(meter_color, YES)
HIR_INTRINSIC(icos, YES)
HIR_INTRINSIC(qid, YES)
HIR_INTRINSIC(xid, NO)
HIR_INTRINSIC(yid, NO)
HIR_INTRINSIC(rid, NO)
HIR_INTRINSIC(bypss_egr, YES)
HIR_INTRINSIC(ct_disable, YES)
HIR_INTRINSIC(ct_mcast, YES)

IER_MAIN_INTRINSIC(egress_unicast_port, NO)
IER_MAIN_INTRINSIC(drop_ctl, YES)
HER_INTRINSIC(force_tx_err, YES)
HER_INTRINSIC(tx_pkt_has_offsets, YES)
HER_INTRINSIC(capture_tx_ts, YES)
HER_INTRINSIC(coal, NO)
HER_INTRINSIC(ecos, YES)

#undef SIMPLE_INTRINSIC
#undef IIR_MAIN_INTRINSIC
#undef IIR_INTRINSIC
#undef HIR_INTRINSIC
#undef IER_INTRINSIC
#undef HER_INTRINSIC

#define TOFINO_DIGEST(GRESS, NAME, CFG, TBL, IFSHIFT, IFID, CNT)                                \
    DEPARSER_DIGEST(Tofino, GRESS, NAME, CNT, IFSHIFT(can_shift = true;)) {                     \
        CFG.phv = data.select->reg.deparser_id();                                               \
        IFSHIFT(CFG.shft = data.shift + data.select->lo;)                                       \
        CFG.valid = 1;                                                                          \
        if (!data.select.pov.empty())                                                           \
            error(data.select.pov.front().lineno, "No POV bit support in tofino %s digest",     \
                  #NAME);                                                                       \
        for (auto &set : data.layout) {                                                         \
            int id = set.first >> data.shift;                                                   \
            unsigned idx = 0;                                                                   \
            bool first = true, ok = true;                                                       \
            int last = -1;                                                                      \
            int maxidx = TBL[id].phvs.size() - 1;                                               \
            for (auto &reg : set.second) {                                                      \
                if (first) {                                                                    \
                    first = false;                                                              \
                    IFID(TBL[id].id_phv = reg->reg.deparser_id(); continue;)                    \
                }                                                                               \
                /* The same 16b/32b container cannot appear consecutively, but 8b can. */       \
                if (last == reg->reg.deparser_id() && reg->reg.size != 8) {                     \
                    error(data.lineno, "%s: %db container %s seen in consecutive locations",    \
                          #NAME, reg->reg.size, reg->reg.name);                                 \
                    continue;                                                                   \
                }                                                                               \
                for (int i = reg->reg.size / 8; i > 0; i--) {                                   \
                    if (idx > maxidx) {                                                         \
                        error(data.lineno, "%s digest limited to %d bytes", #NAME, maxidx + 1); \
                        ok = false;                                                             \
                        break;                                                                  \
                    }                                                                           \
                    TBL[id].phvs[idx++] = reg->reg.deparser_id();                               \
                }                                                                               \
                last = reg->reg.deparser_id();                                                  \
                if (!ok) break;                                                                 \
            }                                                                                   \
            TBL[id].valid = 1;                                                                  \
            TBL[id].len = idx;                                                                  \
        }                                                                                       \
    }

TOFINO_DIGEST(INGRESS, learning, regs.input.iir.ingr.learn_cfg, regs.input.iir.ingr.learn_tbl, NO,
              NO, 8)
TOFINO_DIGEST(INGRESS, mirror, regs.header.hir.main_i.mirror_cfg, regs.header.hir.main_i.mirror_tbl,
              YES, YES, 8)
TOFINO_DIGEST(EGRESS, mirror, regs.header.her.main_e.mirror_cfg, regs.header.her.main_e.mirror_tbl,
              YES, YES, 8)
TOFINO_DIGEST(INGRESS, resubmit, regs.input.iir.ingr.resub_cfg, regs.input.iir.ingr.resub_tbl, YES,
              NO, 8)

void tofino_field_dictionary(checked_array_base<fde_pov> &fde_control,
                             checked_array_base<fde_phv> &fde_data,
                             checked_array_base<ubits<8>> &pov_layout,
                             std::vector<Phv::Ref> &pov_order,
                             ordered_map<const Phv::Register *, unsigned> &reg_pov,
                             std::vector<Deparser::FDEntry> &dict, json::vector &fd_gress,
                             json::vector &fd_entries, gress_t gress) {
    std::map<unsigned, unsigned> pov;
    json::vector chunk_bytes;
    json::vector fd_entry_chunk_bytes;
    unsigned pov_byte = 0, pov_size = 0, total_headers = 0;
    for (auto &ent : pov_order)
        if (pov.count(ent->reg.deparser_id()) == 0) {
            total_headers++;
            pov[ent->reg.deparser_id()] = pov_size;
            pov_size += ent->reg.size;
            for (unsigned i = 0; i < ent->reg.size; i += 8) {
                if (pov_byte >= Target::Tofino::DEPARSER_MAX_POV_BYTES) {
                    error(ent.lineno,
                          "Exceeded hardware limit for POV bits (%d) in deparser. "
                          "Using %d or more headers. Please reduce the number of headers",
                          Target::Tofino::DEPARSER_MAX_POV_BYTES * 8, total_headers);
                    return;
                }
                pov_layout[pov_byte++] = ent->reg.deparser_id();
            }
        }
    while (pov_byte < Target::Tofino::DEPARSER_MAX_POV_BYTES) pov_layout[pov_byte++] = 0xff;

    int row = -1, prev = -1, prev_pov = -1;
    bool prev_is_checksum = false;
    unsigned pos = 0;
    unsigned total_bytes = 0;
    int prev_row = 0;
    for (auto &ent : dict) {
        unsigned size = ent.what->size();
        total_bytes += size;
        int pov_bit = pov[ent.pov.front()->reg.deparser_id()] + ent.pov.front()->lo;

        if (options.match_compiler) {
            if (ent.what->is<Deparser::FDEntry::Checksum>()) {
                /* checksum unit -- make sure it gets its own dictionary line */
                prev_pov = -1;
                prev_is_checksum = true;
            } else {
                if (prev_is_checksum) prev_pov = -1;
                prev_is_checksum = false;
            }
        }

        if (ent.what->is<Deparser::FDEntry::Phv>() && prev_pov == pov_bit &&
            int(ent.what->encode()) == prev && ent.what->size() & 6)
            error(ent.lineno, "16 and 32-bit container cannot be repeatedly deparsed");
        while (size--) {
            if (pov_bit != prev_pov || pos >= 4 /*|| (pos & (size-1)) != 0*/) {
                if (row >= 0) {
                    fde_control[row].num_bytes = pos & 3;
                    fde_data[row].num_bytes = pos & 3;
                }
                // Entries used - (192 each in INGRESS & EGRESS for Tofino)
                if (++row >= Target::Tofino::DEPARSER_MAX_FD_ENTRIES) {
                    error(ent.lineno,
                          "Exceeded hardware limit for "
                          "deparser field dictionary entries (%d). Using %d headers and %" PRIu64
                          " containers. Please reduce the number of headers and/or their length.",
                          Target::Tofino::DEPARSER_MAX_FD_ENTRIES, total_headers,
                          uint64_t(dict.size()));
                    return;
                }
                fde_control[row].pov_sel = pov_bit;
                fde_control[row].version = 0xf;
                fde_control[row].valid = 1;
                pos = 0;
            }
            if (prev_row != row) {
                json::map fd;
                json::map fd_entry;
                fd["Field Dictionary Number"] = prev_row;
                fd_entry["entry"] = prev_row;
                auto prevPovReg = Phv::reg(pov_layout[fde_control[prev_row].pov_sel.value / 8]);
                auto prevPovBit = fde_control[prev_row].pov_sel.value;
                auto prevPovOffset = prevPovBit - reg_pov[prevPovReg];
                Deparser::write_pov_in_json(fd, fd_entry, prevPovReg, prevPovBit, prevPovOffset);
                fd["Content"] = std::move(chunk_bytes);
                fd_entry["chunks"] = std::move(fd_entry_chunk_bytes);
                fd_gress.push_back(std::move(fd));
                fd_entries.push_back(std::move(fd_entry));
                prev_row = row;
            }
            auto povReg = Phv::reg(pov_layout[fde_control[row].pov_sel.value / 8]);
            auto povBit = fde_control[row].pov_sel.value % povReg->size;
            json::map chunk_byte;
            json::map fd_entry_chunk_byte;
            json::map fd_entry_chunk;
            chunk_byte["Byte"] = pos;
            fd_entry_chunk_byte["chunk_number"] = pos;
            auto phvReg = Phv::reg(ent.what->encode());
            if (ent.what->encode() < CHECKSUM_ENGINE_PHVID_TOFINO_LOW ||
                ent.what->encode() > CHECKSUM_ENGINE_PHVID_TOFINO_HIGH) {
                write_field_name_in_json(phvReg, povReg, povBit, chunk_byte, fd_entry_chunk, 11,
                                         gress);
            } else {
                write_csum_const_in_json(ent.what->encode(), chunk_byte, fd_entry_chunk, gress);
            }
            fd_entry_chunk_byte["chunk"] = std::move(fd_entry_chunk);
            chunk_bytes.push_back(std::move(chunk_byte.clone()));
            fd_entry_chunk_bytes.push_back(std::move(fd_entry_chunk_byte.clone()));
            fde_data[row].phv[pos++] = ent.what->encode();
            prev_pov = pov_bit;
        }

        prev = ent.what->encode();
    }
    if (pos) {
        fde_control[row].num_bytes = pos & 3;
        fde_data[row].num_bytes = pos & 3;
    }

    // Compute average occupancy.  For deparser FDE compression to work,
    // need to make sure have certain average occupancy.
    // This error check may still be too high level.  I think it needs a finer granularity,
    // but I'm not sure how to model the allowed variability of packet headers.

    // Tofino deparser has a maximum output header size of 480 bytes.  This is done in 2 phases.
    // Each phase can do 240 bytes, corresponding to 18 QFDEs (4 * 18 * 4 bytes = 288 bytes)
    // This means that average occupancy must be better than 240 / 288 bytes, or roughly 83%.
    // This is the value we will check.
    // We gate the check on total bytes occupied being greater than 64 bytes in an attempt
    // to consider the QFDE constraint that it can only drive four stage 2 buses for compression.

    unsigned max_bytes_for_rows_occupied = 4 * (row + 1);
    double occupancy = 0.0;

    if (max_bytes_for_rows_occupied > 0)
        occupancy =
            static_cast<double>(total_bytes) / static_cast<double>(max_bytes_for_rows_occupied);

    if (total_bytes > 64 && occupancy < (240.0 / 288.0)) {
        std::stringstream warn_msg;
        warn_msg.precision(4);
        warn_msg << "Deparser field dictionary occupancy is too sparse.";
        warn_msg << "\nHardware requires an occupancy of " << 100.0 * 240.0 / 288.0
                 << " to deparse the output header,";
        warn_msg << "\nbut the PHV layout for the header structures was such that"
                    "  the occupancy was only "
                 << 100.0 * occupancy << ".";
        warn_msg << "\nThis situation is usually caused by a program that has one or"
                    " more of the following requirements:";
        warn_msg << "\n  1. many 'short' headers that are not guaranteed to coexist"
                    " (e.g. less than 4 bytes)";
        warn_msg << "\n  2. many packet headers that are not multiples of 4 bytes";
        warn_msg << "\n  3. many conditionally updated checksums";
        warning(0, "%s", warn_msg.str().c_str());
    }
}

template <typename IN_GRP, typename IN_SPLIT, typename EG_GRP, typename EG_SPLIT>
void tofino_phv_ownership(bitvec phv_use[2], IN_GRP &in_grp, IN_SPLIT &in_split, EG_GRP &eg_grp,
                          EG_SPLIT &eg_split, unsigned first, unsigned count) {
    BUG_CHECK(in_grp.val.size() == eg_grp.val.size(), "in_grp and eg_grp must have same size");
    BUG_CHECK(in_split.val.size() == eg_split.val.size(),
              "in_split and eg_split must have same size");
    BUG_CHECK((in_grp.val.size() + 1) * in_split.val.size() == count,
              "in_grp and in_split must have same size");
    unsigned group_size = in_split.val.size();
    // DANGER -- this only works because tofino Phv::Register uids happend to match
    // DANGER -- the deparser encoding of phv containers.
    unsigned reg = first;
    for (unsigned i = 0; i < in_grp.val.size(); i++, reg += group_size) {
        unsigned last = reg + group_size - 1;
        int count = 0;
        if (phv_use[INGRESS].getrange(reg, group_size)) {
            in_grp.val |= 1U << i;
            if (i * group_size >= 16 && i * group_size < 32)
                error(0, "%s..%s(R%d..R%d) used by ingress deparser but only available to egress",
                      Phv::reg(reg)->name, Phv::reg(last)->name, reg, last);
            else
                count++;
        }
        if (phv_use[EGRESS].getrange(reg, group_size)) {
            eg_grp.val |= 1U << i;
            if (i * group_size < 16)
                error(0, "%s..%s(R%d..R%d) used by egress deparser but only available to ingress",
                      Phv::reg(reg)->name, Phv::reg(last)->name, reg, last);
            else
                count++;
        }
        if (count > 1)
            error(0, "%s..%s(R%d..R%d) used by both ingress and egress deparser",
                  Phv::reg(reg)->name, Phv::reg(last)->name, reg, last);
    }
    in_split.val = phv_use[INGRESS].getrange(reg, group_size);
    eg_split.val = phv_use[EGRESS].getrange(reg, group_size);
}

static short tofino_phv2cksum[Target::Tofino::Phv::NUM_PHV_REGS][2] = {
    // normal {LSWord, MSWord}
    {287, 286},
    {283, 282},
    {279, 278},
    {275, 274},
    {271, 270},
    {267, 266},
    {263, 262},
    {259, 258},
    {255, 254},
    {251, 250},
    {247, 246},
    {243, 242},
    {239, 238},
    {235, 234},
    {231, 230},
    {227, 226},
    {223, 222},
    {219, 218},
    {215, 214},
    {211, 210},
    {207, 206},
    {203, 202},
    {199, 198},
    {195, 194},
    {191, 190},
    {187, 186},
    {183, 182},
    {179, 178},
    {175, 174},
    {171, 170},
    {167, 166},
    {163, 162},
    {285, 284},
    {281, 280},
    {277, 276},
    {273, 272},
    {269, 268},
    {265, 264},
    {261, 260},
    {257, 256},
    {253, 252},
    {249, 248},
    {245, 244},
    {241, 240},
    {237, 236},
    {233, 232},
    {229, 228},
    {225, 224},
    {221, 220},
    {217, 216},
    {213, 212},
    {209, 208},
    {205, 204},
    {201, 200},
    {197, 196},
    {193, 192},
    {189, 188},
    {185, 184},
    {181, 180},
    {177, 176},
    {173, 172},
    {169, 168},
    {165, 164},
    {161, 160},
    {147, -1},
    {145, -1},
    {143, -1},
    {141, -1},
    {127, -1},
    {125, -1},
    {123, -1},
    {121, -1},
    {107, -1},
    {105, -1},
    {103, -1},
    {101, -1},
    {87, -1},
    {85, -1},
    {83, -1},
    {81, -1},
    {67, -1},
    {65, -1},
    {63, -1},
    {61, -1},
    {47, -1},
    {45, -1},
    {43, -1},
    {41, -1},
    {27, -1},
    {25, -1},
    {23, -1},
    {21, -1},
    {7, -1},
    {5, -1},
    {3, -1},
    {1, -1},
    {146, -1},
    {144, -1},
    {142, -1},
    {140, -1},
    {126, -1},
    {124, -1},
    {122, -1},
    {120, -1},
    {106, -1},
    {104, -1},
    {102, -1},
    {100, -1},
    {86, -1},
    {84, -1},
    {82, -1},
    {80, -1},
    {66, -1},
    {64, -1},
    {62, -1},
    {60, -1},
    {46, -1},
    {44, -1},
    {42, -1},
    {40, -1},
    {26, -1},
    {24, -1},
    {22, -1},
    {20, -1},
    {6, -1},
    {4, -1},
    {2, -1},
    {0, -1},
    {159, -1},
    {157, -1},
    {155, -1},
    {153, -1},
    {151, -1},
    {149, -1},
    {139, -1},
    {137, -1},
    {135, -1},
    {133, -1},
    {131, -1},
    {129, -1},
    {119, -1},
    {117, -1},
    {115, -1},
    {113, -1},
    {111, -1},
    {109, -1},
    {99, -1},
    {97, -1},
    {95, -1},
    {93, -1},
    {91, -1},
    {89, -1},
    {79, -1},
    {77, -1},
    {75, -1},
    {73, -1},
    {71, -1},
    {69, -1},
    {59, -1},
    {57, -1},
    {55, -1},
    {53, -1},
    {51, -1},
    {49, -1},
    {39, -1},
    {37, -1},
    {35, -1},
    {33, -1},
    {31, -1},
    {29, -1},
    {19, -1},
    {17, -1},
    {15, -1},
    {13, -1},
    {11, -1},
    {9, -1},
    {158, -1},
    {156, -1},
    {154, -1},
    {152, -1},
    {150, -1},
    {148, -1},
    {138, -1},
    {136, -1},
    {134, -1},
    {132, -1},
    {130, -1},
    {128, -1},
    {118, -1},
    {116, -1},
    {114, -1},
    {112, -1},
    {110, -1},
    {108, -1},
    {98, -1},
    {96, -1},
    {94, -1},
    {92, -1},
    {90, -1},
    {88, -1},
    {78, -1},
    {76, -1},
    {74, -1},
    {72, -1},
    {70, -1},
    {68, -1},
    {58, -1},
    {56, -1},
    {54, -1},
    {52, -1},
    {50, -1},
    {48, -1},
    {38, -1},
    {36, -1},
    {34, -1},
    {32, -1},
    {30, -1},
    {28, -1},
    {18, -1},
    {16, -1},
    {14, -1},
    {12, -1},
    {10, -1},
    {8, -1},

    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},

    // tagalong {LSWord, MSWord}
    {1, 0},
    {3, 2},
    {5, 4},
    {7, 6},
    {9, 8},
    {11, 10},
    {13, 12},
    {15, 14},
    {17, 16},
    {19, 18},
    {21, 20},
    {23, 22},
    {25, 24},
    {27, 26},
    {29, 28},
    {31, 30},
    {33, 32},
    {35, 34},
    {37, 36},
    {39, 38},
    {41, 40},
    {43, 42},
    {45, 44},
    {47, 46},
    {49, 48},
    {51, 50},
    {53, 52},
    {55, 54},
    {57, 56},
    {59, 58},
    {61, 60},
    {63, 62},
    {64, -1},
    {65, -1},
    {66, -1},
    {67, -1},
    {68, -1},
    {69, -1},
    {70, -1},
    {71, -1},
    {72, -1},
    {73, -1},
    {74, -1},
    {75, -1},
    {76, -1},
    {77, -1},
    {78, -1},
    {79, -1},
    {80, -1},
    {81, -1},
    {82, -1},
    {83, -1},
    {84, -1},
    {85, -1},
    {86, -1},
    {87, -1},
    {88, -1},
    {89, -1},
    {90, -1},
    {91, -1},
    {92, -1},
    {93, -1},
    {94, -1},
    {95, -1},
    {96, -1},
    {97, -1},
    {98, -1},
    {99, -1},
    {100, -1},
    {101, -1},
    {102, -1},
    {103, -1},
    {104, -1},
    {105, -1},
    {106, -1},
    {107, -1},
    {108, -1},
    {109, -1},
    {110, -1},
    {111, -1},
    {112, -1},
    {113, -1},
    {114, -1},
    {115, -1},
    {116, -1},
    {117, -1},
    {118, -1},
    {119, -1},
    {120, -1},
    {121, -1},
    {122, -1},
    {123, -1},
    {124, -1},
    {125, -1},
    {126, -1},
    {127, -1},
    {128, -1},
    {129, -1},
    {130, -1},
    {131, -1},
    {132, -1},
    {133, -1},
    {134, -1},
    {135, -1},
    {136, -1},
    {137, -1},
    {138, -1},
    {139, -1},
    {140, -1},
    {141, -1},
    {142, -1},
    {143, -1}};

#define TAGALONG_THREAD_BASE                                                        \
    (Target::Tofino::Phv::COUNT_8BIT_TPHV + Target::Tofino::Phv::COUNT_16BIT_TPHV + \
     2 * Target::Tofino::Phv::COUNT_32BIT_TPHV)

template <typename DTYPE, typename STYPE>
static void copy_csum_cfg_entry(DTYPE &dst_unit, STYPE &src_unit) {
    BUG_CHECK(dst_unit.size() == src_unit.size(), "dst_unit and src_unit have different sizes");

    for (unsigned i = 0; i < dst_unit.size(); i++) {
        auto &src = src_unit[i];
        auto &dst = dst_unit[i];

        dst.zero_l_s_b = src.zero_l_s_b;
        dst.zero_m_s_b = src.zero_m_s_b;
        dst.swap = src.swap;
    }
}

template <class ENTRIES>
static void init_tofino_checksum_entry(ENTRIES &entry) {
    entry.zero_l_s_b = 1;
    entry.zero_l_s_b.rewrite();
    entry.zero_m_s_b = 1;
    entry.zero_m_s_b.rewrite();
    entry.swap = 0;
    entry.swap.rewrite();
}

template <typename IPO, typename HPO>
static void tofino_checksum_units(checked_array_base<IPO> &main_csum_units,
                                  checked_array_base<HPO> &tagalong_csum_units, gress_t gress,
                                  Deparser::FullChecksumUnit checksum_unit[]) {
    BUG_CHECK(tofino_phv2cksum[Target::Tofino::Phv::NUM_PHV_REGS - 1][0] == 143,
              "invalid phv2cksum");
    for (int i = 0; i < Target::Tofino::DEPARSER_CHECKSUM_UNITS; i++) {
        auto &main_unit = main_csum_units[i].csum_cfg_entry;
        auto &tagalong_unit = tagalong_csum_units[i].csum_cfg_entry;
        auto &tagalong_unit_zeros_as_ones = tagalong_csum_units[i].zeros_as_ones;
        for (auto &ent : main_unit) init_tofino_checksum_entry(ent);
        for (auto &ent : tagalong_unit) init_tofino_checksum_entry(ent);
        if (checksum_unit[i].entries.empty()) continue;
        // Tofino does not support checksum calculation using multiple
        // partial checksum unit.
        // Full checksum unit and partial checksum unit will always be same
        BUG_CHECK(checksum_unit[i].entries.size() == 1,
                  "multiple partial checksum unit not supported");
        auto &checksum_unit_entries = checksum_unit[i].entries[i];
        for (auto &reg : checksum_unit_entries) {
            int mask = reg.mask;
            int swap = reg.swap;
            int idx = reg->reg.deparser_id();
            if (!reg.pov.empty())
                error(reg.pov.front().lineno, "No POV support in tofino checksum");
            auto cksum_idx0 = tofino_phv2cksum[idx][0];
            auto cksum_idx1 = tofino_phv2cksum[idx][1];
            BUG_CHECK(cksum_idx0 >= 0, "invalid phv2cksum");
            if (idx >= 256) {
                write_checksum_entry(tagalong_unit[cksum_idx0], mask & 3, swap & 1, i,
                                     reg->reg.name);
                if (cksum_idx1 >= 0)
                    write_checksum_entry(tagalong_unit[cksum_idx1], mask >> 2, swap >> 1, i,
                                         reg->reg.name);
                else
                    BUG_CHECK((mask >> 2 == 0) && (swap >> 1 == 0), "invalid phv2cksum");
            } else {
                write_checksum_entry(main_unit[cksum_idx0], mask & 3, swap & 1, i, reg->reg.name);
                if (cksum_idx1 >= 0)
                    write_checksum_entry(main_unit[cksum_idx1], mask >> 2, swap >> 1, i,
                                         reg->reg.name);
                else
                    BUG_CHECK((mask >> 2 == 0) && (swap >> 1 == 0), "invalid phv2cksum");
            }
        }
        // Thread non-tagalong checksum results through the tagalong unit
        int idx = i + TAGALONG_THREAD_BASE + gress * Target::Tofino::DEPARSER_CHECKSUM_UNITS;
        write_checksum_entry(tagalong_unit[idx], 0x3, 0x0, i);
        // Setting Zeros_As_Ones enable
        tagalong_unit_zeros_as_ones.en = checksum_unit[i].zeros_as_ones_en;
        main_unit.set_modified();
        tagalong_unit.set_modified();
    }
}

static void tofino_checksum_units(
    Target::Tofino::deparser_regs &regs,
    Deparser::FullChecksumUnit full_checksum_unit[2][MAX_DEPARSER_CHECKSUM_UNITS]) {
    for (unsigned id = 2; id < MAX_DEPARSER_CHECKSUM_UNITS; id++) {
        if (!full_checksum_unit[0][id].entries.empty() &&
            !full_checksum_unit[1][id].entries.empty())
            error(-1, "deparser checksum unit %d used in both ingress and egress", id);
    }

    tofino_checksum_units(regs.input.iim.ii_phv_csum.csum_cfg,
                          regs.header.him.hi_tphv_csum.csum_cfg, INGRESS,
                          full_checksum_unit[INGRESS]);
    tofino_checksum_units(regs.input.iem.ie_phv_csum.csum_cfg,
                          regs.header.hem.he_tphv_csum.csum_cfg, EGRESS,
                          full_checksum_unit[EGRESS]);

    // make sure shared units are configured identically
    for (unsigned id = 2; id < Target::Tofino::DEPARSER_CHECKSUM_UNITS; id++) {
        auto &eg_main_unit = regs.input.iem.ie_phv_csum.csum_cfg[id].csum_cfg_entry;
        auto &ig_main_unit = regs.input.iim.ii_phv_csum.csum_cfg[id].csum_cfg_entry;

        auto &eg_tphv_unit = regs.header.hem.he_tphv_csum.csum_cfg[id].csum_cfg_entry;
        auto &ig_tphv_unit = regs.header.him.hi_tphv_csum.csum_cfg[id].csum_cfg_entry;

        if (!full_checksum_unit[0][id].entries.empty()) {
            copy_csum_cfg_entry(eg_main_unit, ig_main_unit);
            copy_csum_cfg_entry(eg_tphv_unit, ig_tphv_unit);
        } else if (!full_checksum_unit[1][id].entries.empty()) {
            copy_csum_cfg_entry(ig_main_unit, eg_main_unit);
            copy_csum_cfg_entry(ig_tphv_unit, eg_tphv_unit);
        }
    }
}

template <>
void Deparser::write_config(Target::Tofino::deparser_regs &regs) {
    regs.input.icr.inp_cfg.disable();
    regs.input.icr.intr.disable();
    regs.header.hem.he_edf_cfg.disable();
    regs.header.him.hi_edf_cfg.disable();

    tofino_checksum_units(regs, full_checksum_unit);
    json::map field_dictionary_alloc;
    json::vector fd_gress;
    json::vector fde_entries_i;
    json::vector fde_entries_e;

    // Deparser resources
    json::vector resources_deparser;

    // Create field dictionaries for ingress
    tofino_field_dictionary(regs.input.iim.ii_fde_pov.fde_pov, regs.header.him.hi_fde_phv.fde_phv,
                            regs.input.iir.main_i.pov.phvs, pov_order[INGRESS], pov[INGRESS],
                            dictionary[INGRESS], fd_gress, fde_entries_i, INGRESS);
    field_dictionary_alloc["ingress"] = std::move(fd_gress);
    // Create field dictionaries for egress
    tofino_field_dictionary(regs.input.iem.ie_fde_pov.fde_pov, regs.header.hem.he_fde_phv.fde_phv,
                            regs.input.ier.main_e.pov.phvs, pov_order[EGRESS], pov[EGRESS],
                            dictionary[EGRESS], fd_gress, fde_entries_e, EGRESS);
    field_dictionary_alloc["egress"] = std::move(fd_gress);

    if (Log::verbosity() > 0) {
        auto json_dump = open_output("logs/field_dictionary.log");
        *json_dump << &field_dictionary_alloc;
    }
    // Output deparser resources
    report_resources_deparser_json(fde_entries_i, fde_entries_e);

    if (Phv::use(INGRESS).intersects(Phv::use(EGRESS))) {
        warning(lineno[INGRESS], "Registers used in both ingress and egress in pipeline: %s",
                Phv::db_regset(Phv::use(INGRESS) & Phv::use(EGRESS)).c_str());
        /* FIXME -- this only (sort-of) works because 'deparser' comes first in the alphabet,
         * FIXME -- so is the first section to have its 'output' method run.  Its a hack
         * FIXME -- anyways to attempt to correct broken asm that should be an error */
        Phv::unsetuse(INGRESS, phv_use[EGRESS]);
        Phv::unsetuse(EGRESS, phv_use[INGRESS]);
    }

    tofino_phv_ownership(phv_use, regs.input.iir.ingr.phv8_grp, regs.input.iir.ingr.phv8_split,
                         regs.input.ier.egr.phv8_grp, regs.input.ier.egr.phv8_split,
                         Target::Tofino::Phv::FIRST_8BIT_PHV, Target::Tofino::Phv::COUNT_8BIT_PHV);
    tofino_phv_ownership(phv_use, regs.input.iir.ingr.phv16_grp, regs.input.iir.ingr.phv16_split,
                         regs.input.ier.egr.phv16_grp, regs.input.ier.egr.phv16_split,
                         Target::Tofino::Phv::FIRST_16BIT_PHV,
                         Target::Tofino::Phv::COUNT_16BIT_PHV);
    tofino_phv_ownership(phv_use, regs.input.iir.ingr.phv32_grp, regs.input.iir.ingr.phv32_split,
                         regs.input.ier.egr.phv32_grp, regs.input.ier.egr.phv32_split,
                         Target::Tofino::Phv::FIRST_32BIT_PHV,
                         Target::Tofino::Phv::COUNT_32BIT_PHV);

    for (unsigned i = 0; i < 8; i++) {
        if (phv_use[EGRESS].intersects(Target::Tofino::Phv::tagalong_groups[i])) {
            regs.input.icr.tphv_cfg.i_e_assign |= 1 << i;
            if (phv_use[INGRESS].intersects(Target::Tofino::Phv::tagalong_groups[i])) {
                error(lineno[INGRESS],
                      "tagalong group %d used in both ingress and "
                      "egress deparser",
                      i);
            }
        }
    }

    for (auto &intrin : intrinsics) intrin.type->setregs(regs, *this, intrin);

    if (!regs.header.hir.ingr.ingress_port.sel.modified())
        regs.header.hir.ingr.ingress_port.sel = 1;

    for (auto &digest : digests) digest.type->setregs(regs, *this, digest);

    // The csum_cfg_entry registers are NOT reset by hardware and must be
    // explicitly configured.  We remove the disable_if_reset_value() calls on
    // these register tree for now, but ideally they should have a flag to indicate no
    // reset value is present and the register tree should prune only those regs
    // if (options.condense_json) {
    //     regs.input.disable_if_reset_value();
    //     regs.header.disable_if_reset_value(); }
    if (error_count == 0 && options.gen_json) {
        regs.input.emit_json(*open_output("regs.all.deparser.input_phase.cfg.json"));
        regs.header.emit_json(*open_output("regs.all.deparser.header_phase.cfg.json"));
    }
    TopLevel::regs<Target::Tofino>()->reg_pipe.deparser.hdr.set("regs.all.deparser.header_phase",
                                                                &regs.header);
    TopLevel::regs<Target::Tofino>()->reg_pipe.deparser.inp.set("regs.all.deparser.input_phase",
                                                                &regs.input);
}

template <>
unsigned Deparser::FDEntry::Checksum::encode<Target::Tofino>() {
    return CHECKSUM_ENGINE_PHVID_TOFINO_LOW + (gress * CHECKSUM_ENGINE_PHVID_TOFINO_PER_GRESS) +
           unit;
}

template <>
unsigned Deparser::FDEntry::Constant::encode<Target::Tofino>() {
    error(lineno, "Tofino deparser does not support constant entries");
    return -1;
}

template <>
void Deparser::gen_learn_quanta(Target::Tofino::parser_regs &regs, json::vector &learn_quanta) {}

template <>
void Deparser::process(Target::Tofino *) {
    // Chip-specific code for process method
    // None for Tofino
}
