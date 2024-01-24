#include "backends/p4tools/modules/testgen/test/lib/format_int.h"

#include <stdint.h>

#include <gtest/gtest.h>

#include <string>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/format_int.h"
#include "ir/ir.h"
#include "ir/irutils.h"

namespace Test {

namespace {

using P4Tools::convertBigIntToBytes;
using P4Tools::convertToIpv4String;
using P4Tools::convertToIpv6String;
using P4Tools::convertToMacString;
using P4Tools::formatBinExpr;
using P4Tools::formatHexExpr;
using P4Tools::formatOctalExpr;
using P4Tools::insertHexSeparators;
using P4Tools::insertOctalSeparators;
using P4Tools::insertSeparators;

// Tests for formatHexExpr
TEST_F(FormatTest, Format01) {
    {
        const auto *typeBits = IR::getBitType(16);
        const auto *sixteenBits = IR::getConstant(typeBits, 0x10);
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, false}).c_str(), "10");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, true}).c_str(), "0x10");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, false}).c_str(), "0010");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, true}).c_str(), "0x0010");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, false}).c_str(), "10");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, true}).c_str(), "0x10");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, false}).c_str(), "0010");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, true}).c_str(), "0x0010");
    }
    {
        const auto *typeBits = IR::getBitType(64);
        const auto *sixteenBits = IR::getConstant(typeBits, 0x060000);
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, false}).c_str(), "60000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, true}).c_str(), "0x60000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, false}).c_str(), "0000000000060000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, true}).c_str(), "0x0000000000060000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, false}).c_str(), "0006_0000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, true}).c_str(), "0x0006_0000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, false}).c_str(),
                     "0000_0000_0006_0000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, true}).c_str(),
                     "0x0000_0000_0006_0000");
    }
    {
        const auto *typeBits = IR::getBitType(62);
        const auto *sixteenBits = IR::getConstant(typeBits, 0x060000);
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, false}).c_str(), "60000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, true}).c_str(), "0x60000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, false}).c_str(), "0000000000060000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, true}).c_str(), "0x0000000000060000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, false}).c_str(), "0006_0000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, true}).c_str(), "0x0006_0000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, false}).c_str(),
                     "0000_0000_0006_0000");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, true}).c_str(),
                     "0x0000_0000_0006_0000");
    }
    {
        const auto *typeBits = IR::getBitType(1);
        const auto *sixteenBits = IR::getConstant(typeBits, 0x1);
        ASSERT_STREQ(formatHexExpr(sixteenBits).c_str(), "0x1");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, false}).c_str(), "1");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, true}).c_str(), "0x1");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, false}).c_str(), "1");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, true}).c_str(), "0x1");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, false}).c_str(), "1");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, true}).c_str(), "0x1");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, false}).c_str(), "1");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, true}).c_str(), "0x1");
    }
    {
        const auto *typeBits = IR::getBitType(16, true);
        const auto *sixteenBits = IR::getConstant(typeBits, -1);
        ASSERT_EQ(static_cast<uint16_t>(sixteenBits->asInt64()), 0xFFFF);
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, false}).c_str(), "FFFF");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, true}).c_str(), "0xFFFF");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, false}).c_str(), "FFFF");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, true}).c_str(), "0xFFFF");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, false}).c_str(), "FFFF");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, true}).c_str(), "0xFFFF");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, false}).c_str(), "FFFF");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, true}).c_str(), "0xFFFF");
    }
    {
        const auto *typeBits = IR::getBitType(16, true);
        const auto *sixteenBits = IR::getConstant(typeBits, -1);
        ASSERT_EQ(static_cast<uint16_t>(sixteenBits->asInt64()), 0xFFFF);
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, false, false}).c_str(), "ffff");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, true, false}).c_str(), "0xffff");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, false, false}).c_str(), "ffff");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, true, false}).c_str(), "0xffff");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, false, false}).c_str(), "ffff");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, true, false}).c_str(), "0xffff");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, false, false}).c_str(), "ffff");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, true, false}).c_str(), "0xffff");
    }
    {
        const auto *typeBits = IR::getBitType(16, true);
        const auto *sixteenBits = IR::getConstant(typeBits, -32767);
        ASSERT_EQ(static_cast<uint16_t>(sixteenBits->asInt64()), 0x8001);
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, false}).c_str(), "8001");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, false, true}).c_str(), "0x8001");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, false}).c_str(), "8001");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {false, true, true}).c_str(), "0x8001");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, false}).c_str(), "8001");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, false, true}).c_str(), "0x8001");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, false}).c_str(), "8001");
        ASSERT_STREQ(formatHexExpr(sixteenBits, {true, true, true}).c_str(), "0x8001");
    }
}

// Tests for formatOctalExpr
TEST_F(FormatTest, Format02) {
    {
        const auto *typeBits = IR::getBitType(8);
        const auto *sixteenBits = IR::getConstant(typeBits, 012);
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, false}).c_str(), "12");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, true}).c_str(), "012");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, false}).c_str(), "0012");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, true}).c_str(), "00012");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, false}).c_str(), "12");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, true}).c_str(), "012");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, false}).c_str(), "0012");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, true}).c_str(), "00012");
    }
    {
        const auto *typeBits = IR::getBitType(8, true);
        const auto *sixteenBits = IR::getConstant(typeBits, -012);
        ASSERT_EQ(static_cast<uint8_t>(sixteenBits->asInt64()), 0366);
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, false}).c_str(), "366");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, true}).c_str(), "0366");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, false}).c_str(), "0366");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, true}).c_str(), "00366");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, false}).c_str(), "366");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, true}).c_str(), "0366");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, false}).c_str(), "0366");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, true}).c_str(), "00366");
    }
    {
        const auto *typeBits = IR::getBitType(16);
        const auto *sixteenBits = IR::getConstant(typeBits, 020);
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, false}).c_str(), "20");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, true}).c_str(), "020");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, false}).c_str(), "00000020");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, true}).c_str(), "000000020");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, false}).c_str(), "20");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, true}).c_str(), "020");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, false}).c_str(), "0000_0020");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, true}).c_str(), "00000_0020");
    }
    {
        const auto *typeBits = IR::getBitType(16, true);
        const auto *sixteenBits = IR::getConstant(typeBits, -020);
        ASSERT_EQ(static_cast<uint16_t>(sixteenBits->asInt64()), 0177760);
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, false}).c_str(), "177760");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, true}).c_str(), "0177760");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, false}).c_str(), "00177760");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, true}).c_str(), "000177760");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, false}).c_str(), "0017_7760");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, true}).c_str(), "00017_7760");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, false}).c_str(), "0017_7760");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, true}).c_str(), "00017_7760");
    }
    {
        const auto *typeBits = IR::getBitType(8);
        const auto *sixteenBits = IR::getConstant(typeBits, 010);
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, false}).c_str(), "10");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, true}).c_str(), "010");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, false}).c_str(), "0010");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, true}).c_str(), "00010");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, false}).c_str(), "10");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, true}).c_str(), "010");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, false}).c_str(), "0010");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, true}).c_str(), "00010");
    }
    {
        const auto *typeBits = IR::getBitType(8, true);
        const auto *sixteenBits = IR::getConstant(typeBits, -010);
        ASSERT_EQ(static_cast<uint8_t>(sixteenBits->asInt64()), 0370);
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, false}).c_str(), "370");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, false, true}).c_str(), "0370");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, false}).c_str(), "0370");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {false, true, true}).c_str(), "00370");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, false}).c_str(), "370");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, false, true}).c_str(), "0370");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, false}).c_str(), "0370");
        ASSERT_STREQ(formatOctalExpr(sixteenBits, {true, true, true}).c_str(), "00370");
    }
}

// Tests for formatBinExpr
TEST_F(FormatTest, Format03) {
    {
        const auto *typeBits = IR::getBitType(8);
        const auto *sixteenBits = IR::getConstant(typeBits, 0b11);
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, false, false}).c_str(), "11");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, false, true}).c_str(), "0b11");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, true, false}).c_str(), "00000011");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, true, true}).c_str(), "0b00000011");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, false, false}).c_str(), "11");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, false, true}).c_str(), "0b11");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, true, false}).c_str(), "0000_0011");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, true, true}).c_str(), "0b0000_0011");
    }
    {
        const auto *typeBits = IR::getBitType(8, true);
        const auto *sixteenBits = IR::getConstant(typeBits, -0b00111111);
        ASSERT_EQ(static_cast<uint8_t>(sixteenBits->asInt64()), 0b11000001);
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, false, false}).c_str(), "11000001");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, false, true}).c_str(), "0b11000001");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, true, false}).c_str(), "11000001");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, true, true}).c_str(), "0b11000001");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, false, false}).c_str(), "1100_0001");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, false, true}).c_str(), "0b1100_0001");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, true, false}).c_str(), "1100_0001");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, true, true}).c_str(), "0b1100_0001");
    }
    {
        const auto *typeBits = IR::getBitType(16);
        const auto *sixteenBits = IR::getConstant(typeBits, 0b10000);
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, false, false}).c_str(), "10000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, false, true}).c_str(), "0b10000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, true, false}).c_str(), "0000000000010000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, true, true}).c_str(), "0b0000000000010000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, false, false}).c_str(), "0001_0000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, false, true}).c_str(), "0b0001_0000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, true, false}).c_str(),
                     "0000_0000_0001_0000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, true, true}).c_str(),
                     "0b0000_0000_0001_0000");
    }
    {
        const auto *typeBits = IR::getBitType(16, true);
        const auto *sixteenBits = IR::getConstant(typeBits, -0b0000000000010000);
        ASSERT_EQ(static_cast<uint16_t>(sixteenBits->asInt64()), 0b1111111111110000);
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, false, false}).c_str(), "1111111111110000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, false, true}).c_str(),
                     "0b1111111111110000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, true, false}).c_str(), "1111111111110000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {false, true, true}).c_str(), "0b1111111111110000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, false, false}).c_str(),
                     "1111_1111_1111_0000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, false, true}).c_str(),
                     "0b1111_1111_1111_0000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, true, false}).c_str(),
                     "1111_1111_1111_0000");
        ASSERT_STREQ(formatBinExpr(sixteenBits, {true, true, true}).c_str(),
                     "0b1111_1111_1111_0000");
    }
}

// Tests for insertOctalSeparators and insertHexSeparators
TEST_F(FormatTest, Format04) {
    {
        ASSERT_STREQ(insertHexSeparators("AEC192").c_str(), "\\xAE\\xC1\\x92");
        ASSERT_STREQ(insertHexSeparators("E3D4").c_str(), "\\xE3\\xD4");
        ASSERT_STREQ(insertOctalSeparators("0712522321").c_str(), "\\000\\712\\522\\321");
        ASSERT_STREQ(insertOctalSeparators("02310").c_str(), "\\002\\310");
        ASSERT_STREQ(insertSeparators("02310", "\\", 2, true).c_str(), "00\\23\\10");
        ASSERT_STREQ(insertSeparators("0712522321", "\\", 2, true).c_str(), "07\\12\\52\\23\\21");
        ASSERT_STREQ(insertSeparators("E3D4", "\\x", 2, true).c_str(), "E3\\xD4");
        ASSERT_STREQ(insertSeparators("AEC192", "\\x", 2, true).c_str(), "AE\\xC1\\x92");
    }
}

// Tests for conversion of integers into IPv4 addresses.
TEST_F(FormatTest, TestIPv4Conversion) {
    {
        std::vector<uint8_t> bytes = {192, 168, 1, 1};
        auto result = convertToIpv4String(bytes);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "192.168.1.1");
    }
    {
        auto result = convertToIpv4String(convertBigIntToBytes(big_int("3232235777"), 32));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "192.168.1.1");
    }
    {
        auto result = convertToIpv4String(convertBigIntToBytes(big_int("65535"), 32));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "255.255.0.0");
    }
    {
        std::vector<uint8_t> bytes = {0x01, 0x00, 0x00, 0x00};
        auto result = convertToIpv4String(bytes);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "1.0.0.0");
    }
    {
        auto result = convertToIpv4String(convertBigIntToBytes(big_int("1"), 32));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "1.0.0.0");
    }
}

TEST_F(FormatTest, TestIPv6Conversion) {
    {
        std::vector<uint8_t> bytes = {
            0x20, 0x01, 0x0d, 0xb8, 0x85, 0xa3, 0x00, 0x00,
            0x00, 0x00, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x34,
        };
        auto result = convertToIpv6String(bytes);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "2001:0db8:85a3:0000:0000:8a2e:0370:7334");
    }
    {
        auto result = convertToIpv6String(
            convertBigIntToBytes(big_int("42540766452641154071740215577757643572"), 128));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "2001:0db8:85a3:0000:0000:8a2e:0370:7334");
    }
    {
        auto result =
            convertToIpv6String(convertBigIntToBytes(big_int("18446744073709551615"), 128));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "ffff:ffff:ffff:ffff:0000:0000:0000:0000");
    }
    {
        std::vector<uint8_t> bytes = {
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };
        auto result = convertToIpv6String(bytes);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "0100:0000:0000:0000:0000:0000:0000:0000");
    }
    {
        auto result = convertToIpv6String(convertBigIntToBytes(big_int("1"), 128));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "0100:0000:0000:0000:0000:0000:0000:0000");
    }
}

TEST_F(FormatTest, TestMACConversion) {
    {
        std::vector<uint8_t> bytes = {0xFF, 0x11, 0x22, 0x33, 0x44, 0x55};
        auto result = convertToMacString(bytes);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "ff:11:22:33:44:55");
    }
    {
        auto result = convertToMacString(convertBigIntToBytes(big_int("280449053312085"), 48));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "ff:11:22:33:44:55");
    }
    {
        auto result = convertToMacString(convertBigIntToBytes(big_int("4294967295"), 48));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "ff:ff:ff:ff:00:00");
    }
    {
        std::vector<uint8_t> bytes = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
        auto result = convertToMacString(bytes);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "01:00:00:00:00:00");
    }
    {
        auto result = convertToMacString(convertBigIntToBytes(big_int("1"), 48));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result, "01:00:00:00:00:00");
    }
}

}  // anonymous namespace

}  // namespace Test
