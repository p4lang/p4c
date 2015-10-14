/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <gtest/gtest.h>

#include "bm_sim/parser.h"
#include "bm_sim/deparser.h"

/* Frame (66 bytes) */
static const unsigned char raw_tcp_pkt[66] = {
  0x00, 0x18, 0x0a, 0x05, 0x5a, 0x10, 0xa0, 0x88, /* ....Z... */
  0x69, 0x0c, 0xc3, 0x03, 0x08, 0x00, 0x45, 0x00, /* i.....E. */
  0x00, 0x34, 0x70, 0x90, 0x40, 0x00, 0x40, 0x06, /* .4p.@.@. */
  0x35, 0x08, 0x0a, 0x36, 0xc1, 0x21, 0x4e, 0x28, /* 5..6.!N( */
  0x7b, 0xac, 0xa2, 0x97, 0x00, 0x50, 0x7f, 0xc2, /* {....P.. */
  0x4c, 0x80, 0x39, 0x77, 0xec, 0xd9, 0x80, 0x10, /* L.9w.... */
  0x00, 0x44, 0x13, 0xcd, 0x00, 0x00, 0x01, 0x01, /* .D...... */
  0x08, 0x0a, 0x00, 0xc3, 0x6d, 0x86, 0xa8, 0x20, /* ....m..  */
  0x21, 0x9b                                      /* !. */
};

/* Frame (82 bytes) */
static const unsigned char raw_udp_pkt[82] = {
  0x8c, 0x04, 0xff, 0xac, 0x28, 0xa0, 0xa0, 0x88, /* ....(... */
  0x69, 0x0c, 0xc3, 0x03, 0x08, 0x00, 0x45, 0x00, /* i.....E. */
  0x00, 0x44, 0x3a, 0xf5, 0x40, 0x00, 0x40, 0x11, /* .D:.@.@. */
  0x5f, 0x0f, 0x0a, 0x00, 0x00, 0x0f, 0x4b, 0x4b, /* _.....KK */
  0x4b, 0x4b, 0x1f, 0x5c, 0x00, 0x35, 0x00, 0x30, /* KK.\.5.0 */
  0xeb, 0x61, 0x85, 0xa6, 0x01, 0x00, 0x00, 0x01, /* .a...... */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x61, /* .......a */
  0x70, 0x69, 0x03, 0x6e, 0x65, 0x77, 0x0a, 0x6c, /* pi.new.l */
  0x69, 0x76, 0x65, 0x73, 0x74, 0x72, 0x65, 0x61, /* ivestrea */
  0x6d, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x01, /* m.com... */
  0x00, 0x01                                      /* .. */
};

// Google Test fixture for parser tests
class ParserTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;

  HeaderType ethernetHeaderType, ipv4HeaderType, udpHeaderType, tcpHeaderType;
  ParseState ethernetParseState, ipv4ParseState, udpParseState, tcpParseState;
  header_id_t ethernetHeader{0}, ipv4Header{1}, udpHeader{2}, tcpHeader{3};

  Parser parser;

  Deparser deparser;

  ParserTest()
    : ethernetHeaderType("ethernet_t", 0), ipv4HeaderType("ipv4_t", 1),
      udpHeaderType("udp_t", 2), tcpHeaderType("tcp_t", 3),
      ethernetParseState("parse_ethernet", 0),
      ipv4ParseState("parse_ipv4", 1),
      udpParseState("parse_udp", 2),
      tcpParseState("parse_tcp", 3),
      parser("test_parser", 0), deparser("test_deparser", 0) {
    ethernetHeaderType.push_back_field("dstAddr", 48);
    ethernetHeaderType.push_back_field("srcAddr", 48);
    ethernetHeaderType.push_back_field("ethertype", 16);

    ipv4HeaderType.push_back_field("version", 4);
    ipv4HeaderType.push_back_field("ihl", 4);
    ipv4HeaderType.push_back_field("diffserv", 8);
    ipv4HeaderType.push_back_field("len", 16);
    ipv4HeaderType.push_back_field("id", 16);
    ipv4HeaderType.push_back_field("flags", 3);
    ipv4HeaderType.push_back_field("flagOffset", 13);
    ipv4HeaderType.push_back_field("ttl", 8);
    ipv4HeaderType.push_back_field("protocol", 8);
    ipv4HeaderType.push_back_field("checksum", 16);
    ipv4HeaderType.push_back_field("srcAddr", 32);
    ipv4HeaderType.push_back_field("dstAddr", 32);

    udpHeaderType.push_back_field("srcPort", 16);
    udpHeaderType.push_back_field("dstPort", 16);
    udpHeaderType.push_back_field("length", 16);
    udpHeaderType.push_back_field("checksum", 16);

    tcpHeaderType.push_back_field("srcPort", 16);
    tcpHeaderType.push_back_field("dstPort", 16);
    tcpHeaderType.push_back_field("seqNo", 32);
    tcpHeaderType.push_back_field("ackNo", 32);
    tcpHeaderType.push_back_field("dataOffset", 4);
    tcpHeaderType.push_back_field("res", 4);
    tcpHeaderType.push_back_field("flags", 8);
    tcpHeaderType.push_back_field("window", 16);
    tcpHeaderType.push_back_field("checksum", 16);
    tcpHeaderType.push_back_field("urgentPtr", 16);

    phv_factory.push_back_header("ethernet", ethernetHeader, ethernetHeaderType);
    phv_factory.push_back_header("ipv4", ipv4Header, ipv4HeaderType);
    phv_factory.push_back_header("udp", udpHeader, udpHeaderType);
    phv_factory.push_back_header("tcp", tcpHeader, tcpHeaderType);
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);

    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2); // ethertype
    ethernetParseState.set_key_builder(ethernetKeyBuilder);

    ParseSwitchKeyBuilder ipv4KeyBuilder;
    ipv4KeyBuilder.push_back_field(ipv4Header, 8); // protocol
    ipv4ParseState.set_key_builder(ipv4KeyBuilder);

    ethernetParseState.add_extract(ethernetHeader);
    ipv4ParseState.add_extract(ipv4Header);
    udpParseState.add_extract(udpHeader);
    tcpParseState.add_extract(tcpHeader);

    char ethernet_ipv4_key[2];
    ethernet_ipv4_key[0] = 0x08;
    ethernet_ipv4_key[1] = 0x00;
    ethernetParseState.add_switch_case(sizeof(ethernet_ipv4_key),
				       ethernet_ipv4_key, &ipv4ParseState);

    char ipv4_udp_key[1];
    ipv4_udp_key[0] = 17;
    ipv4ParseState.add_switch_case(sizeof(ipv4_udp_key), ipv4_udp_key,
				   &udpParseState);

    char ipv4_tcp_key[1];
    ipv4_tcp_key[0] = 6;
    ipv4ParseState.add_switch_case(sizeof(ipv4_tcp_key),
				   ipv4_tcp_key, &tcpParseState);

    parser.set_init_state(&ethernetParseState);

    deparser.push_back_header(ethernetHeader);
    deparser.push_back_header(ipv4Header);
    deparser.push_back_header(tcpHeader);
    deparser.push_back_header(udpHeader);
  }

  Packet get_tcp_pkt() {
    Packet pkt = Packet(
	0, 0, 0, sizeof(raw_tcp_pkt),
	PacketBuffer(256, (const char *) raw_tcp_pkt, sizeof(raw_tcp_pkt))
    );
    return pkt;
  }

  Packet get_udp_pkt() {
    Packet pkt = Packet(
	0, 0, 0, sizeof(raw_udp_pkt),
	PacketBuffer(256, (const char *) raw_udp_pkt, sizeof(raw_udp_pkt))
    );
    return pkt;
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(ParserTest, ParseEthernetIPv4TCP) {
  Packet packet = get_tcp_pkt();
  PHV *phv = packet.get_phv();
  parser.parse(&packet);

  const Header &ethernet_hdr = phv->get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());

  const Header &ipv4_hdr = phv->get_header(ipv4Header);
  ASSERT_TRUE(ipv4_hdr.is_valid());

  Field &ipv4_version = phv->get_field(ipv4Header, 0);
  ASSERT_EQ((unsigned) 0x4, ipv4_version.get_uint());

  Field &ipv4_ihl = phv->get_field(ipv4Header, 1);
  ASSERT_EQ((unsigned) 0x5, ipv4_ihl.get_uint());

  Field &ipv4_diffserv = phv->get_field(ipv4Header, 2);
  ASSERT_EQ((unsigned) 0x00, ipv4_diffserv.get_uint());

  Field &ipv4_len = phv->get_field(ipv4Header, 3);
  ASSERT_EQ((unsigned) 0x0034, ipv4_len.get_uint());

  Field &ipv4_identification = phv->get_field(ipv4Header, 4);
  ASSERT_EQ((unsigned) 0x7090, ipv4_identification.get_uint());

  Field &ipv4_flags = phv->get_field(ipv4Header, 5);
  ASSERT_EQ((unsigned) 0x2, ipv4_flags.get_uint());

  Field &ipv4_flagOffset = phv->get_field(ipv4Header, 6);
  ASSERT_EQ((unsigned) 0x0000, ipv4_flagOffset.get_uint());

  Field &ipv4_ttl = phv->get_field(ipv4Header, 7);
  ASSERT_EQ((unsigned) 0x40, ipv4_ttl.get_uint());

  Field &ipv4_protocol = phv->get_field(ipv4Header, 8);
  ASSERT_EQ((unsigned) 0x06, ipv4_protocol.get_uint());

  Field &ipv4_checksum = phv->get_field(ipv4Header, 9);
  ASSERT_EQ((unsigned) 0x3508, ipv4_checksum.get_uint());

  Field &ipv4_srcAddr = phv->get_field(ipv4Header, 10);
  ASSERT_EQ((unsigned) 0x0a36c121, ipv4_srcAddr.get_uint());

  Field &ipv4_dstAddr = phv->get_field(ipv4Header, 11);
  ASSERT_EQ((unsigned) 0x4e287bac, ipv4_dstAddr.get_uint());

  const Header &tcp_hdr = phv->get_header(tcpHeader);
  ASSERT_TRUE(tcp_hdr.is_valid());

  const Header &udp_hdr = phv->get_header(udpHeader);
  ASSERT_FALSE(udp_hdr.is_valid());
}

TEST_F(ParserTest, ParseEthernetIPv4UDP) {
  Packet packet = get_udp_pkt();
  PHV *phv = packet.get_phv();
  parser.parse(&packet);

  const Header &ethernet_hdr = phv->get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());

  const Header &ipv4_hdr = phv->get_header(ipv4Header);
  ASSERT_TRUE(ipv4_hdr.is_valid());

  const Header &udp_hdr = phv->get_header(udpHeader);
  ASSERT_TRUE(udp_hdr.is_valid());

  const Header &tcp_hdr = phv->get_header(tcpHeader);
  ASSERT_FALSE(tcp_hdr.is_valid());
}


TEST_F(ParserTest, ParseEthernetIPv4TCP_Stress) {
  for(int t = 0; t < 10000; t++) {
    Packet packet = get_tcp_pkt();
    PHV *phv = packet.get_phv();
    parser.parse(&packet);

    ASSERT_TRUE(phv->get_header(ethernetHeader).is_valid());
    ASSERT_TRUE(phv->get_header(ipv4Header).is_valid());
    ASSERT_TRUE(phv->get_header(tcpHeader).is_valid());
    ASSERT_FALSE(phv->get_header(udpHeader).is_valid());
  }
}

TEST_F(ParserTest, DeparseEthernetIPv4TCP) {
  Packet packet = get_tcp_pkt();
  parser.parse(&packet);
  
  deparser.deparse(&packet);

  ASSERT_EQ(sizeof(raw_tcp_pkt), packet.get_data_size());
  ASSERT_EQ(0, memcmp(raw_tcp_pkt, packet.data(), sizeof(raw_tcp_pkt)));
}

TEST_F(ParserTest, DeparseEthernetIPv4UDP) {
  Packet packet = get_udp_pkt();
  parser.parse(&packet);
  
  deparser.deparse(&packet);

  ASSERT_EQ(sizeof(raw_udp_pkt), packet.get_data_size());
  ASSERT_EQ(0, memcmp(raw_udp_pkt, packet.data(), sizeof(raw_udp_pkt)));
}

TEST_F(ParserTest, DeparseEthernetIPv4_Stress) {
  const char *ref_pkt;
  size_t size;

  Packet packet;
  for(int t = 0; t < 10000; t++) {
    if(t % 2 == 0) {
      packet = get_tcp_pkt();
      ref_pkt = (const char *) raw_tcp_pkt;
      size = sizeof(raw_tcp_pkt);
    }
    else {
      packet = get_udp_pkt();
      ref_pkt = (const char *) raw_udp_pkt;
      size = sizeof(raw_udp_pkt);
    }
    parser.parse(&packet);
    deparser.deparse(&packet);
    ASSERT_EQ(0, memcmp(ref_pkt, packet.data(), size));
  }

}

TEST(LookAhead, Peek) {
  ByteContainer res;
  // 1011 0101, 1001 1101, 1111 1101, 0001 0111, 1101 0101, 1101 0111
  const unsigned char data_[6] = {0xb5, 0x9d, 0xfd, 0x17, 0xd5, 0xd7};
  const char *data = (char *) data_;

  ParserLookAhead lookahead1(0, 16);
  lookahead1.peek(data, res);
  ASSERT_EQ(ByteContainer("0xb59d"), res);
  res.clear();

  ParserLookAhead lookahead2(3, 16);
  lookahead2.peek(data, res);
  // 1010 1100, 1110 1111
  ASSERT_EQ(ByteContainer("0xacef"), res);
  res.clear();

  ParserLookAhead lookahead3(0, 21);
  lookahead3.peek(data, res);
  // 0001 0110, 1011 0011, 1011 1111
  ASSERT_EQ(ByteContainer("0x16b3bf"), res);
  res.clear();

  ParserLookAhead lookahead4(18, 15);
  lookahead4.peek(data, res);
  // 0111 1010, 0010 1111
  ASSERT_EQ(ByteContainer("0x7a2f"), res);
  res.clear();

  ParserLookAhead lookahead5_1(0, 16);
  lookahead5_1.peek(data, res);
  ParserLookAhead lookahead5_2(16, 16);
  lookahead5_2.peek(data, res);
  ASSERT_EQ(ByteContainer("0xb59dfd17"), res);
}

// Google Test fixture for ParserOpSet tests
class ParserOpSetTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  ParserOpSetTest()
    : testHeaderType("test_t", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f32", 32);
    testHeaderType.push_back_field("f48", 48);

    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet(0, 0, 0, 64, PacketBuffer(128));
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(ParserOpSetTest, SetFromData) {
  Data src("0xaba");
  ParserOpSet<Data> op(testHeader1, 1, src); // f32
  ParserOp &opRef = op;
  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 1);
  ASSERT_EQ(0u, f.get_uint());
  opRef(&pkt, nullptr, nullptr);
  ASSERT_EQ(0xaba, f.get_uint());
}

TEST_F(ParserOpSetTest, SetFromField) {
  ParserOpSet<field_t> op(testHeader1, 1, field_t::make(testHeader2, 1)); // f32
  ParserOp &opRef = op;
  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 1);
  Field &f_src = pkt.get_phv()->get_field(testHeader2, 1);
  f_src.set("0xaba");
  ASSERT_EQ(0u, f.get_uint());
  opRef(&pkt, nullptr, nullptr);
  ASSERT_EQ(0xaba, f.get_uint());
}

TEST_F(ParserOpSetTest, SetFromLookahead) {
  const unsigned char data_[6] = {0xb5, 0x9d, 0xfd, 0x17, 0xd5, 0xd7};
  const char *data = (char *) data_;

  ParserLookAhead lookahead1(0, 32);
  const ParserOpSet<ParserLookAhead> op1(testHeader1, 1, lookahead1); // f32
  const ParserOp &op1Ref = op1;
  Packet pkt1 = get_pkt();
  Field &f1 = pkt1.get_phv()->get_field(testHeader1, 1);
  ASSERT_EQ(0u, f1.get_uint());
  op1Ref(&pkt1, (const char *) data, nullptr);
  ASSERT_EQ(0xb59dfd17, f1.get_uint());

  ParserLookAhead lookahead2(8, 8);
  const ParserOpSet<ParserLookAhead> op2(testHeader1, 1, lookahead2); // f32
  const ParserOp &op2Ref = op2;
  Packet pkt2 = get_pkt();
  Field &f2 = pkt2.get_phv()->get_field(testHeader1, 1);
  ASSERT_EQ(0u, f2.get_uint());
  op2Ref(&pkt2, (const char *) data, nullptr);
  ASSERT_EQ(0x9d, f2.get_uint());
}

// Google Test fixture for ParseSwitchKeyBuilder tests
class ParseSwitchKeyBuilderTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  header_id_t testHeaderStack1{2}, testHeaderStack2{3};
  header_stack_id_t testHeaderStack{0};

  ParseSwitchKeyBuilderTest()
    : testHeaderType("test_t", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f32", 32);
    testHeaderType.push_back_field("f48", 48);

    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
    phv_factory.push_back_header("test_stack_1", testHeaderStack1, testHeaderType);
    phv_factory.push_back_header("test_stack_2", testHeaderStack2, testHeaderType);
    phv_factory.push_back_header_stack("test_stack",
				       testHeaderStack, testHeaderType,
				       {testHeaderStack1, testHeaderStack2});
  }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet(0, 0, 0, 64, PacketBuffer(128));
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(ParseSwitchKeyBuilderTest, Mix) {
  ParseSwitchKeyBuilder builder;
  builder.push_back_field(testHeader1, 2);
  builder.push_back_lookahead(0, 16);
  builder.push_back_field(testHeader2, 0);
  builder.push_back_field(testHeader2, 1);
  builder.push_back_lookahead(16, 32);
  builder.push_back_lookahead(20, 20);
  builder.push_back_stack_field(testHeaderStack, 1);

  const unsigned char data_[6] = {0xb5, 0x9d, 0xfd, 0x17, 0xd5, 0xd7};
  const char *data = (char *) data_;

  Packet pkt = get_pkt();
  PHV *phv = pkt.get_phv();
  Field &f1_2 = phv->get_field(testHeader1, 2);
  Field &f2_0 = phv->get_field(testHeader2, 0);
  Field &f2_1 = phv->get_field(testHeader2, 1);

  f1_2.set("0xaabbccddeeff");
  f2_0.set("0x1122");
  f2_1.set("0xababab");

  HeaderStack &stack = phv->get_header_stack(testHeaderStack);
  ASSERT_EQ(1u, stack.push_back());
  Header &h = phv->get_header(testHeaderStack1);
  ASSERT_TRUE(h.is_valid());
  h.get_field(1).set("0x44332211");

  // aabbccddeeff b59d 1122 00ababab fd17d5d7 0d17d5 44332211
  ByteContainer expected("0xaabbccddeeffb59d112200abababfd17d5d70d17d544332211");
  ByteContainer res;
  builder(*phv, data, res);
  ASSERT_EQ(expected, res);
}

static const unsigned char raw_mpls_pkt[93] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x88, 0x47, 0x00, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x31, 0x00, 0x45, 0x00, 0x00, 0x43, 0x00, 0x01,
  0x00, 0x00, 0x40, 0x06, 0x7c, 0xb2, 0x7f, 0x00,
  0x00, 0x01, 0x7f, 0x00, 0x00, 0x01, 0x00, 0x14,
  0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0x3e, 0x6f,
  0x00, 0x00, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
  0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
  0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
  0x61, 0x61, 0x61, 0x61, 0x61
};

// Google Test fixture for special MPLS test
// This test complements the ParserTest by using header stacks
class MPLSParserTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;

  HeaderType ethernetHeaderType, MPLSHeaderType;
  ParseState ethernetParseState, MPLSParseState;
  header_id_t ethernetHeader{0};
  header_id_t MPLSHeader1{1}, MPLSHeader2{2}, MPLSHeader3{3}, MPLSHeader4{4};

  header_stack_id_t MPLSStack{0};

  Parser parser;

  Deparser deparser;

  MPLSParserTest()
    : ethernetHeaderType("ethernet_t", 0), MPLSHeaderType("mpls_t", 1),
      ethernetParseState("parse_ethernet", 0),
      MPLSParseState("parse_mpls", 1),
      parser("test_parser", 0), deparser("test_deparser", 0) {
    ethernetHeaderType.push_back_field("dstAddr", 48);
    ethernetHeaderType.push_back_field("srcAddr", 48);
    ethernetHeaderType.push_back_field("ethertype", 16);

    MPLSHeaderType.push_back_field("label", 20);
    MPLSHeaderType.push_back_field("exp", 3);
    MPLSHeaderType.push_back_field("bos", 1);
    MPLSHeaderType.push_back_field("ttl", 8);

    phv_factory.push_back_header("ethernet", ethernetHeader, ethernetHeaderType);
    phv_factory.push_back_header("mpls0", MPLSHeader1, MPLSHeaderType);
    phv_factory.push_back_header("mpls1", MPLSHeader2, MPLSHeaderType);
    phv_factory.push_back_header("mpls2", MPLSHeader3, MPLSHeaderType);
    phv_factory.push_back_header("mpls3", MPLSHeader4, MPLSHeaderType);
    phv_factory.push_back_header_stack(
      "mpls", MPLSStack, MPLSHeaderType,
      {MPLSHeader1, MPLSHeader2, MPLSHeader3, MPLSHeader4});
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);

    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2); // ethertype
    ethernetParseState.set_key_builder(ethernetKeyBuilder);

    ParseSwitchKeyBuilder MPLSKeyBuilder;
    MPLSKeyBuilder.push_back_stack_field(MPLSStack, 2); // bos
    MPLSParseState.set_key_builder(MPLSKeyBuilder);

    ethernetParseState.add_extract(ethernetHeader);
    MPLSParseState.add_extract_to_stack(MPLSStack);

    char ethernet_ipv4_key[2];
    ethernet_ipv4_key[0] = 0x88;
    ethernet_ipv4_key[1] = 0x47;
    ethernetParseState.add_switch_case(sizeof(ethernet_ipv4_key),
				       ethernet_ipv4_key, &MPLSParseState);

    char mpls_mpls_key[1];
    mpls_mpls_key[0] = 0x00;
    MPLSParseState.add_switch_case(sizeof(mpls_mpls_key),
				   mpls_mpls_key, &MPLSParseState);

    parser.set_init_state(&ethernetParseState);

    deparser.push_back_header(ethernetHeader);
    // TODO
    // would it be better to have a push_back_stack here, for the deparser ?
    deparser.push_back_header(MPLSHeader1);
    deparser.push_back_header(MPLSHeader2);
    deparser.push_back_header(MPLSHeader3);
    deparser.push_back_header(MPLSHeader4);
  }

  Packet get_mpls_pkt() {
    Packet pkt = Packet(
	0, 0, 0, sizeof(raw_mpls_pkt),
	PacketBuffer(256, (const char *) raw_mpls_pkt, sizeof(raw_mpls_pkt))
    );
    return pkt;
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(MPLSParserTest, ParseEthernetMPLS3) {
  Packet packet = get_mpls_pkt();
  PHV *phv = packet.get_phv();
  parser.parse(&packet);

  const Header &ethernet_hdr = phv->get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());

  const Header &MPLS_hdr_1 = phv->get_header(MPLSHeader1);
  ASSERT_TRUE(MPLS_hdr_1.is_valid());
  const Header &MPLS_hdr_2 = phv->get_header(MPLSHeader2);
  ASSERT_TRUE(MPLS_hdr_2.is_valid());
  const Header &MPLS_hdr_3 = phv->get_header(MPLSHeader3);
  ASSERT_TRUE(MPLS_hdr_3.is_valid());
  const Header &MPLS_hdr_4 = phv->get_header(MPLSHeader4);
  ASSERT_FALSE(MPLS_hdr_4.is_valid());

  ASSERT_EQ(1u, MPLS_hdr_1.get_field(0)); // label
  ASSERT_EQ(2u, MPLS_hdr_2.get_field(0)); // label
  ASSERT_EQ(3u, MPLS_hdr_3.get_field(0)); // label
}

class SwitchCaseTest : public ::testing::Test {
protected:
  PHVFactory phv_factory;

  SwitchCaseTest() { }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet(0, 0, 0, 64, PacketBuffer(128));
  }

  unsigned int bc_as_uint(const ByteContainer &bc) {
    unsigned int res = 0;
    for(auto c : bc)
      res = (res << 8) + static_cast<unsigned char>(c);
    return res;
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(SwitchCaseTest, Mask) {
  ParseState pstate("pstate", 0);
  std::unique_ptr<ParseState> next_state_1(new ParseState("s1", 1));
  std::unique_ptr<ParseState> next_state_2(new ParseState("s2", 2));
  std::unique_ptr<ParseState> next_state_3(new ParseState("s3", 3));
  pstate.set_default_switch_case(nullptr);
  const ByteContainer key_1("0x00ab");
  const ByteContainer key_2("0x1989");
  const ByteContainer key_3("0xabcd");
  const ByteContainer mask_1("0x00ff");
  const ByteContainer mask_2("0x39d9");
  const ByteContainer mask_3("0xebcf");
  pstate.add_switch_case_with_mask(key_1, mask_1, next_state_1.get());
  pstate.add_switch_case_with_mask(key_2, mask_2, next_state_2.get());
  pstate.add_switch_case_with_mask(key_3, mask_3, next_state_3.get());

  ParseSwitchKeyBuilder builder;
  builder.push_back_lookahead(0, 16);
  pstate.set_key_builder(builder);

  std::vector<ParseState *> expected_next_states(65536);
  for(int i = 0; i < 65536; i++) {
    if((i & bc_as_uint(mask_1)) == (bc_as_uint(key_1) & bc_as_uint(mask_1)))
      expected_next_states[i] = next_state_1.get();
    else if((i & bc_as_uint(mask_2)) == (bc_as_uint(key_2) & bc_as_uint(mask_2)))
      expected_next_states[i] = next_state_2.get();
    else if((i & bc_as_uint(mask_3)) == (bc_as_uint(key_3) & bc_as_uint(mask_3)))
      expected_next_states[i] = next_state_3.get();
    else
      expected_next_states[i] = nullptr;
  }

  Packet packet = get_pkt();
  for(int i = 0; i < 65536; i++) {
    size_t bytes_parsed = 0;
    const char data[2] = {static_cast<char>(i >> 8),
			  static_cast<char>(i & 0xff)};
    const ParseState *next_state = pstate(&packet, data, &bytes_parsed);

    ASSERT_EQ(expected_next_states[i], next_state);
  }
}
