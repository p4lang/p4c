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
      ethernetParseState("parse_ethernet"),
      ipv4ParseState("parse_ipv4"),
      udpParseState("parse_udp"),
      tcpParseState("parse_tcp"),
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
	0, 0, 0,
	PacketBuffer(256, (const char *) raw_tcp_pkt, sizeof(raw_tcp_pkt))
    );
    return pkt;
  }

  Packet get_udp_pkt() {
    Packet pkt = Packet(
	0, 0, 0, 
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
    assert(packet.get_phv());
    parser.parse(&packet);
    deparser.deparse(&packet);
    ASSERT_EQ(0, memcmp(ref_pkt, packet.data(), size));
  }

}

