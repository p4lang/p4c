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

#include "deparser.h"
#include "gtest/gtest.h"
#include "sections.h"

namespace {

/* Tests for mirror
 *
 * Currently we cannot run tests for multiple targets (e.g., Tofino and JBay)
 * in a single run. As a result, all tests except Tofino are disabled.
 */

#define TOF_MIRR_CFG regs.header.hir.main_i.mirror_cfg
#define TOF_MIRR_TBL regs.header.hir.main_i.mirror_tbl

#define JBAY_MIRR_BASE regs.dprsrreg.ho_i
#define JBAY_MIRR_ENTRY him.mirr_hdr_tbl.entry
#define JBAY_MIRR_SEL regs.dprsrreg.inp.ipp.ingr.m_mirr_sel

#define FTR_MDP_MIRR_BASE regs.mdp_mem.tmm_ext_ram.tmm_ext[0]
#define FTR_DPRSR_MIRR_BASE regs.dprsr.dprsr_phvxb_rspec.ehm_xb

/// Mirror configuration for Tofino
struct TofinoMirrorCfg {
    std::string sel_phv_;
    int sel_phv_lo_;

    std::map<int, std::string> entry_id_phv;
    std::map<int, std::vector<std::string>> entry_phvs;

    TofinoMirrorCfg(std::string sel_phv, int sel_phv_lo)
        : sel_phv_(sel_phv), sel_phv_lo_(sel_phv_lo) {}
};

/// Mirror configuration for JBay
struct JBayMirrorCfg {
    std::string sel_phv_;
    int sel_phv_lo_;

    std::string sel_pov_;
    int sel_pov_lo_;

    std::map<int, std::string> entry_id_phv;
    std::map<int, std::vector<std::string>> entry_phvs;

    JBayMirrorCfg(std::string sel_phv, int sel_phv_lo, std::string sel_pov, int sel_pov_lo)
        : sel_phv_(sel_phv), sel_phv_lo_(sel_phv_lo), sel_pov_(sel_pov), sel_pov_lo_(sel_pov_lo) {}
};

/// Map from register name to Phv::Register*
std::map<std::string, const Phv::Register *> phvRegs;

/// Populate register name -> register map
void populateRegIds() {
    if (!phvRegs.size()) {
        // Initialize the PHVs.
        // Triggered by requesting a slice for a field. The field does not need to exist.
        Phv::get(INGRESS, 0, "jbay_dummy$");

        // Walk through the registers and record them
        for (int i = 0; i < Phv::num_regs(); ++i) {
            if (const auto *reg = Phv::reg(i)) phvRegs[reg->name] = reg;
        }
    }
}

/// Get the MAU ID of a given register name
int mau_id(std::string name) { return phvRegs.count(name) ? phvRegs.at(name)->mau_id() : -1; }

/// Get the deparser ID of a given register name
int deparser_id(std::string name) {
    return phvRegs.count(name) ? phvRegs.at(name)->deparser_id() : -1;
}

/// Find a Digest for a given target
Deparser::Digest *findDigest(Deparser *dprsr, target_t target) {
    for (auto &digest : dprsr->digests) {
        if (digest.type->target == target) return &digest;
    }

    BUG("Could not find the Digest for %s", toString(target).c_str());
    return nullptr;
}

/** Reset all target information
 *
 * This function should be called when switching from one target to another
 * (e.g., Tofino to JBay) in tests to reset state.
 */
void resetTarget() {
    options.target = NO_TARGET;
    Phv::test_clear();
    phvRegs.clear();
    Deparser *dprsr = dynamic_cast<Deparser *>(Section::test_get("deparser"));
    dprsr->gtest_clear();
}

/// Verify that registers match a mirror configuration (Tofino)
void tofinoCheckMirrorRegs(Target::Tofino::deparser_regs &regs, TofinoMirrorCfg &cfg) {
    populateRegIds();

    Deparser *dprsr = dynamic_cast<Deparser *>(Section::test_get("deparser"));
    auto *digest = findDigest(dprsr, TOFINO);

    // Tell the digest code to set the registers
    digest->type->setregs(regs, *dprsr, *digest);

    // Verify the registers:
    // 1. Verify common registers
    EXPECT_EQ(TOF_MIRR_CFG.phv, deparser_id(cfg.sel_phv_));
    EXPECT_EQ(TOF_MIRR_CFG.shft, cfg.sel_phv_lo_);
    EXPECT_EQ(TOF_MIRR_CFG.valid, 1);

    // 2. Verify the entries
    for (auto &kv : cfg.entry_id_phv) {
        int id = kv.first;
        EXPECT_EQ(TOF_MIRR_TBL[id].id_phv, deparser_id(cfg.entry_id_phv[id]));
        int idx = 0;
        for (auto &phv : cfg.entry_phvs[id]) {
            EXPECT_EQ(TOF_MIRR_TBL[id].phvs[idx], deparser_id(phv));
            idx++;
        }
        EXPECT_EQ(TOF_MIRR_TBL[id].len, cfg.entry_phvs[id].size());
    }
}

/// Verify that registers match a mirror configuration (JBay)
void jbayCheckMirrorRegs(Target::JBay::deparser_regs &regs, JBayMirrorCfg &cfg) {
    // Base index for POV PHV. Want this to be non-zero.
    const int povBase = 64;

    populateRegIds();

    Deparser *dprsr = dynamic_cast<Deparser *>(Section::test_get("deparser"));
    auto *digest = findDigest(dprsr, JBAY);

    // Ensure the POV register in the config is actually recorded as a POV in
    // the deparser object
    int povReg = mau_id(cfg.sel_pov_);
    dprsr->pov[INGRESS][Phv::reg(povReg)] = povBase;

    // Tell the digest code to set the registers
    digest->type->setregs(regs, *dprsr, *digest);

    // Verify the registers:
    // 1. Verify common registers
    EXPECT_EQ(JBAY_MIRR_SEL.phv, deparser_id(cfg.sel_phv_));
    EXPECT_EQ(JBAY_MIRR_SEL.pov, povBase + cfg.sel_pov_lo_);
    EXPECT_EQ(JBAY_MIRR_SEL.shft, cfg.sel_phv_lo_);
    EXPECT_EQ(JBAY_MIRR_SEL.disable_, 0);

    // 2. Verify the entries
    for (auto &base : JBAY_MIRR_BASE) {
        for (auto &kv : cfg.entry_id_phv) {
            int id = kv.first;
            EXPECT_EQ(base.JBAY_MIRR_ENTRY[id].id_phv, deparser_id(cfg.entry_id_phv[id]));
            int idx = 0;
            for (auto &phv : cfg.entry_phvs[id]) {
                EXPECT_EQ(base.JBAY_MIRR_ENTRY[id].phvs[idx], deparser_id(phv));
                idx++;
            }
            EXPECT_EQ(base.JBAY_MIRR_ENTRY[id].len, cfg.entry_phvs[id].size());
        }
    }
}

TEST(mirror, digest_tofino) {
    const char *mirror_str = R"MIRR_CFG(
version:
  target: Tofino
deparser ingress:
  mirror:
    select: B9(0..3)  # bit[3..0]: ingress::ig_intr_md_for_dprsr.mirror_type
    1:
      - H19(0..7)  # bit[7..0]: ingress::Thurmond.Circle.LaUnion[7:0].0-7
      - B9  # ingress::Thurmond.Longwood.Matheson
      - B9  # ingress::Thurmond.Longwood.Matheson
      - H56(0..8)  # bit[8..0]: ingress::Thurmond.Armagh.Moorcroft
)MIRR_CFG";

    resetTarget();

    auto *digest = ::get(Deparser::Digest::Type::all[TOFINO][INGRESS], "mirror");
    ASSERT_NE(digest, nullptr) << "Unable to find the mirror digest";

    Target::Tofino::deparser_regs regs;
    asm_parse_string(mirror_str);

    TofinoMirrorCfg mirrorCfg("B9", 0);
    mirrorCfg.entry_id_phv[1] = "H19";
    mirrorCfg.entry_phvs[1] = {"B9", "B9", "H56", "H56"};
    tofinoCheckMirrorRegs(regs, mirrorCfg);
}

TEST(mirror, digest_jbay) {
    const char *mirror_str = R"MIRR_CFG(
version:
  target: Tofino2
deparser ingress:
  mirror:
    select: { B9(0..3): B8(1) }  # bit[3..0]: ingress::ig_intr_md_for_dprsr.mirror_type
    1:
      - H19(0..7)  # bit[7..0]: ingress::Thurmond.Circle.LaUnion[7:0].0-7
      - B9  # ingress::Thurmond.Longwood.Matheson
      - B9  # ingress::Thurmond.Longwood.Matheson
      - H56(0..8)  # bit[8..0]: ingress::Thurmond.Armagh.Moorcroft
)MIRR_CFG";

    resetTarget();

    auto *digest = ::get(Deparser::Digest::Type::all[JBAY][INGRESS], "mirror");
    ASSERT_NE(digest, nullptr) << "Unable to find the mirror digest";

    Target::JBay::deparser_regs regs;
    asm_parse_string(mirror_str);

    JBayMirrorCfg mirrorCfg("B9", 0, "B8", 1);
    mirrorCfg.entry_id_phv[1] = "H19";
    mirrorCfg.entry_phvs[1] = {"B9", "B9", "H56", "H56"};
    jbayCheckMirrorRegs(regs, mirrorCfg);
}

}  // namespace
