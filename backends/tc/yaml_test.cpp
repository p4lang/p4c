#include <memory>

#include "backends/tc/instruction.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/util.h"
#include "backends/tc/yaml_parser.h"
#include "backends/tc/yaml_serializer.h"
#include "gtest/gtest.h"

// Tests for serializing and parsing YAML.

namespace backends::tc {
namespace {

class YamlTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset the TCAM program and set up header information
    tcam_program_ =
        TCAMProgram{.header_instantiations_ = {{"ethernet", "ethernet_t"},
                                               {"ipv4", "ipv4_t"}},
                    .header_type_definitions_ = {
                        {"ethernet_t",
                         {
                             {"dstAddr", 48},
                             {"srcAddr", 48},
                             {"etherType", 16},
                         }},
                        {
                            "ipv4_t",
                            {
                                {"version", 4},
                                {"ihl", 4},
                                {"diffserv", 8},
                                {"totalLen", 16},
                                {"identification", 16},
                                {"flags", 3},
                                {"fragOffset", 13},
                                {"ttl", 8},
                                {"protocol", 8},
                                {"hdrChecksum", 16},
                                {"srcAddr", 32},
                                {"dstAddr", 32},
                            },
                        },
                    }};
    // Add TCAM entries
    tcam_program_.InsertTCAMEntry("start", {}, {}).instructions = {
        std::make_shared<StoreHeaderField>(util::Range::Create(0, 48).value(),
                                           "ethernet.dstAddr"),
        std::make_shared<StoreHeaderField>(util::Range::Create(48, 96).value(),
                                           "ethernet.srcAddr"),
        std::make_shared<StoreHeaderField>(util::Range::Create(96, 112).value(),
                                           "ethernet.etherType"),
        std::make_shared<Move>(112),
        std::make_shared<SetKey>(util::Range::Create(96, 112).value()),
        std::make_shared<NextState>("start_out"),
    };
    tcam_program_
        .InsertTCAMEntry("start_out", util::UInt64ToBitString(0x0800, 16),
                         util::UInt64ToBitString(0xFFFF, 16))
        .instructions = {
        std::make_shared<NextState>("ipv4_in"),
    };
    tcam_program_
        .InsertTCAMEntry("start_out", util::UInt64ToBitString(0, 16),
                         util::UInt64ToBitString(0, 16))
        .instructions = {
        std::make_shared<NextState>("accept"),
    };
    tcam_program_.InsertTCAMEntry("ipv4_in", {}, {}).instructions = {
        std::make_shared<NextState>("reject"),
    };
  }

  // TCAM program and the corresponding YAML serialization used for testing. The
  // YAML string can be initialized statically, the TCAM program is a complex
  // object so it is instantiated by `SetUp`.
  TCAMProgram tcam_program_;
  static constexpr char kSerializedProgram[] = R"yaml(declare-header:
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
add-header-instance:
  -
    - ethernet
    - ethernet_t
  -
    - ipv4
    - ipv4_t
add-transition:
  - [ipv4_in, 0w0, 0w0, set-next-state, reject]
  - [start, 0w0, 0w0, store, 0..48, ethernet.dstAddr, store, 48..96, ethernet.srcAddr, store, 96..112, ethernet.etherType, move, 112, set-key, 96..112, set-next-state, start_out]
  - [start_out, 16w0x0800, 16w0xFFFF, set-next-state, ipv4_in]
  - [start_out, 16w0x0000, 16w0x0000, set-next-state, accept])yaml";
};

constexpr char YamlTest::kSerializedProgram[];

TEST_F(YamlTest, ParseYaml) {
  auto actual_program = ParseYaml(kSerializedProgram);
  ASSERT_TRUE(actual_program.ok())
      << "YAML parser failed: " << actual_program.status();
  EXPECT_EQ(actual_program->header_type_definitions_,
            tcam_program_.header_type_definitions_);
  EXPECT_EQ(actual_program->header_instantiations_,
            tcam_program_.header_instantiations_);
  EXPECT_EQ(actual_program->tcam_table_, tcam_program_.tcam_table_);
}

TEST_F(YamlTest, SerializeToYaml) {
  auto actual_yaml_output = SerializeToYaml(tcam_program_);
  ASSERT_TRUE(actual_yaml_output.ok())
      << "YAML serializer failed : " << actual_yaml_output.status();
  ASSERT_EQ(*actual_yaml_output, kSerializedProgram);
}

}  // namespace
}  // namespace backends::tc
