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

#include <boost/filesystem.hpp>

#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/deparser.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/phv_source.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/P4Objects.h>

#include <chrono>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <utility>  // for std::pair

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

class ParserTestGeneric : public ::testing::Test {
 protected:
  PHVFactory phv_factory{};
  ErrorCodeMap error_codes;
  Parser parser;
  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  ParserTestGeneric()
      : error_codes(ErrorCodeMap::make_with_core()),
        parser("test_parser", 0, &error_codes),
        phv_source(PHVSourceIface::make_phv_source()),
        no_error(error_codes.from_core(ErrorCodeMap::Core::NoError)) { }

  void parse_and_check_no_error(Packet *packet) {
    parser.parse(packet);
    ASSERT_EQ(no_error, packet->get_error_code());
  }

  void parse_and_check_error(Packet *packet, ErrorCodeMap::Core core) {
    parser.parse(packet);
    ASSERT_EQ(error_codes.from_core(core), packet->get_error_code());
  }

 private:
  ErrorCode no_error;
};

// Google Test fixture for parser tests
class ParserTest : public ParserTestGeneric {
 protected:
  HeaderType ethernetHeaderType, ipv4HeaderType, udpHeaderType, tcpHeaderType;
  ParseState ethernetParseState, ipv4ParseState, udpParseState, tcpParseState;
  header_id_t ethernetHeader{0}, ipv4Header{1}, udpHeader{2}, tcpHeader{3};

  Deparser deparser;

  ParserTest()
      : ethernetHeaderType("ethernet_t", 0), ipv4HeaderType("ipv4_t", 1),
        udpHeaderType("udp_t", 2), tcpHeaderType("tcp_t", 3),
        ethernetParseState("parse_ethernet", 0),
        ipv4ParseState("parse_ipv4", 1),
        udpParseState("parse_udp", 2),
        tcpParseState("parse_tcp", 3),
        deparser("test_deparser", 0) {
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

    phv_factory.push_back_header("ethernet", ethernetHeader,
                                 ethernetHeaderType);
    phv_factory.push_back_header("ipv4", ipv4Header, ipv4HeaderType);
    phv_factory.push_back_header("udp", udpHeader, udpHeaderType);
    phv_factory.push_back_header("tcp", tcpHeader, tcpHeaderType);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16);  // ethertype
    ethernetParseState.set_key_builder(ethernetKeyBuilder);

    ParseSwitchKeyBuilder ipv4KeyBuilder;
    ipv4KeyBuilder.push_back_field(ipv4Header, 8, 8);  // protocol
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
  auto packet = get_tcp_pkt();
  auto phv = packet.get_phv();
  parse_and_check_no_error(&packet);

  const auto &ethernet_hdr = phv->get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());

  const auto &ipv4_hdr = phv->get_header(ipv4Header);
  ASSERT_TRUE(ipv4_hdr.is_valid());

  const auto &ipv4_version = phv->get_field(ipv4Header, 0);
  ASSERT_EQ(0x4u, ipv4_version.get_uint());

  const auto &ipv4_ihl = phv->get_field(ipv4Header, 1);
  ASSERT_EQ(0x5u, ipv4_ihl.get_uint());

  const auto &ipv4_diffserv = phv->get_field(ipv4Header, 2);
  ASSERT_EQ(0x00u, ipv4_diffserv.get_uint());

  const auto &ipv4_len = phv->get_field(ipv4Header, 3);
  ASSERT_EQ(0x0034u, ipv4_len.get_uint());

  const auto &ipv4_identification = phv->get_field(ipv4Header, 4);
  ASSERT_EQ(0x7090u, ipv4_identification.get_uint());

  const auto &ipv4_flags = phv->get_field(ipv4Header, 5);
  ASSERT_EQ(0x2u, ipv4_flags.get_uint());

  const auto &ipv4_flagOffset = phv->get_field(ipv4Header, 6);
  ASSERT_EQ(0x0000u, ipv4_flagOffset.get_uint());

  const auto &ipv4_ttl = phv->get_field(ipv4Header, 7);
  ASSERT_EQ(0x40u, ipv4_ttl.get_uint());

  const auto &ipv4_protocol = phv->get_field(ipv4Header, 8);
  ASSERT_EQ(0x06u, ipv4_protocol.get_uint());

  const auto &ipv4_checksum = phv->get_field(ipv4Header, 9);
  ASSERT_EQ(0x3508u, ipv4_checksum.get_uint());

  const auto &ipv4_srcAddr = phv->get_field(ipv4Header, 10);
  ASSERT_EQ(0x0a36c121u, ipv4_srcAddr.get_uint());

  const auto &ipv4_dstAddr = phv->get_field(ipv4Header, 11);
  ASSERT_EQ(0x4e287bacu, ipv4_dstAddr.get_uint());

  const auto &tcp_hdr = phv->get_header(tcpHeader);
  ASSERT_TRUE(tcp_hdr.is_valid());

  const auto &udp_hdr = phv->get_header(udpHeader);
  ASSERT_FALSE(udp_hdr.is_valid());
}

TEST_F(ParserTest, ParseEthernetIPv4UDP) {
  auto packet = get_udp_pkt();
  auto phv = packet.get_phv();
  parse_and_check_no_error(&packet);

  const auto &ethernet_hdr = phv->get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());

  const auto &ipv4_hdr = phv->get_header(ipv4Header);
  ASSERT_TRUE(ipv4_hdr.is_valid());

  const auto &udp_hdr = phv->get_header(udpHeader);
  ASSERT_TRUE(udp_hdr.is_valid());

  const auto &tcp_hdr = phv->get_header(tcpHeader);
  ASSERT_FALSE(tcp_hdr.is_valid());
}


TEST_F(ParserTest, ParseEthernetIPv4TCP_Stress) {
  for (int t = 0; t < 10000; t++) {
    auto packet = get_tcp_pkt();
    auto phv = packet.get_phv();
    parse_and_check_no_error(&packet);

    ASSERT_TRUE(phv->get_header(ethernetHeader).is_valid());
    ASSERT_TRUE(phv->get_header(ipv4Header).is_valid());
    ASSERT_TRUE(phv->get_header(tcpHeader).is_valid());
    ASSERT_FALSE(phv->get_header(udpHeader).is_valid());
  }
}

TEST_F(ParserTest, DeparseEthernetIPv4TCP) {
  auto packet = get_tcp_pkt();
  parse_and_check_no_error(&packet);

  deparser.deparse(&packet);

  ASSERT_EQ(sizeof(raw_tcp_pkt), packet.get_data_size());
  ASSERT_EQ(0, memcmp(raw_tcp_pkt, packet.data(), sizeof(raw_tcp_pkt)));
}

TEST_F(ParserTest, DeparseEthernetIPv4UDP) {
  auto packet = get_udp_pkt();
  parse_and_check_no_error(&packet);

  deparser.deparse(&packet);

  ASSERT_EQ(sizeof(raw_udp_pkt), packet.get_data_size());
  ASSERT_EQ(0, memcmp(raw_udp_pkt, packet.data(), sizeof(raw_udp_pkt)));
}

TEST_F(ParserTest, DeparseEthernetIPv4_Stress) {
  const char *ref_pkt;
  size_t size;

  auto packet = Packet::make_new(phv_source.get());
  for (int t = 0; t < 10000; t++) {
    if (t % 2 == 0) {
      packet = get_tcp_pkt();
      ref_pkt = (const char *) raw_tcp_pkt;
      size = sizeof(raw_tcp_pkt);
    } else {
      packet = get_udp_pkt();
      ref_pkt = (const char *) raw_udp_pkt;
      size = sizeof(raw_udp_pkt);
    }
    parse_and_check_no_error(&packet);
    deparser.deparse(&packet);
    ASSERT_EQ(0, memcmp(ref_pkt, packet.data(), size));
  }
}

TEST(LookAhead, Peek) {
  ByteContainer res;
  // 1011 0101, 1001 1101, 1111 1101, 0001 0111, 1101 0101, 1101 0111
  const unsigned char data_[6] = {0xb5, 0x9d, 0xfd, 0x17, 0xd5, 0xd7};
  const char *data = reinterpret_cast<const char *>(data_);

  auto lookahead1 = ParserLookAhead::make(0, 16);
  lookahead1.peek(data, &res);
  ASSERT_EQ(ByteContainer("0xb59d"), res);
  res.clear();

  auto lookahead2 = ParserLookAhead::make(3, 16);
  lookahead2.peek(data, &res);
  // 1010 1100, 1110 1111
  ASSERT_EQ(ByteContainer("0xacef"), res);
  res.clear();

  auto lookahead3 = ParserLookAhead::make(0, 21);
  lookahead3.peek(data, &res);
  // 0001 0110, 1011 0011, 1011 1111
  ASSERT_EQ(ByteContainer("0x16b3bf"), res);
  res.clear();

  auto lookahead4 = ParserLookAhead::make(18, 15);
  lookahead4.peek(data, &res);
  // 0111 1010, 0010 1111
  ASSERT_EQ(ByteContainer("0x7a2f"), res);
  res.clear();

  auto lookahead5_1 = ParserLookAhead::make(0, 16);
  lookahead5_1.peek(data, &res);
  auto lookahead5_2 = ParserLookAhead::make(16, 16);
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

  Packet get_pkt(const char *data, size_t size) {
    return Packet::make_new(
        size, PacketBuffer(size + 128, data, size), phv_source.get());
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }
};

TEST_F(ParserOpSetTest, SetFromData) {
  Data src("0xaba");
  ParserOpSet<Data> op(testHeader1, 1, src);  // f32
  ParserOp &opRef = op;
  Packet pkt = get_pkt();
  Field &f = pkt.get_phv()->get_field(testHeader1, 1);
  ASSERT_EQ(0u, f.get_uint());
  opRef(&pkt, nullptr, nullptr);
  ASSERT_EQ(0xaba, f.get_uint());
}

TEST_F(ParserOpSetTest, SetFromField) {
  ParserOpSet<field_t> op(testHeader1, 1,
                          field_t::make(testHeader2, 1));  // f32
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
  const char *data = reinterpret_cast<const char *>(data_);

  auto test = [data, this](int offset, int bitwidth, unsigned int v) {
    auto lookahead = ParserLookAhead::make(offset, bitwidth);
    const ParserOpSet<ParserLookAhead> op(testHeader1, 1, lookahead);  // f32
    const ParserOp &opRef = op;
    auto pkt = get_pkt(data, sizeof(data_));
    auto &f = pkt.get_phv()->get_field(testHeader1, 1);
    f.set(0);
    size_t bytes_parsed = 0;
    opRef(&pkt, data, &bytes_parsed);
    ASSERT_EQ(v, f.get_uint());
  };

  test(0, 32, 0xb59dfd17);
  test(8, 8, 0x9d);
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
    phv_factory.push_back_header("test_stack_1", testHeaderStack1,
                                 testHeaderType);
    phv_factory.push_back_header("test_stack_2", testHeaderStack2,
                                 testHeaderType);
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
  const char *data = reinterpret_cast<const char *>(data_);

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
  ByteContainer expected(
      "0xaabbccddeeffb59d112200abababfd17d5d70d17d544332211");
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
class MPLSParserTest : public ParserTestGeneric {
 protected:
  HeaderType ethernetHeaderType, MPLSHeaderType;
  ParseState ethernetParseState, MPLSParseState;
  header_id_t ethernetHeader{0};
  header_id_t MPLSHeader1{1}, MPLSHeader2{2}, MPLSHeader3{3}, MPLSHeader4{4};

  header_stack_id_t MPLSStack{0};

  Deparser deparser;

  MPLSParserTest()
      : ethernetHeaderType("ethernet_t", 0), MPLSHeaderType("mpls_t", 1),
        ethernetParseState("parse_ethernet", 0),
        MPLSParseState("parse_mpls", 1),
        deparser("test_deparser", 0) {
    ethernetHeaderType.push_back_field("dstAddr", 48);
    ethernetHeaderType.push_back_field("srcAddr", 48);
    ethernetHeaderType.push_back_field("ethertype", 16);

    MPLSHeaderType.push_back_field("label", 20);
    MPLSHeaderType.push_back_field("exp", 3);
    MPLSHeaderType.push_back_field("bos", 1);
    MPLSHeaderType.push_back_field("ttl", 8);

    phv_factory.push_back_header("ethernet", ethernetHeader,
                                 ethernetHeaderType);
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
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16);  // ethertype
    ethernetParseState.set_key_builder(ethernetKeyBuilder);

    ParseSwitchKeyBuilder MPLSKeyBuilder;
    MPLSKeyBuilder.push_back_stack_field(MPLSStack, 2, 1);  // bos
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
    // TODO(antonin)
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
  auto packet = get_mpls_pkt();
  auto phv = packet.get_phv();
  parse_and_check_no_error(&packet);

  const auto &ethernet_hdr = phv->get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());

  const auto &MPLS_hdr_1 = phv->get_header(MPLSHeader1);
  ASSERT_TRUE(MPLS_hdr_1.is_valid());
  const auto &MPLS_hdr_2 = phv->get_header(MPLSHeader2);
  ASSERT_TRUE(MPLS_hdr_2.is_valid());
  const auto &MPLS_hdr_3 = phv->get_header(MPLSHeader3);
  ASSERT_TRUE(MPLS_hdr_3.is_valid());
  const auto &MPLS_hdr_4 = phv->get_header(MPLSHeader4);
  ASSERT_FALSE(MPLS_hdr_4.is_valid());

  ASSERT_EQ(1u, MPLS_hdr_1.get_field(0).get_uint());  // label
  ASSERT_EQ(2u, MPLS_hdr_2.get_field(0).get_uint());  // label
  ASSERT_EQ(3u, MPLS_hdr_3.get_field(0).get_uint());  // label
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
    for (auto c : bc)
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
  auto cmp_w_mask = [this](const ByteContainer &mask, const ByteContainer &key,
                           int v) {
    return (v & bc_as_uint(mask)) == (bc_as_uint(key) & bc_as_uint(mask));
  };
  for (int i = 0; i < 65536; i++) {
    if (cmp_w_mask(mask_1, key_1, i))
      expected_next_states[i] = &next_state_1;
    else if (cmp_w_mask(mask_2, key_2, i))
      expected_next_states[i] = &next_state_2;
    else if (cmp_w_mask(mask_3, key_3, i))
      expected_next_states[i] = &next_state_3;
    else
      expected_next_states[i] = nullptr;
  }

  Packet packet = get_pkt();
  for (int i = 0; i < 65536; i++) {
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
class IPv4TLVParsingTest : public ParserTestGeneric {
 protected:
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
        ipv4PaddingParseState("parse_ipv4_padding", 5) {
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

    const std::vector<header_id_t> hs =
        {ipv4PaddingHeader1, ipv4PaddingHeader2,
         ipv4PaddingHeader3, ipv4PaddingHeader4};
    phv_factory.push_back_header_stack(
      "ipv4Padding", ipv4PaddingStack, ipv4PaddingHeaderType, hs);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    // parse_ethernet
    ethernetParseState.add_extract(ethernetHeader);
    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16);  // ethertype
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
    expr.push_back_load_field(ipv4Header, 1);  // IHL
    expr.push_back_load_const(Data(4));
    expr.push_back_op(ExprOpcode::MUL);
    expr.push_back_load_const(Data(20));
    expr.push_back_op(ExprOpcode::SUB);
    expr.build();
    ipv4ParseState.add_set_from_expression(pMeta, 0, expr);
    ParseSwitchKeyBuilder ipv4KeyBuilder;
    ipv4KeyBuilder.push_back_field(ipv4Header, 1, 4);  // IHL
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
    ipv4OptionsKeyBuilder.push_back_field(pMeta, 0, 8);  // byte_counter
    ipv4OptionsKeyBuilder.push_back_lookahead(0, 8);
    ipv4OptionsParseState.set_key_builder(ipv4OptionsKeyBuilder);
    ipv4OptionsParseState.add_switch_case_with_mask(
      ByteContainer("0x0000"), ByteContainer("0xff00"), nullptr);
    ipv4OptionsParseState.add_switch_case_with_mask(
      ByteContainer("0x00aa"), ByteContainer("0x00ff"), &ipv4OptionAParseState);
    ipv4OptionsParseState.add_switch_case_with_mask(
      ByteContainer("0x00bb"), ByteContainer("0x00ff"), &ipv4OptionBParseState);
    ipv4OptionsParseState.add_switch_case_with_mask(
      ByteContainer("0x0000"), ByteContainer("0x00ff"), &ipv4PaddingParseState);

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
    size_t IHL = buf->size() - 14u;  // - ethernet
    size_t IHL_words = (IHL + 3) / 4;
    assert(IHL_words < 16u);
    (*buf)[14] = ((*buf)[14] & 0xf0) | (static_cast<char>(IHL_words));
    // pad
    for (size_t i = 0; i < (IHL_words * 4 - IHL); i++) {
      buf->push_back('\x00');
    }
  }

  void check_base(const PHV &phv) {
    const auto &ethernet_hdr = phv.get_header(ethernetHeader);
    ASSERT_TRUE(ethernet_hdr.is_valid());
    const auto &ipv4_hdr = phv.get_header(ipv4Header);
    ASSERT_TRUE(ipv4_hdr.is_valid());
  }

  void check_optionA(const PHV &phv,
                     const ByteContainer &f1, const ByteContainer &f2) {
    const auto &ipv4OptionA_hdr = phv.get_header(ipv4OptionAHeader);
    ASSERT_TRUE(ipv4OptionA_hdr.is_valid());
    ASSERT_EQ(0xaa, ipv4OptionA_hdr.get_field(0).get_int());
    ASSERT_EQ(0x07, ipv4OptionA_hdr.get_field(1).get_int());
    ASSERT_EQ(f1, ipv4OptionA_hdr.get_field(2).get_bytes());
    ASSERT_EQ(f2, ipv4OptionA_hdr.get_field(3).get_bytes());
  }

  void check_no_optionA(const PHV &phv) {
    const auto &ipv4OptionA_hdr = phv.get_header(ipv4OptionAHeader);
    ASSERT_FALSE(ipv4OptionA_hdr.is_valid());
  }

  void check_optionB(const PHV &phv) {
    const auto &ipv4OptionB_hdr = phv.get_header(ipv4OptionBHeader);
    ASSERT_TRUE(ipv4OptionB_hdr.is_valid());
  }

  void check_no_optionB(const PHV &phv) {
    const auto &ipv4OptionB_hdr = phv.get_header(ipv4OptionBHeader);
    ASSERT_FALSE(ipv4OptionB_hdr.is_valid());
  }

  void check_padding(const PHV &phv, size_t expected_count) {
    const auto &ipv4Padding_stack = phv.get_header_stack(ipv4PaddingStack);
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
  auto packet = get_pkt(buf);
  const auto &phv = *packet.get_phv();

  parse_and_check_no_error(&packet);

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
  auto packet = get_pkt(buf);
  const auto &phv = *packet.get_phv();

  parse_and_check_no_error(&packet);

  check_base(phv);
  check_optionA(phv, f1, f2);
  check_no_optionB(phv);
  check_padding(phv, 0u);
}

TEST_F(IPv4TLVParsingTest, OptionB) {
  ByteContainer buf = get_ipv4_base();
  add_optionB(&buf);
  do_padding(&buf);
  auto packet = get_pkt(buf);
  const auto &phv = *packet.get_phv();

  parse_and_check_no_error(&packet);

  check_base(phv);
  check_no_optionA(phv);
  check_optionB(phv);
  check_padding(phv, 3u);
}

TEST_F(IPv4TLVParsingTest, BothOptions) {
  const ByteContainer f1("0x00a1");
  const ByteContainer f2("0x000000a2");

  enum class Order { AB, BA };

  for (const auto order : {Order::AB, Order::BA}) {
    ByteContainer buf = get_ipv4_base();

    if (order == Order::AB) {
      add_optionA(&buf, f1, f2);
      add_optionB(&buf);
    } else {
      add_optionB(&buf);
      add_optionA(&buf, f1, f2);
    }

    do_padding(&buf);
    auto packet = get_pkt(buf);
    const auto &phv = *packet.get_phv();

    parse_and_check_no_error(&packet);

    check_base(phv);
    check_optionA(phv, f1, f2);
    check_optionB(phv);
    check_padding(phv, 3u);
  }
}


using ::testing::WithParamInterface;
using ::testing::Range;

// Google Test fixture for IPv4 Variable Length parsing test
// This test parses the options as one VL field
class IPv4VLParsingTest : public ParserTestGeneric,
                          public WithParamInterface<int> {
 protected:
  HeaderType ethernetHeaderType, ipv4HeaderType;
  ParseState ethernetParseState, ipv4ParseState;
  header_id_t ethernetHeader{0}, ipv4Header{1};

  Deparser deparser;

  IPv4VLParsingTest()
      : ethernetHeaderType("ethernet_t", 0), ipv4HeaderType("ipv4_t", 1),
        ethernetParseState("parse_ethernet", 0),
        ipv4ParseState("parse_ipv4", 1),
        deparser("test_deparser", 0) {
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
    raw_expr.push_back_load_local(1);  // IHL
    raw_expr.push_back_load_const(Data(4));
    raw_expr.push_back_op(ExprOpcode::MUL);
    raw_expr.push_back_load_const(Data(20));
    raw_expr.push_back_op(ExprOpcode::SUB);
    raw_expr.push_back_load_const(Data(8));
    raw_expr.push_back_op(ExprOpcode::MUL);  // to bits
    raw_expr.build();
    std::unique_ptr<VLHeaderExpression> expr(new VLHeaderExpression(raw_expr));
    ipv4HeaderType.push_back_VL_field("options", 320  /* max header bytes */,
                                      std::move(expr));

    phv_factory.push_back_header("ethernet", ethernetHeader,
                                 ethernetHeaderType);
    phv_factory.push_back_header("ipv4", ipv4Header, ipv4HeaderType);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    // parse_ethernet
    ethernetParseState.add_extract(ethernetHeader);
    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.push_back_field(ethernetHeader, 2, 16);  // ethertype
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
    for (size_t i = 0; i < options_words * 4; i++)
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
    const auto &ethernet_hdr = phv.get_header(ethernetHeader);
    ASSERT_TRUE(ethernet_hdr.is_valid());
    const auto &ipv4_hdr = phv.get_header(ipv4Header);
    ASSERT_TRUE(ipv4_hdr.is_valid());
    ASSERT_TRUE(ipv4_hdr.is_VL_header());
    ASSERT_EQ(20 + option_words * 4, ipv4_hdr.get_nbytes_packet());
  }

  void check_option(const PHV &phv, size_t option_words,
                    const ByteContainer &v) {
    assert(v.size() == option_words * 4);
    const auto &f_options = phv.get_field(ipv4Header, 12);
    ASSERT_EQ(option_words * 4 * 8, f_options.get_nbits());
    ASSERT_EQ(option_words * 4, f_options.get_nbytes());
    ASSERT_EQ(v, f_options.get_bytes());
  }

  // virtual void TearDown() { }
};

TEST_P(IPv4VLParsingTest, ParseAndDeparse) {
  const auto OptionWords = GetParam();

  const ByteContainer buf = get_ipv4_bytes(OptionWords);
  const ByteContainer buf_save = buf;
  auto packet = get_pkt(buf);
  const auto &phv = *packet.get_phv();

  parse_and_check_no_error(&packet);

  check_base(phv, OptionWords);
  ByteContainer expected_value = option_value(OptionWords);
  check_option(phv, OptionWords, expected_value);

  deparser.deparse(&packet);

  ASSERT_EQ(buf_save.size(), packet.get_data_size());
  ASSERT_EQ(0, memcmp(buf_save.data(), packet.data(), buf_save.size()));
}

INSTANTIATE_TEST_CASE_P(IPv4VLOptionWords, IPv4VLParsingTest,
                        Range(0, 10));


class ParseVSetTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  HeaderType headerType;
  ParseState parseState;
  header_id_t header_id{0};

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  ParseVSetTest()
      : headerType("header_t", 0),
        parseState("parse_header", 0),
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

extern bool WITH_VALGRIND;  // defined in main.cpp

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
      if (do_add)
        vset.add(v1);
      else
        vset.remove(v1);
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


class ParserErrorTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory{};
  ErrorCodeMap error_codes;
  Parser parser;
  ParseState parse_state;
  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  ParserErrorTest(ErrorCodeMap error_codes)
      : error_codes(std::move(error_codes)),
        parser("test_parser", 0, &this->error_codes),
        parse_state("test_parse_state", 0),
        phv_source(PHVSourceIface::make_phv_source()),
        no_error(this->error_codes.from_core(ErrorCodeMap::Core::NoError)) { }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    parser.set_init_state(&parse_state);
  }

  // virtual void TearDown() { }

  void parse_and_check_no_error(Packet *packet) {
    parser.parse(packet);
    ASSERT_EQ(no_error, packet->get_error_code());
  }

  void parse_and_check_error(Packet *packet, ErrorCodeMap::Core core) {
    parser.parse(packet);
    ASSERT_EQ(error_codes.from_core(core), packet->get_error_code());
  }

  void parse_and_check_error(Packet *packet, const std::string &name) {
    parser.parse(packet);
    ASSERT_EQ(error_codes.from_name(name), packet->get_error_code());
  }

  Packet get_pkt(int length) {
    std::vector<char> data(length, 0);
    PacketBuffer buffer(length * 2, data.data(), data.size());
    return Packet::make_new(length, std::move(buffer), phv_source.get());
  }

 private:
  ErrorCode no_error;
};

class ParserCoreErrorsTest : public ParserErrorTest {
 protected:
  static constexpr int packet_nbytes = 2;

  ParserCoreErrorsTest()
      : ParserErrorTest(ErrorCodeMap::make_with_core()) { }
};

class ParserPacketTooShortTest : public ParserCoreErrorsTest {
 protected:
  HeaderType testHeaderType;
  header_id_t testHeader{0};

  // needs to be larger than the number of available packet bytes
  static constexpr int header_nbits = (packet_nbytes + 1) * 8;

  ParserPacketTooShortTest()
      : testHeaderType("test_header_t", 0) {
    testHeaderType.push_back_field("f", (packet_nbytes + 1) * 8);
    phv_factory.push_back_header("test_header", testHeader, testHeaderType);
  }
};

TEST_F(ParserPacketTooShortTest, Extract) {
  auto packet = get_pkt(packet_nbytes);
  parse_state.add_extract(testHeader);
  parse_and_check_error(&packet, ErrorCodeMap::Core::PacketTooShort);
}

TEST_F(ParserPacketTooShortTest, LookAhead) {
  auto packet = get_pkt(packet_nbytes);
  parse_state.add_set_from_lookahead(testHeader, 0, 0, header_nbits);
  parse_and_check_error(&packet, ErrorCodeMap::Core::PacketTooShort);
}

TEST_F(ParserPacketTooShortTest, Shift) {
  auto packet = get_pkt(packet_nbytes);
  parse_state.add_shift(packet_nbytes + 1);
  parse_and_check_error(&packet, ErrorCodeMap::Core::PacketTooShort);
}

TEST_F(ParserCoreErrorsTest, StackOutOfBounds) {
  HeaderType testHeaderType("test_header_t", 0);
  testHeaderType.push_back_field("f", 8);
  // for this test, a header stack with a single element is sufficient
  header_id_t testHeaderStack1(0);
  header_stack_id_t testHeaderStack(0);
  phv_factory.push_back_header("test_stack_1", testHeaderStack1,
                               testHeaderType);
  phv_factory.push_back_header_stack("test_stack", testHeaderStack,
                                     testHeaderType, {testHeaderStack1});

  auto packet = get_pkt(packet_nbytes);
  // 2 extracts to ensure we overflow
  parse_state.add_extract_to_stack(testHeaderStack);
  parse_state.add_extract_to_stack(testHeaderStack);
  parse_and_check_error(&packet, ErrorCodeMap::Core::StackOutOfBounds);
}

namespace {

template <typename It>
ErrorCodeMap make_error_codes(It first, It last) {
  ErrorCodeMap error_codes;
  ErrorCode::type_t v(0);
  for (auto it = first; it < last; ++it) error_codes.add(*it, v++);
  error_codes.add_core();
  return error_codes;
}

}  // namespace

// test verify statements in parser, with arch-specific (not core) errors
class ParserVerifyTest : public ParserErrorTest {
 protected:
  static constexpr const char *errors[] = {"error1", "error2"};
  static constexpr size_t errors_size = sizeof(errors) / sizeof(errors[0]);
  static constexpr const char *verify_error = "error2";

  ParserVerifyTest()
      : ParserErrorTest(
            make_error_codes(&errors[0], &errors[errors_size])) { }

  BoolExpression build_simple_condition(bool value) {
    BoolExpression condition;
    condition.push_back_load_bool(value);
    condition.build();
    return condition;
  }

  ArithExpression build_simple_error_expr(ErrorCode error_code) {
    ArithExpression expr;
    expr.push_back_load_const(Data(error_code.get()));
    expr.build();
    return expr;
  }

  void verify_test(bool with_error) {
    bool value = !with_error;
    auto condition = build_simple_condition(value);
    static constexpr int arbitrary_packet_length = 64;
    auto packet = get_pkt(arbitrary_packet_length);
    const auto &phv = *packet.get_phv();
    ASSERT_EQ(value, condition.eval(phv));

    auto error_code = error_codes.from_name(verify_error);
    parse_state.add_verify(condition, build_simple_error_expr(error_code));
    if (with_error)
      parse_and_check_error(&packet, verify_error);
    else
      parse_and_check_no_error(&packet);
  }
};

constexpr const char *ParserVerifyTest::errors[];
constexpr size_t ParserVerifyTest::errors_size;
constexpr const char *ParserVerifyTest::verify_error;

TEST_F(ParserVerifyTest, Basic) {
  for (size_t i = 0; i < errors_size; i++) {
    const std::string name(errors[i]);
    ASSERT_TRUE(error_codes.exists(name));
    auto code = error_codes.from_name(name);
    ASSERT_EQ(static_cast<ErrorCode::type_t>(i), code.get());
    ASSERT_EQ(name, error_codes.to_name(code));
    ASSERT_TRUE(error_codes.exists(code.get()));
  }
}

TEST_F(ParserVerifyTest, Noop) {
  verify_test(false);
}

TEST_F(ParserVerifyTest, Error) {
  verify_test(true);
}


class ParserMethodCallTest : public ParserTestGeneric {
 protected:
  ParseState parse_state;
  HeaderType headerType;
  header_id_t testHeader{0};

  ParserMethodCallTest()
      : parse_state("parse_state", 0),
        headerType("header_type", 0) {
    headerType.push_back_field("f32", 32);
    phv_factory.push_back_header("test", testHeader, headerType);
  }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet::make_new(64, PacketBuffer(128), phv_source.get());
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    parser.set_init_state(&parse_state);
  }

  // virtual void TearDown() { }
};

struct ParserTestSetField : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) override {
    f.set(d);
    has_been_called = true;
  }

  bool has_been_called{false};
};

REGISTER_PRIMITIVE(ParserTestSetField);

TEST_F(ParserMethodCallTest, SetField) {
  ActionFn action_fn("test_action", 0, 0);
  ParserTestSetField primitive;
  constexpr int value(97);
  action_fn.push_back_primitive(&primitive);
  action_fn.parameter_push_back_field(testHeader, 0);  // f32
  action_fn.parameter_push_back_const(Data(value));
  parse_state.add_method_call(&action_fn);
  auto pkt = get_pkt();
  auto phv = pkt.get_phv();
  auto &f = phv->get_field(testHeader, 0);  // f32

  ASSERT_FALSE(primitive.has_been_called);
  parse_and_check_no_error(&pkt);
  ASSERT_TRUE(primitive.has_been_called);
  ASSERT_EQ(value, f.get<decltype(value)>());
}

TEST_F(ParserMethodCallTest, RegisterSync) {
  RegisterArray register_array("register_test", 0, 128u, 32u);
  ActionFn action_fn("test_action", 0, 0);
  auto primitive_spin = ActionOpcodesMap::get_instance()->get_primitive(
      "RegisterSpin");
  constexpr unsigned int msecs_to_sleep(1000u);
  action_fn.push_back_primitive(primitive_spin.get());
  action_fn.parameter_push_back_register_array(&register_array);
  action_fn.parameter_push_back_const(Data(msecs_to_sleep));
  parse_state.add_method_call(&action_fn);
  auto pkt = get_pkt();
  ActionFnEntry testActionFnEntry(&action_fn);

  using clock = std::chrono::system_clock;
  clock::time_point start = clock::now();

  // access same register through parser method call and direct action call,
  // check that the 2 calls are executing sequentially
  std::thread t([this, &pkt]() { this->parse_and_check_no_error(&pkt); });
  testActionFnEntry(&pkt);
  t.join();

  clock::time_point end = clock::now();

  constexpr unsigned int expected_timedelta = msecs_to_sleep * 2;
  auto timedelta = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start).count();
  ASSERT_LT(expected_timedelta * 0.95, timedelta);
  ASSERT_GT(expected_timedelta * 1.2, timedelta);
}

class ParserShiftTest : public ParserTestGeneric {
 protected:
  ParseState parse_state;
  HeaderType headerType;
  header_id_t testHeader{0}, metaHeader{1};

  ParserShiftTest()
      : parse_state("parse_state", 0),
        headerType("header_type", 0) {
    headerType.push_back_field("f8", 8);
    phv_factory.push_back_header("test", testHeader, headerType);
    phv_factory.push_back_header("meta", metaHeader, headerType, true);
  }

  Packet get_pkt(const std::string &data) {
    assert(data.size() <= 128);
    return Packet::make_new(data.size(),
                            PacketBuffer(128, data.data(), data.size()),
                            phv_source.get());
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    parser.set_init_state(&parse_state);
  }

  // virtual void TearDown() { }
};

TEST_F(ParserShiftTest, ShiftOneByte) {
  parse_state.add_shift(1);  // 1-byte shift
  parse_state.add_extract(testHeader);

  std::string packet_data("\xab\xcd");
  auto packet = get_pkt(packet_data);
  parse_and_check_no_error(&packet);
  const auto &f = packet.get_phv()->get_field(testHeader, 0);  // f8
  ASSERT_EQ(static_cast<unsigned char>(packet_data.at(1)),
            f.get<unsigned char>());
}

TEST_F(ParserShiftTest, AdvanceByData) {
  parse_state.add_advance_from_data(Data(1 * 8));  // 1-byte shift
  parse_state.add_extract(testHeader);

  std::string packet_data("\xab\xcd");
  auto packet = get_pkt(packet_data);
  parse_and_check_no_error(&packet);
  const auto &f = packet.get_phv()->get_field(testHeader, 0);  // f8
  ASSERT_EQ(static_cast<unsigned char>(packet_data.at(1)),
            f.get<unsigned char>());
}

TEST_F(ParserShiftTest, AdvanceByDataInvalidArgument) {
  parse_state.add_advance_from_data(Data(11));  // 11-bit, not a multiple of 8
  parse_state.add_extract(testHeader);

  std::string packet_data("\xab\xcd");
  auto packet = get_pkt(packet_data);
  parse_and_check_error(&packet, ErrorCodeMap::Core::ParserInvalidArgument);
}

TEST_F(ParserShiftTest, AdvanceByExpression) {
  ArithExpression expr;
  expr.push_back_load_const(Data(1 * 8));
  expr.build();
  parse_state.add_advance_from_expression(expr);  // 1-byte shift
  parse_state.add_extract(testHeader);

  std::string packet_data("\xab\xcd");
  auto packet = get_pkt(packet_data);
  parse_and_check_no_error(&packet);
  const auto &f = packet.get_phv()->get_field(testHeader, 0);  // f8
  ASSERT_EQ(static_cast<unsigned char>(packet_data.at(1)),
            f.get<unsigned char>());
}

TEST_F(ParserShiftTest, AdvanceByExpressionInvalidArgument) {
  ArithExpression expr;
  expr.push_back_load_const(Data(11));  // 11-bit, not a multiple of 8
  expr.build();
  parse_state.add_advance_from_expression(expr);
  parse_state.add_extract(testHeader);

  std::string packet_data("\xab\xcd");
  auto packet = get_pkt(packet_data);
  parse_and_check_error(&packet, ErrorCodeMap::Core::ParserInvalidArgument);
}

TEST_F(ParserShiftTest, AdvanceByField) {
  parse_state.add_advance_from_field(metaHeader, 0);
  parse_state.add_extract(testHeader);

  std::string packet_data("\xab\xcd");
  auto packet = get_pkt(packet_data);

  auto &f_shift = packet.get_phv()->get_field(metaHeader, 0);  // f8
  f_shift.set(1 * 8);

  parse_and_check_no_error(&packet);
  const auto &f = packet.get_phv()->get_field(testHeader, 0);  // f8
  ASSERT_EQ(static_cast<unsigned char>(packet_data.at(1)),
            f.get<unsigned char>());
}

TEST_F(ParserShiftTest, AdvanceByFieldInvalidArgument) {
  parse_state.add_advance_from_field(metaHeader, 0);
  parse_state.add_extract(testHeader);

  std::string packet_data("\xab\xcd");
  auto packet = get_pkt(packet_data);

  auto &f_shift = packet.get_phv()->get_field(metaHeader, 0);  // f8
  f_shift.set(11);  // 11-bit, not a multiple of 8

  parse_and_check_error(&packet, ErrorCodeMap::Core::ParserInvalidArgument);
}


class ParserExtractVLTest : public ParserTestGeneric {
 protected:
  ParseState parse_state;
  HeaderType headerType;
  header_id_t testHeader{0};

  static constexpr size_t max_header_bytes = 4;

  ParserExtractVLTest()
      : parse_state("parse_state", 0),
        headerType("header_type", 0) {
    headerType.push_back_VL_field("fVL", max_header_bytes, nullptr);
    phv_factory.push_back_header("test", testHeader, headerType);
  }

  Packet get_pkt(const std::string &data) {
    assert(data.size() <= 128);
    return Packet::make_new(data.size(),
                            PacketBuffer(128, data.data(), data.size()),
                            phv_source.get());
  }

  ArithExpression make_expr(size_t nbits) {
    ArithExpression expr;
    expr.push_back_load_const(Data(nbits));
    expr.build();
    return expr;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    parser.set_init_state(&parse_state);
  }
};

constexpr size_t ParserExtractVLTest::max_header_bytes;

TEST_F(ParserExtractVLTest, Basic) {
  parse_state.add_extract_VL(testHeader, make_expr(max_header_bytes * 8),
                             max_header_bytes);
  std::string packet_data(max_header_bytes, '\xaa');
  auto packet = get_pkt(packet_data);
  parse_and_check_no_error(&packet);
  const auto &f = packet.get_phv()->get_field(testHeader, 0);  // fVL
  ASSERT_EQ(packet_data, f.get_string());
}

TEST_F(ParserExtractVLTest, MultiplePackets) {
  parse_state.add_extract_VL(testHeader, make_expr(max_header_bytes * 8),
                             max_header_bytes);
  std::string packet_data(max_header_bytes, '\xaa');
  for (size_t i = 0; i < 10; i++) {
    auto packet = get_pkt(packet_data);
    parse_and_check_no_error(&packet);
    const auto &f = packet.get_phv()->get_field(testHeader, 0);  // fVL
    ASSERT_EQ(packet_data, f.get_string());
  }
}

TEST_F(ParserExtractVLTest, PacketTooShort) {
  // packet is half the required size
  constexpr size_t expr_bytes = max_header_bytes;
  constexpr size_t packet_bytes = max_header_bytes / 2;
  parse_state.add_extract_VL(testHeader, make_expr(expr_bytes * 8),
                             max_header_bytes);
  std::string packet_data(packet_bytes, '\xaa');
  auto packet = get_pkt(packet_data);
  parse_and_check_error(&packet, ErrorCodeMap::Core::PacketTooShort);
}

TEST_F(ParserExtractVLTest, HeaderTooShort) {
  // the VL expression returns a value which is twice the maximum number of
  // bytes for the field / header
  constexpr size_t expr_bytes = max_header_bytes * 2;
  constexpr size_t packet_bytes = max_header_bytes * 2;
  parse_state.add_extract_VL(testHeader, make_expr(expr_bytes * 8),
                             max_header_bytes);
  std::string packet_data(packet_bytes, '\xaa');
  auto packet = get_pkt(packet_data);
  parse_and_check_error(&packet, ErrorCodeMap::Core::HeaderTooShort);
}

TEST_F(ParserExtractVLTest, InvalidArgument) {
  // computed bitwidth is 11, not a multiple of 8
  parse_state.add_extract_VL(testHeader, make_expr(11), max_header_bytes);
  std::string packet_data(16, '\xaa');
  auto packet = get_pkt(packet_data);
  parse_and_check_error(&packet, ErrorCodeMap::Core::ParserInvalidArgument);
}


// Google Test fixture for header union stacks
class HeaderUnionStackParserTest : public ParserTestGeneric {
 protected:
  HeaderType optionAHeaderType, optionBHeaderType;
  ParseState optionsParseState, optionAParseState, optionBParseState;
  header_id_t optionA_0{0}, optionB_0{1}, optionA_1{2}, optionB_1{3};
  header_union_id_t union_0{0}, union_1{1};
  header_union_stack_id_t options{0};

  Deparser deparser;

  HeaderUnionStackParserTest()
      : optionAHeaderType("optionA_t", 0), optionBHeaderType("optionB_t", 1),
        optionsParseState("options_parse", 0),
        optionAParseState("optionA_parse", 0),
        optionBParseState("optionB_parse", 0),
        deparser("test_deparser", 0) {
    optionAHeaderType.push_back_field("type", 8);
    optionAHeaderType.push_back_field("bos", 8);
    optionBHeaderType.push_back_field("type", 8);
    optionBHeaderType.push_back_field("bos", 8);
    optionBHeaderType.push_back_field("f8", 8);

    phv_factory.push_back_header("optionA_0", optionA_0, optionAHeaderType);
    phv_factory.push_back_header("optionA_1", optionA_1, optionAHeaderType);
    phv_factory.push_back_header("optionB_0", optionB_0, optionBHeaderType);
    phv_factory.push_back_header("optionB_1", optionB_1, optionBHeaderType);

    const std::vector<header_id_t> headers_0({optionA_0, optionB_0});
    phv_factory.push_back_header_union("option_0", union_0, headers_0);
    const std::vector<header_id_t> headers_1({optionA_1, optionB_1});
    phv_factory.push_back_header_union("option_1", union_1, headers_1);

    const std::vector<header_union_id_t> unions({union_0, union_1});
    phv_factory.push_back_header_union_stack("options", options, unions);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);

    // we assume at least one option for the parse graph definition

    {
      ParseSwitchKeyBuilder optionsKeyBuilder;
      optionsKeyBuilder.push_back_lookahead(0, 8);  // type
      optionsParseState.set_key_builder(optionsKeyBuilder);
      char keyA[] = {'\x0a'};
      optionsParseState.add_switch_case(sizeof(keyA), keyA, &optionAParseState);
      char keyB[] = {'\x0b'};
      optionsParseState.add_switch_case(sizeof(keyB), keyB, &optionBParseState);
    }
    {
      ParseSwitchKeyBuilder optionAKeyBuilder;
      // options[last].optionA.bos
      optionAKeyBuilder.push_back_union_stack_field(options, 0, 1, 8);
      optionAParseState.set_key_builder(optionAKeyBuilder);
      char key[] = {'\x00'};
      optionAParseState.add_switch_case(sizeof(key), key, &optionsParseState);
    }
    {
      ParseSwitchKeyBuilder optionBKeyBuilder;
      // options[last].optionA.bos
      optionBKeyBuilder.push_back_union_stack_field(options, 1, 1, 8);
      optionBParseState.set_key_builder(optionBKeyBuilder);
      char key[] = {'\x00'};
      optionBParseState.add_switch_case(sizeof(key), key, &optionsParseState);
    }

    // optionA has offset 0 in union, optionB has offset 1 in union
    optionAParseState.add_extract_to_union_stack(options, 0);
    optionBParseState.add_extract_to_union_stack(options, 1);

    parser.set_init_state(&optionsParseState);

    // TODO(antonin): ideally we would like a push_back_header_union method here
    deparser.push_back_header(optionA_0);
    deparser.push_back_header(optionB_0);
    deparser.push_back_header(optionA_1);
    deparser.push_back_header(optionB_1);
  }

  Packet get_pkt(const std::string &data) {
    assert(data.size() <= 128);
    return Packet::make_new(data.size(),
                            PacketBuffer(128, data.data(), data.size()),
                            phv_source.get());
  }

  // virtual void TearDown() { }
};

TEST_F(HeaderUnionStackParserTest, ParseAndDeparse2Options) {
  // optionB followed by optionA
  const std::string data("\x0b\x00\xab\x0a\x01", 5);
  auto packet = get_pkt(data);
  auto phv = packet.get_phv();
  parse_and_check_no_error(&packet);

  const auto &optionA_0_hdr = phv->get_header(optionA_0);
  const auto &optionA_1_hdr = phv->get_header(optionA_1);
  const auto &optionB_0_hdr = phv->get_header(optionB_0);
  const auto &optionB_1_hdr = phv->get_header(optionB_1);
  const auto &options_union_0 = phv->get_header_union(union_0);
  const auto &options_union_1 = phv->get_header_union(union_1);
  const auto &options_union_stack = phv->get_header_union_stack(options);

  EXPECT_FALSE(optionA_0_hdr.is_valid());
  EXPECT_TRUE(optionA_1_hdr.is_valid());
  EXPECT_TRUE(optionB_0_hdr.is_valid());
  EXPECT_FALSE(optionB_1_hdr.is_valid());
  EXPECT_EQ(&optionB_0_hdr, options_union_0.get_valid_header());
  EXPECT_EQ(&optionA_1_hdr, options_union_1.get_valid_header());
  EXPECT_EQ(2u, options_union_stack.get_count());
  EXPECT_EQ(0xab, optionB_0_hdr.get_field(2).get<int>());

  deparser.deparse(&packet);

  ASSERT_EQ(data.size(), packet.get_data_size());
  ASSERT_EQ(0, memcmp(data.data(), packet.data(), data.size()));
}


namespace fs = boost::filesystem;

// unsure whether these tests really belong here, or in test_p4objects...

class HeaderUnionStackE2ETest : public ::testing::Test {
 protected:
  P4Objects objects;
  LookupStructureFactory factory;
  Parser *parser;
  Deparser *deparser;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  static constexpr size_t num_packets = 10u;

  HeaderUnionStackE2ETest()
      : phv_source(PHVSourceIface::make_phv_source()) { }

  void init_objects(const fs::path &json_path) {
    std::ifstream is(json_path.string());
    EXPECT_EQ(0, objects.init_objects(&is, &factory));
    parser = get_parser();
    ASSERT_TRUE(parser != nullptr);
    deparser = get_deparser();
    ASSERT_TRUE(deparser != nullptr);
  }

  void parse_and_deparse(const std::string &data) {
    for (size_t i = 0; i < num_packets; i++) {
      auto packet = get_pkt(data);
      parse_and_check_no_error(&packet);
      deparser->deparse(&packet);
      ASSERT_EQ(data.size(), packet.get_data_size());
      ASSERT_EQ(0, memcmp(data.data(), packet.data(), data.size()));
    }
  }

 private:
  Parser *get_parser() const {
    return objects.get_parser_rt("parser");
  }

  Deparser *get_deparser() const {
    return objects.get_deparser_rt("deparser");
  }

  void parse_and_check_no_error(Packet *packet) {
    auto error_codes = objects.get_error_codes();
    ErrorCode no_error(error_codes.from_core(ErrorCodeMap::Core::NoError));
    parser->parse(packet);
    ASSERT_EQ(no_error, packet->get_error_code());
  }

  Packet get_pkt(const std::string &data) {
    phv_source->set_phv_factory(0, &objects.get_phv_factory());
    assert(data.size() <= 128);
    return Packet::make_new(data.size(),
                            PacketBuffer(128, data.data(), data.size()),
                            phv_source.get());
  }
};

constexpr size_t HeaderUnionStackE2ETest::num_packets;

TEST_F(HeaderUnionStackE2ETest, OptionsE2eCount) {
  fs::path json_path =
      fs::path(TESTDATADIR) / fs::path("unions_e2e_options_count.json");
  init_objects(json_path);

  // the last byte is a payload byte, to avoid an invalid memory read because of
  // the parser lookahead
  std::string data("\x02\x0a\x0b\x66\xab", 5);
  parse_and_deparse(data);
}

TEST_F(HeaderUnionStackE2ETest, OptionsE2eBos) {
  fs::path json_path =
      fs::path(TESTDATADIR) / fs::path("unions_e2e_options_bos.json");
  init_objects(json_path);

  std::string data("\x0b\x00\x77\x0a\x01", 5);
  parse_and_deparse(data);
}


// test extraction to header stack including a VL field
class HeaderStackVLTest : public ParserTestGeneric {
 protected:
  ParseState parse_state;
  HeaderType headerType;
  header_id_t header0{0}, header1{1}, header2{2};
  header_stack_id_t headerStack{0};

  static constexpr size_t max_header_bytes = 4;

  HeaderStackVLTest()
      : parse_state("parse_state", 0),
        headerType("test_t", 0) {
    headerType.push_back_VL_field("fVL", max_header_bytes, nullptr);
    phv_factory.push_back_header("test_0", header0, headerType);
    phv_factory.push_back_header("test_1", header1, headerType);
    phv_factory.push_back_header("test_2", header2, headerType);
    phv_factory.push_back_header_stack(
        "test_stack", headerStack, headerType, {header0, header1, header2});
  }

  Packet get_pkt(const ByteContainer &data) {
    assert(data.size() <= 128);
    return Packet::make_new(data.size(),
                            PacketBuffer(128, data.data(), data.size()),
                            phv_source.get());
  }

  ArithExpression make_expr(size_t nbits) {
    ArithExpression expr;
    expr.push_back_load_const(Data(nbits));
    expr.build();
    return expr;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    parser.set_init_state(&parse_state);
  }
};

constexpr size_t HeaderStackVLTest::max_header_bytes;

TEST_F(HeaderStackVLTest, ExtractVL) {
  parse_state.add_extract_to_stack_VL(
      headerStack, make_expr(16), max_header_bytes);
  parse_state.add_extract_to_stack_VL(
      headerStack, make_expr(24), max_header_bytes);

  ByteContainer v0("0x1234"), v1("0x567890");
  ByteContainer data;
  data.append(v0).append(v1).append("0xaaaaaaaaaa");
  auto pkt = get_pkt(data);
  parse_and_check_no_error(&pkt);

  auto *phv = pkt.get_phv();
  auto &hdr0 = phv->get_header(header0);
  auto &hdr1 = phv->get_header(header1);
  auto &hdr2 = phv->get_header(header2);
  EXPECT_TRUE(hdr0.is_valid());
  EXPECT_TRUE(hdr1.is_valid());
  EXPECT_FALSE(hdr2.is_valid());
  EXPECT_EQ(hdr0.get_field(0).get_bytes(), v0);
  EXPECT_EQ(hdr1.get_field(0).get_bytes(), v1);
}

// test extraction to header union stack including a VL field
class HeaderUnionStackVLTest : public ParserTestGeneric {
 protected:
  ParseState parse_state;
  HeaderType headerType0, headerType1;
  header_id_t header00{0}, header01{1}, header10{2}, header11{3};
  header_union_id_t headerUnion0{0}, headerUnion1{1};
  header_union_stack_id_t headerUnionStack{0};

  static constexpr size_t max_header_bytes = 4;

  HeaderUnionStackVLTest()
      : parse_state("parse_state", 0),
        headerType0("test_0_t", 0), headerType1("test_1_t", 1) {
    headerType0.push_back_field("f", 32);
    headerType1.push_back_VL_field("fVL", max_header_bytes, nullptr);
    phv_factory.push_back_header("test_00", header00, headerType0);
    phv_factory.push_back_header("test_01", header01, headerType1);
    phv_factory.push_back_header("test_10", header10, headerType0);
    phv_factory.push_back_header("test_11", header11, headerType1);
    phv_factory.push_back_header_union(
        "test_union_0", headerUnion0, {header00, header01});
    phv_factory.push_back_header_union(
        "test_union_1", headerUnion1, {header10, header11});
    phv_factory.push_back_header_union_stack(
        "test_union_stack", headerUnionStack, {headerUnion0, headerUnion1});
  }

  Packet get_pkt(const ByteContainer &data) {
    assert(data.size() <= 128);
    return Packet::make_new(data.size(),
                            PacketBuffer(128, data.data(), data.size()),
                            phv_source.get());
  }

  ArithExpression make_expr(size_t nbits) {
    ArithExpression expr;
    expr.push_back_load_const(Data(nbits));
    expr.build();
    return expr;
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    parser.set_init_state(&parse_state);
  }
};

constexpr size_t HeaderUnionStackVLTest::max_header_bytes;

TEST_F(HeaderUnionStackVLTest, ExtractVL) {
  parse_state.add_extract_to_union_stack_VL(
      headerUnionStack, 1  /* offset of VL header in union */,
      make_expr(24), max_header_bytes);

  ByteContainer v01("0x567890");
  ByteContainer data;
  data.append(v01).append("0xaaaaaaaaaa");
  auto pkt = get_pkt(data);
  parse_and_check_no_error(&pkt);

  auto *phv = pkt.get_phv();
  auto &header_union0 = phv->get_header_union(headerUnion0);
  auto &header_union1 = phv->get_header_union(headerUnion1);
  auto &hdr01 = phv->get_header(header01);
  EXPECT_TRUE(hdr01.is_valid());
  EXPECT_EQ(&hdr01, header_union0.get_valid_header());
  EXPECT_EQ(nullptr, header_union1.get_valid_header());
  EXPECT_EQ(hdr01.get_field(0).get_bytes(), v01);
}
