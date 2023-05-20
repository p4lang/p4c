#include "backends/tc/simulator.h"

#include "backends/tc/instruction.h"
#include "backends/tc/test_util.h"
#include "backends/tc/util.h"
#include "backends/tc/yaml_parser.h"

#include "gtest/gtest.h"

namespace backends::tc {
namespace {

// EtherType values
constexpr uint16_t kEtherTypeIPv4 = 0x0800;
constexpr uint16_t kEtherTypeVLan = 0x8100;

// A TCAM program that parses Ethernet, VLAN, and IPv4
constexpr char kTcamProgram[] = R"yaml(
declare-header:
  -
    - ethernet_t
    - dstAddr:48
    - srcAddr:48
    - etherType:16
  -
    - ipv4_t
    - version:4
    - ihl:4
    - diffserv:8
    - totalLen:16
    - identification:16
    - flags:3
    - fragOffset:13
    - ttl:8
    - protocol:8
    - hdrChecksum:16
    - srcAddr:32
    - dstAddr:32
  -
    - vlan_t
    - tci.pcp:3
    - tci.dei:1
    - tci.vlan_id:12
    - etherType:16
add-header-instance:
  -
    - ethernet
    - ethernet_t
  -
    - ipv4
    - ipv4_t
  -
    - vlan
    - vlan_t
add-transition:
  - [parse_vlan_out, 16w0x0800, 16w0xFFFF, store, 0..4, ipv4.version, store, 4..8, ipv4.ihl, store, 8..16, ipv4.diffserv, store, 16..32, ipv4.totalLen, store, 32..48, ipv4.identification, store, 48..51, ipv4.flags, store, 51..64, ipv4.fragOffset, store, 64..72, ipv4.ttl, store, 72..80, ipv4.protocol, store, 80..96, ipv4.hdrChecksum, store, 96..128, ipv4.srcAddr, store, 128..160, ipv4.dstAddr, move, 160]
  - [parse_vlan_out, 16w0x0000, 16w0x0000, set-next-state, accept]
  - [start, 0w0, 0w0, store, 0..48, ethernet.dstAddr, store, 48..96, ethernet.srcAddr, store, 96..112, ethernet.etherType, move, 112, set-key, 96..112, set-next-state, start_out]
  - [start_out, 16w0x8100, 16w0xFFFF, store, 0..3, vlan.tci.pcp, store, 3..4, vlan.tci.dei, store, 4..16, vlan.tci.vlan_id, store, 16..32, vlan.etherType, move, 32, set-key, 16..32, set-next-state, parse_vlan_out]
  - [start_out, 16w0x0800, 16w0xFFFF, store, 0..4, ipv4.version, store, 4..8, ipv4.ihl, store, 8..16, ipv4.diffserv, store, 16..32, ipv4.totalLen, store, 32..48, ipv4.identification, store, 48..51, ipv4.flags, store, 51..64, ipv4.fragOffset, store, 64..72, ipv4.ttl, store, 72..80, ipv4.protocol, store, 80..96, ipv4.hdrChecksum, store, 96..128, ipv4.srcAddr, store, 128..160, ipv4.dstAddr, move, 160]
  - [start_out, 16w0x0000, 16w0x0000, set-next-state, accept]
)yaml";

class SimulatorTest : public testing::Test {
 protected:
  void SetUp() override {
    auto tcam_program = ParseYaml(kTcamProgram);
    EXPECT_TRUE(tcam_program.ok()) << tcam_program.status();
    simulator_ = ParserSimulator(tcam_program.value());
  }
  absl::optional<ParserSimulator> simulator_;
};

// Create the encoded Ethernet header, and append it to given packet buffer
void EthernetHeader(uint64_t dst_addr, uint64_t src_addr, uint16_t ether_type,
                    std::vector<uint8_t>& packet_buffer) {
  WriteBytes(dst_addr, 6, packet_buffer);
  WriteBytes(src_addr, 6, packet_buffer);
  WriteBytes(ether_type, 2, packet_buffer);
}

// Create the encoded Ethernet header, and append it to given packet buffer
void VLanHeader(uint16_t tag_control_info, uint16_t ether_type,
                std::vector<uint8_t>& packet_buffer) {
  WriteBytes(tag_control_info, 2, packet_buffer);
  WriteBytes(ether_type, 2, packet_buffer);
}

TEST_F(SimulatorTest, TooShortPackets) {
  // The parser should abort if the packet is too short

  // Empty packet
  EXPECT_FALSE(simulator_->Parse(std::vector<bool>{}));

  // An IPv4 packet that is too short
  std::vector<uint8_t> packet;
  EthernetHeader(0, 0, kEtherTypeIPv4, packet);
  packet.push_back(0);
  EXPECT_FALSE(simulator_->Parse(ToBitString(packet.begin(), packet.end())));

  // A VLAN-routed IPv4 packet that is too short
  packet.clear();
  EthernetHeader(0, 0, kEtherTypeVLan, packet);
  VLanHeader(0, kEtherTypeIPv4, packet);
  packet.push_back(0);
  ASSERT_FALSE(simulator_->Parse(ToBitString(packet.begin(), packet.end())));
}

TEST_F(SimulatorTest, ValidEthernetPackets) {
  // The parser should abort if the packet is too short

  // A packet with no payload
  std::vector<uint8_t> packet;
  EthernetHeader(0x1234, 0x5678, /* an unparsed EtherType */ 0x000F, packet);
  auto cursor = simulator_->Parse(ToBitString(packet.begin(), packet.end()));
  ASSERT_TRUE(cursor);
  EXPECT_EQ(*cursor, 112);

  auto extracted_header_fields = simulator_->extracted_header_fields();
  absl::flat_hash_map<HeaderField, Value> expected_header_fields{
    {"ethernet.dstAddr", util::UInt64ToBitString(0x1234, 48)},
    {"ethernet.srcAddr", util::UInt64ToBitString(0x5678, 48)},
    {"ethernet.etherType", util::UInt64ToBitString(0x000F, 16)},
  };
  EXPECT_EQ(extracted_header_fields, expected_header_fields);

  // Add random payload, the result should not change
  packet.push_back(0x01);
  packet.push_back(0x23);
  packet.push_back(0x45);
  packet.push_back(0x67);

  cursor = simulator_->Parse(ToBitString(packet.begin(), packet.end()));
  ASSERT_TRUE(cursor);
  EXPECT_EQ(*cursor, 112);

  extracted_header_fields = simulator_->extracted_header_fields();
  EXPECT_EQ(extracted_header_fields, expected_header_fields);
}

TEST_F(SimulatorTest, ValidVLanPacket) {
  // The parser should abort if the packet is too short

  // A packet with no payload
  std::vector<uint8_t> packet;
  EthernetHeader(0x1234, 0x5678, kEtherTypeVLan, packet);
  VLanHeader(0x4321, /* an unused EtherType */ 0x000F, packet);
  auto cursor = simulator_->Parse(ToBitString(packet.begin(), packet.end()));
  ASSERT_TRUE(cursor);
  EXPECT_EQ(*cursor, 144);

  auto extracted_header_fields = simulator_->extracted_header_fields();
  absl::flat_hash_map<HeaderField, Value> expected_header_fields{
    {"ethernet.dstAddr", util::UInt64ToBitString(0x1234, 48)},
    {"ethernet.srcAddr", util::UInt64ToBitString(0x5678, 48)},
    {"ethernet.etherType", util::UInt64ToBitString(kEtherTypeVLan, 16)},
    {"vlan.tci.pcp", util::UInt64ToBitString(2, 3)},
    {"vlan.tci.dei", Value{false}},
    {"vlan.tci.vlan_id", util::UInt64ToBitString(0x321, 12)},
    {"vlan.etherType", util::UInt64ToBitString(0x000F, 16)},
  };
  EXPECT_EQ(extracted_header_fields, expected_header_fields);
}

}  // namespace
}  // namespace backends::tc
