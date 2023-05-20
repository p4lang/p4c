#include "backends/tc/test_util.h"


#include "gtest/gtest.h"

// Tests for some testing utilities

namespace backends::tc {
namespace {

TEST(TestUtilTest, WriteBytes) {
  std::vector<uint8_t> output;
  WriteBytes<uint16_t>(0x1234, 2, output);
  EXPECT_EQ(output, (std::vector<uint8_t>{0x12, 0x34}));
  output.clear();
  WriteBytes<uint16_t>(0x1234, 1, output);
  EXPECT_EQ(output, (std::vector<uint8_t>{0x34}));
  output.clear();
  WriteBytes<uint32_t>(0x1234, 2, output);
  EXPECT_EQ(output, (std::vector<uint8_t>{0x12, 0x34}));
  output.clear();
  WriteBytes<uint32_t>(0x1234, 3, output);
  EXPECT_EQ(output, (std::vector<uint8_t>{0x00, 0x12, 0x34}));
  output.clear();
  WriteBytes<uint64_t>(0x123456789ABC, 6, output);
  EXPECT_EQ(output, (std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}));
  output.clear();
  WriteBytes<uint64_t>(0x123456789ABCDEF0, 6, output);
  EXPECT_EQ(output, (std::vector<uint8_t>{0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}));
}

}  // namespace
}  // namespace backends::tc
