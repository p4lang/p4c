#include "backends/tc/util.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

namespace backends::tc::util {
namespace {

static const char* kHexDigits[] = {"0", "1", "2", "3", "4", "5", "6", "7",
                                   "8", "9", "A", "B", "C", "D", "E", "F"};

TEST(RangeTest, Create) {
  const auto max_u32 = std::numeric_limits<uint32_t>::max();

  EXPECT_FALSE(Range::Create(1, 0).ok());
  EXPECT_FALSE(Range::Create(1, 0).ok());
  EXPECT_FALSE(Range::Create(2, 0).ok());
  EXPECT_FALSE(Range::Create(max_u32, 0).ok());
  EXPECT_FALSE(Range::Create(max_u32, max_u32 - 1).ok());

  const auto range1 = Range::Create(0, 0);
  EXPECT_TRUE(range1.ok());
  EXPECT_EQ(range1->from(), 0);
  EXPECT_EQ(range1->to(), 0);
  const auto range2 = Range::Create(0, max_u32);
  EXPECT_TRUE(range2.ok());
  EXPECT_EQ(range2->from(), 0);
  EXPECT_EQ(range2->to(), max_u32);
  const auto range3 = Range::Create(max_u32 - 1, max_u32);
  EXPECT_TRUE(range3.ok());
  EXPECT_EQ(range3->from(), max_u32 - 1);
  EXPECT_EQ(range3->to(), max_u32);

  // Test small ranges
  for (uint32_t i = 0; i < 16; ++i) {
    for (uint32_t j = 0; i < 16; ++i) {
      const auto range = Range::Create(i, j);
      if (i > j) {
        EXPECT_FALSE(range.ok()) << "The range " << i << ".." << j
                                 << " is invalid, but constructed " << *range;
      } else {
        EXPECT_TRUE(range.ok());
        EXPECT_EQ(range->from(), i);
        EXPECT_EQ(range->to(), j);
      }
    }
  }
}

// Helper function to get the pretty-printed bitstring
std::string PrettyPrintToString(const std::vector<bool>& bit_string) {
  std::ostringstream out;
  PrettyPrintBitString(bit_string, out);
  return std::move(out).str();
}

TEST(UtilTest, UInt64ToBitStringTest) {
  EXPECT_EQ(UInt64ToBitString(0), (std::vector<bool>{false}));
  EXPECT_EQ(UInt64ToBitString(1), (std::vector<bool>{true}));
  EXPECT_EQ(UInt64ToBitString(2), (std::vector<bool>{true, false}));
  EXPECT_EQ(UInt64ToBitString(3), (std::vector<bool>{true, true}));
  EXPECT_EQ(UInt64ToBitString(4), (std::vector<bool>{true, false, false}));
  EXPECT_EQ(UInt64ToBitString(5), (std::vector<bool>{true, false, true}));
  EXPECT_EQ(UInt64ToBitString(6), (std::vector<bool>{true, true, false}));
  EXPECT_EQ(UInt64ToBitString(7), (std::vector<bool>{true, true, true}));
  EXPECT_EQ(UInt64ToBitString(8),
            (std::vector<bool>{true, false, false, false}));
  EXPECT_EQ(UInt64ToBitString(9),
            (std::vector<bool>{true, false, false, true}));
  EXPECT_EQ(UInt64ToBitString(10),
            (std::vector<bool>{true, false, true, false}));
  EXPECT_EQ(UInt64ToBitString(11),
            (std::vector<bool>{true, false, true, true}));
  EXPECT_EQ(UInt64ToBitString(12),
            (std::vector<bool>{true, true, false, false}));
  EXPECT_EQ(UInt64ToBitString(13),
            (std::vector<bool>{true, true, false, true}));
  EXPECT_EQ(UInt64ToBitString(14),
            (std::vector<bool>{true, true, true, false}));
  EXPECT_EQ(UInt64ToBitString(15), (std::vector<bool>{true, true, true, true}));
  EXPECT_EQ(UInt64ToBitString(16),
            (std::vector<bool>{true, false, false, false, false}));
  // Powers of two, and powers of two - 1
  for (uint8_t i = 1; i < 64; ++i) {
    EXPECT_EQ(UInt64ToBitString((uint64_t(1) << i) - 1),
              std::vector<bool>(i, true));
    std::vector<bool> twoToI{true};
    for (auto j = i; j > 0; --j) {
      twoToI.push_back(false);
    }
    EXPECT_EQ(UInt64ToBitString(uint64_t(1) << i), twoToI)
        << "Did not convert 2^i = 2^" << int(i) << " to a bit string properly";
  }
}

TEST(UtilTest, UInt64ToBitStringWithSize) {
  // Cases where given size is larger than the size of the input
  EXPECT_EQ(UInt64ToBitString(0, 4),
            (std::vector<bool>{false, false, false, false}));
  EXPECT_EQ(UInt64ToBitString(1, 4),
            (std::vector<bool>{false, false, false, true}));
  EXPECT_EQ(UInt64ToBitString(2, 4),
            (std::vector<bool>{false, false, true, false}));
  EXPECT_EQ(UInt64ToBitString(3, 4),
            (std::vector<bool>{false, false, true, true}));
  EXPECT_EQ(UInt64ToBitString(4, 4),
            (std::vector<bool>{false, true, false, false}));
  EXPECT_EQ(UInt64ToBitString(5, 4),
            (std::vector<bool>{false, true, false, true}));
  EXPECT_EQ(UInt64ToBitString(6, 4),
            (std::vector<bool>{false, true, true, false}));
  EXPECT_EQ(UInt64ToBitString(7, 4),
            (std::vector<bool>{false, true, true, true}));
  // Cases where the size is exact
  EXPECT_EQ(UInt64ToBitString(8, 4),
            (std::vector<bool>{true, false, false, false}));
  EXPECT_EQ(UInt64ToBitString(9, 4),
            (std::vector<bool>{true, false, false, true}));
  EXPECT_EQ(UInt64ToBitString(10, 4),
            (std::vector<bool>{true, false, true, false}));
  EXPECT_EQ(UInt64ToBitString(11, 4),
            (std::vector<bool>{true, false, true, true}));
  EXPECT_EQ(UInt64ToBitString(12, 4),
            (std::vector<bool>{true, true, false, false}));
  EXPECT_EQ(UInt64ToBitString(13, 4),
            (std::vector<bool>{true, true, false, true}));
  EXPECT_EQ(UInt64ToBitString(14, 4),
            (std::vector<bool>{true, true, true, false}));
  EXPECT_EQ(UInt64ToBitString(15, 4),
            (std::vector<bool>{true, true, true, true}));
  // Cases where the given size is too small to represent the input
  EXPECT_EQ(UInt64ToBitString(16, 4),
            (std::vector<bool>{false, false, false, false}));
  for (uint64_t i = 0; i < 16; ++i) {
    EXPECT_EQ(UInt64ToBitString(i, 4), UInt64ToBitString(i + 16, 4));
  }
}

TEST(UtilTest, PrettyPrintSmallBitStrings) {
  EXPECT_EQ(PrettyPrintToString({}), "0w0");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(0)), "1w0x0");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(0)), "1w0x0");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(1)), "1w0x1");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(2)), "2w0x2");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(3)), "2w0x3");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(4)), "3w0x4");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(5)), "3w0x5");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(6)), "3w0x6");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(7)), "3w0x7");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(8)), "4w0x8");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(9)), "4w0x9");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(10)), "4w0xA");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(11)), "4w0xB");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(12)), "4w0xC");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(13)), "4w0xD");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(14)), "4w0xE");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(15)), "4w0xF");
  EXPECT_EQ(PrettyPrintToString(UInt64ToBitString(16)), "5w0x10");
}

TEST(UtilTest, PrettyPrintStringsStartingWithZeros) {
  EXPECT_EQ(PrettyPrintToString(std::vector<bool>{false, false}), "2w0x0");
  EXPECT_EQ(PrettyPrintToString(std::vector<bool>{false, false, false}),
            "3w0x0");
  EXPECT_EQ(PrettyPrintToString(std::vector<bool>(9, false)), "9w0x000");
  EXPECT_EQ(
      PrettyPrintToString(std::vector<bool>{false, false, false, false, true}),
      "5w0x01");
  EXPECT_EQ(PrettyPrintToString(std::vector<bool>{false, false, false, true,
                                                  false, false, false}),
            "7w0x08");
  EXPECT_EQ(PrettyPrintToString(std::vector<bool>{false, false, false, false,
                                                  true, false, false, false}),
            "8w0x08");
  EXPECT_EQ(PrettyPrintToString(std::vector<bool>{
                false, false, false, false, false, true, false, false, false}),
            "9w0x008");
}

TEST(UtilTest, HexStringToBitString) {
  // Simple size test for zeros
  EXPECT_EQ(HexStringToBitString(0, "0"), (std::vector<bool>{}));
  EXPECT_EQ(HexStringToBitString(1, "0"), (std::vector<bool>{false}));
  EXPECT_EQ(HexStringToBitString(2, "0"), (std::vector<bool>{false, false}));
  EXPECT_EQ(HexStringToBitString(3, "0"),
            (std::vector<bool>{false, false, false}));
  EXPECT_EQ(HexStringToBitString(4, "0"),
            (std::vector<bool>{false, false, false, false}));
  EXPECT_EQ(HexStringToBitString(5, "0"),
            (std::vector<bool>{false, false, false, false, false}));

  // Test single digits
  for (size_t i = 0; i < 16; ++i) {
    EXPECT_EQ(HexStringToBitString(4, kHexDigits[i]),
              (std::vector<bool>{bool(i & (1 << 3)), bool(i & (1 << 2)),
                                 bool(i & (1 << 1)), bool(i & (1 << 0))}))
        << "hex_string = " << kHexDigits[i];
    // Test the case where the given size is larger than the given string's
    // length
    EXPECT_EQ(HexStringToBitString(5, kHexDigits[i]),
              (std::vector<bool>{false, bool(i & (1 << 3)), bool(i & (1 << 2)),
                                 bool(i & (1 << 1)), bool(i & (1 << 0))}))
        << "hex_string = " << kHexDigits[i];
    // Test the case where the given size is smaller than the given string's
    // length
    EXPECT_EQ(HexStringToBitString(3, kHexDigits[i]),
              (std::vector<bool>{bool(i & (1 << 2)), bool(i & (1 << 1)),
                                 bool(i & (1 << 0))}))
        << "hex_string = " << kHexDigits[i];
  }

  // Test two digits
  for (size_t first_digit = 0; first_digit < 16; ++first_digit) {
    for (size_t second_digit = 0; second_digit < 16; ++second_digit) {
      std::string hex_string{kHexDigits[first_digit]};
      hex_string += kHexDigits[second_digit];
      std::vector<bool> expected_result{
          bool(first_digit & (1 << 3)),  bool(first_digit & (1 << 2)),
          bool(first_digit & (1 << 1)),  bool(first_digit & (1 << 0)),
          bool(second_digit & (1 << 3)), bool(second_digit & (1 << 2)),
          bool(second_digit & (1 << 1)), bool(second_digit & (1 << 0))};
      EXPECT_EQ(HexStringToBitString(8, hex_string), expected_result)
          << "hex_string = " << hex_string;
      // Test lowercase as well
      for (auto& c : hex_string) {
        c = std::tolower(c);
      }
      EXPECT_EQ(HexStringToBitString(8, hex_string), expected_result)
          << "hex_string = " << hex_string;
    }
  }
}

TEST(UtilTest, SizedStringToBitString) {
  // Simple size test for handling zeros specially
  EXPECT_EQ(SizedStringToBitString("0w0").value(), (std::vector<bool>{}));
  EXPECT_EQ(SizedStringToBitString("1w0").value(), (std::vector<bool>{false}));
  EXPECT_EQ(SizedStringToBitString("2w0").value(),
            (std::vector<bool>{false, false}));
  EXPECT_EQ(SizedStringToBitString("3w0").value(),
            (std::vector<bool>{false, false, false}));
  EXPECT_EQ(SizedStringToBitString("4w0").value(),
            (std::vector<bool>{false, false, false, false}));
  EXPECT_EQ(SizedStringToBitString("5w0").value(),
            (std::vector<bool>{false, false, false, false, false}));

  // Test single digits
  for (size_t i = 0; i < 16; ++i) {
    std::ostringstream sized_string;
    sized_string << "4w0x" << kHexDigits[i];
    EXPECT_EQ(SizedStringToBitString(sized_string.str()).value(),
              (std::vector<bool>{bool(i & (1 << 3)), bool(i & (1 << 2)),
                                 bool(i & (1 << 1)), bool(i & (1 << 0))}))
        << "sized_string = " << sized_string.str();
    // Test the case where the given size is larger than the given string's
    // length
    sized_string = {};
    sized_string << "5w0x" << kHexDigits[i];
    EXPECT_EQ(SizedStringToBitString(sized_string.str()).value(),
              (std::vector<bool>{false, bool(i & (1 << 3)), bool(i & (1 << 2)),
                                 bool(i & (1 << 1)), bool(i & (1 << 0))}))
        << "sized_string = " << sized_string.str();
    // Test the case where the given size is smaller than the given string's
    // length
    sized_string = {};
    sized_string << "3w0x" << kHexDigits[i];
    EXPECT_EQ(SizedStringToBitString(sized_string.str()).value(),
              (std::vector<bool>{bool(i & (1 << 2)), bool(i & (1 << 1)),
                                 bool(i & (1 << 0))}))
        << "sized_string = " << sized_string.str();
  }
  // Test two digits
  for (size_t first_digit = 0; first_digit < 16; ++first_digit) {
    for (size_t second_digit = 0; second_digit < 16; ++second_digit) {
      std::ostringstream sized_string;
      sized_string << "8w0x" << kHexDigits[first_digit]
                   << kHexDigits[second_digit];
      std::vector<bool> expected_result{
          bool(first_digit & (1 << 3)),  bool(first_digit & (1 << 2)),
          bool(first_digit & (1 << 1)),  bool(first_digit & (1 << 0)),
          bool(second_digit & (1 << 3)), bool(second_digit & (1 << 2)),
          bool(second_digit & (1 << 1)), bool(second_digit & (1 << 0))};
      EXPECT_EQ(SizedStringToBitString(sized_string.str()).value(),
                expected_result);
    }
  }

  // Test invalid inputs

  // Values with no specified width
  EXPECT_FALSE(SizedStringToBitString("0").ok());
  EXPECT_FALSE(SizedStringToBitString("FFFF").ok());
  EXPECT_FALSE(SizedStringToBitString("0xFF").ok());

  // Empty string as width
  EXPECT_FALSE(SizedStringToBitString("w0").ok());
  EXPECT_FALSE(SizedStringToBitString("w0xFF").ok());

  // Negative width
  EXPECT_FALSE(SizedStringToBitString("-2w0").ok());
  EXPECT_FALSE(SizedStringToBitString("-2w0xFF").ok());

  // Values with invalid characters
  EXPECT_FALSE(SizedStringToBitString("2w0q").ok());
  EXPECT_FALSE(SizedStringToBitString("q2w0").ok());
  EXPECT_FALSE(SizedStringToBitString("%0w2").ok());

  // Hexadecimal values without 0x
  EXPECT_FALSE(SizedStringToBitString("8wF0").ok());
  EXPECT_FALSE(SizedStringToBitString("8wf0").ok());
}

TEST(UtilTest, ParseUInt32) {
  // All 16-bit numbers
  for (uint32_t i = 0; i <= std::numeric_limits<uint16_t>::max(); ++i) {
    auto result = ParseUInt32(std::to_string(i));
    ASSERT_TRUE(result.ok()) << "failed to parse the number " << i;
    ASSERT_EQ(*result, i) << "parsed the number " << i << " incorrectly";
  }

  // Some large 32-bit numbers
  EXPECT_EQ(
      ParseUInt32(std::to_string(std::numeric_limits<uint32_t>::max())).value(),
      std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(
      ParseUInt32(std::to_string(std::numeric_limits<uint32_t>::max() - 1))
          .value(),
      std::numeric_limits<uint32_t>::max() - 1);
  EXPECT_EQ(
      ParseUInt32(std::to_string(std::numeric_limits<uint32_t>::max() / 2 + 1))
          .value(),
      std::numeric_limits<uint32_t>::max() / 2 + 1);

  // Invalid inputs:

  // Negative integers
  EXPECT_FALSE(ParseUInt32("-2").ok());
  // Strings that are not valid decimal numbers
  EXPECT_FALSE(ParseUInt32("0x2").ok());
  EXPECT_FALSE(ParseUInt32("0w").ok());
  EXPECT_FALSE(ParseUInt32("0-").ok());
  EXPECT_FALSE(ParseUInt32("+0").ok());
}

}  // namespace
}  // namespace backends::tc::util
