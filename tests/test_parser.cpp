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

#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/deparser.h>

#include <chrono>
#include <thread>
#include <mutex>

using namespace bm;

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

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  ParserTest()
      : ethernetHeaderType("ethernet_t", 0), ipv4HeaderType("ipv4_t", 1),
        udpHeaderType("udp_t", 2), tcpHeaderType("tcp_t", 3),
        ethernetParseState("parse_ethernet", 0),
        ipv4ParseState("parse_ipv4", 1),
        udpParseState("parse_udp", 2),
        tcpParseState("parse_tcp", 3),
        parser("test_parser", 0), deparser("test_deparser", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
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
    phv_source->set_phv_factory(0, &phv_factory);

    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16); // ethertype
    ethernetParseState.set_key_builder(ethernetKeyBuilder);

    ParseSwitchKeyBuilder ipv4KeyBuilder;
    ipv4KeyBuilder.push_back_field(ipv4Header, 8, 8); // protocol
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
    return Packet::make_new(
	sizeof(raw_tcp_pkt),
	PacketBuffer(256, (const char *) raw_tcp_pkt, sizeof(raw_tcp_pkt)),
        phv_source.get());
  }

  Packet get_udp_pkt() {
    return Packet::make_new(
	sizeof(raw_udp_pkt),
	PacketBuffer(256, (const char *) raw_udp_pkt, sizeof(raw_udp_pkt)),
        phv_source.get());
  }

  // virtual void TearDown() { }
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

  Packet packet = Packet::make_new(phv_source.get());
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
  lookahead1.peek(data, &res);
  ASSERT_EQ(ByteContainer("0xb59d"), res);
  res.clear();

  ParserLookAhead lookahead2(3, 16);
  lookahead2.peek(data, &res);
  // 1010 1100, 1110 1111
  ASSERT_EQ(ByteContainer("0xacef"), res);
  res.clear();

  ParserLookAhead lookahead3(0, 21);
  lookahead3.peek(data, &res);
  // 0001 0110, 1011 0011, 1011 1111
  ASSERT_EQ(ByteContainer("0x16b3bf"), res);
  res.clear();

  ParserLookAhead lookahead4(18, 15);
  lookahead4.peek(data, &res);
  // 0111 1010, 0010 1111
  ASSERT_EQ(ByteContainer("0x7a2f"), res);
  res.clear();

  ParserLookAhead lookahead5_1(0, 16);
  lookahead5_1.peek(data, &res);
  ParserLookAhead lookahead5_2(16, 16);
  lookahead5_2.peek(data, &res);
  ASSERT_EQ(ByteContainer("0xb59dfd17"), res);
}

// Google Test fixture for ParserOpSet tests
class ParserOpSetTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  ParserOpSetTest()
      : testHeaderType("test_t", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f32", 32);
    testHeaderType.push_back_field("f48", 48);

    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet::make_new(64, PacketBuffer(128), phv_source.get());
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
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

TEST_F(ParserOpSetTest, SetFromExpression) {
  ArithExpression expr;
  expr.push_back_load_field(testHeader2, 1);
  expr.push_back_load_const(Data(1));
  expr.push_back_op(ExprOpcode::ADD);
  expr.build();

  ParserOpSet<ArithExpression> op(testHeader1, 1, expr);
  ParserOp &opRef = op;
  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 1);
  Field &f_src = pkt.get_phv()->get_field(testHeader2, 1);
  f_src.set("0xaba");
  ASSERT_EQ(0u, f.get_uint());
  opRef(&pkt, nullptr, nullptr);
  ASSERT_EQ(0xabb, f.get_uint());
}

// Google Test fixture for ParseSwitchKeyBuilder tests
class ParseSwitchKeyBuilderTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  header_id_t testHeaderStack1{2}, testHeaderStack2{3};
  header_stack_id_t testHeaderStack{0};

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  ParseSwitchKeyBuilderTest()
      : testHeaderType("test_t", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
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
    return Packet::make_new(64, PacketBuffer(128), phv_source.get());
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
};

TEST_F(ParseSwitchKeyBuilderTest, Mix) {
  ParseSwitchKeyBuilder builder;
  builder.push_back_field(testHeader1, 2, 48);
  builder.push_back_lookahead(0, 16);
  builder.push_back_field(testHeader2, 0, 16);
  builder.push_back_field(testHeader2, 1, 32);
  builder.push_back_lookahead(16, 32);
  builder.push_back_lookahead(20, 20);
  builder.push_back_stack_field(testHeaderStack, 1, 32);

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
  builder(*phv, data, &res);
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

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  MPLSParserTest()
      : ethernetHeaderType("ethernet_t", 0), MPLSHeaderType("mpls_t", 1),
        ethernetParseState("parse_ethernet", 0),
        MPLSParseState("parse_mpls", 1),
        parser("test_parser", 0), deparser("test_deparser", 0),
        phv_source(PHVSourceIface::make_phv_source()) {
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
    phv_source->set_phv_factory(0, &phv_factory);

    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16); // ethertype
    ethernetParseState.set_key_builder(ethernetKeyBuilder);

    ParseSwitchKeyBuilder MPLSKeyBuilder;
    MPLSKeyBuilder.push_back_stack_field(MPLSStack, 2, 1); // bos
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
    return Packet::make_new(
	sizeof(raw_mpls_pkt),
	PacketBuffer(256, (const char *) raw_mpls_pkt, sizeof(raw_mpls_pkt)),
        phv_source.get());
  }

  // virtual void TearDown() { }
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

  ASSERT_EQ(1u, MPLS_hdr_1.get_field(0).get_uint()); // label
  ASSERT_EQ(2u, MPLS_hdr_2.get_field(0).get_uint()); // label
  ASSERT_EQ(3u, MPLS_hdr_3.get_field(0).get_uint()); // label
}

class SwitchCaseTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  SwitchCaseTest()
      : phv_source(PHVSourceIface::make_phv_source()) { }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet::make_new(64, PacketBuffer(128), phv_source.get());
  }

  unsigned int bc_as_uint(const ByteContainer &bc) {
    unsigned int res = 0;
    for(auto c : bc)
      res = (res << 8) + static_cast<unsigned char>(c);
    return res;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
};

TEST_F(SwitchCaseTest, NoSwitch) {
  ParseState pstate("pstate", 0);
  Packet packet = get_pkt();
  size_t bytes_parsed = 0;

  {
    const ParseState *next_state = pstate(&packet, nullptr, &bytes_parsed);
    ASSERT_EQ(nullptr, next_state);
  }

  {
    const ParseState expected_next_state("next_state", 1);
    pstate.set_default_switch_case(&expected_next_state);
    const ParseState *next_state = pstate(&packet, nullptr, &bytes_parsed);
    ASSERT_EQ(&expected_next_state, next_state);
  }
}

TEST_F(SwitchCaseTest, Mask) {
  ParseState pstate("pstate", 0);
  const ParseState next_state_1("s1", 1);
  const ParseState next_state_2("s2", 2);
  const ParseState next_state_3("s3", 3);
  pstate.set_default_switch_case(nullptr);
  const ByteContainer key_1("0x00ab");
  const ByteContainer key_2("0x1989");
  const ByteContainer key_3("0xabcd");
  const ByteContainer mask_1("0x00ff");
  const ByteContainer mask_2("0x39d9");
  const ByteContainer mask_3("0xebcf");
  pstate.add_switch_case_with_mask(key_1, mask_1, &next_state_1);
  pstate.add_switch_case_with_mask(key_2, mask_2, &next_state_2);
  pstate.add_switch_case_with_mask(key_3, mask_3, &next_state_3);

  ParseSwitchKeyBuilder builder;
  builder.push_back_lookahead(0, 16);
  pstate.set_key_builder(builder);

  std::vector<const ParseState *> expected_next_states(65536);
  for(int i = 0; i < 65536; i++) {
    if((i & bc_as_uint(mask_1)) == (bc_as_uint(key_1) & bc_as_uint(mask_1)))
      expected_next_states[i] = &next_state_1;
    else if((i & bc_as_uint(mask_2)) == (bc_as_uint(key_2) & bc_as_uint(mask_2)))
      expected_next_states[i] = &next_state_2;
    else if((i & bc_as_uint(mask_3)) == (bc_as_uint(key_3) & bc_as_uint(mask_3)))
      expected_next_states[i] = &next_state_3;
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


// Google Test fixture for IPv4 TLV parsing test
// This test is targetted a TLV parsing but covers many aspects of the parser
// (e.g. header stacks)
class IPv4TLVParsingTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  HeaderType ethernetHeaderType, ipv4HeaderType;
  HeaderType ipv4OptionAHeaderType, ipv4OptionBHeaderType;
  HeaderType ipv4PaddingHeaderType;
  HeaderType pMetaType;
  ParseState ethernetParseState, ipv4ParseState, ipv4OptionsParseState;
  ParseState ipv4OptionAParseState, ipv4OptionBParseState;
  ParseState ipv4PaddingParseState;
  header_id_t ethernetHeader{0}, ipv4Header{1};
  header_id_t ipv4OptionAHeader{2}, ipv4OptionBHeader{3};
  header_id_t ipv4PaddingHeader1{4}, ipv4PaddingHeader2{5};
  header_id_t ipv4PaddingHeader3{6}, ipv4PaddingHeader4{7};
  header_id_t pMeta{8};
  header_stack_id_t ipv4PaddingStack{0};

  Parser parser;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  IPv4TLVParsingTest()
      : ethernetHeaderType("ethernet_t", 0), ipv4HeaderType("ipv4_t", 1),
        ipv4OptionAHeaderType("ipv4_optionA_t", 2),
        ipv4OptionBHeaderType("ipv4_optionB_t", 3),
        ipv4PaddingHeaderType("ipv4_padding_t", 4),
        pMetaType("pmeta_t", 5),
        ethernetParseState("parse_ethernet", 0),
        ipv4ParseState("parse_ipv4", 1),
        ipv4OptionsParseState("parse_ipv4_options", 2),
        ipv4OptionAParseState("parse_ipv4_optionA", 3),
        ipv4OptionBParseState("parse_ipv4_optionB", 4),
        ipv4PaddingParseState("parse_ipv4_padding", 5),
        parser("test_parser", 0),
        phv_source(PHVSourceIface::make_phv_source()) {

    pMetaType.push_back_field("byte_counter", 8);

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

    ipv4OptionAHeaderType.push_back_field("type", 8);
    ipv4OptionAHeaderType.push_back_field("length", 8);
    ipv4OptionAHeaderType.push_back_field("f1", 16);
    ipv4OptionAHeaderType.push_back_field("f2", 32);

    ipv4OptionBHeaderType.push_back_field("type", 8);

    ipv4PaddingHeaderType.push_back_field("pad", 8);

    phv_factory.push_back_header("pMeta", pMeta, pMetaType);

    phv_factory.push_back_header("ethernet", ethernetHeader,
				 ethernetHeaderType);
    phv_factory.push_back_header("ipv4", ipv4Header, ipv4HeaderType);
    phv_factory.push_back_header("ipv4OptionA", ipv4OptionAHeader,
				 ipv4OptionAHeaderType);
    phv_factory.push_back_header("ipv4OptionB", ipv4OptionBHeader,
				 ipv4OptionBHeaderType);
    phv_factory.push_back_header("ipv4Padding1", ipv4PaddingHeader1,
				 ipv4PaddingHeaderType);
    phv_factory.push_back_header("ipv4Padding2", ipv4PaddingHeader2,
				 ipv4PaddingHeaderType);
    phv_factory.push_back_header("ipv4Padding3", ipv4PaddingHeader3,
				 ipv4PaddingHeaderType);
    phv_factory.push_back_header("ipv4Padding4", ipv4PaddingHeader4,
				 ipv4PaddingHeaderType);

    const std::vector<header_id_t> hs = {ipv4PaddingHeader1, ipv4PaddingHeader2,
					 ipv4PaddingHeader3, ipv4PaddingHeader4};
    phv_factory.push_back_header_stack(
      "ipv4Padding", ipv4PaddingStack, ipv4PaddingHeaderType, hs
    );
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    // parse_ethernet
    ethernetParseState.add_extract(ethernetHeader);
    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16); // ethertype
    ethernetParseState.set_key_builder(ethernetKeyBuilder);
    ethernetParseState.add_switch_case(ByteContainer("0x0800"),
				       &ipv4ParseState);

    // parse_ipv4
    /* extract(ipv4);
       set(pMeta.byte_counter, ipv4.ihl * 4 - 20);
       return select(ipv4.ihl) {
         0x05 : accept;
         default : parse_ipv4_options;
       }
    */
    ipv4ParseState.add_extract(ipv4Header);
    ArithExpression expr;
    expr.push_back_load_field(ipv4Header, 1); // IHL
    expr.push_back_load_const(Data(4));
    expr.push_back_op(ExprOpcode::MUL);
    expr.push_back_load_const(Data(20));
    expr.push_back_op(ExprOpcode::SUB);
    expr.build();
    ipv4ParseState.add_set_from_expression(pMeta, 0, expr);
    ParseSwitchKeyBuilder ipv4KeyBuilder;
    ipv4KeyBuilder.push_back_field(ipv4Header, 1, 4); // IHL
    ipv4ParseState.set_key_builder(ipv4KeyBuilder);
    ipv4ParseState.add_switch_case(ByteContainer("0x05"), nullptr);
    ipv4ParseState.set_default_switch_case(&ipv4OptionsParseState);

    // parse_ipv4_options
    /* return select(pMeta.byte_counter, current(0, 8)) {
         0x0000 mask 0xff00 : accept;
         0x00aa mask 0x00ff : parse_ipv4_optionA;
         0x00bb mask 0x00ff : parse_ipv4_optionB;
         0x0000 mask 0x00ff : parse_ipv4_padding;
       }
    */
    ParseSwitchKeyBuilder ipv4OptionsKeyBuilder;
    ipv4OptionsKeyBuilder.push_back_field(pMeta, 0, 8); // byte_counter
    ipv4OptionsKeyBuilder.push_back_lookahead(0, 8);
    ipv4OptionsParseState.set_key_builder(ipv4OptionsKeyBuilder);
    ipv4OptionsParseState.add_switch_case_with_mask(
      ByteContainer("0x0000"), ByteContainer("0xff00"), nullptr
    );
    ipv4OptionsParseState.add_switch_case_with_mask(
      ByteContainer("0x00aa"), ByteContainer("0x00ff"), &ipv4OptionAParseState
    );
    ipv4OptionsParseState.add_switch_case_with_mask(
      ByteContainer("0x00bb"), ByteContainer("0x00ff"), &ipv4OptionBParseState
    );
    ipv4OptionsParseState.add_switch_case_with_mask(
      ByteContainer("0x0000"), ByteContainer("0x00ff"), &ipv4PaddingParseState
    );

    // parse_ipv4_optionA
    /* extract(ipv4_optionA);
       set(pMeta.byte_counter, pMeta.byte_counter - 8);
       return parse_ipv4_options;
    */
    ipv4OptionAParseState.add_extract(ipv4OptionAHeader);
    ArithExpression exprA;
    exprA.push_back_load_field(pMeta, 0);
    exprA.push_back_load_const(Data(8));
    exprA.push_back_op(ExprOpcode::SUB);
    exprA.build();
    ipv4OptionAParseState.add_set_from_expression(pMeta, 0, exprA);
    ipv4OptionAParseState.set_default_switch_case(&ipv4OptionsParseState);

    // parse_ipv4_optionB
    /* extract(ipv4_optionB);
       set(pMeta.byte_counter, pMeta.byte_counter - 1);
       return parse_ipv4_options;
    */
    ipv4OptionBParseState.add_extract(ipv4OptionBHeader);
    ArithExpression exprB;
    exprB.push_back_load_field(pMeta, 0);
    exprB.push_back_load_const(Data(1));
    exprB.push_back_op(ExprOpcode::SUB);
    exprB.build();
    ipv4OptionBParseState.add_set_from_expression(pMeta, 0, exprB);
    ipv4OptionBParseState.set_default_switch_case(&ipv4OptionsParseState);

    // parse_ipv4_padding
    /* extract(ipv4_padding[next]);
       set(pMeta.byte_counter, pMeta.byte_counter - 1);
       return parse_ipv4_options;
    */
    ipv4PaddingParseState.add_extract_to_stack(ipv4PaddingStack);
    ArithExpression exprPad;
    exprPad.push_back_load_field(pMeta, 0);
    exprPad.push_back_load_const(Data(1));
    exprPad.push_back_op(ExprOpcode::SUB);
    exprPad.build();
    ipv4PaddingParseState.add_set_from_expression(pMeta, 0, exprPad);
    ipv4PaddingParseState.set_default_switch_case(&ipv4OptionsParseState);

    parser.set_init_state(&ethernetParseState);
  }

  ByteContainer get_ipv4_base() const {
    static const unsigned char base[34] = {
      0x00, 0x18, 0x0a, 0x05, 0x5a, 0x10, 0xa0, 0x88,
      0x69, 0x0c, 0xc3, 0x03, 0x08, 0x00, 0x45, 0x00,
      0x00, 0x34, 0x70, 0x90, 0x40, 0x00, 0x40, 0x06,
      0x35, 0x08, 0x0a, 0x36, 0xc1, 0x21, 0x4e, 0x28,
      0x7b, 0xac
    };
    return ByteContainer((const char *) base, sizeof(base));
  }

  void add_optionA(ByteContainer *buf,
		   const ByteContainer &f1, const ByteContainer &f2) const {
    buf->push_back('\xaa');
    buf->push_back('\x07');
    buf->append(f1);
    buf->append(f2);
  }

  void add_optionB(ByteContainer *buf) const {
    buf->push_back('\xbb');
  }

  void do_padding(ByteContainer *buf) const {
    size_t IHL = buf->size() - 14u; // - ethernet
    size_t IHL_words = (IHL + 3) / 4;
    assert(IHL_words < 16u);
    (*buf)[14] = ((*buf)[14] & 0xf0) | (static_cast<char>(IHL_words));
    // pad
    for(size_t i = 0; i < (IHL_words * 4 - IHL); i++) {
      buf->push_back('\x00');
    }
  }

  void check_base(const PHV &phv) {
    const Header &ethernet_hdr = phv.get_header(ethernetHeader);
    ASSERT_TRUE(ethernet_hdr.is_valid());
    const Header &ipv4_hdr = phv.get_header(ipv4Header);
    ASSERT_TRUE(ipv4_hdr.is_valid());
  }

  void check_optionA(const PHV &phv,
		     const ByteContainer &f1, const ByteContainer &f2) {
    const Header &ipv4OptionA_hdr = phv.get_header(ipv4OptionAHeader);
    ASSERT_TRUE(ipv4OptionA_hdr.is_valid());
    ASSERT_EQ(0xaa, ipv4OptionA_hdr.get_field(0).get_int());
    ASSERT_EQ(0x07, ipv4OptionA_hdr.get_field(1).get_int());
    ASSERT_EQ(f1, ipv4OptionA_hdr.get_field(2).get_bytes());
    ASSERT_EQ(f2, ipv4OptionA_hdr.get_field(3).get_bytes());
  }

  void check_no_optionA(const PHV &phv) {
    const Header &ipv4OptionA_hdr = phv.get_header(ipv4OptionAHeader);
    ASSERT_FALSE(ipv4OptionA_hdr.is_valid());
  }

  void check_optionB(const PHV &phv) {
    const Header &ipv4OptionB_hdr = phv.get_header(ipv4OptionBHeader);
    ASSERT_TRUE(ipv4OptionB_hdr.is_valid());
  }

  void check_no_optionB(const PHV &phv) {
    const Header &ipv4OptionB_hdr = phv.get_header(ipv4OptionBHeader);
    ASSERT_FALSE(ipv4OptionB_hdr.is_valid());
  }

  void check_padding(const PHV &phv, size_t expected_count) {
    const HeaderStack &ipv4Padding_stack =
      phv.get_header_stack(ipv4PaddingStack);
    ASSERT_EQ(expected_count, ipv4Padding_stack.get_count());
  }

  Packet get_pkt(const ByteContainer &buf) const {
    // add one byte at the end of the buffer
    // this is because our parser does a lookahead on the first byte after (to
    // make the parser simpler) and Valgrind complains
    ByteContainer buf_(buf);
    buf_.push_back('\xab');
    return Packet::make_new(buf_.size(),
                            PacketBuffer(512, buf_.data(), buf_.size()),
                            phv_source.get());
  }

  // virtual void TearDown() { }
};

TEST_F(IPv4TLVParsingTest, NoOption) {
  ByteContainer buf = get_ipv4_base();
  Packet packet = get_pkt(buf);
  const PHV &phv = *packet.get_phv();

  parser.parse(&packet);

  check_base(phv);
  check_no_optionA(phv);
  check_no_optionB(phv);
  check_padding(phv, 0u);
}

TEST_F(IPv4TLVParsingTest, OptionA) {
  ByteContainer buf = get_ipv4_base();
  const ByteContainer f1("0x00a1");
  const ByteContainer f2("0x000000a2");
  add_optionA(&buf, f1, f2);
  do_padding(&buf);
  Packet packet = get_pkt(buf);
  const PHV &phv = *packet.get_phv();

  parser.parse(&packet);

  check_base(phv);
  check_optionA(phv, f1, f2);
  check_no_optionB(phv);
  check_padding(phv, 0u);
}

TEST_F(IPv4TLVParsingTest, OptionB) {
  ByteContainer buf = get_ipv4_base();
  add_optionB(&buf);
  do_padding(&buf);
  Packet packet = get_pkt(buf);
  const PHV &phv = *packet.get_phv();

  parser.parse(&packet);

  check_base(phv);
  check_no_optionA(phv);
  check_optionB(phv);
  check_padding(phv, 3u);
}

TEST_F(IPv4TLVParsingTest, BothOptions) {
  const ByteContainer f1("0x00a1");
  const ByteContainer f2("0x000000a2");

  enum class Order { AB, BA };

  for(const auto order: {Order::AB, Order::BA}) {
    ByteContainer buf = get_ipv4_base();

    if(order == Order::AB) {
      add_optionA(&buf, f1, f2);
      add_optionB(&buf);
    }
    else {
      add_optionB(&buf);
      add_optionA(&buf, f1, f2);
    }

    do_padding(&buf);
    Packet packet = get_pkt(buf);
    const PHV &phv = *packet.get_phv();

    parser.parse(&packet);

    check_base(phv);
    check_optionA(phv, f1, f2);
    check_optionB(phv);
    check_padding(phv, 3u);
  }
}


// Google Test fixture for IPv4 Variable Length parsing test
// This test parses the options as one VL field
class IPv4VLParsingTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  HeaderType ethernetHeaderType, ipv4HeaderType;
  ParseState ethernetParseState, ipv4ParseState;
  header_id_t ethernetHeader{0}, ipv4Header{1};

  Parser parser;

  Deparser deparser;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  IPv4VLParsingTest()
      : ethernetHeaderType("ethernet_t", 0), ipv4HeaderType("ipv4_t", 1),
        ethernetParseState("parse_ethernet", 0),
        ipv4ParseState("parse_ipv4", 1),
        parser("test_parser", 0), deparser("test_deparser", 0),
        phv_source(PHVSourceIface::make_phv_source()) {

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
    ArithExpression raw_expr;
    raw_expr.push_back_load_local(1); // IHL
    raw_expr.push_back_load_const(Data(4));
    raw_expr.push_back_op(ExprOpcode::MUL);
    raw_expr.push_back_load_const(Data(20));
    raw_expr.push_back_op(ExprOpcode::SUB);
    raw_expr.push_back_load_const(Data(8));
    raw_expr.push_back_op(ExprOpcode::MUL); // to bits
    raw_expr.build();
    std::unique_ptr<VLHeaderExpression> expr(new VLHeaderExpression(raw_expr));
    ipv4HeaderType.push_back_VL_field("options", std::move(expr));

    phv_factory.push_back_header("ethernet", ethernetHeader,
				 ethernetHeaderType);
    phv_factory.push_back_header("ipv4", ipv4Header, ipv4HeaderType);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    // parse_ethernet
    ethernetParseState.add_extract(ethernetHeader);
    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16); // ethertype
    ethernetParseState.set_key_builder(ethernetKeyBuilder);
    ethernetParseState.add_switch_case(ByteContainer("0x0800"),
				       &ipv4ParseState);

    ipv4ParseState.add_extract(ipv4Header);
    ipv4ParseState.set_default_switch_case(nullptr);

    parser.set_init_state(&ethernetParseState);

    deparser.push_back_header(ethernetHeader);
    deparser.push_back_header(ipv4Header);
  }

  ByteContainer option_value(size_t options_words) const {
    ByteContainer buf;
    for(size_t i = 0; i < options_words * 4; i++)
      buf.push_back(static_cast<char>(options_words));
    return buf;
  }

  ByteContainer get_ipv4_bytes(size_t options_words) const {
    static const unsigned char base[34] = {
      0x00, 0x18, 0x0a, 0x05, 0x5a, 0x10, 0xa0, 0x88,
      0x69, 0x0c, 0xc3, 0x03, 0x08, 0x00, 0x45, 0x00,
      0x00, 0x34, 0x70, 0x90, 0x40, 0x00, 0x40, 0x06,
      0x35, 0x08, 0x0a, 0x36, 0xc1, 0x21, 0x4e, 0x28,
      0x7b, 0xac
    };
    ByteContainer buf((const char *) base, sizeof(base));
    buf.append(option_value(options_words));
    size_t IHL_words = options_words + 5;
    assert(IHL_words < 16u);
    buf[14] = (buf[14] & 0xf0) | (static_cast<char>(IHL_words));
    return buf;
  }

  Packet get_pkt(const ByteContainer &buf) const {
    return Packet::make_new(buf.size(),
                            PacketBuffer(512, buf.data(), buf.size()),
                            phv_source.get());
  }

  void check_base(const PHV &phv, size_t option_words) {
    const Header &ethernet_hdr = phv.get_header(ethernetHeader);
    ASSERT_TRUE(ethernet_hdr.is_valid());
    const Header &ipv4_hdr = phv.get_header(ipv4Header);
    ASSERT_TRUE(ipv4_hdr.is_valid());
    ASSERT_TRUE(ipv4_hdr.is_VL_header());
    ASSERT_EQ(20 + option_words * 4, ipv4_hdr.get_nbytes_packet());
  }

  void check_option(const PHV &phv, size_t option_words,
		    const ByteContainer &v) {
    assert(v.size() == option_words * 4);
    const Field &f_options = phv.get_field(ipv4Header, 12);
    ASSERT_EQ(option_words * 4 * 8, f_options.get_nbits());
    ASSERT_EQ(option_words * 4, f_options.get_nbytes());
    ASSERT_EQ(v, f_options.get_bytes());
  }

  template<size_t OptionWords>
  void test() {
    const ByteContainer buf = get_ipv4_bytes(OptionWords);
    Packet packet = get_pkt(buf);
    const PHV &phv = *packet.get_phv();

    parser.parse(&packet);

    check_base(phv, OptionWords);
    ByteContainer expected_value = option_value(OptionWords);
    check_option(phv, OptionWords, expected_value);
  }

  // virtual void TearDown() { }
};

TEST_F(IPv4VLParsingTest, NoOption) {
  test<0u>();
}

TEST_F(IPv4VLParsingTest, SmallOption) {
  test<3u>();
}

TEST_F(IPv4VLParsingTest, BigOption) {
  test<9u>(); // max value
}

TEST_F(IPv4VLParsingTest, Deparser) {
  const size_t option_words = 4;
  const ByteContainer buf = get_ipv4_bytes(option_words);
  const ByteContainer buf_save = buf;
  Packet packet = get_pkt(buf);
  const PHV &phv = *packet.get_phv();

  parser.parse(&packet);

  check_base(phv, option_words);
  ByteContainer expected_value = option_value(option_words);
  check_option(phv, option_words, expected_value);

  deparser.deparse(&packet);

  ASSERT_EQ(buf_save.size(), packet.get_data_size());
  ASSERT_EQ(0, memcmp(buf_save.data(), packet.data(), buf_save.size()));
}


class ParseVSetTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  HeaderType headerType;
  ParseState parseState;
  header_id_t header_id{0};

  Parser parser;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  ParseVSetTest()
      : headerType("header_t", 0),
        parseState("parse_header", 0),
        parser("test_parser", 0),
        phv_source(PHVSourceIface::make_phv_source()) {

    headerType.push_back_field("f16", 16);
    headerType.push_back_field("f32", 32);
    headerType.push_back_field("f8", 8);
    headerType.push_back_field("f3", 3);
    headerType.push_back_field("f13", 13);

    phv_factory.push_back_header("header", header_id, headerType);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    parser.set_init_state(&parseState);
  }

  void set_parse_kb(const std::vector<std::pair<int, int> > &fields) {
    ParseSwitchKeyBuilder kb;
    for (const auto &f : fields)
      kb.push_back_field(header_id, f.first, f.second);
    parseState.set_key_builder(kb);
  }

  Packet get_pkt() const {
    Packet pkt = Packet::make_new(phv_source.get());
    pkt.get_phv()->get_header(header_id).mark_valid();
    return pkt;
  }

  // virtual void TearDown() { }
};

TEST_F(ParseVSetTest, Basic) {
  ParseVSet vset("vset", 0, 24);
  set_parse_kb({{0, 16}, {2, 8}});  // f16, f8
  ParseState dummy1("dummy1", 1);
  ParseState dummy2("dummy2", 2);
  int f16_v, f8_v;
  size_t bytes_parsed = 0;

  auto pkt = get_pkt();
  PHV *phv = pkt.get_phv();

  f16_v = 0xabcd; f8_v = 0xef;
  phv->get_field(header_id, 0).set(f16_v);
  phv->get_field(header_id, 2).set(f8_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  ByteContainer mask("0xff00ff");
  parseState.add_switch_case_vset(&vset, &dummy1);
  parseState.add_switch_case_vset_with_mask(&vset, mask, &dummy2);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  ByteContainer v1("0xabcdef");
  vset.add(v1);
  ASSERT_EQ(&dummy1, parseState(&pkt, nullptr, &bytes_parsed));

  f16_v = 0xab11; f8_v = 0xef;
  phv->get_field(header_id, 0).set(f16_v);
  phv->get_field(header_id, 2).set(f8_v);
  ASSERT_EQ(&dummy2, parseState(&pkt, nullptr, &bytes_parsed));

  f16_v = 0x1b11; f8_v = 0xef;
  phv->get_field(header_id, 0).set(f16_v);
  phv->get_field(header_id, 2).set(f8_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  vset.remove(v1);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));
}

TEST_F(ParseVSetTest, OneField) {
  ParseVSet vset("vset", 0, 16);
  set_parse_kb({{0, 16}});  // f16
  ParseState dummy1("dummy1", 1);
  int f16_v;
  size_t bytes_parsed = 0;

  auto pkt = get_pkt();
  PHV *phv = pkt.get_phv();
  f16_v = 0xabcd;
  phv->get_field(header_id, 0).set(f16_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  parseState.add_switch_case_vset(&vset, &dummy1);
  ByteContainer v1("0xabcd");
  vset.add(v1);
  ASSERT_EQ(&dummy1, parseState(&pkt, nullptr, &bytes_parsed));

  f16_v = 0xab0d;
  phv->get_field(header_id, 0).set(f16_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));
}

TEST_F(ParseVSetTest, OneFieldNonAligned) {
  ParseVSet vset("vset", 0, 13);
  set_parse_kb({{4, 13}});  // f13
  ParseState dummy1("dummy1", 1);
  int f13_v;
  size_t bytes_parsed = 0;

  auto pkt = get_pkt();
  PHV *phv = pkt.get_phv();
  f13_v = 0x12e6;
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  parseState.add_switch_case_vset(&vset, &dummy1);
  ByteContainer v1("0x12e6");
  vset.add(v1);
  ASSERT_EQ(&dummy1, parseState(&pkt, nullptr, &bytes_parsed));

  f13_v = 0x02e6;
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  f13_v = 0x12e7;
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));
}

TEST_F(ParseVSetTest, NonAlignedWithMask) {
  ParseVSet vset("vset", 0, 16);
  set_parse_kb({{3, 3}, {4, 13}});  // f3, f13
  ParseState dummy1("dummy1", 1);
  int f3_v, f13_v;
  size_t bytes_parsed = 0;

  auto pkt = get_pkt();
  PHV *phv = pkt.get_phv();
  f3_v = 0x5; f13_v = 0x12e6;
  phv->get_field(header_id, 3).set(f3_v);
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  ByteContainer mask("0x0201c0");
  parseState.add_switch_case_vset_with_mask(&vset, mask, &dummy1);
  ByteContainer v1("0xb2e6");  // compressed
  vset.add(v1);
  ASSERT_TRUE(vset.contains(v1));
  // testing with exactly v1
  ASSERT_EQ(&dummy1, parseState(&pkt, nullptr, &bytes_parsed));

  // change f3 to break match
  f3_v = 0x7; f13_v = 0x12e6;
  phv->get_field(header_id, 3).set(f3_v);
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  // change f13 to break match
  f3_v = 0x5; f13_v = 0x1266;
  phv->get_field(header_id, 3).set(f3_v);
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));

  // change f3 without breaking match
  f3_v = 0x4; f13_v = 0x12e6;
  phv->get_field(header_id, 3).set(f3_v);
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(&dummy1, parseState(&pkt, nullptr, &bytes_parsed));

  // change f13 without breaking match
  f3_v = 0x5; f13_v = 0x16e7;
  phv->get_field(header_id, 3).set(f3_v);
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(&dummy1, parseState(&pkt, nullptr, &bytes_parsed));

  // removing v1
  vset.remove(v1);
  ASSERT_FALSE(vset.contains(v1));
  f3_v = 0x5; f13_v = 0x12e6;
  phv->get_field(header_id, 3).set(f3_v);
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(nullptr, parseState(&pkt, nullptr, &bytes_parsed));
}

extern bool WITH_VALGRIND; // defined in main.cpp

// test added after I detected a race condition in ParseVSet's shadow copies
TEST_F(ParseVSetTest, ConcurrentRuntimeAccess) {
  ParseVSet vset("vset", 0, 16);
  set_parse_kb({{3, 3}, {4, 13}});  // f3, f13
  ParseState dummy1("dummy1", 1);
  int f3_v, f13_v;
  size_t bytes_parsed = 0;

  parseState.add_switch_case_vset(&vset, &dummy1);
  ByteContainer v1("0xb2e6");  // compressed
  vset.add(v1);
  ASSERT_TRUE(vset.contains(v1));

  // testing with exactly v1
  auto pkt = get_pkt();
  PHV *phv = pkt.get_phv();
  f3_v = 0x5; f13_v = 0x12e6;
  phv->get_field(header_id, 3).set(f3_v);
  phv->get_field(header_id, 4).set(f13_v);
  ASSERT_EQ(&dummy1, parseState(&pkt, nullptr, &bytes_parsed));

  std::mutex m1;
  std::mutex m2;
  using clock = std::chrono::system_clock;

  auto runtime_access = [&vset, &v1](std::mutex &m, bool do_add,
                                     clock::time_point end) {
    while (clock::now() < end) {
      std::unique_lock<std::mutex> L(m);
      if (do_add) vset.add(v1);
      else vset.remove(v1);
    }
  };

  std::vector<bool> consistency_results;
  auto consistency_check = [&vset, &v1, &m1, &m2, &consistency_results,
                            &dummy1, &pkt, this](
      clock::time_point end) {
    size_t bparsed = 0;
    while (clock::now() < end) {
      std::unique_lock<std::mutex> L1(m1, std::defer_lock);
      std::unique_lock<std::mutex> L2(m2, std::defer_lock);
      std::lock(L1, L2);
      bool r1 = vset.contains(v1);
      bool r2 = (&dummy1 == parseState(&pkt, nullptr, &bparsed));
      consistency_results.push_back(r1 == r2);
    }
  };

  int runtime_secs = WITH_VALGRIND ? 20 : 5;
  clock::time_point end = clock::now() + std::chrono::seconds(runtime_secs);
  std::thread t1(runtime_access, std::ref(m1), true, end);
  std::thread t2(runtime_access, std::ref(m2), false, end);
  std::thread t3(consistency_check, end);
  t1.join(); t2.join(); t3.join();
  size_t count = std::count(
      consistency_results.begin(), consistency_results.end(), false);
  EXPECT_LT(0, consistency_results.size());
  ASSERT_EQ(0, count);
}
