#include "lib/nethash.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string_view>

namespace Test {

using namespace NetHash;

template <typename T>
struct Hex {
    Hex(T value) : value(value) {}
    bool operator==(const Hex &o) const { return value == o.value; }
    friend std::ostream &operator<<(std::ostream &os, Hex val) {
        auto fl = os.flags();
        os << "0x" << std::hex << val.value;
        os.flags(fl);
        return os;
    }
    T value;
};

template <auto fn>
auto apply(std::string_view str) {
    return Hex(fn(reinterpret_cast<const uint8_t *>(str.data()), str.size()));
}

template <auto fn>
auto apply(std::initializer_list<uint8_t> data) {
    return Hex(fn(std::data(data), std::size(data)));
}

Hex<uint32_t> operator""_u32(unsigned long long v) { return v; }
Hex<uint16_t> operator""_u16(unsigned long long v) { return v; }

TEST(NetHash, crc32) {
    EXPECT_EQ(apply<crc32>({}), 0x00000000_u32);
    EXPECT_EQ(apply<crc32>({0}), 0xD202EF8D_u32);
    EXPECT_EQ(apply<crc32>({0, 0, 0, 0, 0}), 0xC622F71D_u32);
    EXPECT_EQ(apply<crc32>("fooo"), 0x43EAF07F_u32);
    EXPECT_EQ(apply<crc32>("a"), 0xE8B7BE43_u32);
    EXPECT_EQ(apply<crc32>("foobar"), 0x9EF61F95_u32);
    EXPECT_EQ(apply<crc32>("foobar%142qrs"), 0x95E1D00B_u32);
}

TEST(NetHash, crc16) {
    EXPECT_EQ(apply<crc16>({}), 0x0000_u16);
    EXPECT_EQ(apply<crc16>({0}), 0x0000_u16);
    EXPECT_EQ(apply<crc16>({0, 0, 0, 0, 0}), 0x0000_u16);
    EXPECT_EQ(apply<crc16>("fooo"), 0x4943_u16);
    EXPECT_EQ(apply<crc16>("a"), 0xE8C1_u16);
    EXPECT_EQ(apply<crc16>("foobar"), 0xB0C8_u16);
    EXPECT_EQ(apply<crc16>("foobar%142qrs"), 0x3DA9_u16);
}

TEST(NetHash, crcCCITT) {
    EXPECT_EQ(apply<crcCCITT>({}), 0xFFFF_u16);
    EXPECT_EQ(apply<crcCCITT>({0}), 0xE1F0_u16);
    EXPECT_EQ(apply<crcCCITT>({0, 0, 0, 0, 0}), 0x110C_u16);
    EXPECT_EQ(apply<crcCCITT>("fooo"), 0xCB8C_u16);
    EXPECT_EQ(apply<crcCCITT>("a"), 0x9D77_u16);
    EXPECT_EQ(apply<crcCCITT>("foobar"), 0xBE35_u16);
    EXPECT_EQ(apply<crcCCITT>("foobar%142qrs"), 0x5A92_u16);
}

}  // namespace Test
