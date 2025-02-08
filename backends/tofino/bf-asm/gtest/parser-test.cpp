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

#include <gtest/gtest.h>

#include "backends/tofino/bf-asm/bfas.h"
#include "backends/tofino/bf-asm/jbay/parser-tofino-jbay.h"

namespace {

// TEST(parser_test, get_parser_deepest_depth)
//
//
// While calculating the maximum depth, the assembler goes through the parser tree
// and visits every state recursively.  The parser depth for a state is taken into account
// and included in the calculation at the time it is visited.
//
// Every state used to be visited at most one time, which was the source of the problem:
//
//     In cases where parsing trees contained states that were called from more than one
//     parent state, the depth calculation would be wrong unless the depth was at its maximum
//     value the first time that state was visited.
//
// Made a change in the parse depth calculation to keep track of the largest parser depth
// "seen" for each state.  When a state has already been visited, the recursion continues
// when the current parser depth is larger than the largest parser depth seen up to that
// point for that state.

// The parser code provided in parser_str below contains that behavior as parse_udp and
// parse_tcp are called from both parse_ipv4 and parse_ipv6, two states with different depths,
// the longest one being parse_ipv6 that is visited after parse_ipv4.  Without the fix,
// parser->get_prsr_max_dph() returns 6 instead of 7.
//
TEST(parser_test, get_parser_deepest_depth) {
    const char *parser_str = R"PARSER_CFG(
version:
  target: Tofino
parser egress:
  start: $entry_point
  init_zero: [ B19, B18, B16 ]
  bitwise_or: [ B16, B18 ]
  hdr_len_adj: 27
  meta_opt: 8191
  states:
    $entry_point:
      *:
        load: { byte1 : 27 }
        buf_req: 28
        next: start
    start:
      match: [ byte1 ]
      0x0a:
        counter:
          imm: 38
        0..1: H16  # bit[7..15] -> H16 bit[8..0]: egress::eg_intr_md.egress_port
        intr_md: 9
        shift: 27
        buf_req: 27
        next: parse_mirror_tagging_state
      0x**:
        counter:
          imm: 38
        0..1: H16  # bit[7..15] -> H16 bit[8..0]: egress::eg_intr_md.egress_port
        intr_md: 9
        shift: 27
        buf_req: 27
        next: parse_normal_tagging_state
    parse_mirror_tagging_state:
      *:
        counter: dec 1
        B19: 10  # value 10 -> B19 bit[7..0]: egress::eg_md.packet_state
        load: { half : 13..14 }
        shift: 1
        buf_req: 15
        next: parse_ethernet
    parse_ethernet:
      match: [ half ]
      0x0800:
        counter: dec 14
        0..1: TH32  # egress::hdr.ethernet.dst_addr[47:32].32-47
        2..5: TW19  # egress::hdr.ethernet.dst_addr[31:0].0-31
        6..7: TH31  # egress::hdr.ethernet.src_addr[47:32].32-47
        8..11: TW18  # egress::hdr.ethernet.src_addr[31:0].0-31
        12..13: TH30  # egress::hdr.ethernet.ether_type
        B18: 1  # value 1 -> B18 bit[0]: egress::hdr.ethernet.$valid
        load: { byte1 : 23 }
        shift: 14
        buf_req: 24
        next: parse_ipv4
      0x86dd:
        counter: dec 14
        0..1: TH32  # egress::hdr.ethernet.dst_addr[47:32].32-47
        2..5: TW19  # egress::hdr.ethernet.dst_addr[31:0].0-31
        6..7: TH31  # egress::hdr.ethernet.src_addr[47:32].32-47
        8..11: TW18  # egress::hdr.ethernet.src_addr[31:0].0-31
        12..13: TH30  # egress::hdr.ethernet.ether_type
        B18: 1  # value 1 -> B18 bit[0]: egress::hdr.ethernet.$valid
        shift: 14
        buf_req: 14
        next: parse_ipv6
      0x****:
        counter: dec 14
        0..1: TH32  # egress::hdr.ethernet.dst_addr[47:32].32-47
        2..5: TW19  # egress::hdr.ethernet.dst_addr[31:0].0-31
        6..7: TH31  # egress::hdr.ethernet.src_addr[47:32].32-47
        8..11: TW18  # egress::hdr.ethernet.src_addr[31:0].0-31
        12..13: TH30  # egress::hdr.ethernet.ether_type
        B18: 1  # value 1 -> B18 bit[0]: egress::hdr.ethernet.$valid
        shift: 14
        buf_req: 14
        next: min_parse_depth_accept_initial
    parse_ipv4:
      match: [ byte1 ]
      0x06:
        counter: dec 20
        0..3: TW4
            # - bit[0..3] -> TW4 bit[31..28]: egress::hdr.ipv4.version
            # - bit[4..7] -> TW4 bit[27..24]: egress::hdr.ipv4.ihl
            # - bit[8..15] -> TW4 bit[23..16]: egress::hdr.ipv4.diffserv
            # - bit[16..31] -> TW4 bit[15..0]: egress::hdr.ipv4.total_len
        4..7: TW6
            # - bit[32..47] -> TW6 bit[31..16]: egress::hdr.ipv4.identification
            # - bit[48..50] -> TW6 bit[15..13]: egress::hdr.ipv4.flags
            # - bit[51..63] -> TW6 bit[12..0]: egress::hdr.ipv4.frag_offset
        8..11: TW5
            # - bit[64..71] -> TW5 bit[31..24]: egress::hdr.ipv4.ttl
            # - bit[72..79] -> TW5 bit[23..16]: egress::hdr.ipv4.protocol
            # - bit[80..95] -> TW5 bit[15..0]: egress::hdr.ipv4.hdr_checksum
        12..13: TH27  # egress::hdr.ipv4.src_addr[31:16].16-31
        14..15: TH26  # egress::hdr.ipv4.src_addr[15:0].0-15
        16..17: TH25  # egress::hdr.ipv4.dst_addr[31:16].16-31
        18..19: TH24  # egress::hdr.ipv4.dst_addr[15:0].0-15
        B18: 2  # value 1 -> B18 bit[1]: egress::hdr.ipv4.$valid
        load: { half : 22..23 }
        shift: 20
        buf_req: 24
        next: parse_tcp
      0x11:
        counter: dec 20
        0..3: TW4
            # - bit[0..3] -> TW4 bit[31..28]: egress::hdr.ipv4.version
            # - bit[4..7] -> TW4 bit[27..24]: egress::hdr.ipv4.ihl
            # - bit[8..15] -> TW4 bit[23..16]: egress::hdr.ipv4.diffserv
            # - bit[16..31] -> TW4 bit[15..0]: egress::hdr.ipv4.total_len
        4..7: TW6
            # - bit[32..47] -> TW6 bit[31..16]: egress::hdr.ipv4.identification
            # - bit[48..50] -> TW6 bit[15..13]: egress::hdr.ipv4.flags
            # - bit[51..63] -> TW6 bit[12..0]: egress::hdr.ipv4.frag_offset
        8..11: TW5
            # - bit[64..71] -> TW5 bit[31..24]: egress::hdr.ipv4.ttl
            # - bit[72..79] -> TW5 bit[23..16]: egress::hdr.ipv4.protocol
            # - bit[80..95] -> TW5 bit[15..0]: egress::hdr.ipv4.hdr_checksum
        12..13: TH27  # egress::hdr.ipv4.src_addr[31:16].16-31
        14..15: TH26  # egress::hdr.ipv4.src_addr[15:0].0-15
        16..17: TH25  # egress::hdr.ipv4.dst_addr[31:16].16-31
        18..19: TH24  # egress::hdr.ipv4.dst_addr[15:0].0-15
        B18: 2  # value 1 -> B18 bit[1]: egress::hdr.ipv4.$valid
        load: { half : 20..21 }
        shift: 20
        buf_req: 22
        next: parse_udp
      0x**:
        counter: dec 20
        0..3: TW4
            # - bit[0..3] -> TW4 bit[31..28]: egress::hdr.ipv4.version
            # - bit[4..7] -> TW4 bit[27..24]: egress::hdr.ipv4.ihl
            # - bit[8..15] -> TW4 bit[23..16]: egress::hdr.ipv4.diffserv
            # - bit[16..31] -> TW4 bit[15..0]: egress::hdr.ipv4.total_len
        4..7: TW6
            # - bit[32..47] -> TW6 bit[31..16]: egress::hdr.ipv4.identification
            # - bit[48..50] -> TW6 bit[15..13]: egress::hdr.ipv4.flags
            # - bit[51..63] -> TW6 bit[12..0]: egress::hdr.ipv4.frag_offset
        8..11: TW5
            # - bit[64..71] -> TW5 bit[31..24]: egress::hdr.ipv4.ttl
            # - bit[72..79] -> TW5 bit[23..16]: egress::hdr.ipv4.protocol
            # - bit[80..95] -> TW5 bit[15..0]: egress::hdr.ipv4.hdr_checksum
        12..13: TH27  # egress::hdr.ipv4.src_addr[31:16].16-31
        14..15: TH26  # egress::hdr.ipv4.src_addr[15:0].0-15
        16..17: TH25  # egress::hdr.ipv4.dst_addr[31:16].16-31
        18..19: TH24  # egress::hdr.ipv4.dst_addr[15:0].0-15
        B18: 2  # value 1 -> B18 bit[1]: egress::hdr.ipv4.$valid
        shift: 20
        buf_req: 20
        next: min_parse_depth_accept_initial
    parse_tcp:
      match: [ half ]
      0x0050:
        counter: dec 20
        0..1: TH8  # egress::hdr.tcp.src_port
        2..3: TH7  # egress::hdr.tcp.dst_port
        4..7: TW17  # egress::hdr.tcp.seq_no
        8..11: TW16  # egress::hdr.tcp.ack_no
        12: TB5
            # - bit[96..99] -> TB5 bit[7..4]: egress::hdr.tcp.data_offset
            # - bit[100..103] -> TB5 bit[3..0]: egress::hdr.tcp.res
        13: TB6  # egress::hdr.tcp.flags
        14..15: TH6  # egress::hdr.tcp.window
        16..19: TW7
            # - bit[128..143] -> TW7 bit[31..16]: egress::hdr.tcp.checksum
            # - bit[144..159] -> TW7 bit[15..0]: egress::hdr.tcp.urgent_ptr
        B18: 4  # value 1 -> B18 bit[2]: egress::hdr.tcp.$valid
        shift: 20
        buf_req: 20
        next: parse_app
      0x01bb:
        counter: dec 20
        0..1: TH8  # egress::hdr.tcp.src_port
        2..3: TH7  # egress::hdr.tcp.dst_port
        4..7: TW17  # egress::hdr.tcp.seq_no
        8..11: TW16  # egress::hdr.tcp.ack_no
        12: TB5
            # - bit[96..99] -> TB5 bit[7..4]: egress::hdr.tcp.data_offset
            # - bit[100..103] -> TB5 bit[3..0]: egress::hdr.tcp.res
        13: TB6  # egress::hdr.tcp.flags
        14..15: TH6  # egress::hdr.tcp.window
        16..19: TW7
            # - bit[128..143] -> TW7 bit[31..16]: egress::hdr.tcp.checksum
            # - bit[144..159] -> TW7 bit[15..0]: egress::hdr.tcp.urgent_ptr
        B18: 4  # value 1 -> B18 bit[2]: egress::hdr.tcp.$valid
        shift: 20
        buf_req: 20
        next: parse_app
      0x15b3:
        counter: dec 20
        0..1: TH8  # egress::hdr.tcp.src_port
        2..3: TH7  # egress::hdr.tcp.dst_port
        4..7: TW17  # egress::hdr.tcp.seq_no
        8..11: TW16  # egress::hdr.tcp.ack_no
        12: TB5
            # - bit[96..99] -> TB5 bit[7..4]: egress::hdr.tcp.data_offset
            # - bit[100..103] -> TB5 bit[3..0]: egress::hdr.tcp.res
        13: TB6  # egress::hdr.tcp.flags
        14..15: TH6  # egress::hdr.tcp.window
        16..19: TW7
            # - bit[128..143] -> TW7 bit[31..16]: egress::hdr.tcp.checksum
            # - bit[144..159] -> TW7 bit[15..0]: egress::hdr.tcp.urgent_ptr
        B18: 4  # value 1 -> B18 bit[2]: egress::hdr.tcp.$valid
        shift: 20
        buf_req: 20
        next: parse_recirculation
      0x****:
        counter: dec 20
        0..1: TH8  # egress::hdr.tcp.src_port
        2..3: TH7  # egress::hdr.tcp.dst_port
        4..7: TW17  # egress::hdr.tcp.seq_no
        8..11: TW16  # egress::hdr.tcp.ack_no
        12: TB5
            # - bit[96..99] -> TB5 bit[7..4]: egress::hdr.tcp.data_offset
            # - bit[100..103] -> TB5 bit[3..0]: egress::hdr.tcp.res
        13: TB6  # egress::hdr.tcp.flags
        14..15: TH6  # egress::hdr.tcp.window
        16..19: TW7
            # - bit[128..143] -> TW7 bit[31..16]: egress::hdr.tcp.checksum
            # - bit[144..159] -> TW7 bit[15..0]: egress::hdr.tcp.urgent_ptr
        B18: 4  # value 1 -> B18 bit[2]: egress::hdr.tcp.$valid
        shift: 20
        buf_req: 20
        next: end
    parse_app:
      *:
        counter: dec 1
        0: TB4  # egress::hdr.app.byte
        B18: 8  # value 1 -> B18 bit[3]: egress::hdr.app.$valid
        shift: 1
        buf_req: 1
        next: end
    parse_recirculation:
      *:
        counter: dec 3
        0: B17  # egress::hdr.recir.packet_state
        1..2: TH33  # egress::hdr.recir.pattern_state_machine_state
        B18: 16  # value 1 -> B18 bit[4]: egress::hdr.recir.$valid
        shift: 3
        buf_req: 3
        next: parse_app
    parse_udp:
      match: [ half ]
      0x0035:
        counter: dec 8
        0..1: TH7  # egress::hdr.udp.src_port
        2..3: TH6  # egress::hdr.udp.dst_port
        4..7: TW7
            # - bit[32..47] -> TW7 bit[31..16]: egress::hdr.udp.hdr_length
            # - bit[48..63] -> TW7 bit[15..0]: egress::hdr.udp.checksum
        B18: 32  # value 1 -> B18 bit[5]: egress::hdr.udp.$valid
        shift: 8
        buf_req: 8
        next: parse_app
      0x15b3:
        counter: dec 8
        0..1: TH7  # egress::hdr.udp.src_port
        2..3: TH6  # egress::hdr.udp.dst_port
        4..7: TW7
            # - bit[32..47] -> TW7 bit[31..16]: egress::hdr.udp.hdr_length
            # - bit[48..63] -> TW7 bit[15..0]: egress::hdr.udp.checksum
        B18: 32  # value 1 -> B18 bit[5]: egress::hdr.udp.$valid
        shift: 8
        buf_req: 8
        next: parse_recirculation
      0x****:
        counter: dec 8
        0..1: TH7  # egress::hdr.udp.src_port
        2..3: TH6  # egress::hdr.udp.dst_port
        4..7: TW7
            # - bit[32..47] -> TW7 bit[31..16]: egress::hdr.udp.hdr_length
            # - bit[48..63] -> TW7 bit[15..0]: egress::hdr.udp.checksum
        B18: 32  # value 1 -> B18 bit[5]: egress::hdr.udp.$valid
        shift: 8
        buf_req: 8
        next: end
    min_parse_depth_accept_initial:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB4  # egress::hdr.prsr_pad_0[0].blob[87:80].80-87
        1..2: TH28  # egress::hdr.prsr_pad_0[0].blob[79:64].64-79
        3..4: TH7  # egress::hdr.prsr_pad_0[0].blob[63:48].48-63
        5..6: TH6  # egress::hdr.prsr_pad_0[0].blob[47:32].32-47
        7..10: TW7  # egress::hdr.prsr_pad_0[0].blob[31:0].0-31
        B16: 4  # value 4 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 11
        buf_req: 11
        next: min_parse_depth_accept_loop.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB5  # egress::hdr.prsr_pad_0[1].blob[87:80].80-87
        1..2: TH29  # egress::hdr.prsr_pad_0[1].blob[79:64].64-79
        3..4: TH11  # egress::hdr.prsr_pad_0[1].blob[63:48].48-63
        5..6: TH10  # egress::hdr.prsr_pad_0[1].blob[47:32].32-47
        7..8: TH9  # egress::hdr.prsr_pad_0[1].blob[31:16].16-31
        B16: 2  # value 2 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 9
        buf_req: 9
        next: min_parse_depth_accept_loop.$it1.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$it1.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        0..1: TH8  # egress::hdr.prsr_pad_0[1].blob[15:0].0-15
        shift: 2
        buf_req: 2
        next: min_parse_depth_accept_loop.$it2
      0b**:
        0..1: TH8  # egress::hdr.prsr_pad_0[1].blob[15:0].0-15
        shift: 2
        buf_req: 2
        next: end
    min_parse_depth_accept_loop.$it2:
      *:
        counter: dec 11
        0: TB6  # egress::hdr.prsr_pad_0[2].blob[87:80].80-87
        1: TB16  # egress::hdr.prsr_pad_0[2].blob[79:72].72-79
        2: TB7  # egress::hdr.prsr_pad_0[2].blob[71:64].64-71
        3..6: TW17  # egress::hdr.prsr_pad_0[2].blob[63:32].32-63
        7..10: TW16  # egress::hdr.prsr_pad_0[2].blob[31:0].0-31
        B16: 1  # value 1 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 11
        buf_req: 11
        next: min_parse_depth_accept_loop.$it2.$split_0
    min_parse_depth_accept_loop.$it2.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        buf_req: 0
        next: end
      0b**:
        buf_req: 0
        next: end
    parse_ipv6:
      *:
        counter: dec 40
        0..3: TW5
            # - bit[0..3] -> TW5 bit[31..28]: egress::hdr.ipv6.version
            # - bit[4..11] -> TW5 bit[27..20]: egress::hdr.ipv6.traffic_class
            # - bit[12..31] -> TW5 bit[19..0]: egress::hdr.ipv6.flow_label
        4..7: TW4
            # - bit[32..47] -> TW4 bit[31..16]: egress::hdr.ipv6.payload_len
            # - bit[48..55] -> TW4 bit[15..8]: egress::hdr.ipv6.next_hdr
            # - bit[56..63] -> TW4 bit[7..0]: egress::hdr.ipv6.hop_limit
        8..11: TW21  # egress::hdr.ipv6.src_addr[127:96].96-127
        12..15: TW20  # egress::hdr.ipv6.src_addr[95:64].64-95
        16: TB16  # egress::hdr.ipv6.src_addr[63:56].56-63
        17: TB7  # egress::hdr.ipv6.src_addr[55:48].48-55
        18..19: TH29  # egress::hdr.ipv6.src_addr[47:32].32-47
        20..21: TH28  # egress::hdr.ipv6.src_addr[31:16].16-31
        22..23: TH27  # egress::hdr.ipv6.src_addr[15:0].0-15
        24..25: TH26  # egress::hdr.ipv6.dst_addr[127:112].112-127
        B18: 64  # value 1 -> B18 bit[6]: egress::hdr.ipv6.$valid
        load: { byte1 : 6 }
        shift: 26
        buf_req: 26
        next: parse_ipv6.$split_0
    parse_ipv6.$split_0:
      *:
        0..1: TH25  # egress::hdr.ipv6.dst_addr[111:96].96-111
        2..3: TH24  # egress::hdr.ipv6.dst_addr[95:80].80-95
        4..5: TH11  # egress::hdr.ipv6.dst_addr[79:64].64-79
        6..7: TH10  # egress::hdr.ipv6.dst_addr[63:48].48-63
        10..13: TW6  # egress::hdr.ipv6.dst_addr[31:0].0-31
        shift: 8
        buf_req: 14
        next: parse_ipv6.$split_1
    parse_ipv6.$split_1:
      match: [ byte1 ]
      0x06:
        0..1: TH9  # egress::hdr.ipv6.dst_addr[47:32].32-47
        load: { half : 8..9 }
        shift: 6
        buf_req: 10
        next: parse_tcp
      0x11:
        0..1: TH9  # egress::hdr.ipv6.dst_addr[47:32].32-47
        load: { half : 6..7 }
        shift: 6
        buf_req: 8
        next: parse_udp
      0x**:
        0..1: TH9  # egress::hdr.ipv6.dst_addr[47:32].32-47
        shift: 6
        buf_req: 6
        next: end
    parse_normal_tagging_state:
      *:
        B19: 1  # value 1 -> B19 bit[7..0]: egress::eg_md.packet_state
        load: { half : 12..13 }
        buf_req: 14
        next: parse_ethernet
)PARSER_CFG";

    options.target = NO_TARGET;
    Phv::test_clear();

    createSingleAsmParser();
    AsmParser *asm_parser = dynamic_cast<AsmParser *>(::asm_parser);
    asm_parse_string(parser_str);
    std::vector<Parser *> parser_vector = asm_parser->test_get_parser(EGRESS);
    EXPECT_GT(parser_vector.size(), 0);
    Parser *parser = parser_vector.back();
    parser->process();
    EXPECT_EQ(parser->get_prsr_max_dph(), 4);
}

// TEST(parser_test, get_parser_deepest_depth_loop_no_stack)
//
//           verify that parser with loops that do not store into
//           header stacks are supported and that the parser max
//           depth is set to the maximum supported by the target.
//
TEST(parser_test, get_parser_depth_loop_no_stack) {
    const char *parser_str = R"PARSER_CFG(
version:
  target: Tofino
parser egress:
  start: $entry_point.start
  init_zero: [ B17, B16 ]
  bitwise_or: [ B16, B17 ]
  hdr_len_adj: 27
  meta_opt: 8191
  states:
    $entry_point.start:
      *:
        counter:
          imm: 65
        0..1: H16  # bit[7..15] -> H16 bit[8..0]: egress::eg_intr_md.egress_port
        27..28: TH14  # egress::hdr.ether.dstAddr[47:32].32-47
        B17: 1  # value 1 -> B17 bit[0]: egress::hdr.ether.$valid
        intr_md: 9
        shift: 29
        buf_req: 29
        next: $entry_point.start.$split_0
    $entry_point.start.$split_0:
      *:
        counter: dec 27
        0..3: TW5  # egress::hdr.ether.dstAddr[31:0].0-31
        4..5: TH13  # egress::hdr.ether.srcAddr[47:32].32-47
        6..9: TW4  # egress::hdr.ether.srcAddr[31:0].0-31
        10..11: TH12  # egress::hdr.ether.etherType
        load: { half : 10..11 }
        shift: 12
        buf_req: 12
        next: $entry_point.start.$split_1
    $entry_point.start.$split_1:
      *:
        counter: dec 14
        buf_req: 0
        next: L3_start_0
    L3_start_0:
      match: [ half ]
      0x0800:
        counter: dec 1
        0: TB4  # egress::hdr.h.a
        B17: 2  # value 1 -> B17 bit[1]: egress::hdr.h.$valid
        shift: 1
        buf_req: 1
        next: min_parse_depth_accept_initial
      0x8100:
        counter: dec 2
        0: TB9  # egress::hdr.i.etherType[15:8].8-15
        1: TB8  # egress::hdr.i.etherType[7:0].0-7
        B17: 4  # value 1 -> B17 bit[2]: egress::hdr.i.$valid
        shift: 2
        buf_req: 2
        next: L3_start_0
      0x****:
        buf_req: 0
        next: min_parse_depth_accept_initial
    min_parse_depth_accept_initial:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB5  # egress::hdr.prsr_pad_0[0].blob[87:80].80-87
        1..2: TH15  # egress::hdr.prsr_pad_0[0].blob[79:64].64-79
        3..6: TW7  # egress::hdr.prsr_pad_0[0].blob[63:32].32-63
        7..10: TW6  # egress::hdr.prsr_pad_0[0].blob[31:0].0-31
        B16: 4  # value 4 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 11
        buf_req: 11
        next: min_parse_depth_accept_loop.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB6  # egress::hdr.prsr_pad_0[1].blob[87:80].80-87
        1..2: TH16  # egress::hdr.prsr_pad_0[1].blob[79:64].64-79
        3..4: TH9  # egress::hdr.prsr_pad_0[1].blob[63:48].48-63
        5..6: TH8  # egress::hdr.prsr_pad_0[1].blob[47:32].32-47
        7..8: TH7  # egress::hdr.prsr_pad_0[1].blob[31:16].16-31
        B16: 2  # value 2 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 9
        buf_req: 9
        next: min_parse_depth_accept_loop.$it1.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$it1.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        0..1: TH6  # egress::hdr.prsr_pad_0[1].blob[15:0].0-15
        shift: 2
        buf_req: 2
        next: min_parse_depth_accept_loop.$it2
      0b**:
        0..1: TH6  # egress::hdr.prsr_pad_0[1].blob[15:0].0-15
        shift: 2
        buf_req: 2
        next: end
    min_parse_depth_accept_loop.$it2:
      *:
        counter: dec 11
        0: TB7  # egress::hdr.prsr_pad_0[2].blob[87:80].80-87
        1..2: TH17  # egress::hdr.prsr_pad_0[2].blob[79:64].64-79
        3..6: TW8  # egress::hdr.prsr_pad_0[2].blob[63:32].32-63
        7..8: TH11  # egress::hdr.prsr_pad_0[2].blob[31:16].16-31
        9..10: TH10  # egress::hdr.prsr_pad_0[2].blob[15:0].0-15
        B16: 1  # value 1 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 11
        buf_req: 11
        next: min_parse_depth_accept_loop.$it2.$split_0
    min_parse_depth_accept_loop.$it2.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        buf_req: 0
        next: end
      0b**:
        buf_req: 0
        next: end
)PARSER_CFG";

    options.target = NO_TARGET;
    Phv::test_clear();

    createSingleAsmParser();
    AsmParser *asm_parser = dynamic_cast<AsmParser *>(::asm_parser);
    asm_parse_string(parser_str);
    std::vector<Parser *> parser_vector = asm_parser->test_get_parser(EGRESS);
    EXPECT_GT(parser_vector.size(), 0);
    Parser *parser = parser_vector.back();
    parser->process();
    EXPECT_EQ(parser->get_prsr_max_dph(), 0x3ff - 1);
}

// TEST(parser_test, get_parser_depth_loop_with_stack)
//
//           verify that when a parser has loops that store into header
//           stacks, that the max parser depth is set according to the
//           number of entries in the stack.
//
TEST(parser_test, get_parser_depth_loop_with_stack) {
    const char *parser_str = R"PARSER_CFG(
version:
  target: Tofino
phv egress:
  eg_intr_md.egress_port: H17(0..8)
  hdr.vlan$0.pcp: TW0(29..31)
  hdr.vlan$0.dei: TW0(28)
  hdr.vlan$0.vid: TW0(16..27)
  hdr.vlan$0.ether_type: TW0(0..15)
  hdr.vlan$1.pcp: TW1(29..31)
  hdr.vlan$1.dei: TW1(28)
  hdr.vlan$1.vid: TW1(16..27)
  hdr.vlan$1.ether_type: TW1(0..15)
  hdr.vlan$2.pcp: TW2(29..31)
  hdr.vlan$2.dei: TW2(28)
  hdr.vlan$2.vid: TW2(16..27)
  hdr.vlan$2.ether_type: TW2(0..15)
  hdr.vlan$3.pcp: TW3(29..31)
  hdr.vlan$3.dei: TW3(28)
  hdr.vlan$3.vid: TW3(16..27)
  hdr.vlan$3.ether_type: TW3(0..15)
  hdr.vlan$4.pcp: TH1(13..15)
  hdr.vlan$4.dei: TH1(12)
  hdr.vlan$4.vid: TH1(0..11)
  hdr.vlan$4.ether_type: TH0
  hdr.vlan$5.pcp: TH3(13..15)
  hdr.vlan$5.dei: TH3(12)
  hdr.vlan$5.vid: TH3(0..11)
  hdr.vlan$5.ether_type: TH2
  hdr.vlan$6.pcp: TH5(13..15)
  hdr.vlan$6.dei: TH5(12)
  hdr.vlan$6.vid: TH5(0..11)
  hdr.vlan$6.ether_type: TH4
  hdr.vlan$7.pcp: TW12(29..31)
  hdr.vlan$7.dei: TW12(28)
  hdr.vlan$7.vid: TW12(16..27)
  hdr.vlan$7.ether_type: TW12(0..15)
  hdr.vlan$8.pcp: TW13(29..31)
  hdr.vlan$8.dei: TW13(28)
  hdr.vlan$8.vid: TW13(16..27)
  hdr.vlan$8.ether_type: TW13(0..15)
  hdr.vlan$9.pcp: TW14(29..31)
  hdr.vlan$9.dei: TW14(28)
  hdr.vlan$9.vid: TW14(16..27)
  hdr.vlan$9.ether_type: TW14(0..15)
  hdr.vlan$10.pcp: TW15(29..31)
  hdr.vlan$10.dei: TW15(28)
  hdr.vlan$10.vid: TW15(16..27)
  hdr.vlan$10.ether_type: TW15(0..15)
  hdr.vlan$11.pcp: TH19(13..15)
  hdr.vlan$11.dei: TH19(12)
  hdr.vlan$11.vid: TH19(0..11)
  hdr.vlan$11.ether_type: TH18
  hdr.vlan$12.pcp: TH21(13..15)
  hdr.vlan$12.dei: TH21(12)
  hdr.vlan$12.vid: TH21(0..11)
  hdr.vlan$12.ether_type: TH20
  hdr.vlan$13.pcp: TH23(13..15)
  hdr.vlan$13.dei: TH23(12)
  hdr.vlan$13.vid: TH23(0..11)
  hdr.vlan$13.ether_type: TH22
  hdr.vlan$14.pcp: TB13(5..7)
  hdr.vlan$14.dei: TB13(4)
  hdr.vlan$14.vid.0-7: TB14
  hdr.vlan$14.vid.8-11: TB13(0..3)
  hdr.vlan$14.ether_type.0-7: TB3
  hdr.vlan$14.ether_type.8-15: TB12
  hdr.prsr_pad_0$0.blob.0-31: TW20
  hdr.prsr_pad_0$0.blob.32-63: TW21
  hdr.prsr_pad_0$0.blob.64-79: TH36
  hdr.prsr_pad_0$0.blob.80-87: TB0
  hdr.prsr_pad_0$1.blob.0-31: TW22
  hdr.prsr_pad_0$1.blob.32-63: TW23
  hdr.prsr_pad_0$1.blob.64-79: TH37
  hdr.prsr_pad_0$1.blob.80-87: TB1
  hdr.prsr_pad_0$2.blob.0-15: TH30
  hdr.prsr_pad_0$2.blob.16-31: TH31
  hdr.prsr_pad_0$2.blob.32-47: TH32
  hdr.prsr_pad_0$2.blob.48-63: TH33
  hdr.prsr_pad_0$2.blob.64-79: TH38
  hdr.prsr_pad_0$2.blob.80-87: TB2
  hdr.eth.dst_addr.0-7: TB15
  hdr.eth.dst_addr.8-15: TB20
  hdr.eth.dst_addr.16-23: TB21
  hdr.eth.dst_addr.24-31: TB22
  hdr.eth.dst_addr.32-47: TH41
  hdr.eth.src_addr.0-15: TH34
  hdr.eth.src_addr.16-31: TH35
  hdr.eth.src_addr.32-47: TH40
  hdr.eth.ethertype: TH39
  hdr.eth.$valid: B17(0)
  hdr.vlan.$stkvalid: H16(0..14)
  hdr.vlan$0.$valid: H16(14)
  hdr.vlan$1.$valid: H16(13)
  hdr.vlan$2.$valid: H16(12)
  hdr.vlan$3.$valid: H16(11)
  hdr.vlan$4.$valid: H16(10)
  hdr.vlan$5.$valid: H16(9)
  hdr.vlan$6.$valid: H16(8)
  hdr.vlan$7.$valid: H16(7)
  hdr.vlan$8.$valid: H16(6)
  hdr.vlan$9.$valid: H16(5)
  hdr.vlan$10.$valid: H16(4)
  hdr.vlan$11.$valid: H16(3)
  hdr.vlan$12.$valid: H16(2)
  hdr.vlan$13.$valid: H16(1)
  hdr.vlan$14.$valid: H16(0)
  hdr.prsr_pad_0.$stkvalid: B16(0..2)
  hdr.prsr_pad_0$0.$valid: B16(2)
  hdr.prsr_pad_0$1.$valid: B16(1)
  hdr.prsr_pad_0$2.$valid: B16(0)
  context_json:
    B16:
    - { name : hdr.prsr_pad_0$0.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.prsr_pad_0.$stkvalid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.prsr_pad_0$1.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.prsr_pad_0$2.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    B17:
    - { name : hdr.eth.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    H16:
    - { name : hdr.vlan$0.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan.$stkvalid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$1.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$2.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$3.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$4.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$5.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$6.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$7.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$8.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$9.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$10.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$11.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$12.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$13.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    - { name : hdr.vlan$14.$valid, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
    H17:
    - { name : eg_intr_md.egress_port, live_start : parser, live_end : deparser, mutually_exclusive_with: [  ] }
parser egress:
  start: $entry_point
  init_zero: [ B17, H16, B16 ]
  bitwise_or: [ TH39, B16, H16 ]
  hdr_len_adj: 27
  meta_opt: 8191
  states:
    $entry_point:
      *:
        counter:
          imm: 24
        0..1: H17  # bit[7..15] -> H17 bit[8..0]: egress::eg_intr_md.egress_port
        27..28: TH41  # egress::hdr.eth.dst_addr[47:32].32-47
        29: TB22  # egress::hdr.eth.dst_addr[31:24].24-31
        30: TB21  # egress::hdr.eth.dst_addr[23:16].16-23
        31: TB20  # egress::hdr.eth.dst_addr[15:8].8-15
        B17: 1  # value 1 -> B17 bit[0]: egress::hdr.eth.$valid
        intr_md: 9
        shift: 32
        buf_req: 32
        next: start.$oob_stall_0
    start.$oob_stall_0:
      *:
        load: { half : 7..8 }
        buf_req: 9
        next: start.$split_0
    start.$split_0:
      match: [ half ]
      0x8100:
        0: TB15  # egress::hdr.eth.dst_addr[7:0].0-7
        1..2: TH40  # egress::hdr.eth.src_addr[47:32].32-47
        3..4: TH35  # egress::hdr.eth.src_addr[31:16].16-31
        5..6: TH34  # egress::hdr.eth.src_addr[15:0].0-15
        7..8: TH39  # egress::hdr.eth.ethertype
        load: { half : 11..12 }
        shift: 9
        buf_req: 13
        next: CommonParser_parse_vlan_0
      0x****:
        0: TB15  # egress::hdr.eth.dst_addr[7:0].0-7
        1..2: TH40  # egress::hdr.eth.src_addr[47:32].32-47
        3..4: TH35  # egress::hdr.eth.src_addr[31:16].16-31
        5..6: TH34  # egress::hdr.eth.src_addr[15:0].0-15
        7..8: TH39  # egress::hdr.eth.ethertype
        shift: 9
        buf_req: 9
        next: min_parse_depth_accept_initial
    CommonParser_parse_vlan_0:
      match: [ half ]
      0x8100:
        counter: dec 4
        0..3: TW0
            # - bit[0..2] -> TW0 bit[31..29]: egress::hdr.vlan[0].pcp
            # - bit[3] -> TW0 bit[28]: egress::hdr.vlan[0].dei
            # - bit[4..15] -> TW0 bit[27..16]: egress::hdr.vlan[0].vid
            # - bit[16..31] -> TW0 bit[15..0]: egress::hdr.vlan[0].ether_type
        H16: 16384  # value 16384 -> H16 bit[14..0]: egress::hdr.vlan.$stkvalid
        TH39: 2  # value 2 -> TH39 bit[15..0]: egress::hdr.eth.ethertype
        load: { half : 2..3 }
        shift: 4
        buf_req: 4
        offset_inc: 1
        next: CommonParser_parse_vlan_0
      0x****:
        counter: dec 4
        0..3: TW0
            # - bit[0..2] -> TW0 bit[31..29]: egress::hdr.vlan[0].pcp
            # - bit[3] -> TW0 bit[28]: egress::hdr.vlan[0].dei
            # - bit[4..15] -> TW0 bit[27..16]: egress::hdr.vlan[0].vid
            # - bit[16..31] -> TW0 bit[15..0]: egress::hdr.vlan[0].ether_type
        H16: 16384  # value 16384 -> H16 bit[14..0]: egress::hdr.vlan.$stkvalid
        TH39: 2  # value 2 -> TH39 bit[15..0]: egress::hdr.eth.ethertype
        shift: 4
        buf_req: 4
        offset_inc: 1
        next: min_parse_depth_accept_initial
    min_parse_depth_accept_initial:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB0  # egress::hdr.prsr_pad_0[0].blob[87:80].80-87
        1..2: TH36  # egress::hdr.prsr_pad_0[0].blob[79:64].64-79
        3..6: TW21  # egress::hdr.prsr_pad_0[0].blob[63:32].32-63
        7..10: TW20  # egress::hdr.prsr_pad_0[0].blob[31:0].0-31
        B16: 4  # value 4 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 11
        buf_req: 11
        next: min_parse_depth_accept_loop.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB1  # egress::hdr.prsr_pad_0[1].blob[87:80].80-87
        1..2: TH37  # egress::hdr.prsr_pad_0[1].blob[79:64].64-79
        3..6: TW23  # egress::hdr.prsr_pad_0[1].blob[63:32].32-63
        7..10: TW22  # egress::hdr.prsr_pad_0[1].blob[31:0].0-31
        B16: 2  # value 2 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 11
        buf_req: 11
        next: min_parse_depth_accept_loop.$it1.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$it1.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB2  # egress::hdr.prsr_pad_0[2].blob[87:80].80-87
        1..2: TH38  # egress::hdr.prsr_pad_0[2].blob[79:64].64-79
        3..4: TH33  # egress::hdr.prsr_pad_0[2].blob[63:48].48-63
        5..6: TH32  # egress::hdr.prsr_pad_0[2].blob[47:32].32-47
        7..8: TH31  # egress::hdr.prsr_pad_0[2].blob[31:16].16-31
        B16: 1  # value 1 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 9
        buf_req: 9
        next: min_parse_depth_accept_loop.$it2.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$it2.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        0..1: TH30  # egress::hdr.prsr_pad_0[2].blob[15:0].0-15
        shift: 2
        buf_req: 2
        next: end
      0b**:
        0..1: TH30  # egress::hdr.prsr_pad_0[2].blob[15:0].0-15
        shift: 2
        buf_req: 2
        next: end
)PARSER_CFG";

    options.target = NO_TARGET;
    Phv::test_clear();

    createSingleAsmParser();
    AsmParser *asm_parser = dynamic_cast<AsmParser *>(::asm_parser);
    asm_parse_string(parser_str);
    std::vector<Parser *> parser_vector = asm_parser->test_get_parser(EGRESS);
    EXPECT_GT(parser_vector.size(), 0);
    Parser *parser = parser_vector.back();
    parser->process();
    EXPECT_EQ(parser->get_prsr_max_dph(), 6);
}

// TEST(parser_test, get_parser_depth_untaken_path)
//
//           verify that untaken paths are not considered
//           in the parser depth calculation.
//
TEST(parser_test, get_parser_depth_untaken_path) {
    const char *parser_str = R"PARSER_CFG(
version:
  target: Tofino
parser egress:
  start: $entry_point.start
  init_zero: [ B17, B16 ]
  bitwise_or: [ TH15, B16, B17 ]
  hdr_len_adj: 27
  meta_opt: 8191
  states:
    $entry_point.start:
      *:
        counter:
          imm: 38
        0..1: H16  # bit[7..15] -> H16 bit[8..0]: egress::eg_intr_md.egress_port
        intr_md: 9
        shift: 27
        buf_req: 27
        next: $entry_point.start.$oob_stall_0
    $entry_point.start.$oob_stall_0:
      *:
        load: { half : 12..13 }
        buf_req: 14
        next: CommonParser_start_0
    CommonParser_start_0:
      match: [ half ]
      0x****:
        counter: dec 14
        0..1: TH17  # egress::hdr.eth.dst_addr[47:32].32-47
        2..5: TW9  # egress::hdr.eth.dst_addr[31:0].0-31
        6..7: TH16  # egress::hdr.eth.src_addr[47:32].32-47
        8..11: TW8  # egress::hdr.eth.src_addr[31:0].0-31
        12..13: TH15  # egress::hdr.eth.ethertype
        B17: 1  # value 1 -> B17 bit[0]: egress::hdr.eth.$valid
        shift: 14
        buf_req: 14
        next: min_parse_depth_accept_initial
      0x8100:
        counter: dec 14
        0..1: TH17  # egress::hdr.eth.dst_addr[47:32].32-47
        2..5: TW9  # egress::hdr.eth.dst_addr[31:0].0-31
        6..7: TH16  # egress::hdr.eth.src_addr[47:32].32-47
        8..11: TW8  # egress::hdr.eth.src_addr[31:0].0-31
        12..13: TH15  # egress::hdr.eth.ethertype
        B17: 1  # value 1 -> B17 bit[0]: egress::hdr.eth.$valid
        load: { half : 16..17 }
        shift: 14
        buf_req: 18
        next: CommonParser_parse_vlan_0
    min_parse_depth_accept_initial:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB0  # egress::hdr.prsr_pad_0[0].blob[87:80].80-87
        1..2: TH12  # egress::hdr.prsr_pad_0[0].blob[79:64].64-79
        3..6: TW2  # egress::hdr.prsr_pad_0[0].blob[63:32].32-63
        7..10: TW1  # egress::hdr.prsr_pad_0[0].blob[31:0].0-31
        B16: 4  # value 4 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 11
        buf_req: 11
        next: min_parse_depth_accept_loop.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB1  # egress::hdr.prsr_pad_0[1].blob[87:80].80-87
        1..2: TH13  # egress::hdr.prsr_pad_0[1].blob[79:64].64-79
        3..4: TH1  # egress::hdr.prsr_pad_0[1].blob[63:48].48-63
        5..6: TH0  # egress::hdr.prsr_pad_0[1].blob[47:32].32-47
        7..10: TW3  # egress::hdr.prsr_pad_0[1].blob[31:0].0-31
        B16: 2  # value 2 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 11
        buf_req: 11
        next: min_parse_depth_accept_loop.$it1.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$it1.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        counter: dec 11
        0: TB2  # egress::hdr.prsr_pad_0[2].blob[87:80].80-87
        1..2: TH14  # egress::hdr.prsr_pad_0[2].blob[79:64].64-79
        3..4: TH5  # egress::hdr.prsr_pad_0[2].blob[63:48].48-63
        5..6: TH4  # egress::hdr.prsr_pad_0[2].blob[47:32].32-47
        7..8: TH3  # egress::hdr.prsr_pad_0[2].blob[31:16].16-31
        B16: 1  # value 1 -> B16 bit[2..0]: egress::hdr.prsr_pad_0.$stkvalid
        shift: 9
        buf_req: 9
        next: min_parse_depth_accept_loop.$it2.$split_0
      0b**:
        buf_req: 0
        next: end
    min_parse_depth_accept_loop.$it2.$split_0:
      match: [ ctr_neg, ctr_zero ]
      0x0:
        0..1: TH2  # egress::hdr.prsr_pad_0[2].blob[15:0].0-15
        shift: 2
        buf_req: 2
        next: end
      0b**:
        0..1: TH2  # egress::hdr.prsr_pad_0[2].blob[15:0].0-15
        shift: 2
        buf_req: 2
        next: end
    CommonParser_parse_vlan_0:
      match: [ half ]
      0x8100:
        counter: dec 4
        0..3: TW0
            # - bit[0..2] -> TW0 bit[31..29]: egress::hdr.vlan.pcp
            # - bit[3] -> TW0 bit[28]: egress::hdr.vlan.dei
            # - bit[4..15] -> TW0 bit[27..16]: egress::hdr.vlan.vid
            # - bit[16..31] -> TW0 bit[15..0]: egress::hdr.vlan.ether_type
        B17: 2  # value 1 -> B17 bit[1]: egress::hdr.vlan.$valid
        TH15: 2  # value 2 -> TH15 bit[15..0]: egress::hdr.eth.ethertype
        load: { half : 16..17 }
        shift: 4
        buf_req: 18
        next: CommonParser_start_0
      0x****:
        counter: dec 4
        0..3: TW0
            # - bit[0..2] -> TW0 bit[31..29]: egress::hdr.vlan.pcp
            # - bit[3] -> TW0 bit[28]: egress::hdr.vlan.dei
            # - bit[4..15] -> TW0 bit[27..16]: egress::hdr.vlan.vid
            # - bit[16..31] -> TW0 bit[15..0]: egress::hdr.vlan.ether_type
        B17: 2  # value 1 -> B17 bit[1]: egress::hdr.vlan.$valid
        TH15: 2  # value 2 -> TH15 bit[15..0]: egress::hdr.eth.ethertype
        shift: 4
        buf_req: 4
        next: min_parse_depth_accept_initial
)PARSER_CFG";

    options.target = NO_TARGET;
    Phv::test_clear();

    createSingleAsmParser();
    AsmParser *asm_parser = dynamic_cast<AsmParser *>(::asm_parser);
    asm_parse_string(parser_str);
    std::vector<Parser *> parser_vector = asm_parser->test_get_parser(EGRESS);
    EXPECT_GT(parser_vector.size(), 0);
    Parser *parser = parser_vector.back();
    parser->process();
    EXPECT_EQ(parser->get_prsr_max_dph(), 4);
}

}  // namespace
