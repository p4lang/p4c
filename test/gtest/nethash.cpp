#include "lib/nethash.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string_view>

namespace Test {

using namespace NetHash;

template <typename T>
struct Hex {
    explicit Hex(T value) : value(value) {}
    bool operator==(const Hex &o) const { return value == o.value; }
    Hex operator~() const { return Hex(~value); }
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

Hex<uint64_t> operator""_u64(unsigned long long v) { return Hex<uint64_t>(v); }
Hex<uint32_t> operator""_u32(unsigned long long v) { return Hex<uint32_t>(v); }
Hex<uint16_t> operator""_u16(unsigned long long v) { return Hex<uint16_t>(v); }

TEST(NetHash, crc32) {
    EXPECT_EQ(apply<crc32>({}), 0x00000000_u32);
    EXPECT_EQ(apply<crc32>({0}), 0xD202EF8D_u32);
    EXPECT_EQ(apply<crc32>({0, 0, 0, 0, 0}), 0xC622F71D_u32);
    EXPECT_EQ(apply<crc32>({0x0b, 0xb8, 0x1f, 0x90}), 0x005d6a6f_u32);
    EXPECT_EQ(apply<crc32>("fooo"), 0x43EAF07F_u32);
    EXPECT_EQ(apply<crc32>("a"), 0xE8B7BE43_u32);
    EXPECT_EQ(apply<crc32>("foobar"), 0x9EF61F95_u32);
    EXPECT_EQ(apply<crc32>("foobar%142qrs"), 0x95E1D00B_u32);
}

TEST(NetHash, crc32FCS) {
    EXPECT_EQ(apply<crc32FCS>({}), 0x00000000_u32);
    EXPECT_EQ(apply<crc32FCS>({0}), 0xB1F7404B_u32);
    EXPECT_EQ(apply<crc32FCS>({0, 0, 0, 0, 0}), 0xB8EF4463_u32);
    EXPECT_EQ(apply<crc32FCS>({0x0b, 0xb8, 0x1f, 0x90}), 0xEDA1EA6D_u32);
    EXPECT_EQ(apply<crc32FCS>("fooo"), 0xCD961E19_u32);
    EXPECT_EQ(apply<crc32FCS>("a"), 0x19939B6B_u32);
    EXPECT_EQ(apply<crc32FCS>("foobar"), 0x52C03DC1_u32);
    EXPECT_EQ(apply<crc32FCS>("foobar%142qrs"), 0xF3630DE4_u32);
}

TEST(NetHash, crc16) {
    EXPECT_EQ(apply<crc16>({}), 0x0000_u16);
    EXPECT_EQ(apply<crc16>({0}), 0x0000_u16);
    EXPECT_EQ(apply<crc16>({0, 0, 0, 0, 0}), 0x0000_u16);
    EXPECT_EQ(apply<crc16>({0x0b, 0xb8, 0x1f, 0x90}), 0x5d8a_u16);
    EXPECT_EQ(apply<crc16>("fooo"), 0x4943_u16);
    EXPECT_EQ(apply<crc16>("a"), 0xE8C1_u16);
    EXPECT_EQ(apply<crc16>("foobar"), 0xB0C8_u16);
    EXPECT_EQ(apply<crc16>("foobar%142qrs"), 0x3DA9_u16);
}

TEST(NetHash, crc16ANSI) {
    EXPECT_EQ(apply<crc16ANSI>({}), 0x0000_u16);
    EXPECT_EQ(apply<crc16ANSI>({0}), 0x0000_u16);
    EXPECT_EQ(apply<crc16ANSI>({0, 0, 0, 0, 0}), 0x0000_u16);
    EXPECT_EQ(apply<crc16ANSI>({0x0b, 0xb8, 0x1f, 0x90}), 0xD400_u16);
    EXPECT_EQ(apply<crc16ANSI>("fooo"), 0x9C3A_u16);
    EXPECT_EQ(apply<crc16ANSI>("a"), 0x8145_u16);
    EXPECT_EQ(apply<crc16ANSI>("foobar"), 0x8F5B_u16);
    EXPECT_EQ(apply<crc16ANSI>("foobar%142qrs"), 0x5A5E_u16);
}

TEST(NetHash, crcCCITT) {
    EXPECT_EQ(apply<crcCCITT>({}), 0xFFFF_u16);
    EXPECT_EQ(apply<crcCCITT>({0}), 0xE1F0_u16);
    EXPECT_EQ(apply<crcCCITT>({0, 0, 0, 0, 0}), 0x110C_u16);
    EXPECT_EQ(apply<crcCCITT>({0x0b, 0xb8, 0x1f, 0x90}), 0x5d75_u16);
    EXPECT_EQ(apply<crcCCITT>("fooo"), 0xCB8C_u16);
    EXPECT_EQ(apply<crcCCITT>("a"), 0x9D77_u16);
    EXPECT_EQ(apply<crcCCITT>("foobar"), 0xBE35_u16);
    EXPECT_EQ(apply<crcCCITT>("foobar%142qrs"), 0x5A92_u16);
}

TEST(NetHash, csum16) {
    EXPECT_EQ(apply<csum16>({0x45, 0x04}), ~0x4504_u16);
    EXPECT_EQ(apply<csum16>({0x45, 0x04, 0x73, 0x00}), ~0xb804_u16);
    EXPECT_EQ(apply<csum16>({0x00, 0x45, 0x04, 0x73}), ~0x04b8_u16);
    EXPECT_EQ(apply<csum16>({0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
                             0x00, 0x00, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7}),
              0xb861_u16);
}

TEST(NetHash, xor16) { EXPECT_EQ(apply<xor16>({0x0b, 0xb8, 0x1f, 0x90}), 0x1428_u16); }

TEST(NetHash, identity) {
    EXPECT_EQ(apply<identity>({1, 2, 3, 4, 5}), 0x0102030405_u64);
    EXPECT_EQ(apply<identity>({0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
                               0x00, 0x00, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7}),
              0x4500'0073'0000'4000_u64);
}

}  // namespace Test
