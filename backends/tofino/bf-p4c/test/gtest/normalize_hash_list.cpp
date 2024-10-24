/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Verify hash list normalization
 */

#include <boost/algorithm/string/replace.hpp>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

using namespace Match;  // To remove noise & reduce line lengths.

namespace NormalizeHashList {

std::string prog() {
    return R"(
header skipBlob_h {
    bit<32> skip;
}

struct myHeaders {
  skipBlob_h skip;
  skipBlob_h skip2;
  skipBlob_h skip3;
}

struct portMetadata_t {
    bit<8> skipKey;
}

struct metadata {
    portMetadata_t portMetadata;
}

struct emetadata {}

parser IngressParser(packet_in pkt,
    out myHeaders hdr,
    out metadata meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t ig_intr_md)
{
    state start {
        pkt.extract(ig_intr_md);
        meta.portMetadata = port_metadata_unpack<portMetadata_t>(pkt);
        pkt.extract(hdr.skip);
        pkt.extract(hdr.skip2);
        pkt.extract(hdr.skip3);
        transition accept;
    }
}

control Ingress(
    inout myHeaders hdr,
    inout metadata meta,
    in ingress_intrinsic_metadata_t ig_intr_md,
    in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{
    bit<32> tmp = 0;



    Hash<bit<32>>(HashAlgorithm_t.CRC32) hasher;
    apply {
      if(hdr.skip.isValid()) {
        bit<32> increaseEntropy = hdr.skip.skip & hdr.skip2.skip;
        hdr.skip3.skip = hasher.get({hdr.skip2.skip,hdr.skip.skip,increaseEntropy});
      }
      ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
      ig_dprsr_md.drop_ctl = 0;
    }
}

control IngressDeparser(
    packet_out pkt,
    inout myHeaders headers,
    in metadata meta,
    in ingress_intrinsic_metadata_for_deparser_t ingressDeparserMetadata
) {
    apply {
        pkt.emit(headers);
    }
}

struct my_egress_headers_t {
}

    /********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_metadata_t {
}

    /***********************  P A R S E R  **************************/

parser EgressParser(packet_in pkt,
    /* User */
    out my_egress_headers_t hdr,
    out my_egress_metadata_t meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t eg_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }
}

    /***************** M A T C H - A C T I O N  *********************/

control Egress(
    /* User */
    inout my_egress_headers_t hdr,
    inout my_egress_metadata_t meta,
    /* Intrinsic */
    in egress_intrinsic_metadata_t eg_intr_md,
    in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_oport_md)
{
    apply {
    }
}

    /*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out pkt,
    /* User */
    inout my_egress_headers_t hdr,
    in my_egress_metadata_t meta,
    /* Intrinsic */
    in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md)
{
    apply {
        pkt.emit(hdr);
    }
}


/************ F I N A L   P A C K A G E ******************************/
Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;

    )";
}

}  // namespace NormalizeHashList

/** Verify that initialization is correctly inserted when we have a nested statement,
 * such as a hash.get inside of an if statement.
 */
TEST(NormalizeHashList, NestedStatements) {
    auto blk = TestCode(TestCode::Hdr::Tofino1arch, NormalizeHashList::prog(), {});
    blk.flags(TrimWhiteSpace | TrimAnnotations);

    // Disable alt phv
    BackendOptions().alt_phv_alloc = false;

    EXPECT_TRUE(blk.CreateBackend());
    EXPECT_TRUE(blk.apply_pass(TestCode::Pass::FullBackend));

    // Get the container for $hash_field_argument0 and verify that there is it
    // is assigned via an AND instruction.
    auto phv_asm = blk.extract_code(TestCode::CodeBlock::PhvAsm);
    auto hash_field_arg = blk.get_field_container("$hash_field_argument0", phv_asm);

    auto res =
        blk.match(TestCode::CodeBlock::MauAsm, CheckList{"`.*`", "and " + hash_field_arg + ","});
    EXPECT_TRUE(res.success) << " pos=" << res.pos << " count=" << res.count << "\n'"
                             << blk.extract_code(TestCode::CodeBlock::MauAsm) << "'\n";
}

}  // namespace P4::Test
