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
 * This test covers initialization of eg_intr_md_for_dprsr.mirror_io_select
 * on Tofino2 targets if it's not explicitly set.
 */

#include "bf-p4c/midend/initialize_mirror_io_select.h"

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf_gtest_helpers.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "gtest/gtest.h"

namespace P4::Test {

namespace InitializeMirrorIOSelectTest {

std::string tofino_egress_shell() {
    return R"(
    struct headers_t { }
    struct local_metadata_t { }
    parser ingress_parser(packet_in packet, out headers_t hdr,
                          out local_metadata_t ig_md, out ingress_intrinsic_metadata_t ig_intr_md)
        {state start {transition accept;}}
    control ingress_control(inout headers_t hdr, inout local_metadata_t ig_md,
                            in ingress_intrinsic_metadata_t ig_intr_md,
                            in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
                            inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
                            inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
        {apply{}}
    control ingress_deparser(packet_out packet, inout headers_t hdr, in local_metadata_t ig_md,
                             in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md)
        {apply{}}
    parser egress_parser(packet_in packet, out headers_t hdr, out local_metadata_t eg_md,
                         out egress_intrinsic_metadata_t eg_intr_md%0%)
        {state start {transition accept;}}
    control egress_control(inout headers_t hdr, inout local_metadata_t eg_md,
                           in egress_intrinsic_metadata_t eg_intr_md,
                           in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
                           inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
                           inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport)
        {apply{}}
    control egress_deparser(packet_out packet, inout headers_t hdr, in local_metadata_t eg_md,
                            in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr)
        {apply{}}
    Pipeline(ingress_parser(),
             ingress_control(),
             ingress_deparser(),
             egress_parser(),
             egress_control(),
             egress_deparser()) pipeline;
    Switch(pipeline) main;)";
}

std::string tofino_shell_egress_parser_body_marker() {
    return "parser egress_parser" + TestCode::any_to_brace();
}

std::string tofino_shell_egress_parser_params_marker() {
    return "parser egress_parser`([^\\(]*\\()`";
}

Visitor *setup_passes() {
    auto refMap = new P4::ReferenceMap;
    auto typeMap = new P4::TypeMap;
    auto typeChecking = new BFN::TypeChecking(refMap, typeMap);
    return new PassManager{typeChecking,
                           new BFN::ArchTranslation(refMap, typeMap, BackendOptions()),
                           new BFN::InitializeMirrorIOSelect(refMap, typeMap)};
}

#define RUN_CHECK(hdr, meta, block_marker, expected)                                            \
    do {                                                                                        \
        auto blk = TestCode(hdr, InitializeMirrorIOSelectTest::tofino_egress_shell(), {meta},   \
                            block_marker, {"-T", "initialize_mirror_io_select:3"});             \
        blk.flags(Match::TrimWhiteSpace | Match::TrimAnnotations);                              \
        EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullFrontend));                              \
        EXPECT_TRUE(blk.apply_pass(InitializeMirrorIOSelectTest::setup_passes()));              \
        auto res = blk.match(expected);                                                         \
        EXPECT_TRUE(res.success) << "    @ expected[" << res.count << "], char pos=" << res.pos \
                                 << "\n"                                                        \
                                 << "    '" << expected[res.count] << "'\n"                     \
                                 << "    '" << blk.extract_code(res.pos) << "'\n";              \
    } while (0)

}  // namespace InitializeMirrorIOSelectTest

TEST(InitializeMirrorIOSelect, Tofino1ParserBody) {
    Match::CheckList expected{"state start { ",
                              "transition accept; "
                              "}"};
    RUN_CHECK(TestCode::Hdr::Tofino1arch, " ",
              InitializeMirrorIOSelectTest::tofino_shell_egress_parser_body_marker(), expected);
}

TEST(InitializeMirrorIOSelect, Tofino2ParserBody) {
    Match::CheckList expected{"state start { ",
                              "eg_intr_md_for_dprsr.mirror_io_select = 1w1; "
                              "transition accept; "
                              "}"};
    RUN_CHECK(TestCode::Hdr::Tofino2arch, " ",
              InitializeMirrorIOSelectTest::tofino_shell_egress_parser_body_marker(), expected);
}

TEST(InitializeMirrorIOSelect, Tofino2ParserBodyPartialMeta) {
    Match::CheckList expected{"state start { ",
                              "eg_intr_md_for_dprsr.mirror_io_select = 1w1; "
                              "transition accept; "
                              "}"};
    RUN_CHECK(TestCode::Hdr::Tofino2arch,
              ", out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr",
              InitializeMirrorIOSelectTest::tofino_shell_egress_parser_body_marker(), expected);
}

TEST(InitializeMirrorIOSelect, Tofino2ParserBodyCompleteMeta) {
    Match::CheckList expected{"state start { ",
                              "eg_intr_md_for_dprsr.mirror_io_select = 1w1; "
                              "transition accept; "
                              "}"};
    RUN_CHECK(TestCode::Hdr::Tofino2arch,
              ", out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, \
              out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr",
              InitializeMirrorIOSelectTest::tofino_shell_egress_parser_body_marker(), expected);
}

TEST(InitializeMirrorIOSelect, Tofino1ParserParams) {
    Match::CheckList expected{"packet_in packet, out headers_t hdr, out local_metadata_t eg_md, ",
                              "out egress_intrinsic_metadata_t eg_intr_m"};
    RUN_CHECK(TestCode::Hdr::Tofino1arch, " ",
              InitializeMirrorIOSelectTest::tofino_shell_egress_parser_params_marker(), expected);
}

TEST(InitializeMirrorIOSelect, Tofino2ParserParams) {
    Match::CheckList expected{"packet_in packet, out headers_t hdr, out local_metadata_t eg_md, ",
                              "out egress_intrinsic_metadata_t eg_intr_md, ",
                              "out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, ",
                              "out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs"};
    RUN_CHECK(TestCode::Hdr::Tofino2arch, " ",
              InitializeMirrorIOSelectTest::tofino_shell_egress_parser_params_marker(), expected);
}

TEST(InitializeMirrorIOSelect, Tofino2ParserParamsPartialMeta) {
    Match::CheckList expected{"packet_in packet, out headers_t hdr, out local_metadata_t eg_md, ",
                              "out egress_intrinsic_metadata_t eg_intr_md, ",
                              "out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, ",
                              "out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs"};
    RUN_CHECK(TestCode::Hdr::Tofino2arch,
              ", out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr",
              InitializeMirrorIOSelectTest::tofino_shell_egress_parser_params_marker(), expected);
}

TEST(InitializeMirrorIOSelect, Tofino2ParserParamsCompleteMeta) {
    Match::CheckList expected{"packet_in packet, out headers_t hdr, out local_metadata_t eg_md, ",
                              "out egress_intrinsic_metadata_t eg_intr_md, ",
                              "out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, ",
                              "out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs"};
    RUN_CHECK(TestCode::Hdr::Tofino2arch,
              ", out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr, \
              out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr",
              InitializeMirrorIOSelectTest::tofino_shell_egress_parser_params_marker(), expected);
}

// Keep definition of RUN_CHECK local for unity builds.
#undef RUN_CHECK

}  // namespace P4::Test
