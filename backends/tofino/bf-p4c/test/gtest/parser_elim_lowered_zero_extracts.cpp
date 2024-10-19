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
 * Verify that parser pad requirements are enforced correctly.
 *
 * Extra states should be added to reach minimum.
 * Error should be generated when exceeding the maximum depth
 */

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

using namespace Match;  // To remove noise & reduce line lengths.

// P4 program shell for all tests
std::string prog_shell_parser_elim_zero() {
    return R"(
    header ethernet_t {
        bit<48> dst_addr;
        bit<48> src_addr;
        bit<16> ethertype;
    }

    header hdr_t {
        bit<16> f1;
        bit<16> f2;
    };

    struct metadata_t {}

    %0% // Define struct headers_t{};

    parser ingress_parser(packet_in pkt, out headers_t hdr,
                          out metadata_t ig_md, out ingress_intrinsic_metadata_t ig_intr_md) {
        %1%
    }

    control ingress_control(inout headers_t hdr, inout metadata_t ig_md,
                            in ingress_intrinsic_metadata_t ig_intr_md,
                            in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
                            inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
                            inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
        apply {
            %2%
        }
    }

    control ingress_deparser(packet_out pkt, inout headers_t hdr, in metadata_t ig_md,
                             in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
        apply {
            pkt.emit(hdr);
        }
    }

    parser egress_parser(packet_in pkt, out headers_t hdr, out metadata_t eg_md,
                         out egress_intrinsic_metadata_t eg_intr_md) {

        state start {
            pkt.extract(eg_intr_md);
            transition accept;
        }
    }

    control egress_control(inout headers_t hdr, inout metadata_t eg_md,
                           in egress_intrinsic_metadata_t eg_intr_md,
                           in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
                           inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
                           inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
        apply {}
    }

    control egress_deparser(packet_out pkt, inout headers_t hdr, in metadata_t eg_md,
                            in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {
        apply { pkt.emit(hdr); } }

    Pipeline(ingress_parser(),
             ingress_control(),
             ingress_deparser(),
             egress_parser(),
             egress_control(),
             egress_deparser()) pipeline;
    Switch(pipeline) main;)";
}

/**
 * Find the container corresponding to a given field string
 */
std::string get_field_container(std::string field, std::string phv_section) {
    std::string field_regex = R"(\b)" + field + R"(: (\w+))";
    std::smatch sm;
    if (std::regex_search(phv_section, sm, std::regex(field_regex))) {
        return *std::next(sm.begin());
    }

    ADD_FAILURE() << "Coult not find the field " << field;
    return "";
}

/** Verify that a zero-assignment before other assignments is eliminated
 */
TEST(ParserElimLoweredZeroExtracts, ZeroBeforeNonZero) {
    std::string hdrs = R"(
        @pa_container_size("ingress", "hdr.h2.f1", 8, 8)
        @pa_container_size("ingress", "hdr.h2.f2", 8, 8)

        struct headers_t {
            ethernet_t eth;
            hdr_t h1;
            hdr_t h2;
        }
    )";

    std::string prsr = R"(
        state start {
            transition parse_ethernet;
        }

        state parse_ethernet {
            pkt.extract(hdr.eth);

            hdr.h2.setValid();
            hdr.h2.f1 = 16w0x0011;
            hdr.h2.f2 = 16w0xfdec;

            transition select(hdr.eth.ethertype[0:0]) {
                0x1 : parse_h1;
                default : accept;
            }
        }

        state parse_h1 {
            pkt.extract(hdr.h1);
            hdr.h2.f1 = 16w0x3344;
            transition accept;
        }
    )";

    std::string control = R"(
        if (hdr.h1.f1[0:0] == 1) {
            hdr.h1.f2 = hdr.h1.f1;
        }
    )";

    auto blk =
        TestCode(TestCode::Hdr::Tofino2arch, prog_shell_parser_elim_zero(), {hdrs, prsr, control});
    blk.flags(TrimWhiteSpace | TrimAnnotations);

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    // Get the container used by hdr.h2.f1[15:8]
    auto phv_sec = blk.extract_code(TestCode::CodeBlock::PhvAsm);
    std::string field_regex = R"(hdr\.h2\.f1\.8-15)";
    std::string container = get_field_container(field_regex, phv_sec);

    // Verify that a zero assignment is never found
    auto res = blk.match(TestCode::CodeBlock::ParserIAsm,
                         CheckList{"`.*`", R"(`\b" + container + ": 0\b`)"});
    EXPECT_FALSE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                              << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";
}

/**
 * Verify that a zero-assignment before other assignments is eliminated, but a zero-assignment
 * after other assignments is not
 */
TEST(ParserElimLoweredZeroExtracts, ZeroBeforeAndAfterNonZero) {
    std::string hdrs = R"(
        @pa_container_size("ingress", "hdr.h3.f1", 8, 8)
        @pa_container_size("ingress", "hdr.h3.f2", 8, 8)

        struct headers_t {
            ethernet_t eth;
            hdr_t h1;
            hdr_t h2;
            hdr_t h3;
        }
    )";

    std::string prsr = R"(
        state start {
            transition parse_ethernet;
        }

        state parse_ethernet {
            pkt.extract(hdr.eth);

            hdr.h3.setValid();
            hdr.h3.f1 = 16w0x0011;
            hdr.h3.f2 = 16w0xfdec;

            transition select(hdr.eth.ethertype[0:0]) {
                0x1 : parse_h1;
                default : accept;
            }
        }

        state parse_h1 {
            pkt.extract(hdr.h1);
            hdr.h3.f1 = 16w0x3344;
            transition select(hdr.eth.ethertype[1:1]) {
                0x1 : parse_h2;
                default : accept;
            }
        }

        state parse_h2 {
            pkt.extract(hdr.h2);
            hdr.h3.f1 = 16w0x0055;
            transition accept;
        }
    )";

    std::string control = R"(
        if (hdr.h1.f1[0:0] == 1) {
            hdr.h1.f2 = hdr.h1.f1;
        }
    )";

    auto blk =
        TestCode(TestCode::Hdr::Tofino2arch, prog_shell_parser_elim_zero(), {hdrs, prsr, control});
    blk.flags(TrimWhiteSpace | TrimAnnotations);

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    // Get the container used by hdr.h3.f1[15:8]
    auto phv_sec = blk.extract_code(TestCode::CodeBlock::PhvAsm);
    std::string field_regex = R"(hdr\.h3\.f1\.8-15)";
    std::string container = get_field_container(field_regex, phv_sec);

    // Excpect one zero-assignment but not two
    auto res = blk.match(TestCode::CodeBlock::ParserIAsm,
                         CheckList{"`.*`", R"(`\b)" + container + R"(: 0\b`)"});
    EXPECT_TRUE(res.success) << " -- Looking for 1 x zero-extract:" << " pos=" << res.pos
                             << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";

    res = blk.match(TestCode::CodeBlock::ParserIAsm,
                    CheckList{"`.*`", R"(`\bMB2: 0\b`)", "`.*`", R"(`\bMB2: 0\b`)"});
    EXPECT_FALSE(res.success) << " -- Looking for 2 x zero-extracts:" << " pos=" << res.pos
                              << " count=" << res.count << "\n'"
                              << blk.extract_code(TestCode::CodeBlock::ParserIAsm) << "'\n";
}

}  // namespace P4::Test
