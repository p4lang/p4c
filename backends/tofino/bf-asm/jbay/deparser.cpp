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

/* deparser template specializations for jbay -- #included directly in top-level deparser.cpp */

#define YES(X) X
#define NO(X)

#define JBAY_POV(GRESS, VAL, REG)                                                      \
    if (VAL.pov.size() == 1)                                                           \
        REG.pov = deparser.pov[GRESS].at(&VAL.pov.front()->reg) + VAL.pov.front()->lo; \
    else                                                                               \
        error(VAL.val.lineno, "POV bit required for Tofino2");

#define JBAY_SIMPLE_INTRINSIC(GRESS, VAL, REG, IFSHIFT) \
    REG.phv = VAL.val->reg.deparser_id();               \
    JBAY_POV(GRESS, VAL, REG)                           \
    IFSHIFT(REG.shft = intrin.vals[0].val->lo;)

#define JBAY_ARRAY_INTRINSIC(GRESS, VAL, ARRAY, REG, POV, IFSHIFT) \
    for (auto &r : ARRAY) {                                        \
        r.REG.phv = VAL.val->reg.deparser_id();                    \
        IFSHIFT(r.REG.shft = intrin.vals[0].val->lo;)              \
    }                                                              \
    JBAY_POV(GRESS, VAL, POV)

#define EI_INTRINSIC(NAME, IFSHIFT)                                                                \
    DEPARSER_INTRINSIC(JBay, EGRESS, NAME, 1) {                                                    \
        JBAY_SIMPLE_INTRINSIC(EGRESS, intrin.vals[0], regs.dprsrreg.inp.ipp.egr.m_##NAME, IFSHIFT) \
    }
#define HO_E_INTRINSIC(NAME, IFSHIFT)                                                       \
    DEPARSER_INTRINSIC(JBay, EGRESS, NAME, 1) {                                             \
        JBAY_ARRAY_INTRINSIC(EGRESS, intrin.vals[0], regs.dprsrreg.ho_e, her.meta.m_##NAME, \
                             regs.dprsrreg.inp.icr.egr_meta_pov.m_##NAME, IFSHIFT)          \
    }
#define II_INTRINSIC(NAME, IFSHIFT)                                                         \
    DEPARSER_INTRINSIC(JBay, INGRESS, NAME, 1) {                                            \
        JBAY_SIMPLE_INTRINSIC(INGRESS, intrin.vals[0], regs.dprsrreg.inp.ipp.ingr.m_##NAME, \
                              IFSHIFT)                                                      \
    }
#define II_INTRINSIC_RENAME(NAME, REGNAME, IFSHIFT)                                            \
    DEPARSER_INTRINSIC(JBay, INGRESS, NAME, 1) {                                               \
        JBAY_SIMPLE_INTRINSIC(INGRESS, intrin.vals[0], regs.dprsrreg.inp.ipp.ingr.m_##REGNAME, \
                              IFSHIFT)                                                         \
    }
#define HO_I_INTRINSIC(NAME, IFSHIFT)                                                        \
    DEPARSER_INTRINSIC(JBay, INGRESS, NAME, 1) {                                             \
        JBAY_ARRAY_INTRINSIC(INGRESS, intrin.vals[0], regs.dprsrreg.ho_i, hir.meta.m_##NAME, \
                             regs.dprsrreg.inp.icr.ingr_meta_pov.m_##NAME, IFSHIFT)          \
    }
#define HO_I_INTRINSIC_RENAME(NAME, REGNAME, IFSHIFT)                                           \
    DEPARSER_INTRINSIC(JBay, INGRESS, NAME, 1) {                                                \
        JBAY_ARRAY_INTRINSIC(INGRESS, intrin.vals[0], regs.dprsrreg.ho_i, hir.meta.m_##REGNAME, \
                             regs.dprsrreg.inp.icr.ingr_meta_pov.m_##REGNAME, IFSHIFT)          \
    }

EI_INTRINSIC(drop_ctl, YES)
EI_INTRINSIC(egress_unicast_port, NO)
HO_E_INTRINSIC(afc, YES)
HO_E_INTRINSIC(capture_tx_ts, YES)
HO_E_INTRINSIC(force_tx_err, YES)
HO_E_INTRINSIC(tx_pkt_has_offsets, YES)
HO_E_INTRINSIC(mirr_c2c_ctrl, YES)
HO_E_INTRINSIC(mirr_coal_smpl_len, YES)
HO_E_INTRINSIC(mirr_dond_ctrl, YES)
HO_E_INTRINSIC(mirr_epipe_port, YES)
HO_E_INTRINSIC(mirr_hash, YES)
HO_E_INTRINSIC(mirr_icos, YES)
HO_E_INTRINSIC(mirr_io_sel, YES)
HO_E_INTRINSIC(mirr_mc_ctrl, YES)
HO_E_INTRINSIC(mirr_qid, YES)
HO_E_INTRINSIC(mtu_trunc_err_f, YES)
HO_E_INTRINSIC(mtu_trunc_len, YES)

II_INTRINSIC(copy_to_cpu, YES)
II_INTRINSIC(drop_ctl, YES)
II_INTRINSIC(egress_unicast_port, NO)
II_INTRINSIC_RENAME(egress_multicast_group_0, mgid1, NO)
II_INTRINSIC_RENAME(egress_multicast_group_1, mgid2, NO)
II_INTRINSIC(pgen, YES)
II_INTRINSIC(pgen_len, YES)
II_INTRINSIC(pgen_addr, YES)
HO_I_INTRINSIC(afc, YES)
HO_I_INTRINSIC(bypss_egr, YES)
HO_I_INTRINSIC(copy_to_cpu_cos, YES)
HO_I_INTRINSIC(ct_disable, YES)
HO_I_INTRINSIC(ct_mcast, YES)
HO_I_INTRINSIC(deflect_on_drop, YES)
HO_I_INTRINSIC(icos, YES)
HO_I_INTRINSIC(mirr_c2c_ctrl, YES)
HO_I_INTRINSIC(mirr_coal_smpl_len, YES)
HO_I_INTRINSIC(mirr_dond_ctrl, YES)
HO_I_INTRINSIC(mirr_epipe_port, YES)
HO_I_INTRINSIC(mirr_hash, YES)
HO_I_INTRINSIC(mirr_icos, YES)
HO_I_INTRINSIC(mirr_io_sel, YES)
HO_I_INTRINSIC(mirr_mc_ctrl, YES)
HO_I_INTRINSIC(mirr_qid, YES)
HO_I_INTRINSIC(mtu_trunc_err_f, YES)
HO_I_INTRINSIC(mtu_trunc_len, YES)
HO_I_INTRINSIC(qid, YES)
HO_I_INTRINSIC(rid, YES)
HO_I_INTRINSIC_RENAME(meter_color, pkt_color, YES)
HO_I_INTRINSIC_RENAME(xid, xid_l1, YES)
HO_I_INTRINSIC_RENAME(yid, xid_l2, YES)
HO_I_INTRINSIC_RENAME(hash_lag_ecmp_mcast_0, hash1, YES)
HO_I_INTRINSIC_RENAME(hash_lag_ecmp_mcast_1, hash2, YES)

#undef EI_INTRINSIC
#undef HO_E_INTRINSIC
#undef II_INTRINSIC
#undef II_INTRINSIC_RENAME
#undef HO_I_INTRINSIC
#undef HO_I_INTRINSIC_RENAME

/** Macros to build Digest::Type objects for JBay --
 * JBAY_SIMPLE_DIGEST: basic digest that appears one place in the config
 * JBAY_ARRAY_DIGEST: config is replicated across Header+Output slices
 * GRESS: INGRESS or EGRESS
 * NAME: keyword use for this digest in the assembler
 * ARRAY: Header+Ouput slice array (ho_i or ho_e, matching ingress or egress)
 * TBL: config register containing the table config
 * SEL: config register with the selection config
 * IFID: YES or NO -- if this config needs to program id_phv
 * CNT: how many patterns can be specified in the array
 * REVERSE: YES or NO -- if the entries in the table are reverse (0 is last byte of header)
 * IFIDX: YES or NO -- if CNT > 1 (if we index by id)
 */

#define JBAY_SIMPLE_DIGEST(GRESS, NAME, TBL, SEL, IFID, CNT, REVERSE, IFIDX) \
    JBAY_COMMON_DIGEST(GRESS, NAME, TBL, SEL, IFID, CNT, REVERSE, IFIDX)     \
    JBAY_DIGEST_TABLE(GRESS, NAME, TBL, IFID, YES, CNT, REVERSE, IFIDX)      \
    }
#define JBAY_ARRAY_DIGEST(GRESS, NAME, ARRAY, TBL, SEL, IFID, CNT, REVERSE, IFIDX) \
    JBAY_COMMON_DIGEST(GRESS, NAME, TBL, SEL, IFID, CNT, REVERSE, IFIDX)           \
    for (auto &r : ARRAY) {                                                        \
        JBAY_DIGEST_TABLE(GRESS, NAME, r.TBL, IFID, NO, CNT, REVERSE, IFIDX)       \
    }                                                                              \
    }

#define JBAY_COMMON_DIGEST(GRESS, NAME, TBL, SEL, IFID, CNT, REVERSE, IFIDX) \
    DEPARSER_DIGEST(JBay, GRESS, NAME, CNT, can_shift = true;) {             \
        SEL.phv = data.select.val->reg.deparser_id();                        \
        JBAY_POV(GRESS, data.select, SEL)                                    \
        SEL.shft = data.shift + data.select->lo;                             \
        SEL.disable_ = 0;

#define JBAY_DIGEST_TABLE(GRESS, NAME, REG, IFID, IFVALID, CNT, REVERSE, IFIDX)                 \
    for (auto &set : data.layout) {                                                             \
        int id = set.first >> data.shift;                                                       \
        int idx = 0;                                                                            \
        int maxidx = REG IFIDX([id]).phvs.size() - 1;                                           \
        bool first = true;                                                                      \
        int last = -1;                                                                          \
        for (auto &reg : set.second) {                                                          \
            if (first) {                                                                        \
                first = false;                                                                  \
                IFID(REG IFIDX([id]).id_phv = reg->reg.deparser_id(); continue;)                \
            }                                                                                   \
            /* The same 16b/32b container cannot appear consecutively, but 8b can. */           \
            if (last == reg->reg.deparser_id() && reg->reg.size != 8) {                         \
                error(data.lineno, "%s: %db container %s seen in consecutive locations", #NAME, \
                      reg->reg.size, reg->reg.name);                                            \
                continue;                                                                       \
            }                                                                                   \
            for (int i = reg->reg.size / 8; i > 0; i--) {                                       \
                if (idx > maxidx) {                                                             \
                    error(data.lineno, "%s digest limited to %d bytes", #NAME, maxidx + 1);     \
                    break;                                                                      \
                }                                                                               \
                REG IFIDX([id]).phvs[REVERSE(maxidx -) idx++] = reg->reg.deparser_id();         \
            }                                                                                   \
            last = reg->reg.deparser_id();                                                      \
        }                                                                                       \
        IFVALID(REG IFIDX([id]).valid = 1;)                                                     \
        REG IFIDX([id]).len = idx;                                                              \
    }

JBAY_SIMPLE_DIGEST(INGRESS, learning, regs.dprsrreg.inp.ipp.ingr.learn_tbl,
                   regs.dprsrreg.inp.ipp.ingr.m_learn_sel, NO, 8, YES, YES)
JBAY_ARRAY_DIGEST(INGRESS, mirror, regs.dprsrreg.ho_i, him.mirr_hdr_tbl.entry,
                  regs.dprsrreg.inp.ipp.ingr.m_mirr_sel, YES, 16, NO, YES)
JBAY_ARRAY_DIGEST(EGRESS, mirror, regs.dprsrreg.ho_e, hem.mirr_hdr_tbl.entry,
                  regs.dprsrreg.inp.ipp.egr.m_mirr_sel, YES, 16, NO, YES)
JBAY_SIMPLE_DIGEST(INGRESS, resubmit, regs.dprsrreg.inp.ipp.ingr.resub_tbl,
                   regs.dprsrreg.inp.ipp.ingr.m_resub_sel, NO, 8, NO, YES)
JBAY_SIMPLE_DIGEST(INGRESS, pktgen, regs.dprsrreg.inp.ipp.ingr.pgen_tbl,
                   regs.dprsrreg.inp.ipp.ingr.m_pgen, NO, 1, NO, NO)

// all the jbay deparser subtrees with a dis or disable_ bit
// FIXME -- should be a way of doing this with a smart template or other metaprogramming.
#define JBAY_DISABLE_REGBITS(M)                                         \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_afc, dis)                     \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_capture_tx_ts, dis)           \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_force_tx_err, dis)            \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_c2c_ctrl, dis)           \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_coal_smpl_len, dis)      \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_dond_ctrl, dis)          \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_epipe_port, dis)         \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_hash, dis)               \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_icos, dis)               \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_io_sel, dis)             \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_mc_ctrl, dis)            \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mirr_qid, dis)                \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mtu_trunc_err_f, dis)         \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_mtu_trunc_len, dis)           \
    M(YES, regs.dprsrreg.ho_e, her.meta.m_tx_pkt_has_offsets, dis)      \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_afc, dis)                     \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_bypss_egr, dis)               \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_copy_to_cpu_cos, dis)         \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_ct_disable, dis)              \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_ct_mcast, dis)                \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_deflect_on_drop, dis)         \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_hash1, dis)                   \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_hash2, dis)                   \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_icos, dis)                    \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_c2c_ctrl, dis)           \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_coal_smpl_len, dis)      \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_dond_ctrl, dis)          \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_epipe_port, dis)         \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_hash, dis)               \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_icos, dis)               \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_io_sel, dis)             \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_mc_ctrl, dis)            \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mirr_qid, dis)                \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mtu_trunc_err_f, dis)         \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_mtu_trunc_len, dis)           \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_pkt_color, dis)               \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_qid, dis)                     \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_rid, dis)                     \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_xid_l1, dis)                  \
    M(YES, regs.dprsrreg.ho_i, hir.meta.m_xid_l2, dis)                  \
    M(NO, , regs.dprsrreg.inp.ipp.egr.m_drop_ctl, disable_)             \
    M(NO, , regs.dprsrreg.inp.ipp.egr.m_egress_unicast_port, disable_)  \
    M(NO, , regs.dprsrreg.inp.ipp.egr.m_mirr_sel, disable_)             \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_copy_to_cpu, disable_)         \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_drop_ctl, disable_)            \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_egress_unicast_port, disable_) \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_learn_sel, disable_)           \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_mgid1, disable_)               \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_mgid2, disable_)               \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_mirr_sel, disable_)            \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_pgen, disable_)                \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_pgen_addr, disable_)           \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_pgen_len, disable_)            \
    M(NO, , regs.dprsrreg.inp.ipp.ingr.m_resub_sel, disable_)

// Compiler workaround for TOF2LAB-44, skip certain chunk indices
void tof2lab44_workaround(int lineno, unsigned &chunk_index) {
    if (options.tof2lab44_workaround) {
        static std::set<unsigned> skipped_chunks = {24, 32, 40, 48,  56,  64, 72,
                                                    80, 88, 96, 104, 112, 120};
        while (skipped_chunks.count(chunk_index)) chunk_index++;
    }
}

// INVARIANT: check_chunk is idempotent.
bool check_chunk(int lineno, unsigned &chunk) {
    tof2lab44_workaround(lineno, chunk);

    const unsigned TOTAL_CHUNKS = Target::JBay::DEPARSER_TOTAL_CHUNKS;
    static bool suppress_repeated = false;
    if (chunk >= TOTAL_CHUNKS) {
        if (!suppress_repeated)
            error(lineno, "Ran out of chunks in field dictionary (%d)", TOTAL_CHUNKS);
        suppress_repeated = true;
        return false;
    }
    return true;
}

/// A callback to write a PHV, constant, or checksum chunk to the field dictionary.
using WriteChunk = std::function<void(
    unsigned & /* chunk_index */, const Phv::Slice & /* prev_pov */, int /* prev_entry_encoded */,
    int /* entry_lineno */, const Phv::Ref & /* entry_pov */,
    Deparser::FDEntry::Base * /* entry_what */, unsigned /* byte */, unsigned /* size */)>;

/// A callback to finish writing a PHV, constant, or checksum chunk to the field dictionary.
using FinishChunk =
    std::function<void(unsigned /* chunk_index */, unsigned /* dictionary entry number */,
                       const Phv::Slice & /* pov_bit */, unsigned /* byte */)>;

/// A callback for writing a CLOT to the field dictionary. This increments the chunk index if the
/// CLOT spans multiple chunks.
using WriteClot = std::function<void(
    unsigned & /* chunk_index */, unsigned & /* dictionary entry number */, int /* segment_tag */,
    int /* clot_tag */, const Phv::Ref & /* pov_bit */, Deparser::FDEntry::Clot * /* clot */)>;

/// Implements common control functionality for outputting field dictionaries and field dictionary
/// slices.
template <class POV, class DICT>
void output_jbay_field_dictionary_helper(int lineno, POV &pov, DICT &dict, WriteChunk write_chunk,
                                         FinishChunk finish_chunk, WriteClot write_clot) {
    const unsigned CHUNK_SIZE = Target::JBay::DEPARSER_CHUNK_SIZE;
    const unsigned CHUNK_GROUPS = Target::JBay::DEPARSER_CHUNK_GROUPS;
    const unsigned CHUNKS_PER_GROUP = Target::JBay::DEPARSER_CHUNKS_PER_GROUP;
    const unsigned CLOTS_PER_GROUP = Target::JBay::DEPARSER_CLOTS_PER_GROUP;
    unsigned ch = 0, entry_n = 0, byte = 0, group = 0, clots_in_group = 0;
    Phv::Slice prev_pov;
    int prev = -1;

    // INVARIANT: check_chunk should be called immediately before doing anything with a chunk.
    // Because check_chunk is idempotent, it is fine to call it on a chunk that has previously been
    // checked.

    for (auto &ent : dict) {
        auto *clot = dynamic_cast<Deparser::FDEntry::Clot *>(ent.what.get());
        // FIXME -- why does the following give an error from gcc?
        // auto *clot = ent.what->to<Deparser::FDEntry::Clot>();
        unsigned size = ent.what->size();

        // Finish the current chunk if needed.
        if (byte &&
            (clot || byte + size > CHUNK_SIZE || (prev_pov && *ent.pov.front() != prev_pov))) {
            finish_chunk(ch++, entry_n++, prev_pov, byte);
            byte = 0;
        }
        if (ch / CHUNKS_PER_GROUP != group) {
            // into a new group
            group = ch / CHUNKS_PER_GROUP;
            clots_in_group = 0;
        }
        if (clot) {
            // Start a new group if needed. Each group has a maximum number of CLOTs that can be
            // deparsed, and CLOTs cannot span multiple groups.
            bool out_of_clots_in_group = clots_in_group >= CLOTS_PER_GROUP;
            auto chunks_in_clot = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;
            bool out_of_chunks_in_group = ch % CHUNKS_PER_GROUP + chunks_in_clot > CHUNKS_PER_GROUP;
            if (out_of_clots_in_group || out_of_chunks_in_group) {
                // go on to the next group
                ch = (ch | (CHUNKS_PER_GROUP - 1)) + 1;
                group = ch / CHUNKS_PER_GROUP;
                clots_in_group = 0;
            }

            // Write the CLOT to the next segment in the current group.
            if (chunks_in_clot == CHUNKS_PER_GROUP && (ch % CHUNKS_PER_GROUP))
                error(clot->lineno, "--tof2lab44-workaround incompatible with clot >56 bytes");
            int clot_tag = Parser::clot_tag(clot->gress, clot->tag);
            int seg_tag = clots_in_group++;
            write_clot(ch, entry_n, seg_tag, clot_tag, ent.pov.front(), clot);

            prev = -1;
        } else {
            // Phv, Constant, or Checksum
            write_chunk(ch, prev_pov, prev, ent.lineno, ent.pov.front(), ent.what.get(), byte,
                        size);
            byte += size;
            prev = ent.what->encode();
        }
        prev_pov = *ent.pov.front();
    }

    if (byte > 0) {
        finish_chunk(ch, entry_n, prev_pov, byte);
    }
}

template <class REGS, class POV_FMT, class POV, class DICT>
void output_jbay_field_dictionary(int lineno, REGS &regs, POV_FMT &pov_layout, POV &pov,
                                  DICT &dict) {
    // Initialize pov_layout.
    unsigned byte = 0;
    for (auto &r : pov) {
        for (int bits = 0; bits < r.first->size; bits += 8) {
            if (byte > pov_layout.size()) error(lineno, "Ran out of space in POV in deparser");
            pov_layout[byte++] = r.first->deparser_id();
        }
    }
    while (byte < pov_layout.size()) pov_layout[byte++] = 0xff;
    LOG5("jbay field dictionary:");

    // Declare some callback functions, and then delegate to helper.
    auto write_chunk = [](unsigned ch, const Phv::Slice &prev_pov, int prev, int ent_lineno,
                          const Phv::Ref &ent_pov, Deparser::FDEntry::Base *ent_what, unsigned byte,
                          unsigned size) {
        // Just do an error check here. Defer actual writing to finish_chunk.
        LOG5("  chunk " << ch << ": " << *ent_what << " (pov " << ent_pov << ")");
        if (dynamic_cast<Deparser::FDEntry::Phv *>(ent_what) && prev_pov == *ent_pov &&
            int(ent_what->encode()) == prev && (size & 6))
            error(ent_lineno, "16 and 32-bit container cannot be repeatedly deparsed");
    };

    auto finish_chunk = [&](unsigned ch, unsigned entry_n, const Phv::Slice &pov_bit,
                            unsigned byte) {
        if (check_chunk(lineno, ch)) {
            regs.chunk_info[ch].chunk_vld = 1;
            regs.chunk_info[ch].pov = pov.at(&pov_bit.reg) + pov_bit.lo;
            regs.chunk_info[ch].seg_vld = 0;
            regs.chunk_info[ch].seg_slice = byte & 7;
            regs.chunk_info[ch].seg_sel = byte >> 3;
        }
    };

    auto write_clot = [&](unsigned &ch, unsigned &entry_n, int seg_tag, int clot_tag,
                          const Phv::Ref &pov_bit, Deparser::FDEntry::Clot *clot) {
        const unsigned CHUNKS_PER_GROUP = Target::JBay::DEPARSER_CHUNKS_PER_GROUP;
        const int group = ch / CHUNKS_PER_GROUP;
        if (group < regs.fd_tags.size()) regs.fd_tags[group].segment_tag[seg_tag] = clot_tag;
        LOG5("  chunk " << ch << ": " << *clot << " (pov " << pov_bit << ")");
        for (int i = 0; i < clot->length; i += 8, ++ch) {
            // CLOTs cannot span multiple groups.
            BUG_CHECK(ch / CHUNKS_PER_GROUP == group || error_count > 0, "CLOT spanning groups");
            if (check_chunk(lineno, ch)) {
                regs.chunk_info[ch].chunk_vld = 1;
                regs.chunk_info[ch].pov = pov.at(&pov_bit->reg) + pov_bit->lo;
                regs.chunk_info[ch].seg_vld = 1;
                regs.chunk_info[ch].seg_sel = seg_tag;
                regs.chunk_info[ch].seg_slice = i / 8U;
            }
        }
    };

    output_jbay_field_dictionary_helper(lineno, pov, dict, write_chunk, finish_chunk, write_clot);
}

template <class CHUNKS, class CLOTS, class POV, class DICT>
void output_jbay_field_dictionary_slice(int lineno, CHUNKS &chunk, CLOTS &clots, POV &pov,
                                        DICT &dict, json::vector &fd_gress,
                                        json::vector &fd_entries, gress_t gress) {
    json::map fd;
    json::map fd_entry;
    json::vector chunk_bytes;
    json::vector fd_entry_chunk_bytes;

    auto write_chunk = [&](unsigned ch, const Phv::Slice &prev_pov, int prev, int ent_lineno,
                           const Phv::Ref &ent_pov, Deparser::FDEntry::Base *ent_what,
                           unsigned byte, unsigned size) {
        while (size--) {
            json::map chunk_byte;
            json::map fd_entry_chunk_byte;
            json::map fd_entry_chunk;
            chunk_byte["Byte"] = byte;
            fd_entry_chunk_byte["chunk_number"] = byte;
            if (ent_what->encode() < CONSTANTS_PHVID_JBAY_LOW) {
                auto *phv = dynamic_cast<Deparser::FDEntry::Phv *>(ent_what);
                auto phv_reg = phv->reg();
                write_field_name_in_json(phv_reg, &ent_pov->reg, ent_pov->lo, chunk_byte,
                                         fd_entry_chunk, 19, gress);
            } else {
                write_csum_const_in_json(ent_what->encode(), chunk_byte, fd_entry_chunk, gress);
            }
            fd_entry_chunk_byte["chunk"] = std::move(fd_entry_chunk);
            chunk_bytes.push_back(std::move(chunk_byte));
            fd_entry_chunk_bytes.push_back(std::move(fd_entry_chunk_byte));
            if (check_chunk(lineno, ch)) {
                chunk[ch].is_phv |= 1 << byte;
                chunk[ch].byte_off.phv_offset[byte++] = ent_what->encode();
            }
        }
    };

    auto finish_chunk = [&](unsigned ch, unsigned entry_n, const Phv::Slice &pov_bit,
                            unsigned byte) {
        fd["Field Dictionary Number"] = entry_n;
        fd["Field Dictionary Chunk"] = ch;
        fd_entry["entry"] = entry_n;
        // fd_entry["fde_chunk"] = ch;  -- requires compiler_interfaces change
        Deparser::write_pov_in_json(fd, fd_entry, &pov_bit.reg, pov.at(&pov_bit.reg) + pov_bit.lo,
                                    pov_bit.lo);
        if (check_chunk(lineno, ch)) {
            chunk[ch].cfg.seg_vld = 0;  // no CLOTs yet
            chunk[ch].cfg.seg_slice = byte & 7;
            chunk[ch].cfg.seg_sel = byte >> 3;
        }

        fd["Content"] = std::move(chunk_bytes);
        fd_entry["chunks"] = std::move(fd_entry_chunk_bytes);
        fd_entries.push_back(std::move(fd_entry));
        fd_gress.push_back(std::move(fd));
    };

    auto write_clot = [&](unsigned &ch, unsigned &entry_n, int seg_tag, int clot_tag,
                          const Phv::Ref &pov_bit, Deparser::FDEntry::Clot *clot) {
        const unsigned CHUNKS_PER_GROUP = Target::JBay::DEPARSER_CHUNKS_PER_GROUP;
        const int group = ch / CHUNKS_PER_GROUP;
        if (group < clots.size()) clots[group].segment_tag[seg_tag] = clot_tag;
        auto phv_repl = clot->phv_replace.begin();
        auto csum_repl = clot->csum_replace.begin();
        for (int i = 0; i < clot->length; i += 8, ++ch, ++entry_n) {
            // CLOTs cannot span multiple groups.
            BUG_CHECK(ch / CHUNKS_PER_GROUP == group || error_count > 0, "CLOT spanning groups");

            fd["Field Dictionary Number"] = entry_n;
            fd["Field Dictionary Chunk"] = ch;
            fd_entry["entry"] = entry_n;
            // fd_entry["fde_chunk"] = ch;  -- requires compiler_interfaces change
            Deparser::write_pov_in_json(fd, fd_entry, &pov_bit->reg,
                                        pov.at(&pov_bit->reg) + pov_bit->lo, pov_bit->lo);

            if (check_chunk(lineno, ch)) {
                chunk[ch].cfg.seg_vld = 1;
                chunk[ch].cfg.seg_sel = seg_tag;
                chunk[ch].cfg.seg_slice = i / 8U;
            }

            for (int j = 0; j < 8 && i + j < clot->length; ++j) {
                json::map chunk_byte;
                json::map fd_entry_chunk_byte;
                json::map fd_entry_chunk;
                chunk_byte["Byte"] = j;
                fd_entry_chunk_byte["chunk_number"] = j;
                if (phv_repl != clot->phv_replace.end() && int(phv_repl->first) <= i + j) {
                    // This is PHV replaced, PHV is used
                    chunk[ch].is_phv |= 1 << j;
                    chunk[ch].byte_off.phv_offset[j] = phv_repl->second->reg.deparser_id();
                    auto phv_reg = &phv_repl->second->reg;
                    write_field_name_in_json(phv_reg, &pov_bit->reg, pov_bit->lo, chunk_byte,
                                             fd_entry_chunk, 19, gress);
                    if (int(phv_repl->first + phv_repl->second->size() / 8U) <= i + j + 1)
                        ++phv_repl;
                } else if (csum_repl != clot->csum_replace.end() &&
                           int(csum_repl->first) <= i + j) {
                    if (check_chunk(lineno, ch)) {
                        chunk[ch].is_phv |= 1 << j;
                        chunk[ch].byte_off.phv_offset[j] = csum_repl->second.encode();
                    }
                    write_csum_const_in_json(csum_repl->second.encode(), chunk_byte, fd_entry_chunk,
                                             gress);
                    if (int(csum_repl->first + 2) <= i + j + 1) ++csum_repl;
                } else {
                    if (check_chunk(lineno, ch)) chunk[ch].byte_off.phv_offset[j] = i + j;
                    chunk_byte["CLOT"] = clot_tag;
                    chunk_byte["CLOT_OFFSET"] = i + j;
                    fd_entry_chunk["clot_tag"] = clot_tag;
                    // fd_entry_chunk["clot_offset"] = i + j;   requires compiler_interfaces change
                }
                fd_entry_chunk_byte["chunk"] = std::move(fd_entry_chunk);
                chunk_bytes.push_back(std::move(chunk_byte));
                fd_entry_chunk_bytes.push_back(std::move(fd_entry_chunk_byte));
            }
            fd["Content"] = std::move(chunk_bytes);
            fd_entry["chunks"] = std::move(fd_entry_chunk_bytes);
            fd_entries.push_back(std::move(fd_entry));
            fd_gress.push_back(std::move(fd));
        }
    };

    output_jbay_field_dictionary_helper(lineno, pov, dict, write_chunk, finish_chunk, write_clot);
}

static void check_jbay_ownership(bitvec phv_use[2]) {
    unsigned mask = 0;
    int group = -1;
    for (auto i : phv_use[INGRESS]) {
        if ((i | mask) == (group | mask)) continue;
        switch (Phv::reg(i)->size) {
            case 8:
            case 16:
                mask = 3;
                break;
            case 32:
                mask = 1;
                break;
            default:
                BUG("unexpected size %d", Phv::reg(i)->size);
        }
        group = i & ~mask;
        if (phv_use[EGRESS].getrange(group, mask + 1)) {
            error(0, "%s..%s used by both ingress and egress deparser", Phv::reg(group)->name,
                  Phv::reg(group | mask)->name);
        }
    }
}

static void setup_jbay_ownership(bitvec phv_use, ubits_base &phv8, ubits_base &phv16,
                                 ubits_base &phv32) {
    std::set<unsigned> phv8_grps, phv16_grps, phv32_grps;

    for (auto i : phv_use) {
        auto *reg = Phv::reg(i);
        switch (reg->size) {
            case 8:
                phv8_grps.insert(1U << ((reg->deparser_id() - 64) / 4U));
                break;
            case 16:
                phv16_grps.insert(1U << ((reg->deparser_id() - 128) / 4U));
                break;
            case 32:
                phv32_grps.insert(1U << (reg->deparser_id() / 2U));
                break;
            default:
                BUG("unexpected size %d", reg->size);
        }
    }

    for (auto v : phv8_grps) phv8 |= v;
    for (auto v : phv16_grps) phv16 |= v;
    for (auto v : phv32_grps) phv32 |= v;
}

static short jbay_phv2cksum[224][2] = {
    // Entries 0-127 are for 32 bit PHV
    // Each 32 bit PHV uses two 16b adders
    // The even addresses are for [31:16], the odd addresses are for [15:0]
    // Note: The current CSR description of these entries for 32 bit containers is incorrect.
    // 128-191 are for 8 bit PHV
    // 192-287 are for 16 bit PHV
    {1, 0},     {3, 2},     {5, 4},     {7, 6},     {9, 8},     {11, 10},   {13, 12},   {15, 14},
    {17, 16},   {19, 18},   {21, 20},   {23, 22},   {25, 24},   {27, 26},   {29, 28},   {31, 30},
    {33, 32},   {35, 34},   {37, 36},   {39, 38},   {41, 40},   {43, 42},   {45, 44},   {47, 46},
    {49, 48},   {51, 50},   {53, 52},   {55, 54},   {57, 56},   {59, 58},   {61, 60},   {63, 62},
    {65, 64},   {67, 66},   {69, 68},   {71, 70},   {73, 72},   {75, 74},   {77, 76},   {79, 78},
    {81, 80},   {83, 82},   {85, 84},   {87, 86},   {89, 88},   {91, 90},   {93, 92},   {95, 94},
    {97, 96},   {99, 98},   {101, 100}, {103, 102}, {105, 104}, {107, 106}, {109, 108}, {111, 110},
    {113, 112}, {115, 114}, {117, 116}, {119, 118}, {121, 120}, {123, 122}, {125, 124}, {127, 126},
    {128, -1},  {129, -1},  {130, -1},  {131, -1},  {132, -1},  {133, -1},  {134, -1},  {135, -1},
    {136, -1},  {137, -1},  {138, -1},  {139, -1},  {140, -1},  {141, -1},  {142, -1},  {143, -1},
    {144, -1},  {145, -1},  {146, -1},  {147, -1},  {148, -1},  {149, -1},  {150, -1},  {151, -1},
    {152, -1},  {153, -1},  {154, -1},  {155, -1},  {156, -1},  {157, -1},  {158, -1},  {159, -1},
    {160, -1},  {161, -1},  {162, -1},  {163, -1},  {164, -1},  {165, -1},  {166, -1},  {167, -1},
    {168, -1},  {169, -1},  {170, -1},  {171, -1},  {172, -1},  {173, -1},  {174, -1},  {175, -1},
    {176, -1},  {177, -1},  {178, -1},  {179, -1},  {180, -1},  {181, -1},  {182, -1},  {183, -1},
    {184, -1},  {185, -1},  {186, -1},  {187, -1},  {188, -1},  {189, -1},  {190, -1},  {191, -1},
    {192, -1},  {193, -1},  {194, -1},  {195, -1},  {196, -1},  {197, -1},  {198, -1},  {199, -1},
    {200, -1},  {201, -1},  {202, -1},  {203, -1},  {204, -1},  {205, -1},  {206, -1},  {207, -1},
    {208, -1},  {209, -1},  {210, -1},  {211, -1},  {212, -1},  {213, -1},  {214, -1},  {215, -1},
    {216, -1},  {217, -1},  {218, -1},  {219, -1},  {220, -1},  {221, -1},  {222, -1},  {223, -1},
    {224, -1},  {225, -1},  {226, -1},  {227, -1},  {228, -1},  {229, -1},  {230, -1},  {231, -1},
    {232, -1},  {233, -1},  {234, -1},  {235, -1},  {236, -1},  {237, -1},  {238, -1},  {239, -1},
    {240, -1},  {241, -1},  {242, -1},  {243, -1},  {244, -1},  {245, -1},  {246, -1},  {247, -1},
    {248, -1},  {249, -1},  {250, -1},  {251, -1},  {252, -1},  {253, -1},  {254, -1},  {255, -1},
    {256, -1},  {257, -1},  {258, -1},  {259, -1},  {260, -1},  {261, -1},  {262, -1},  {263, -1},
    {264, -1},  {265, -1},  {266, -1},  {267, -1},  {268, -1},  {269, -1},  {270, -1},  {271, -1},
    {272, -1},  {273, -1},  {274, -1},  {275, -1},  {276, -1},  {277, -1},  {278, -1},  {279, -1},
    {280, -1},  {281, -1},  {282, -1},  {283, -1},  {284, -1},  {285, -1},  {286, -1},  {287, -1},
};

template <class ENTRIES>
static void write_jbay_checksum_entry(ENTRIES &entry, unsigned mask, int swap, int pov, int id,
                                      const char *reg = nullptr) {
    write_checksum_entry(entry, mask, swap, id, reg);
    entry.pov = pov;
}

// Populates pov_map which maps the bit in the main POV array [127:0]
// to bit in the checksum pov array [32:0]
// The checksum pov array is 32 bits / 4 bytes - pov_cfg.byte_set[4].
// Each element of the pov_cfg.byte_sel array maps to the byte in the main POV array
template <class POV>
void jbay_csum_pov_config(Phv::Ref povRef, POV &pov_cfg,
                          ordered_map<const Phv::Register *, unsigned> &pov,
                          std::map<unsigned, unsigned> &pov_map, unsigned *prev_byte,
                          int csum_unit) {
    unsigned bit = pov.at(&povRef->reg) + povRef->lo;
    if (pov_map.count(bit)) return;
    for (unsigned i = 0; i < (*prev_byte); ++i) {
        if (pov_cfg.byte_sel[i] == bit / 8U) {
            pov_map[bit] = i * 8U + bit % 8U;
            break;
        }
    }
    if (pov_map.count(bit)) return;
    if (*prev_byte >= (int)pov_cfg.byte_sel.size()) {
        error(povRef.lineno, "Checksum unit %d exceeds %d bytes of POV", csum_unit,
              (int)pov_cfg.byte_sel.size());
        return;
    }
    pov_map[bit] = (*prev_byte) * 8U + bit % 8U;
    pov_cfg.byte_sel[(*prev_byte)++] = bit / 8U;
    return;
}

template <class POV>
void set_jbay_pov_cfg(POV &pov_cfg, std::map<unsigned, unsigned> &pov_map,
                      Deparser::FullChecksumUnit &full_csum,
                      ordered_map<const Phv::Register *, unsigned> &pov, int csum_unit,
                      unsigned *prev_byte) {
    for (auto &unit_entry : full_csum.entries) {
        for (auto val : unit_entry.second) {
            if (val.pov.size() != 1) {
                error(val.val.lineno, "one POV bit required for Tofino2");
                continue;
            }
            jbay_csum_pov_config(val.pov.front(), pov_cfg, pov, pov_map, prev_byte, csum_unit);
        }
    }
    for (auto &val : full_csum.clot_entries) {
        if (val.pov.size() != 1) {
            error(val.val.lineno, "one POV bit required for Tofino2");
            continue;
        }
        jbay_csum_pov_config(val.pov.front(), pov_cfg, pov, pov_map, prev_byte, csum_unit);
    }
    for (auto &checksum_pov : full_csum.pov) {
        jbay_csum_pov_config(checksum_pov.second, pov_cfg, pov, pov_map, prev_byte, csum_unit);
    }
    return;
}

template <class CSUM, class ENTRIES>
void write_jbay_full_checksum_config(
    CSUM &csum, ENTRIES &phv_entries, int unit, std::set<int> &visited,
    std::array<std::map<unsigned, unsigned>, MAX_DEPARSER_CHECKSUM_UNITS> &pov_map,
    Deparser::FullChecksumUnit &full_csum, ordered_map<const Phv::Register *, unsigned> &pov) {
    for (auto &unit_entry : full_csum.entries) {
        // Same partial checksum unit can be used in multiple full checksum unit.
        // No need to rewrite the checksum entries multiple times for the same unit
        if (visited.count(unit_entry.first)) continue;
        visited.insert(unit_entry.first);
        for (auto val : unit_entry.second) {
            if (val.pov.size() != 1) continue;
            int povbit =
                pov_map[unit_entry.first].at(pov.at(&val.pov.front()->reg) + val.pov.front()->lo);
            int mask = val.mask;
            int swap = val.swap;
            auto &remap = jbay_phv2cksum[val->reg.deparser_id()];
            write_jbay_checksum_entry(phv_entries[unit_entry.first].entry[remap[0]], mask & 3,
                                      swap & 1, povbit, unit_entry.first, val->reg.name);
            if (remap[1] >= 0)
                write_jbay_checksum_entry(phv_entries[unit_entry.first].entry[remap[1]], mask >> 2,
                                          swap >> 1, povbit, unit_entry.first, val->reg.name);
            else
                BUG_CHECK((mask >> 2 == 0) && (swap >> 1 == 0), "Invalid checksum");
        }
    }
    int tag_idx = 0;
    for (auto &val : full_csum.clot_entries) {
        if (val.pov.size() != 1) continue;
        int povbit = pov_map[unit].at(pov.at(&val.pov.front()->reg) + val.pov.front()->lo);
        if (tag_idx == 16) error(-1, "Ran out of clot entries in deparser checksum unit %d", unit);
        csum.clot_entry[tag_idx].pov = povbit;
        csum.clot_entry[tag_idx].vld = 1;
        csum.tags[tag_idx].tag = val.tag;
        tag_idx++;
    }
    for (auto &checksum_pov : full_csum.pov) {
        csum.phv_entry[checksum_pov.first].pov =
            pov_map[unit].at(pov.at(&checksum_pov.second->reg) + checksum_pov.second->lo);
        csum.phv_entry[checksum_pov.first].vld = 1;
    }
    csum.zeros_as_ones.en = full_csum.zeros_as_ones_en;

    // FIXME -- use/set csum.csum_constant?
}
// Engine 0: scratch[23:0]
// Engine 1: { scratch2[15:0], scratch[31:24] }
// Engine 2: { scratch[7:0] , scratch2[31:16] }
// Engine 3: scratch[31:8]
// So each engine gets a cfg_vector[23:0]
// There are 16 CLOT csums and 8 PHV csums that can be inverted:
// CLOT csum [15:0] are controlled by cfg_vector [15:0]
// PHV csums [7:0] are controlled by cfg_vector [23:16]

template <class SCRATCH1, class SCRATCH2, class SCRATCH3>
void write_jbay_full_checksum_invert_config(SCRATCH1 &scratch1, SCRATCH2 &scratch2,
                                            SCRATCH3 &scratch3, int unit,
                                            Deparser::FullChecksumUnit &full_csum) {
    ubits<32> value1;
    ubits<32> value2;
    ubits<32> value3;
    for (auto checksum_unit : full_csum.checksum_unit_invert) {
        if (unit == 0) {
            value1 |= (1 << (16 + checksum_unit));
        } else if (unit == 1) {
            value1 |= (1 << (8 + checksum_unit));
        } else if (unit == 2) {
            value3 |= (1 << checksum_unit);
        } else if (unit == 3) {
            value3 |= (1 << (24 + checksum_unit));
        }
    }
    for (auto clot_tag : full_csum.clot_tag_invert) {
        if (unit == 0) {
            value1 |= (1 << clot_tag);
        } else if (unit == 1) {
            if (clot_tag > 7) {
                value1 |= (1 << (clot_tag - 8));
            } else {
                value3 |= (1 << (16 + clot_tag));
            }
        } else if (unit == 2) {
            value2 |= (1 << (16 + clot_tag));
        } else if (unit == 3) {
            value3 |= (1 << (8 + clot_tag));
        }
    }
    if (value1 || value2 || value3) {
        scratch1.value |= value1;
        scratch2.value |= value2;
        scratch3.value |= value3;
    }
    return;
}

template <class CONS>
void write_jbay_constant_config(CONS &cons, const std::set<int> &vals) {
    unsigned idx = 0;
    for (auto v : vals) {
        cons[idx] = v;
        idx++;
    }
}

template <>
void Deparser::write_config(Target::JBay::deparser_regs &regs) {
    regs.dprsrreg.inp.icr.disable();          // disable this whole tree
    regs.dprsrreg.inp.icr.disabled_ = false;  // then enable just certain subtrees
    regs.dprsrreg.inp.icr.csum_engine.enable();
    regs.dprsrreg.inp.icr.egr.enable();
    regs.dprsrreg.inp.icr.egr_meta_pov.enable();
    regs.dprsrreg.inp.icr.ingr.enable();
    regs.dprsrreg.inp.icr.ingr_meta_pov.enable();
    regs.dprsrreg.inp.icr.scratch.enable();
    regs.dprsrreg.inp.icr.scratch2.enable();
    regs.dprsrreg.inp.ipp.scratch.enable();
    regs.dprsrreg.inp.iim.disable();
    regs.dprsrreg.inpslice.disable();
    for (auto &r : regs.dprsrreg.ho_i) r.out_ingr.disable();
    for (auto &r : regs.dprsrreg.ho_e) r.out_egr.disable();

    for (auto &r : regs.dprsrreg.ho_i)
        write_jbay_constant_config(r.hir.h.hdr_xbar_const.value, constants[INGRESS]);
    for (auto &r : regs.dprsrreg.ho_e)
        write_jbay_constant_config(r.her.h.hdr_xbar_const.value, constants[EGRESS]);
    std::set<int> visited_i;
    std::array<std::map<unsigned, unsigned>, MAX_DEPARSER_CHECKSUM_UNITS> pov_map_i;
    for (int csum_unit = 0; csum_unit < Target::JBay::DEPARSER_CHECKSUM_UNITS; csum_unit++) {
        unsigned prev_byte = 0;
        if (full_checksum_unit[INGRESS][csum_unit].clot_entries.empty() &&
            full_checksum_unit[INGRESS][csum_unit].entries.empty())
            continue;
        set_jbay_pov_cfg(regs.dprsrreg.inp.ipp.phv_csum_pov_cfg.csum_pov_cfg[csum_unit],
                         pov_map_i[csum_unit], full_checksum_unit[INGRESS][csum_unit], pov[INGRESS],
                         csum_unit, &prev_byte);
        if (error_count > 0) break;
    }
    for (int csum_unit = 0; csum_unit < Target::JBay::DEPARSER_CHECKSUM_UNITS && error_count == 0;
         csum_unit++) {
        if (full_checksum_unit[INGRESS][csum_unit].clot_entries.empty() &&
            full_checksum_unit[INGRESS][csum_unit].entries.empty())
            continue;
        regs.dprsrreg.inp.ipp.phv_csum_pov_cfg.thread.thread[csum_unit] = INGRESS;
        write_jbay_full_checksum_config(
            regs.dprsrreg.inp.icr.csum_engine[csum_unit], regs.dprsrreg.inp.ipp_m.i_csum.engine,
            csum_unit, visited_i, pov_map_i, full_checksum_unit[INGRESS][csum_unit], pov[INGRESS]);
        write_jbay_full_checksum_invert_config(
            regs.dprsrreg.inp.icr.scratch, regs.dprsrreg.inp.icr.scratch2,
            regs.dprsrreg.inp.ipp.scratch, csum_unit, full_checksum_unit[INGRESS][csum_unit]);
    }
    std::set<int> visited_e;
    std::array<std::map<unsigned, unsigned>, MAX_DEPARSER_CHECKSUM_UNITS> pov_map_e;
    for (int csum_unit = 0; csum_unit < Target::JBay::DEPARSER_CHECKSUM_UNITS; csum_unit++) {
        unsigned prev_byte = 0;
        if (full_checksum_unit[EGRESS][csum_unit].clot_entries.empty() &&
            full_checksum_unit[EGRESS][csum_unit].entries.empty())
            continue;
        set_jbay_pov_cfg(regs.dprsrreg.inp.ipp.phv_csum_pov_cfg.csum_pov_cfg[csum_unit],
                         pov_map_e[csum_unit], full_checksum_unit[EGRESS][csum_unit], pov[EGRESS],
                         csum_unit, &prev_byte);
        if (error_count > 0) break;
    }
    for (int csum_unit = 0; csum_unit < Target::JBay::DEPARSER_CHECKSUM_UNITS && error_count == 0;
         csum_unit++) {
        if (full_checksum_unit[EGRESS][csum_unit].clot_entries.empty() &&
            full_checksum_unit[EGRESS][csum_unit].entries.empty())
            continue;
        regs.dprsrreg.inp.ipp.phv_csum_pov_cfg.thread.thread[csum_unit] = EGRESS;
        write_jbay_full_checksum_config(
            regs.dprsrreg.inp.icr.csum_engine[csum_unit], regs.dprsrreg.inp.ipp_m.i_csum.engine,
            csum_unit, visited_e, pov_map_e, full_checksum_unit[EGRESS][csum_unit], pov[EGRESS]);
        write_jbay_full_checksum_invert_config(
            regs.dprsrreg.inp.icr.scratch, regs.dprsrreg.inp.icr.scratch2,
            regs.dprsrreg.inp.ipp.scratch, csum_unit, full_checksum_unit[EGRESS][csum_unit]);
    }

    output_jbay_field_dictionary(lineno[INGRESS], regs.dprsrreg.inp.icr.ingr,
                                 regs.dprsrreg.inp.ipp.main_i.pov.phvs, pov[INGRESS],
                                 dictionary[INGRESS]);
    json::map field_dictionary_alloc;
    json::vector fde_entries_i;
    json::vector fde_entries_e;
    json::vector fde_entries;
    json::vector fd_gress;
    for (auto &rslice : regs.dprsrreg.ho_i) {
        output_jbay_field_dictionary_slice(lineno[INGRESS], rslice.him.fd_compress.chunk,
                                           rslice.hir.h.compress_clot_sel, pov[INGRESS],
                                           dictionary[INGRESS], fd_gress, fde_entries, INGRESS);
        field_dictionary_alloc["ingress"] = std::move(fd_gress);
        fde_entries_i = std::move(fde_entries);
    }
    output_jbay_field_dictionary(lineno[EGRESS], regs.dprsrreg.inp.icr.egr,
                                 regs.dprsrreg.inp.ipp.main_e.pov.phvs, pov[EGRESS],
                                 dictionary[EGRESS]);
    for (auto &rslice : regs.dprsrreg.ho_e) {
        output_jbay_field_dictionary_slice(lineno[EGRESS], rslice.hem.fd_compress.chunk,
                                           rslice.her.h.compress_clot_sel, pov[EGRESS],
                                           dictionary[EGRESS], fd_gress, fde_entries, EGRESS);
        field_dictionary_alloc["egress"] = std::move(fd_gress);
        fde_entries_e = std::move(fde_entries);
    }
    if (Log::verbosity() > 0) {
        auto json_dump = open_output("logs/field_dictionary.log");
        *json_dump << &field_dictionary_alloc;
    }
    // Output deparser resources
    report_resources_deparser_json(fde_entries_i, fde_entries_e);

    if (Phv::use(INGRESS).intersects(Phv::use(EGRESS))) {
        if (!options.match_compiler) {
            error(lineno[INGRESS], "Registers used in both ingress and egress in pipeline: %s",
                  Phv::db_regset(Phv::use(INGRESS) & Phv::use(EGRESS)).c_str());
        } else {
            warning(lineno[INGRESS], "Registers used in both ingress and egress in pipeline: %s",
                    Phv::db_regset(Phv::use(INGRESS) & Phv::use(EGRESS)).c_str());
        }
        /* FIXME -- this only (sort-of) works because 'deparser' comes first in the alphabet,
         * FIXME -- so is the first section to have its 'output' method run.  Its a hack
         * FIXME -- anyways to attempt to correct broken asm that should be an error */
        Phv::unsetuse(INGRESS, phv_use[EGRESS]);
        Phv::unsetuse(EGRESS, phv_use[INGRESS]);
    }

    check_jbay_ownership(phv_use);
    regs.dprsrreg.inp.icr.i_phv8_grp.enable();
    regs.dprsrreg.inp.icr.i_phv16_grp.enable();
    regs.dprsrreg.inp.icr.i_phv32_grp.enable();
    //  regs.dprsrreg.inp.icr.scratch.enable();
    regs.dprsrreg.inp.icr.i_phv8_grp.val = 0;
    regs.dprsrreg.inp.icr.i_phv16_grp.val = 0;
    regs.dprsrreg.inp.icr.i_phv32_grp.val = 0;
    //  regs.dprsrreg.inp.icr.scratch.value = 0;
    setup_jbay_ownership(phv_use[INGRESS], regs.dprsrreg.inp.icr.i_phv8_grp.val,
                         regs.dprsrreg.inp.icr.i_phv16_grp.val,
                         regs.dprsrreg.inp.icr.i_phv32_grp.val);
    regs.dprsrreg.inp.icr.e_phv8_grp.enable();
    regs.dprsrreg.inp.icr.e_phv16_grp.enable();
    regs.dprsrreg.inp.icr.e_phv32_grp.enable();
    setup_jbay_ownership(phv_use[EGRESS], regs.dprsrreg.inp.icr.e_phv8_grp.val,
                         regs.dprsrreg.inp.icr.e_phv16_grp.val,
                         regs.dprsrreg.inp.icr.e_phv32_grp.val);

    for (auto &intrin : intrinsics) intrin.type->setregs(regs, *this, intrin);

    /* resubmit_mode specifies whether this pipe can perform a resubmit operation on
       a packet. i.e. tell the IPB to resubmit a packet to the MAU pipeline for a second
       time. If the compiler determines that no resubmit is possible, then it can set this
       bit, which should lower latency in some circumstances.
       0 = Resubmit is allowed.  1 = Resubmit is not allowed */
    bool resubmit = false;
    for (auto &digest : digests) {
        if (digest.type->name == "resubmit" ||
            digest.type->name == "resubmit_preserving_field_list") {
            resubmit = true;
            break;
        }
    }
    if (resubmit)
        regs.dprsrreg.inp.ipp.ingr.resubmit_mode.mode = 0;
    else
        regs.dprsrreg.inp.ipp.ingr.resubmit_mode.mode = 1;

    for (auto &digest : digests) digest.type->setregs(regs, *this, digest);

    /* Set learning digest mask for JBay */
    for (auto &digest : digests) {
        if (digest.type->name == "learning") {
            regs.dprsrreg.inp.icr.lrnmask.enable();
            for (auto &set : digest.layout) {
                int id = set.first;
                int len = regs.dprsrreg.inp.ipp.ingr.learn_tbl[id].len;
                if (len == 0) continue;  // Allow empty param list

                // Fix for TF2LAB-37s:
                // This fixes a hardware limitation where the container following
                // the last PHV used cannot be the same non 8 bit container as the last entry.
                // E.g. For len = 5, (active entries start at index 47)
                // Used   - PHV[47] ... PHV[43] = 0;
                // Unused - PHV[42] ... PHV[0] = 0; // Defaults to 0
                // This causes issues in hardware as container 0 is used.
                // We fix by setting the default as 64 an 8 - bit container. It can be any
                // other 8 bit container value.
                // The hardware does not cause any issues for 8 bit conatiners.
                for (int i = 47 - len; i >= 0; i--)
                    regs.dprsrreg.inp.ipp.ingr.learn_tbl[id].phvs[i] = 64;
                // Fix for TF2LAB-37 end

                // Create a bitvec of all phv masks stacked up next to each
                // other in big-endian. 'setregs' above stacks the digest fields
                // in a similar manner to setup the phvs per byte on learn_tbl
                // regs. To illustrate with an example - tna_digest.p4 (since
                // this is not clear based on reg descriptions);
                //
                // BFA Output:
                //
                //   learning:
                //      select: { B1(0..2): B0(1) }  # L[0..2]b:
                //      ingress::ig_intr_md_for_dprsr.digest_type 0:
                //        - B1(0..2)  # L[0..2]b: ingress::ig_intr_md_for_dprsr.digest_type
                //        - MW0  # ingress::hdr.ethernet.dst_addr.16-47
                //        - MH1  # ingress::hdr.ethernet.dst_addr.0-15
                //        - MH0(0..8)  # L[0..8]b: ingress::ig_md.port
                //        - MW1  # ingress::hdr.ethernet.src_addr.16-47
                //        - MH2  # ingress::hdr.ethernet.src_addr.0-15
                //
                // PHV packing for digest,
                //
                //    B1(7..0) | MW0 (31..24) | MW0(23..16) | MW0(15..8)  |
                //   MW0(7..0) | MH1 (15..8)  | MH1(7..0)   | MH0(16..8)  |
                //   MH0(7..0) | MW1 (31..24) | MW1(23..16) | MW1(15..8)  |
                //   MW1(7..0) | MH2 (15..8)  | MH2(7..0)   | ----------  |
                //
                // Learn Mask Regs for above digest
                //   deparser.regs.dprsrreg.inp.icr.lrnmask[0].mask[11] = 4294967047 (0x07ffffff)
                //   deparser.regs.dprsrreg.inp.icr.lrnmask[0].mask[10] = 4294967295 (0xffffff01)
                //   deparser.regs.dprsrreg.inp.icr.lrnmask[0].mask[9]  = 4278321151 (0xffffffff)
                //   deparser.regs.dprsrreg.inp.icr.lrnmask[0].mask[8]  = 4294967040 (0xffffff00)

                bitvec lrnmask;
                int startBit = 0;
                int size = 0;
                for (auto p : set.second) {
                    if (size > 0) lrnmask <<= p->reg.size;
                    auto psliceSize = p.size();
                    startBit = p.lobit();
                    lrnmask.setrange(startBit, psliceSize);
                    size += p->reg.size;
                }
                // Pad to a 32 bit word
                auto shift = (size % 32) ? (32 - (size % 32)) : 0;
                lrnmask <<= shift;
                int num_words = (size + 31) / 32;
                int quanta_index = 11;
                for (int index = num_words - 1; index >= 0; index--) {
                    BUG_CHECK(quanta_index >= 0, "quanta_index < 0");
                    unsigned word = lrnmask.getrange(index * 32, 32);
                    regs.dprsrreg.inp.icr.lrnmask[id].mask[quanta_index--] = word;
                }
            }
        }
    }

#define DISBALE_IF_NOT_SET(ISARRAY, ARRAY, REGS, DISABLE) \
    ISARRAY(for (auto &r : ARRAY)) if (!ISARRAY(r.) REGS.modified()) ISARRAY(r.) REGS.DISABLE = 1;
    JBAY_DISABLE_REGBITS(DISBALE_IF_NOT_SET)

    if (options.condense_json) regs.disable_if_reset_value();
    if (error_count == 0 && options.gen_json)
        regs.emit_json(*open_output("regs.deparser.cfg.json"));
    TopLevel::regs<Target::JBay>()->reg_pipe.pardereg.dprsrreg.set("regs.deparser", &regs);
}

#if 0
namespace {
static struct JbayChecksumReg : public Phv::Register {
    JbayChecksumReg(int unit) : Phv::Register("", Phv::Register::CHECKSUM, unit,
                                              unit+CONSTANTS_PHVID_JBAY_HIGH, 16) {
        snprintf(name, "csum%d", unit); }
    int deparser_id() const override { return uid; }
} jbay_checksum_units[8] = { {0}, {1}, {2}, {3}, {4}, {5}, {6}, {7} };
}

template<> Phv::Slice Deparser::RefOrChksum::lookup<Target::JBay>() const {
    if (lo != hi || lo < 0 || lo >= Target::JBay::DEPARSER_CHECKSUM_UNITS) {
        error(lineno, "Invalid checksum unit number");
        return Phv::Slice(); }
    return Phv::Slice(tofino_checksum_units[lo], 0, 15);
}
#endif

template <>
unsigned Deparser::FDEntry::Checksum::encode<Target::JBay>() {
    return CONSTANTS_PHVID_JBAY_HIGH + unit;
}

template <>
unsigned Deparser::FDEntry::Constant::encode<Target::JBay>() {
    return CONSTANTS_PHVID_JBAY_LOW + Deparser::constant_idx(gress, val);
}

template <>
void Deparser::gen_learn_quanta(Target::JBay::parser_regs &regs, json::vector &learn_quanta) {}

template <>
void Deparser::process(Target::JBay *) {
    // Chip-specific code for process method
    // None for JBay
}
