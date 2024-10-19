/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <optional>

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/mau/instruction_selection.h"
#include "bf-p4c/parde/clot/allocate_clot.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/clot/field_pov_analysis.h"
#include "bf-p4c/parde/clot/pragma/do_not_use_clot.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/pragma/pa_alias.h"
#include "bf-p4c/phv/pragma/pa_no_overlay.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class ClotTest : public JBayBackendTest {};

namespace {

std::optional<TofinoPipeTestCase> createClotTest(const std::string &doNotUseClotPragmas,
                                                 const std::string &headerTypes,
                                                 const std::string &headerInstances,
                                                 const std::string &parser, const std::string &mau,
                                                 const std::string &deparser) {
    auto source = P4_SOURCE(P4Headers::V1MODEL, R"(
%DO_NOT_USE_CLOT_PRAGMAS%

%HEADER_TYPES%

struct Headers {
  %HEADER_INSTANCES%
}

struct Metadata { }

parser parse(packet_in pkt,
             out Headers hdr,
             inout Metadata metadata,
             inout standard_metadata_t sm) {
  %PARSER%
}

control mau(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
  %MAU%
}

control deparse(packet_out pkt, in Headers hdr) {
  apply {
    %DEPARSER%
  }
}

control verifyChecksum(inout Headers hdr, inout Metadata meta) {
  apply {}
}

control computeChecksum(inout Headers hdr, inout Metadata meta) {
  apply {}
}

V1Switch(parse(), verifyChecksum(), mau(), mau(), computeChecksum(), deparse()) main;)");

    boost::replace_first(source, "%DO_NOT_USE_CLOT_PRAGMAS%", doNotUseClotPragmas);
    boost::replace_first(source, "%HEADER_TYPES%", headerTypes);
    boost::replace_first(source, "%HEADER_INSTANCES%", headerInstances);
    boost::replace_first(source, "%PARSER%", parser);
    boost::replace_first(source, "%MAU%", mau);
    boost::replace_first(source, "%DEPARSER%", deparser);

    auto &options = BackendOptions();
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.target = "tofino"_cs;
    options.arch = "v1model"_cs;
    options.disable_parse_min_depth_limit = true;

    return TofinoPipeTestCase::createWithThreadLocalInstances(source);
}

}  // namespace

struct SliceSpec {
    std::string fieldName;
    std::optional<le_bitrange> slice;

    SliceSpec(const char *fieldName)  // NOLINT(runtime/explicit)
        : fieldName(fieldName),       // NOLINT(runtime/explicit)
          slice(std::nullopt) {}
    SliceSpec(const cstring fieldName)  // NOLINT(runtime/explicit)
        : fieldName(fieldName),         // NOLINT(runtime/explicit)
          slice(std::nullopt) {}

    SliceSpec(const char *fieldName, le_bitrange slice) : fieldName(fieldName), slice(slice) {}
    SliceSpec(const cstring fieldName, le_bitrange slice) : fieldName(fieldName), slice(slice) {}

    bool operator<(const SliceSpec other) const {
        if (fieldName != other.fieldName) return fieldName < other.fieldName;
        return slice < other.slice;
    }

    bool operator==(const SliceSpec other) const {
        return fieldName == other.fieldName && slice == other.slice;
    }

    bool operator>(const SliceSpec other) const {
        if (fieldName != other.fieldName) return fieldName > other.fieldName;
        return other.slice < slice;
    }

    bool operator<=(const SliceSpec other) const { return !(*this > other); }

    bool operator!=(const SliceSpec other) const { return !(*this == other); }

    bool operator>=(const SliceSpec other) const { return !(*this < other); }

 private:
    friend std::ostream &operator<<(std::ostream &, const SliceSpec &);
};

std::ostream &operator<<(std::ostream &out, const SliceSpec &sliceSpec) {
    out << sliceSpec.fieldName;
    if (sliceSpec.slice) out << "[" << sliceSpec.slice->hi << ".." << sliceSpec.slice->lo << "]";
    return out;
}

using ExpectedClot = std::vector<SliceSpec>;
using ExpectedAllocation = std::set<ExpectedClot>;

void runClotTest(std::optional<TofinoPipeTestCase> test, ExpectedAllocation expectedAlloc,
                 ExpectedAllocation expectedAliases = {}) {
    PhvInfo phvInfo;
    PhvUse phvUse(phvInfo);
    ClotInfo clotInfo(phvUse);
    ReductionOrInfo red_info;

    CollectHeaderStackInfo collectHeaderStackInfo;
    CollectPhvInfo collectPhvInfo(phvInfo);
    PragmaNoOverlay pragmaNoOverlay(phvInfo);
    PragmaAlias pragmaAlias(phvInfo, pragmaNoOverlay);
    PragmaDoNotUseClot pragmaDoNotUseClot(phvInfo);
    FieldPovAnalysis fieldPovAnalysis(clotInfo, phvInfo);
    CollectClotInfo collectClotInfo(phvInfo, clotInfo, pragmaDoNotUseClot);
    InstructionSelection instructionSelection(BackendOptions(), phvInfo, red_info);
    AllocateClot allocateClot(clotInfo, phvInfo, phvUse, pragmaDoNotUseClot, pragmaAlias, false);

    PassManager passes = {
        &collectHeaderStackInfo, &collectPhvInfo,  &clotInfo.parserInfo,
        &instructionSelection,   &pragmaAlias,     &pragmaDoNotUseClot,
        &fieldPovAnalysis,       &collectClotInfo, &allocateClot,
    };

    ASSERT_TRUE(test);
    test->pipe->apply(passes);

    // Specialize each expected CLOT for each gress and populate any missing slice ranges.
    ExpectedAllocation expectedClots;
    for (auto &clot : expectedAlloc) {
        EXPECT_NE(clot.size(), 0UL);
        for (auto gress : {INGRESS, EGRESS}) {
            ExpectedClot specializedClot;
            for (auto &slice : clot) {
                auto fieldName = toString(gress) + "::hdr." + slice.fieldName;
                BUG_CHECK(phvInfo.field(fieldName),
                          "Field %1% in expected test result is not present in the P4 program",
                          fieldName);
                auto bitrange =
                    slice.slice ? *slice.slice : StartLen(0, phvInfo.field(fieldName)->size);
                specializedClot.emplace_back(fieldName, bitrange);
            }

            expectedClots.insert(specializedClot);
        }
    }

    // Convert the actual allocation into an ExpectedAllocation.
    ExpectedAllocation actualClots;
    for (const auto *clot : clotInfo.clots()) {
        // Convert the actual CLOT into an ExpectedClot.
        ExpectedClot expectedClot;
        for (const auto &kv : clot->fields_to_slices()) {
            expectedClot.emplace_back(kv.first->name, kv.second->range());
        }

        EXPECT_NE(expectedClot.size(), 0UL);
        actualClots.insert(expectedClot);
    }

    if (expectedClots != actualClots) {
        std::cerr << clotInfo.print(&phvInfo);
    }

    EXPECT_EQ(expectedClots, actualClots);

    if (expectedAliases.size()) {
        // Identify all fields participating in aliases
        std::map<cstring, cstring> aliasSrcToDst;
        std::set<cstring> aliasDst;
        for (auto &[src, dst] : pragmaAlias.getAliasMap()) {
            aliasSrcToDst.emplace(src, dst.field);
            aliasDst.emplace(dst.field);
        }

        // Verify that the fields in each group participate in the same alias
        for (auto &aliasGrp : expectedAliases) {
            EXPECT_NE(aliasGrp.size(), 0UL);
            for (auto gress : {INGRESS, EGRESS}) {
                cstring dst = ""_cs;
                for (auto &slice : aliasGrp) {
                    auto fieldName = toString(gress) + "::hdr." + slice.fieldName;

                    EXPECT_EQ(aliasSrcToDst.count(fieldName) || aliasDst.count(fieldName), true)
                        << "Field " << fieldName << " was not found in the alias map.";

                    if (aliasSrcToDst.count(fieldName)) {
                        if (dst == "")
                            dst = aliasSrcToDst.at(fieldName);
                        else
                            EXPECT_EQ(aliasSrcToDst.at(fieldName), dst)
                                << "Alias destition for " << fieldName << " is "
                                << aliasSrcToDst.at(fieldName) << " but expecting " << dst;
                    }

                    if (aliasDst.count(fieldName)) {
                        if (dst == "")
                            dst = fieldName;
                        else
                            EXPECT_EQ(fieldName, dst)
                                << "Alias destition is " << fieldName << " but expecting " << dst;
                    }
                }
            }
        }
    }
}

TEST_F(ClotTest, Basic1) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H {
                bit<64> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H h;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() { hdr.h.f2 = 0; }

            table t1 {
                key = { hdr.h.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h);
        )"));

    // -----
    // h.f1  CLOT
    // h.f2  written
    // -----
    runClotTest(test, {{"h.f1"}});
}

TEST_F(ClotTest, AdjacentHeaders1) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H {
                bit<64> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H h1;
            H h2;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                pkt.extract(hdr.h2);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() { hdr.h2.f2 = 0; }

            table t1 {
                key = { hdr.h2.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h2);
        )"));

    // -----
    // h1.f1  CLOT
    // h1.f2  CLOT
    // h2.f1  CLOT
    // -----
    // h2.f2  written
    // -----
    runClotTest(test, {{"h1.f1", "h1.f2", "h2.f1"}});
}

TEST_F(ClotTest, HeaderRemoval1) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<32> f;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<96> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 h1;
            H2 h2;
            H3 h3;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                pkt.extract(hdr.h2);
                pkt.extract(hdr.h3);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.h2.setInvalid();
                hdr.h3.f2 = 0;
            }

            table t1 {
                key = { hdr.h3.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h2);
            pkt.emit(hdr.h3);
        )"));

    // -----------
    // h1.f         CLOT
    // -----------
    // h2.f[63:24]  CLOT
    // h2.f[23:0]   3-byte gap, required by h2's possible removal
    // -----------
    // h3.f1        CLOT
    // h3.f2        written
    // -----------
    runClotTest(test, {
                          {"h1.f"},
                          {{"h2.f", FromTo(24, 63)}},
                          {"h3.f1"},
                      });
}

TEST_F(ClotTest, HeaderRemoval2) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<32> f;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<96> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 h1;
            H2 h2;
            H3 h3;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                pkt.extract(hdr.h2);
                pkt.extract(hdr.h3);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.h2.setInvalid();
                hdr.h3.setInvalid();
            }

            table t1 {
                key = { hdr.h3.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h2);
            pkt.emit(hdr.h3);
        )"));

    // -----
    // h1.f   CLOT
    // -----  no gap necessary: h1 always valid
    // h2.f   CLOT
    // -----  no gap necessary: h2 always valid when h3 is valid
    // h3.f1  CLOT
    // h3.f2  CLOT
    // -----
    runClotTest(test, {
                          {"h1.f"},
                          {"h2.f", "h3.f1", "h3.f2"},
                      });
}

TEST_F(ClotTest, MutualExclusion1) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<32> f1;
                bit<8> f2;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<96> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 h1;
            H2 h2a;
            H2 h2b;
            H3 h3;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                transition select(hdr.h1.f2) {
                    0: parse2a;
                    1: parse2b;
                }
            }

            state parse2a {
                pkt.extract(hdr.h2a);
                transition parse3;
            }

            state parse2b {
                pkt.extract(hdr.h2b);
                transition parse3;
            }

            state parse3 {
                pkt.extract(hdr.h3);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.h3.f2 = 0;
            }

            table t1 {
                key = { hdr.h3.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h2b);
            pkt.emit(hdr.h2a);
            pkt.emit(hdr.h3);
        )"));

    // No gaps needed: headers are neither removed, nor rearranged, nor added.
    //           -----
    //           h1.f1  CLOT
    //           h1.f2  CLOT
    //           -----
    // CLOT  h2a.f | h2b.f  CLOT
    //           -----
    //           h3.f1  CLOT
    //           h3.f2  written
    //           -----
    runClotTest(test, {
                          {"h1.f1", "h1.f2"},
                          {"h2a.f"},
                          {"h2b.f"},
                          {"h3.f1"},
                      });
}

TEST_F(ClotTest, Insertion1) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<32> f1;
                bit<8> f2;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<96> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 h1;
            H2 h2a;
            H2 h2b;
            H3 h3;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                transition select(hdr.h1.f2) {
                    0: parse2a;
                    1: parse2b;
                }
            }

            state parse2a {
                pkt.extract(hdr.h2a);
                transition parse3;
            }

            state parse2b {
                pkt.extract(hdr.h2b);
                transition parse3;
            }

            state parse3 {
                pkt.extract(hdr.h3);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.h2b.setValid();
                hdr.h3.f2 = 0;
            }

            table t1 {
                key = { hdr.h3.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h2b);
            pkt.emit(hdr.h2a);
            pkt.emit(hdr.h3);
        )"));

    //           -----
    //           h1.f1[31:16]  CLOT
    //           h1.f1[15:0]   gap, needed because h2b can be inserted between h1 and h2a
    //           h1.f2         gap
    //           -----
    // CLOT  h2a.f | h2b.f     might be added
    //           -----
    //           h3.f1         CLOT
    //           h3.f2         written
    //           -----
    runClotTest(test, {
                          {{"h1.f1", FromTo(16, 31)}},
                          {"h2a.f"},
                          {"h3.f1"},
                      });
}

TEST_F(ClotTest, Insertion2) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<32> f1;
                bit<8> f2;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<96> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 h1;
            H2 h2a;
            H2 h2b;
            H3 h3;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                transition select(hdr.h1.f2) {
                    0: parse2a;
                    1: parse2b;
                }
            }

            state parse2a {
                pkt.extract(hdr.h2a);
                transition parse3;
            }

            state parse2b {
                pkt.extract(hdr.h2b);
                transition parse3;
            }

            state parse3 {
                pkt.extract(hdr.h3);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.h2a.setValid();
                hdr.h3.f2 = 0;
            }

            table t1 {
                key = { hdr.h3.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h2b);
            pkt.emit(hdr.h2a);
            pkt.emit(hdr.h3);
        )"));

    // -----
    // h1.f1         CLOT
    // h1.f2         CLOT
    // -----
    // h2b.f[63:24]  CLOT
    // h2b.f[23:0]   3-byte gap, needed because h2a can be inserted between h2b and h3
    // -----
    // h2a.f         might be added
    // -----
    // h3.f1         CLOT
    // h3.f2         written
    // -----
    runClotTest(test, {
                          {"h1.f1", "h1.f2"},
                          {{"h2b.f", FromTo(24, 63)}},
                          {"h3.f1"},
                      });
}

TEST_F(ClotTest, Reorder1) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<32> f;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<96> f;
            }

            header H4 {
                bit<104> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 h1;
            H2 h2;
            H3 h3;
            H4 h4;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                pkt.extract(hdr.h2);
                pkt.extract(hdr.h3);
                pkt.extract(hdr.h4);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.h4.f2 = 0;
            }

            table t1 {
                key = { hdr.h4.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h3);
            pkt.emit(hdr.h2);
            pkt.emit(hdr.h4);
        )"));

    // -----
    // h1.f[31:24]  CLOT
    // h1.f[23:0]   3-byte gap, needed because h3 is inserted between h1 and h2
    // -----
    // h2.f[63:24]  CLOT
    // h2.f[23:0]   3-byte gap, needed because h2 and h3 are reordered
    // -----
    // h3.f[95:24]  CLOT
    // h3.f[23:0]   3-byte gap, needed because h2 is inserted between h3 and h4
    // -----
    // h4.f1        CLOT
    // h4.f2        written
    // -----
    runClotTest(test, {
                          {{"h1.f", FromTo(24, 31)}},
                          {{"h2.f", FromTo(24, 63)}},
                          {{"h3.f", FromTo(24, 95)}},
                          {"h4.f1"},
                      });
}

TEST_F(ClotTest, Reorder2) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<32> f;
            }

            header H2 {
                bit<40> f;
            }

            header H3 {
                bit<64> f;
            }

            header H4 {
                bit<96> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 h1;
            H2 h2;
            H3 h3;
            H4 h4;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                pkt.extract(hdr.h2);
                pkt.extract(hdr.h3);
                pkt.extract(hdr.h4);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.h4.f2 = 0;
            }

            table t1 {
                key = { hdr.h4.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h3);
            pkt.emit(hdr.h2);
            pkt.emit(hdr.h4);
        )"));

    // -----
    // h1.f         CLOT
    // -----
    // h2.f         gap: slice [39:16] needed because h3 is inserted between h1 and h2
    //                   slice [23:0] needed because h2 and h3 are reordered
    // -----
    // h3.f[63:24]  CLOT
    // h3.f[23:0]   3-byte gap, needed because h2 is inserted between h3 and h4
    // -----
    // h4.f1        CLOT
    // h4.f2        written
    // -----
    runClotTest(test, {
                          {"h1.f"},
                          {{"h3.f", FromTo(24, 63)}},
                          {"h4.f1"},
                      });
}

TEST_F(ClotTest, AdjacentHeaders2) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<96> f1;
                bit<8> f2;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<16> f;
            }

            header H4 {
                bit<32> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 h1;
            H2 h2;
            H3 h3;
            H4 h4;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h1);
                transition select(hdr.h1.f2) {
                    0: parseH2;
                    1: parseH4;
                }
            }

            state parseH2 {
                pkt.extract(hdr.h2);
                pkt.extract(hdr.h3);
                transition parseH4;
            }

            state parseH4 {
                pkt.extract(hdr.h4);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.h4.f2 = 0;
            }

            table t1 {
                key = { hdr.h4.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h1);
            pkt.emit(hdr.h2);
            pkt.emit(hdr.h3);
            pkt.emit(hdr.h4);
        )"));

    // Before the introduction of multiheader CLOTs, CLOT allocation would have returned the
    // following result:
    //
    // -----
    // h1.f1        CLOT 0
    // h1.f2        CLOT 0
    // -----
    //   | h2.f     CLOT 1
    //   | ----
    //   | h3.f     CLOT 2
    // -----
    // h4.f1[31:8]  3-byte gap. Slice [31:24] needed because h2 and h4 need to be separated by at
    //                          least 3 bytes, and h3 is only 2 bytes long. This induces slice
    //                          [31:8] to satisfy gap requirement between h3 and h4, and h1 and h4.
    // h4.f1[7:0]   CLOT 3
    // h4.f2        written
    // -----
    //
    // However, since h2 and h3 are immediately adjacent in the parser and deparser, they are now
    // grouped in a multiheader CLOT, making the gap unnecessary. This leads to the following
    // CLOT allocation:
    //
    // -----
    // h1.f1        CLOT 0
    // h1.f2        CLOT 0
    // -----
    // h2.f         CLOT 1
    // h3.f         CLOT 1
    // -----
    // h4.f1        CLOT 2
    // h4.f2        written
    runClotTest(test, {{"h1.f1", "h1.f2"}, {"h2.f", "h3.f"}, {"h4.f1"}});
}

TEST_F(ClotTest, FieldPragma) {
    auto test = createClotTest(P4_SOURCE(R"(
            @do_not_use_clot("ingress", "hdr.h.f1")
            @do_not_use_clot("egress", "hdr.h.f1")
        )"),
                               P4_SOURCE(R"(
            header H {
                bit<64> f1;
                bit<64> f2;
                bit<8> f3;
            }
        )"),
                               P4_SOURCE(R"(
            H h;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() { hdr.h.f3 = 0; }

            table t1 {
                key = { hdr.h.f3 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h);
        )"));

    // -----
    // h.f1  @pragma do_not_use_clot
    // h.f2  CLOT
    // h.f3  written
    // -----
    runClotTest(test, {{"h.f2"}});
}

TEST_F(ClotTest, HeaderPragma) {
    auto test = createClotTest(P4_SOURCE(R"(
            @do_not_use_clot("ingress", "hdr.h")
            @do_not_use_clot("egress", "hdr.h")
        )"),
                               P4_SOURCE(R"(
            header H {
                bit<64> f1;
                bit<64> f2;
                bit<8> f3;
            }
        )"),
                               P4_SOURCE(R"(
            H h;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.h);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() { hdr.h.f3 = 0; }

            table t1 {
                key = { hdr.h.f3 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.h);
        )"));

    // -----
    // h.f1  @pragma do_not_use_clot
    // h.f2  @pragma do_not_use_clot
    // h.f3  written, @pragma do_not_use_clot
    // -----
    runClotTest(test, {});
}

TEST_F(ClotTest, AdjacentMultiheaderVariableSize) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<96> f1;
                bit<8> f2;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<16> f;
            }

            header H4 {
                bit<32> f1;
                bit<8> f2;
            }

            header H5 {
                bit<40> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 a;
            H2 b;
            H3 c;
            H4 d;
            H5 e;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.a);
                transition select(hdr.a.f2) {
                    0: parseB;
                    1: parseBC;
                    2: parseBD;
                }
            }

            state parseB {
                pkt.extract(hdr.b);
                transition parseE;
            }

            state parseBC {
                pkt.extract(hdr.b);
                pkt.extract(hdr.c);
                transition parseE;
            }

            state parseBD {
                pkt.extract(hdr.b);
                pkt.extract(hdr.d);
                transition parseE;
            }

            state parseE {
                pkt.extract(hdr.e);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.e.f2 = 0;
            }

            table t1 {
                key = { hdr.e.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.a);
            pkt.emit(hdr.b);
            pkt.emit(hdr.c);
            pkt.emit(hdr.d);
            pkt.emit(hdr.e);
        )"));

    // -----
    // a.f1        CLOT 1
    // a.f2        CLOT 1
    // -----
    // b.f         CLOT 0 (This is CLOT 0 because it has the most unused bits)
    // c.f         CLOT 0
    // d.f1        CLOT 0
    // d.f2        CLOT 0
    //   Content and length varies based on parser state:
    //     - parseB: { b.f } (length = 8B)
    //     - parseBC: { b.f, c.f } (length = 10B)
    //     - parseBD: { b.f, d.f1, d.f2 } (length = 13B)
    // -----
    //   "e" is adjacent to all possible variations of CLOT 0, no gap required.
    // e.f1        CLOT 2
    // e.f2        written

    runClotTest(test, {{"b.f", "c.f", "d.f1", "d.f2"}, {"a.f1", "a.f2"}, {"e.f1"}});
}

TEST_F(ClotTest, NotAdjacentMultiheaderVariableSize) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header H1 {
                bit<96> f1;
                bit<8> f2;
            }

            header H2 {
                bit<64> f;
            }

            header H3 {
                bit<16> f;
            }

            header H4 {
                bit<32> f1;
                bit<8> f2;
            }

            header H5 {
                bit<40> f1;
                bit<8> f2;
            }
        )"),
                               P4_SOURCE(R"(
            H1 a;
            H2 b;
            H3 c;
            H4 d;
            H5 e;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.a);
                transition select(hdr.a.f2) {
                    0: parseB;
                    1: parseBC;
                    2: parseBD;
                }
            }

            state parseB {
                pkt.extract(hdr.b);
                transition parseE;
            }

            state parseBC {
                pkt.extract(hdr.b);
                pkt.extract(hdr.c);
                transition parseE;
            }

            state parseBD {
                pkt.extract(hdr.b);
                pkt.extract(hdr.d);
                transition parseE;
            }

            state parseE {
                pkt.extract(hdr.e);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.d.f2 = 0;
            }

            table t1 {
                key = { hdr.d.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr.a);
            pkt.emit(hdr.b);
            pkt.emit(hdr.c);
            pkt.emit(hdr.d);
            pkt.emit(hdr.e);
        )"));

    // -----
    // a.f1        CLOT 1
    // a.f2        CLOT 1
    // -----
    // b.f         CLOT 0 (This is CLOT 0 because it has the most unused bits)
    // c.f         CLOT 0
    // d.f1        CLOT 0
    // d.f2        written
    //   Content and length varies based on parser state:
    //     - parseB: { b.f } (length = 8B)
    //     - parseBC: { b.f, c.f } (length = 10B)
    //     - parseBD: { b.f, d.f1 } (length = 12B)
    // -----
    //   "e" is not adjacent in "parseBD" variation of CLOT 0: a gap is required.
    // e.f1[39:16]  3-byte gap
    // e.f1[15:0]   CLOT 2
    // e.f2         CLOT 2

    runClotTest(test,
                {{"b.f", "c.f", "d.f1"}, {"a.f1", "a.f2"}, {{"e.f1", FromTo(0, 15)}, "e.f2"}});
}

TEST_F(ClotTest, AdjacentMultiheaderVariableSizeHeaderStack) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header hdr_t {
                bit<32> f1;
                bit<16> f2;
            }
        )"),
                               P4_SOURCE(R"(
            hdr_t[5] stack;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.stack[0]);
                transition select(hdr.stack[0].f1) {
                    0: parse_index_1;
                    1: parse_index_1_2;
                    2: parse_index_1_3;
                }
            }

            state parse_index_1 {
                pkt.extract(hdr.stack[1]);
                transition parse_index_4;
            }

            state parse_index_1_2 {
                pkt.extract(hdr.stack[1]);
                pkt.extract(hdr.stack[2]);
                transition parse_index_4;
            }

            state parse_index_1_3 {
                pkt.extract(hdr.stack[1]);
                pkt.extract(hdr.stack[3]);
                transition parse_index_4;
            }

            state parse_index_4 {
                pkt.extract(hdr.stack[4]);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.stack[4].f2 = 0;
            }

            table t1 {
                key = { hdr.stack[4].f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr);
        )"));

    // -----
    // stack[0].f1        CLOT 1
    // stack[0].f2        CLOT 1
    // -----
    // stack[1].f1        CLOT 0 (This is CLOT 0 because it has the most unused bits)
    // stack[1].f2        CLOT 0
    // stack[2].f1        CLOT 0
    // stack[2].f2        CLOT 0
    // stack[3].f1        CLOT 0
    // stack[3].f2        CLOT 0
    //   Content and length varies based on parser state:
    //     - parse_index_1: { stack[0].f1, stack[0].f1 } (length = 6B)
    //     - parse_index_1_2: { stack[1].f1, stack[1].f2, stack[2].f1, stack[2].f2 } (length = 12B)
    //     - parse_index_1_3: { stack[1].f1, stack[1].f2, stack[3].f1, stack[3].f2 } (length = 12B)
    // -----
    //   "stack[4]" is adjacent to all possible variations of CLOT 0, no gap required.
    // stack[4].f1        CLOT 2
    // stack[4].f2        written

    runClotTest(test, {{"stack[1].f1", "stack[1].f2", "stack[2].f1", "stack[2].f2", "stack[3].f1",
                        "stack[3].f2"},
                       {"stack[0].f1", "stack[0].f2"},
                       {"stack[4].f1"}});
}

TEST_F(ClotTest, NotAdjacentMultiheaderVariableSizeHeaderStack) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header hdr_t {
                bit<32> f1;
                bit<16> f2;
            }
        )"),
                               P4_SOURCE(R"(
            hdr_t[5] stack;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.stack[0]);
                transition select(hdr.stack[0].f1) {
                    0: parse_index_1;
                    1: parse_index_1_2;
                    2: parse_index_1_3;
                }
            }

            state parse_index_1 {
                pkt.extract(hdr.stack[1]);
                transition parse_index_4;
            }

            state parse_index_1_2 {
                pkt.extract(hdr.stack[1]);
                pkt.extract(hdr.stack[2]);
                transition parse_index_4;
            }

            state parse_index_1_3 {
                pkt.extract(hdr.stack[1]);
                pkt.extract(hdr.stack[3]);
                transition parse_index_4;
            }

            state parse_index_4 {
                pkt.extract(hdr.stack[4]);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() {
                hdr.stack[3].f2 = 0;
            }

            table t1 {
                key = { hdr.stack[3].f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr);
        )"));

    // -----
    // stack[0].f1        CLOT 1
    // stack[0].f2        CLOT 1
    // -----
    // stack[1].f1        CLOT 0 (This is CLOT 0 because it has the most unused bits)
    // stack[1].f2        CLOT 0
    // stack[2].f1        CLOT 0
    // stack[2].f2        CLOT 0
    // stack[3].f1        CLOT 0
    // stack[3].f2        CLOT 0
    //   Content and length varies based on parser state:
    //     - parse_index_1: { stack[0].f1, stack[0].f1 } (length = 6B)
    //     - parse_index_1_2: { stack[1].f1, stack[1].f2, stack[2].f1, stack[2].f2 } (length = 12B)
    //     - parse_index_1_3: { stack[1].f1, stack[1].f2, stack[3].f1 } (length = 10B)
    // -----
    //   "stack[4]" is not adjacent in "parse_index_1_3" variation of CLOT 0: a gap is required.
    // stack[4].f1[31..8] 3-byte gap
    // stack[4].f1[7..0]  CLOT 2
    // stack[4].f2        CLOT 2

    runClotTest(test, {{"stack[1].f1", "stack[1].f2", "stack[2].f1", "stack[2].f2", "stack[3].f1"},
                       {"stack[0].f1", "stack[0].f2"},
                       {{"stack[4].f1", FromTo(0, 7)}, "stack[4].f2"}});
}

// If fields are zero intialized or extracted by a constant between two CLOTs, those two CLOT are
// adjacent in the parser. However, if those fields are deparsed between the two CLOTs, then they
// are also not adjacent in the deparser. Therefore, a gap needs to be inserted between those two
// CLOTs.
TEST_F(ClotTest, ZeroInitsBetweenTwoClots) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header hdr_t {
                bit<32> f1;
                bit<32> f2;
            }
        )"),
                               P4_SOURCE(R"(
            hdr_t a;
            hdr_t b;
            hdr_t c;
            hdr_t d;
            hdr_t e;
            hdr_t f;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.a);
                transition parse_b;
            }

            state parse_b {
                pkt.extract(hdr.b);
                transition select(hdr.b.f1) {
                    0: zero_init_c;
                    1: parse_c;
                }
            }

            state zero_init_c {
                hdr.c.setValid();
                hdr.c.f1 = 0;
                hdr.c.f2 = 0;
                transition parse_e;
            }

            state parse_c {
                pkt.extract(hdr.c);
                transition parse_d;
            }

            state parse_d {
                pkt.extract(hdr.d);
                transition parse_e;
            }

            state parse_e {
                pkt.extract(hdr.e);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() { hdr.e.f2 = 0; }

            table t1 {
                key = { hdr.e.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr);
        )"));

    runClotTest(test,
                {{"a.f1", "a.f2", "b.f1", "b.f2"}, {"d.f1", "d.f2"}, {{"e.f1", FromTo(0, 7)}}});
}

TEST_F(ClotTest, ExtractConstantsBetweenTwoClots) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header hdr_t {
                bit<32> f1;
                bit<32> f2;
            }
        )"),
                               P4_SOURCE(R"(
            hdr_t a;
            hdr_t b;
            hdr_t c;
            hdr_t d;
            hdr_t e;
            hdr_t f;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.a);
                transition parse_b;
            }

            state parse_b {
                pkt.extract(hdr.b);
                transition select(hdr.b.f1) {
                    0: zero_init_c;
                    1: parse_c;
                }
            }

            state zero_init_c {
                hdr.c.setValid();
                hdr.c.f1 = 192;
                hdr.c.f2 = 168;
                transition parse_e;
            }

            state parse_c {
                pkt.extract(hdr.c);
                transition parse_d;
            }

            state parse_d {
                pkt.extract(hdr.d);
                transition parse_e;
            }

            state parse_e {
                pkt.extract(hdr.e);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() { hdr.e.f2 = 0; }

            table t1 {
                key = { hdr.e.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr);
        )"));

    runClotTest(test,
                {{"a.f1", "a.f2", "b.f1", "b.f2"}, {"d.f1", "d.f2"}, {{"e.f1", FromTo(0, 7)}}});
}

TEST_F(ClotTest, ZeroInitAndExtractConstantBetweenTwoClots) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header hdr_t {
                bit<32> f1;
                bit<32> f2;
            }
        )"),
                               P4_SOURCE(R"(
            hdr_t a;
            hdr_t b;
            hdr_t c;
            hdr_t d;
            hdr_t e;
            hdr_t f;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.a);
                transition parse_b;
            }

            state parse_b {
                pkt.extract(hdr.b);
                transition select(hdr.b.f1) {
                    0: zero_init_c;
                    1: parse_c;
                }
            }

            state zero_init_c {
                hdr.c.setValid();
                hdr.c.f1 = 0;
                hdr.c.f2 = 42;
                transition parse_e;
            }

            state parse_c {
                pkt.extract(hdr.c);
                transition parse_d;
            }

            state parse_d {
                pkt.extract(hdr.d);
                transition parse_e;
            }

            state parse_e {
                pkt.extract(hdr.e);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            action act1() { hdr.e.f2 = 0; }

            table t1 {
                key = { hdr.e.f2 : exact; }
                actions = { act1; }
                size = 256;
            }

            apply {
                t1.apply();
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr);
        )"));

    runClotTest(test,
                {{"a.f1", "a.f2", "b.f1", "b.f2"}, {"d.f1", "d.f2"}, {{"e.f1", FromTo(0, 7)}}});
}

TEST_F(ClotTest, MergeVarbitValidsForClots) {
    auto test = createClotTest(P4_SOURCE(R"(
        )"),
                               P4_SOURCE(R"(
            header variable_h {
                varbit<64> bits;
            }

            header sample_h {
                bit<8> counterIndex;
            }
        )"),
                               P4_SOURCE(R"(
            sample_h sample;
            variable_h var;
            sample_h sample2;
            sample_h sample3;
        )"),
                               P4_SOURCE(R"(
            state start {
                pkt.extract(hdr.sample);
                transition select(hdr.sample.counterIndex[6:0]) {
                    1 : parse_var_1;
                    2 : parse_var_1;
                    3 : parse_var_1;
                    4 : parse_var_1;
                    default : accept;
                }
            }

            state parse_var_1 {
                pkt.extract(hdr.var, (bit<32>)(((bit<16>)hdr.sample.counterIndex[3:0]) * 8));
                transition select(hdr.sample.counterIndex[6:6]) {
                    0 : parse_sample2;
                    1 : accept;
                }
            }

            state parse_sample2 {
                pkt.extract(hdr.sample2);
                transition accept;
            }
        )"),
                               P4_SOURCE(R"(
            apply {
                if (hdr.sample.counterIndex[7:7] == 1w1) {
                    hdr.sample.counterIndex = 8w0xff;
                    hdr.var.setInvalid();
                } else {
                    hdr.sample3.setValid();
                    hdr.sample3.counterIndex = 8w0xf8;
                }
            }
        )"),
                               P4_SOURCE(R"(
            pkt.emit(hdr);
        )"));

    runClotTest(test,
                {{"variable_h_var_bits_8b.field", "variable_h_var_bits_16b.field",
                  "variable_h_var_bits_24b.field", "variable_h_var_bits_32b.field",
                  "variable_h_var_bits_40b.field", "variable_h_var_bits_48b.field",
                  "variable_h_var_bits_56b.field", "variable_h_var_bits_64b.field"}},
                {{"variable_h_var_bits_8b.$valid", "variable_h_var_bits_16b.$valid",
                  "variable_h_var_bits_24b.$valid", "variable_h_var_bits_32b.$valid",
                  "variable_h_var_bits_40b.$valid", "variable_h_var_bits_48b.$valid",
                  "variable_h_var_bits_56b.$valid", "variable_h_var_bits_64b.$valid"}});
}

}  // namespace P4::Test
