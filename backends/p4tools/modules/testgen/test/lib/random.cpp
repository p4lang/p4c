// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <cstdlib>
#include <limits>
#include <vector>

#include "backends/p4tools/common/lib/util.h"

namespace P4::P4Tools::Test {

TEST(Random, UnseededValues) {
    EXPECT_FALSE(Utils::getCurrentSeed().has_value());
    EXPECT_EQ(Utils::getRandInt(123), 0U);
    EXPECT_EQ(Utils::getRandBigInt(big_int(1) << 128), 0);
    EXPECT_EQ(Utils::getRandBigInt(100, 200), 0);
}

TEST(Random, SeededRanges) {
    // Isolate the process-global seed, which may only be initialized once.
    ASSERT_EXIT(
        {
            Utils::setRandomSeed(0);
            EXPECT_EQ(Utils::getRandBigInt(0), 0);
            EXPECT_EQ(Utils::getRandBigInt(-17, -17), -17);
            EXPECT_EQ(Utils::getRandInt(0), 0U);
            EXPECT_EQ(Utils::getRandInt(-17, -17), -17);

            for (unsigned bits :
                 std::vector<unsigned>({1U, 31U, 32U, 33U, 63U, 64U, 65U, 128U, 257U})) {
                const big_int limit = big_int(1) << bits;
                bool sawUpperHalf = false;
                bool sawLowerHalf = false;
                for (unsigned i = 0; i < 256; ++i) {
                    const big_int value = Utils::getRandBigInt(limit);
                    EXPECT_GE(value, 0);
                    EXPECT_LE(value, limit);
                    sawUpperHalf |= value >= limit / 2;
                    sawLowerHalf |= value < limit / 2;
                    const big_int negative = Utils::getRandBigInt(-limit, -limit / 2);
                    EXPECT_GE(negative, -limit);
                    EXPECT_LE(negative, -limit / 2);
                    const big_int mixed = Utils::getRandBigInt(-limit, limit);
                    EXPECT_GE(mixed, -limit);
                    EXPECT_LE(mixed, limit);
                }
                EXPECT_TRUE(sawUpperHalf);
                EXPECT_TRUE(sawLowerHalf);
            }
            bool seen[3] = {};
            bool sawWideUnsigned = false;
            bool sawPositive = false;
            bool sawNegative = false;
            for (unsigned i = 0; i < 256; ++i) {
                const auto value = Utils::getRandBigInt(2).convert_to<unsigned>();
                ASSERT_LE(value, 2U);
                seen[value] = true;
                sawWideUnsigned |= Utils::getRandInt(std::numeric_limits<uint64_t>::max()) >
                                   std::numeric_limits<uint32_t>::max();
                const auto signedValue = Utils::getRandInt(std::numeric_limits<int64_t>::min(),
                                                           std::numeric_limits<int64_t>::max());
                sawPositive |= signedValue > 0;
                sawNegative |= signedValue < 0;
            }
            EXPECT_TRUE(seen[0] && seen[1] && seen[2]);
            EXPECT_TRUE(sawWideUnsigned && sawPositive && sawNegative);
            std::exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
}

}  // namespace P4::P4Tools::Test
