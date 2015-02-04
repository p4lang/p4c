#include <gtest/gtest.h>

#include "parser.h"

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

// Google Test fixture for parser tests
class ParserTest : public ::testing::Test {
protected:

  PHV phv;
  HeaderType ethernetHeaderType, ipv4HeaderType;
  ParseState ethernetParseState, ipv4ParseState;
  header_id_t ethernetHeader, ipv4Header;

  Parser parser;

  ParserTest()
    : ethernetHeaderType(0), ipv4HeaderType(1) {
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

    ethernetHeader = phv.add_header(ethernetHeaderType);
    ipv4Header = phv.add_header(ipv4HeaderType);
  }

  virtual void SetUp() {
    ParseSwitchKeyBuilder ethernetKeyBuilder;
    ethernetKeyBuilder.add_field(ethernetHeader, 2); // ethertype
    ethernetParseState.set_key_builder(2, ethernetKeyBuilder);

    ethernetParseState.add_extract(ethernetHeader);
    ipv4ParseState.add_extract(ipv4Header);

    char ethernet_ipv4_key[2];
    ethernet_ipv4_key[0] = 0x08;
    ethernet_ipv4_key[1] = 0x00;
    
    ethernetParseState.add_switch_case(ethernet_ipv4_key, &ipv4ParseState);

    parser.set_init_state(&ethernetParseState);
  }

  // virtual void TearDown() {}
};

TEST_F(ParserTest, ParseEthernetIPv4) {
  parser.parse((const char *) tcp_pkt, phv);

  const Header &ethernet_hdr = phv.get_header(ethernetHeader);
  ASSERT_TRUE(ethernet_hdr.is_valid());

  const Header &ipv4_hdr = phv.get_header(ipv4Header);
  ASSERT_TRUE(ipv4_hdr.is_valid());
}
