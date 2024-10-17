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

/*
 * This test covers folding of hash functions with constant inputs.
 */

#include "bf-p4c/midend/fold_constant_hashes.h"

#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"

#include "bf-p4c/midend/type_checker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"

namespace P4::Test {

namespace FoldConstantHashesTest {

inline auto defs = R"(
    struct headers_t { }
    struct local_metadata_t { bit<16> hash_16b; bit<32> hash_32b; })";

#define RUN_CHECK(pass, input, expected)                                                        \
    do {                                                                                        \
        auto blk = TestCode(TestCode::Hdr::Tofino1arch, TestCode::tofino_shell(),               \
                            {FoldConstantHashesTest::defs, TestCode::empty_state(), input,      \
                             TestCode::empty_appy()},                                           \
                            TestCode::tofino_shell_control_marker());                           \
        blk.flags(Match::TrimWhiteSpace | Match::TrimAnnotations);                              \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullFrontend));                              \
        EXPECT_TRUE(blk.apply_pass(pass));                                                      \
        auto res = blk.match(expected);                                                         \
        EXPECT_TRUE(res.success) << "    @ expected[" << res.count << "], char pos=" << res.pos \
                                 << "\n"                                                        \
                                 << "    '" << expected[res.count] << "'\n"                     \
                                 << "    '" << blk.extract_code(res.pos) << "'\n";              \
    } while (0)

Visitor *setup_passes() {
    auto refMap = new P4::ReferenceMap;
    auto typeMap = new P4::TypeMap;
    auto typeChecking = new BFN::TypeChecking(refMap, typeMap);
    return new BFN::FoldConstantHashes(refMap, typeMap, typeChecking);
}

}  // namespace FoldConstantHashesTest

TEST(FoldConstantHashes, Identity16bConstant) {
    auto input = R"(
        Hash<bit<16>>(HashAlgorithm_t.IDENTITY) hash;
        action hash_action() {
            ig_md.hash_16b = hash.get(16w0xABCD);
        }
        apply {
            hash_action();
        })";
    Match::CheckList expected{"action hash_action() {",
                              "ig_md.hash_16b = 16w43981;",
                              "}",
                              "apply {",
                              "hash_action();",
                              "}"};
    RUN_CHECK(FoldConstantHashesTest::setup_passes(), input, expected);
}

TEST(FoldConstantHashes, Identity16bTuple) {
    auto input = R"(
        Hash<bit<16>>(HashAlgorithm_t.IDENTITY) hash;
        action hash_action() {
            ig_md.hash_16b = hash.get<tuple<bit<8>, bit<8>, bit<8>, bit<8>>>({ 1, 2, 3, 4 });
        }
        apply {
            hash_action();
        })";
    Match::CheckList expected{"action hash_action() {",
                              "ig_md.hash_16b = 16w772;",
                              "}",
                              "apply {",
                              "hash_action();",
                              "}"};
    RUN_CHECK(FoldConstantHashesTest::setup_passes(), input, expected);
}

TEST(FoldConstantHashes, Identity16bTupleTupleTuple) {
    auto input = R"(
        Hash<bit<16>>(HashAlgorithm_t.IDENTITY) hash;
        action hash_action() {
            ig_md.hash_16b =
                hash.get<tuple<bit<8>, tuple<bit<8>, bit<8>, tuple<bit<8>>>>>
                ({ 1, { 2, 3, { 4 } } });
        }
        apply {
            hash_action();
        })";
    Match::CheckList expected{"action hash_action() {",
                              "ig_md.hash_16b = 16w772;",
                              "}",
                              "apply {",
                              "hash_action();",
                              "}"};
    RUN_CHECK(FoldConstantHashesTest::setup_passes(), input, expected);
}

TEST(FoldConstantHashes, CRC32) {
    auto input = R"(
        Hash<bit<32>>(HashAlgorithm_t.CRC32) hash;
        action hash_action() {
            ig_md.hash_32b = hash.get(32w0xABCDEF01);
        }
        apply {
            hash_action();
        })";
    Match::CheckList expected{"action hash_action() {",
                              "ig_md.hash_32b = 32w2360908734;",
                              "}",
                              "apply {",
                              "hash_action();",
                              "}"};
    RUN_CHECK(FoldConstantHashesTest::setup_passes(), input, expected);
}

TEST(FoldConstantHashes, Custom) {  // Parameters of CRC32
    auto input = R"(
        CRCPolynomial<bit<32>>(32w0x04C11DB7, // polynomial
                               true,          // reversed
                               false,         // use msb?
                               false,         // extended?
                               32w0xFFFFFFFF, // initial shift register value
                               32w0xFFFFFFFF  // result xor
                               ) poly;
        Hash<bit<32>>(HashAlgorithm_t.CUSTOM, poly) hash;
        action hash_action() {
            ig_md.hash_32b = hash.get(32w0x01234567);
        }
        apply {
            hash_action();
        })";
    Match::CheckList expected{"action hash_action() {",
                              "ig_md.hash_32b = 32w4247457659;",
                              "}",
                              "apply {",
                              "hash_action();",
                              "}"};
    RUN_CHECK(FoldConstantHashesTest::setup_passes(), input, expected);
}

TEST(FoldConstantHashes, UnusedResult) {
    auto input = R"(
        CRCPolynomial<bit<32>>(32w0x04C11DB7, // polynomial
                               true,          // reversed
                               false,         // use msb?
                               false,         // extended?
                               32w0xFFFFFFFF, // initial shift register value
                               32w0xFFFFFFFF  // result xor
                               ) poly;
        Hash<bit<32>>(HashAlgorithm_t.CUSTOM, poly) hash;
        action hash_action() {
            hash.get(32w0x01234567);
        }
        apply {
            hash_action();
        })";
    Match::CheckList expected{"action hash_action() {", "}", "apply {", "hash_action();", "}"};
    RUN_CHECK(FoldConstantHashesTest::setup_passes(), input, expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
