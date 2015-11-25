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

#include "bm_sim/checksums.h"
#include "bm_sim/parser.h"

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

// Google Test fixture for checksums tests
class ChecksumTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;
  HeaderType ethernetHeaderType, ipv4HeaderType, udpHeaderType, tcpHeaderType;
  HeaderType metaHeaderType;
  ParseState ethernetParseState, ipv4ParseState, udpParseState, tcpParseState;
  header_id_t ethernetHeader{0}, ipv4Header{1}, udpHeader{2}, tcpHeader{3};
  header_id_t metaHeader{4};

  Parser parser;

  std::unique_ptr<NamedCalculation> tcp_cksum_engine_calc{nullptr};
  std::unique_ptr<CalcBasedChecksum> tcp_cksum_engine{nullptr};

  ChecksumTest()
    : ethernetHeaderType("ethernet_t", 0), ipv4HeaderType("ipv4_t", 1),
      udpHeaderType("udp_t", 2), tcpHeaderType("tcp_t", 3),
      metaHeaderType("meta_t", 4),
      ethernetParseState("parse_ethernet", 0),
      ipv4ParseState("parse_ipv4", 1),
      udpParseState("parse_udp", 2),
      tcpParseState("parse_tcp", 3),
      parser("test_parser", 0) {
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

    metaHeaderType.push_back_field("f16", 16);

    phv_factory.push_back_header("ethernet", ethernetHeader, ethernetHeaderType);
    phv_factory.push_back_header("ipv4", ipv4Header, ipv4HeaderType);
    phv_factory.push_back_header("udp", udpHeader, udpHeaderType);
    phv_factory.push_back_header("tcp", tcpHeader, tcpHeaderType);
    phv_factory.push_back_header("meta", metaHeader, metaHeaderType, true);

    BufBuilder tcp_cksum_engine_builder;
    tcp_cksum_engine_builder.push_back_field(ipv4Header, 10); // ipv4.srcAddr
    tcp_cksum_engine_builder.push_back_field(ipv4Header, 11); // ipv4.dstAddr
    tcp_cksum_engine_builder.push_back_constant(ByteContainer({'\x00'}), 8);
    tcp_cksum_engine_builder.push_back_field(ipv4Header, 8); // ipv4.protocol
    tcp_cksum_engine_builder.push_back_field(metaHeader, 0); // for tcpLength
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 0); // tcp.srcPort
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 1); // tcp.dstPort
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 2); // tcp.seqNo
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 3); // tcp.ackNo
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 4); // tcp.dataOffset
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 5); // tcp.res
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 6); // tcp.flags
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 7); // tcp.window
    // skip tcp.checksum of course
    tcp_cksum_engine_builder.push_back_field(tcpHeader, 9); // tcp.urgentPtr
    // no headers after that, directly payload
    tcp_cksum_engine_builder.append_payload();

    tcp_cksum_engine_calc = std::unique_ptr<NamedCalculation>(
      new NamedCalculation(
        "tcp_cksum_engine_calc", 0,
        tcp_cksum_engine_builder, "cksum16"
      )
    );
    tcp_cksum_engine = std::unique_ptr<CalcBasedChecksum>(
      new CalcBasedChecksum(
        "tcp_cksum_engine", 1, tcpHeader, 8,
        tcp_cksum_engine_calc.get()
      )
    );
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
  }

  void get_ipv4_pkt(Packet *pkt, unsigned short *cksum) {
    *pkt = Packet(
        0, 0, 0, sizeof(raw_tcp_pkt),
	PacketBuffer(256, (const char *) raw_tcp_pkt, sizeof(raw_tcp_pkt))
    );
    *cksum = 0x3508; // big endian
  }

  void get_tcp_pkt(Packet *pkt, unsigned short *cksum, unsigned short *tcp_len) {
    get_ipv4_pkt(pkt, cksum);
    *cksum = 0x13cd; // big endian
    *tcp_len = sizeof(raw_tcp_pkt) - 14 - 20; // - ethernet and ipv4
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(ChecksumTest, IPv4ChecksumVerify) {
  Packet packet;
  unsigned short cksum;
  get_ipv4_pkt(&packet, &cksum);
  PHV *phv = packet.get_phv();
  parser.parse(&packet);
  
  IPv4Checksum cksum_engine("ipv4_checksum", 0, ipv4Header, 9);
  ASSERT_TRUE(cksum_engine.verify(packet));

  Field &ipv4_checksum = phv->get_field(ipv4Header, 9);
  ipv4_checksum.set(0);  
  ASSERT_FALSE(cksum_engine.verify(packet));
}

TEST_F(ChecksumTest, IPv4ChecksumUpdate) {
  Packet packet;
  unsigned short cksum;
  get_ipv4_pkt(&packet, &cksum);
  PHV *phv = packet.get_phv();
  parser.parse(&packet);

  Field &ipv4_checksum = phv->get_field(ipv4Header, 9);
  ASSERT_EQ(cksum, ipv4_checksum.get_uint());

  ipv4_checksum.set(0);
  ASSERT_EQ((unsigned) 0, ipv4_checksum.get_uint());

  IPv4Checksum cksum_engine("ipv4_checksum", 0, ipv4Header, 9);
  cksum_engine.update(&packet);
  ASSERT_EQ(cksum, ipv4_checksum.get_uint());
}

TEST_F(ChecksumTest, IPv4ChecksumUpdateStress) {
  Packet packet;
  unsigned short cksum;
  get_ipv4_pkt(&packet, &cksum);
  PHV *phv = packet.get_phv();
  parser.parse(&packet);

  Field &ipv4_checksum = phv->get_field(ipv4Header, 9);
  ASSERT_EQ(cksum, ipv4_checksum.get_uint());

  IPv4Checksum cksum_engine("ipv4_checksum", 0, ipv4Header, 9);
  for(int i = 0; i < 100000; i++) {
    ipv4_checksum.set(0);
    ASSERT_EQ((unsigned) 0, ipv4_checksum.get_uint());

    cksum_engine.update(&packet);
    ASSERT_EQ(cksum, ipv4_checksum.get_uint());
  }
}

// actually more general than just TCP: CalcBasedChecksum test
TEST_F(ChecksumTest, TCPChecksumVerify) {
  Packet packet;
  unsigned short cksum;
  unsigned short tcp_len;
  get_tcp_pkt(&packet, &cksum, &tcp_len);
  PHV *phv = packet.get_phv();
  parser.parse(&packet);

  phv->get_field(metaHeader, 0).set(tcp_len);

  ASSERT_TRUE(tcp_cksum_engine->verify(packet));

  Field &tcp_checksum = phv->get_field(tcpHeader, 8);
  tcp_checksum.set(0);  
  ASSERT_FALSE(tcp_cksum_engine->verify(packet));
}

TEST_F(ChecksumTest, TCPChecksumUpdate) {
  Packet packet;
  unsigned short cksum;
  unsigned short tcp_len;
  get_tcp_pkt(&packet, &cksum, &tcp_len);
  PHV *phv = packet.get_phv();
  parser.parse(&packet);

  phv->get_field(metaHeader, 0).set(tcp_len);

  Field &tcp_checksum = phv->get_field(tcpHeader, 8);
  ASSERT_EQ(cksum, tcp_checksum.get_uint());

  tcp_checksum.set(0);
  ASSERT_EQ((unsigned) 0, tcp_checksum.get_uint());

  tcp_cksum_engine->update(&packet);
  ASSERT_EQ(cksum, tcp_checksum.get_uint());
}
