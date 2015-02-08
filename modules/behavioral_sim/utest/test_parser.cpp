#include <gtest/gtest.h>

#include "parser.h"
#include "deparser.h"

int pull_test_parser() { return 0; }

/* Frame (66 bytes) */
static const unsigned char tcp_pkt[66] = {
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
static const unsigned char udp_pkt[82] = {
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

  PHV phv;
  HeaderType ethernetHeaderType, ipv4HeaderType, udpHeaderType, tcpHeaderType;
  ParseState ethernetParseState, ipv4ParseState, udpParseState, tcpParseState;
  header_id_t ethernetHeader, ipv4Header, udpHeader, tcpHeader;

  Parser parser;

  Deparser deparser;

  ParserTest()
    : ethernetHeaderType(0), ipv4HeaderType(1),
      udpHeaderType(2), tcpHeaderType(3) {
    ethernetHeaderType.add_field(48); // dstAddr
    ethernetHeaderType.add_field(48); // srcAddr
    ethernetHeaderType.add_field(16); // ethertype

    ipv4HeaderType.add_field(4); // version
    ipv4HeaderType.add_field(4); // ihl
    ipv4HeaderType.add_field(8); // diffserv
    ipv4HeaderType.add_field(16); // len
    ipv4HeaderType.add_field(16); // identification
    ipv4HeaderType.add_field(3); // flags
    ipv4HeaderType.add_field(13); // flagOffset
    ipv4HeaderType.add_field(8); // ttl
    ipv4HeaderType.add_field(8); // protocol
    ipv4HeaderType.add_field(16); // checksum
    ipv4HeaderType.add_field(32); // srcAddr
    ipv4HeaderType.add_field(32); // dstAddr

    udpHeaderType.add_field(16); // srcPort
    udpHeaderType.add_field(16); // dstPort
    udpHeaderType.add_field(16); // length
    udpHeaderType.add_field(16); // checksum  

    tcpHeaderType.add_field(16); // srcPort
    tcpHeaderType.add_field(16); // dstPort
    tcpHeaderType.add_field(32); // seqNo
    tcpHeaderType.add_field(32); // ackNo
    tcpHeaderType.add_field(4); // dataOffset
    tcpHeaderType.add_field(4); // res
    tcpHeaderType.add_field(8); // flags
    tcpHeaderType.add_field(16); // window
    tcpHeaderType.add_field(16); // checksum
    tcpHeaderType.add_field(16); // urgentPtr

    ethernetHeader = phv.add_header(ethernetHeaderType);
    ipv4Header = phv.add_header(ipv4HeaderType);
    udpHeader = phv.add_header(udpHeaderType);
    tcpHeader = phv.add_header(tcpHeaderType);
  }

  virtual void SetUp() {
    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.add_field(ethernetHeader, 2); // ethertype
    ethernetParseState.set_key_builder(2, ethernetKeyBuilder);

    ParseSwitchKeyBuilder ipv4KeyBuilder;
    ipv4KeyBuilder.add_field(ipv4Header, 8); // protocol
    ipv4ParseState.set_key_builder(1, ipv4KeyBuilder);

    ethernetParseState.add_extract(ethernetHeader);
    ipv4ParseState.add_extract(ipv4Header);
    udpParseState.add_extract(udpHeader);
    tcpParseState.add_extract(tcpHeader);

    char ethernet_ipv4_key[2];
    ethernet_ipv4_key[0] = 0x08;
    ethernet_ipv4_key[1] = 0x00;
    ethernetParseState.add_switch_case(ethernet_ipv4_key, &ipv4ParseState);

    char ipv4_udp_key[1];
    ipv4_udp_key[0] = 17;
    ipv4ParseState.add_switch_case(ipv4_udp_key, &udpParseState);

    char ipv4_tcp_key[1];
    ipv4_tcp_key[0] = 6;
    ipv4ParseState.add_switch_case(ipv4_tcp_key, &tcpParseState);

    parser.set_init_state(&ethernetParseState);

    deparser.add_header(ethernetHeader);
    deparser.add_header(ipv4Header);
    deparser.add_header(tcpHeader);
    deparser.add_header(udpHeader);
  }

  // virtual void TearDown() {}
};

TEST_F(ParserTest, ParseEthernetIPv4TCP) {
  parser.parse((const char *) tcp_pkt, phv);

  const Header &ethernet_hdr = phv.get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());


  const Header &ipv4_hdr = phv.get_header(ipv4Header);
  ASSERT_TRUE(ipv4_hdr.is_valid());

  Field &ipv4_version = phv.get_field(ipv4Header, 0);
  ASSERT_EQ(ipv4_version.get_ui(), (unsigned) 0x4);

  Field &ipv4_ihl = phv.get_field(ipv4Header, 1);
  ASSERT_EQ(ipv4_ihl.get_ui(), (unsigned) 0x5);

  Field &ipv4_diffserv = phv.get_field(ipv4Header, 2);
  ASSERT_EQ(ipv4_diffserv.get_ui(), (unsigned) 0x00);

  Field &ipv4_len = phv.get_field(ipv4Header, 3);
  ASSERT_EQ(ipv4_len.get_ui(), (unsigned) 0x0034);

  Field &ipv4_identification = phv.get_field(ipv4Header, 4);
  ASSERT_EQ(ipv4_identification.get_ui(), (unsigned) 0x7090);

  Field &ipv4_flags = phv.get_field(ipv4Header, 5);
  ASSERT_EQ(ipv4_flags.get_ui(), (unsigned) 0x2);

  Field &ipv4_flagOffset = phv.get_field(ipv4Header, 6);
  ASSERT_EQ(ipv4_flagOffset.get_ui(), (unsigned) 0x0000);

  Field &ipv4_ttl = phv.get_field(ipv4Header, 7);
  ASSERT_EQ(ipv4_ttl.get_ui(), (unsigned) 0x40);

  Field &ipv4_protocol = phv.get_field(ipv4Header, 8);
  ASSERT_EQ(ipv4_protocol.get_ui(), (unsigned) 0x06);

  Field &ipv4_checksum = phv.get_field(ipv4Header, 9);
  ASSERT_EQ(ipv4_checksum.get_ui(), (unsigned) 0x3508);

  Field &ipv4_srcAddr = phv.get_field(ipv4Header, 10);
  ASSERT_EQ(ipv4_srcAddr.get_ui(), (unsigned) 0x0a36c121);

  Field &ipv4_dstAddr = phv.get_field(ipv4Header, 11);
  ASSERT_EQ(ipv4_dstAddr.get_ui(), (unsigned) 0x4e287bac);

  const Header &tcp_hdr = phv.get_header(tcpHeader);
  ASSERT_TRUE(tcp_hdr.is_valid());

  const Header &udp_hdr = phv.get_header(udpHeader);
  ASSERT_FALSE(udp_hdr.is_valid());
}

TEST_F(ParserTest, ParseEthernetIPv4UDP) {
  parser.parse((const char *) udp_pkt, phv);

  const Header &ethernet_hdr = phv.get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());

  const Header &ipv4_hdr = phv.get_header(ipv4Header);
  ASSERT_TRUE(ipv4_hdr.is_valid());

  const Header &udp_hdr = phv.get_header(udpHeader);
  ASSERT_TRUE(udp_hdr.is_valid());

  const Header &tcp_hdr = phv.get_header(tcpHeader);
  ASSERT_FALSE(tcp_hdr.is_valid());
}


TEST_F(ParserTest, ParseEthernetIPv4TCP_Stress) {
  const Header &ethernet_hdr = phv.get_header(ethernetHeader);
  const Header &ipv4_hdr = phv.get_header(ipv4Header);
  const Header &tcp_hdr = phv.get_header(tcpHeader);
  const Header &udp_hdr = phv.get_header(udpHeader);

  for(int t = 0; t < 1000000; t++) {
    parser.parse((const char *) tcp_pkt, phv);

    ASSERT_TRUE(ethernet_hdr.is_valid());
    ASSERT_TRUE(ipv4_hdr.is_valid());
    ASSERT_TRUE(tcp_hdr.is_valid());
    ASSERT_FALSE(udp_hdr.is_valid());

    phv.reset();
  }
}

TEST_F(ParserTest, DeparseEthernetIPv4TCP) {
  parser.parse((const char *) tcp_pkt, phv);
  
  char deparsed_tcp_pkt[sizeof(tcp_pkt)];
  memset(deparsed_tcp_pkt, 0, sizeof(deparsed_tcp_pkt));
  deparser.deparse(phv, deparsed_tcp_pkt);

  // 54 is size without payload
  ASSERT_EQ(memcmp(tcp_pkt, deparsed_tcp_pkt, 54), 0);
}

TEST_F(ParserTest, DeparseEthernetIPv4UDP) {
  parser.parse((const char *) udp_pkt, phv);
  
  char deparsed_udp_pkt[sizeof(udp_pkt)];
  memset(deparsed_udp_pkt, 0, sizeof(deparsed_udp_pkt));
  deparser.deparse(phv, deparsed_udp_pkt);

  // 54 is size without payload
  ASSERT_EQ(memcmp(udp_pkt, deparsed_udp_pkt, 42), 0);
}

TEST_F(ParserTest, DeparseEthernetIPv4_Stress) {
  
  char deparsed_tcp_pkt[sizeof(tcp_pkt)];
  char deparsed_udp_pkt[sizeof(udp_pkt)];
  const char *pkt;
  char *deparsed_pkt;
  int size;

  for(int t = 0; t < 10000; t++) {
    if(t % 2 == 0) {
      pkt = (const char *) tcp_pkt;
      deparsed_pkt = deparsed_tcp_pkt;
      size = 54;
    }
    else {
      pkt = (const char *) udp_pkt;
      deparsed_pkt = deparsed_udp_pkt;
      size = 42;
    }
    parser.parse(pkt, phv);
    memset(deparsed_pkt, 0, size);
    deparser.deparse(phv, deparsed_pkt);
    ASSERT_EQ(memcmp(tcp_pkt, deparsed_tcp_pkt, size), 0);

    phv.reset();
  }

}

