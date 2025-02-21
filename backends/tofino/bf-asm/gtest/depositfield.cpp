/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backends/tofino/bf-asm/depositfield.h"

#include <gtest/gtest.h>

#if __cplusplus < 201402L && __cpp_binary_literals < 201304
#error "Binary literals are required"
// We could fall back on boost/utility/binary.hpp
#endif

namespace {

constexpr int conSize8 = 8;
constexpr int conSize32 = 32;
constexpr int tooLarge = 8;
constexpr int tooSmall = -9;
constexpr int tooSmall2 = -5;

TEST(depositfield, 0) {
    int32_t zero = 0;
    auto res = DepositField::discoverRotation(zero, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, zero);
    res = DepositField::discoverRotation(zero, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, zero);
    res = DepositField::discoverRotation(zero, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, zero);
}

TEST(depositfield, large) {
    int32_t value = tooLarge - 1;
    auto res = DepositField::discoverRotation(value, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, value);
    res = DepositField::discoverRotation(value, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, value);
    res = DepositField::discoverRotation(value, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, value);
}

TEST(depositfield, small) {
    int32_t value = tooSmall + 1;
    int32_t value2 = tooSmall2 + 1;
    auto res = DepositField::discoverRotation(value, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, value);
    res = DepositField::discoverRotation(value, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, value);
    ASSERT_TRUE(value < tooSmall2);
    res = DepositField::discoverRotation(value, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 0U);  // Not possible '0b11111000'
    EXPECT_EQ(res.value, value);
    res = DepositField::discoverRotation(value2, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, value2);
}

TEST(depositfield, numTooLarge) {  // 0b00001000
    // N.B. other solutions are valid, these are the ones we expect.
    auto res = DepositField::discoverRotation(8, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 5U);
    EXPECT_EQ(res.value, 1);
    res = DepositField::discoverRotation(8, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 29U);
    EXPECT_EQ(res.value, 1);
    res = DepositField::discoverRotation(8, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 5U);
    EXPECT_EQ(res.value, 1);
}

TEST(depositfield, numTooSmall) {  // 0b11110111
    auto res = DepositField::discoverRotation(-9, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 5U);
    EXPECT_EQ(res.value, -2);
    res = DepositField::discoverRotation(-9, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 29U);
    EXPECT_EQ(res.value, -2);
    res = DepositField::discoverRotation(-9, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 5U);
    EXPECT_EQ(res.value, -2);
}

TEST(depositfield, 0b00110000) {
    auto res = DepositField::discoverRotation(0b00110000, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 4U);
    EXPECT_EQ(res.value, 0b00000011);
    res = DepositField::discoverRotation(0b00110000, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 28U);
    EXPECT_EQ(res.value, 0b00000011);
    res = DepositField::discoverRotation(0b00110000, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 4U);
    EXPECT_EQ(res.value, 0b00000011);
}

TEST(depositfield, 0b00100001) {
    // Failures are sent back with zero rotation and the value unchanged.
    auto res = DepositField::discoverRotation(0b00100001, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, 0b00100001);
    res = DepositField::discoverRotation(0b00100001, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, 0b00100001);
    res = DepositField::discoverRotation(0b00100001, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, 0b00100001);
}

TEST(depositfield, 0b01111111) {  // 127
    auto res = DepositField::discoverRotation(0b01111111, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 1U);
    EXPECT_EQ(res.value, -2);
    res = DepositField::discoverRotation(0b01111111, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 0U);
    EXPECT_EQ(res.value, 0b01111111);  // Can't do.
    res = DepositField::discoverRotation(0b01111111, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 1U);
    EXPECT_EQ(res.value, -2);
}

TEST(depositfield, 0b10011111) {  // -97
    auto res = DepositField::discoverRotation(-97, conSize8, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 3U);
    EXPECT_EQ(res.value, -4);
    res = DepositField::discoverRotation(-97, conSize32, tooLarge, tooSmall);
    EXPECT_EQ(res.rotate, 27U);
    EXPECT_EQ(res.value, -4);
    res = DepositField::discoverRotation(-97, conSize8, tooLarge, tooSmall2);
    EXPECT_EQ(res.rotate, 3U);
    EXPECT_EQ(res.value, -4);
}

}  // namespace
