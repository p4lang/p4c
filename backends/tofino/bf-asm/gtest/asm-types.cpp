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

#include "backends/tofino/bf-asm/asm-types.h"

#include <gtest/gtest.h>

namespace {

auto CaptureStderr = ::testing::internal::CaptureStderr;
auto Stderr = ::testing::internal::GetCapturedStderr;
auto terminate = ::testing::KilledBySignal(SIGABRT);

TEST(asm_types, get_int64_0) {
    uint32_t i = 0;
    value_t v{tINT, 0, 0};
    v.i = i;
    CaptureStderr();
    EXPECT_EQ(get_int64(v), i);
    EXPECT_EQ(get_int64(v, 0), i);
    EXPECT_EQ(get_int64(v, 0, "no error check"), i);
    EXPECT_EQ(get_int64(v, 1), i);
    EXPECT_EQ(get_int64(v, 1, "no error"), i);
    EXPECT_EQ(get_int64(v, 64), i);
    EXPECT_EQ(get_int64(v, 64, "no error"), i);
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    // Slow tests...
    EXPECT_EXIT(get_int64(v, 128), terminate, "Assembler BUG");
    EXPECT_EXIT(get_int64(v, 128, "terminates"), terminate, "Assembler BUG");
}

TEST(asm_types, get_int64_32bit) {
    uint32_t i = 0xAAAAAAAA;
    value_t v{tINT, 0, 0};
    v.i = i;
    CaptureStderr();
    EXPECT_EQ(get_int64(v), i);
    EXPECT_EQ(get_int64(v, 0), i);
    EXPECT_EQ(get_int64(v, 0, "no error check"), i);
    EXPECT_EQ(get_int64(v, 32), i);
    EXPECT_EQ(get_int64(v, 32, "no error"), i);
    EXPECT_EQ(get_int64(v, 16), 0xAAAA);
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_int64(v, 16, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

TEST(asm_types, get_int64_64bit) {
    uint64_t i = 0xAAAAAAAAAAAAAAAA;
    value_t v{tINT, 0, 0};
    v.i = i;
    CaptureStderr();
    EXPECT_EQ(get_int64(v), i);
    EXPECT_EQ(get_int64(v, 0), i);
    EXPECT_EQ(get_int64(v, 0, "no error check"), i);
    EXPECT_EQ(get_int64(v, 64), i);
    EXPECT_EQ(get_int64(v, 64, "no error"), i);
    EXPECT_EQ(get_int64(v, 48), 0xAAAAAAAAAAAA);
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_int64(v, 48, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

TEST(asm_types, get_bigi_empty) {
    value_t v{tBIGINT, 0, 0};
    v.bigi = EMPTY_VECTOR_INIT;
    EXPECT_EQ(get_int64(v), 0);
    EXPECT_EQ(get_bitvec(v), bitvec());
}

TEST(asm_types, get_int64_bigi_0) {
    uint32_t i = 0;
    value_t v{tBIGINT, 0, 0};
    VECTOR_init1(v.bigi, i);
    CaptureStderr();
    EXPECT_EQ(get_int64(v), i);
    EXPECT_EQ(get_int64(v, 0), i);
    EXPECT_EQ(get_int64(v, 0, "no error check"), i);
    EXPECT_EQ(get_int64(v, 1), i);
    EXPECT_EQ(get_int64(v, 1, "no error"), i);
    EXPECT_EQ(get_int64(v, 64), i);
    EXPECT_EQ(get_int64(v, 64, "no error"), i);
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    // Slow tests...
    EXPECT_EXIT(get_int64(v, 128), terminate, "Assembler BUG");
    EXPECT_EXIT(get_int64(v, 128, "terminates"), terminate, "Assembler BUG");
}

TEST(asm_types, get_int64_bigi_32bit) {
    uint32_t i = 0xAAAAAAAA;
    value_t v{tBIGINT, 0, 0};
    VECTOR_init1(v.bigi, i);
    CaptureStderr();
    EXPECT_EQ(get_int64(v), i);
    EXPECT_EQ(get_int64(v, 0), i);
    EXPECT_EQ(get_int64(v, 0, "no error check"), i);
    EXPECT_EQ(get_int64(v, 32), i);
    EXPECT_EQ(get_int64(v, 32, "no error"), i);
    EXPECT_EQ(get_int64(v, 16), 0xAAAA);
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_int64(v, 16, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

TEST(asm_types, get_int64_bigi_64bit) {
    uint64_t i = 0xAAAAAAAAAAAAAAAA;
    value_t v{tBIGINT, 0, 0};
    if (sizeof(uintptr_t) == sizeof(uint32_t))
        VECTOR_init2(v.bigi, 0xAAAAAAAA, 0xAAAAAAAA);
    else
        VECTOR_init1(v.bigi, i);
    CaptureStderr();
    EXPECT_EQ(get_int64(v), i);
    EXPECT_EQ(get_int64(v, 0), i);
    EXPECT_EQ(get_int64(v, 0, "no error check"), i);
    EXPECT_EQ(get_int64(v, 64), i);
    EXPECT_EQ(get_int64(v, 64, "no error"), i);
    EXPECT_EQ(get_int64(v, 48), 0xAAAAAAAAAAAA);
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_int64(v, 48, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

TEST(asm_types, get_bitvec_0) {
    value_t v{tINT, 0, 0};
    v.i = 0;
    auto i = bitvec(0);
    CaptureStderr();
    EXPECT_EQ(get_bitvec(v), i);
    EXPECT_EQ(get_bitvec(v, 0), i);
    EXPECT_EQ(get_bitvec(v, 0, "no error check"), i);
    EXPECT_EQ(get_bitvec(v, 1), i);
    EXPECT_EQ(get_bitvec(v, 1, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 64), i);
    EXPECT_EQ(get_bitvec(v, 64, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 128), i);
    EXPECT_EQ(get_bitvec(v, 128, "no error"), i);
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
}

TEST(asm_types, get_bitvec_32bit) {
    value_t v{tINT, 0, 0};
    v.i = 0xAAAAAAAA;
    auto i = bitvec(0xAAAAAAAA);
    CaptureStderr();
    EXPECT_EQ(get_bitvec(v), i);
    EXPECT_EQ(get_bitvec(v, 0), i);
    EXPECT_EQ(get_bitvec(v, 0, "no error check"), i);
    EXPECT_EQ(get_bitvec(v, 32), i);
    EXPECT_EQ(get_bitvec(v, 32, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 16), bitvec(0xAAAA));
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_bitvec(v, 16, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

TEST(asm_types, get_bitvec_64bit) {
    value_t v{tINT, 0, 0};
    v.i = 0xAAAAAAAAAAAAAAAA;
    auto i = bitvec(0xAAAAAAAAAAAAAAAA);
    CaptureStderr();
    EXPECT_EQ(get_bitvec(v), i);
    EXPECT_EQ(get_bitvec(v, 0), i);
    EXPECT_EQ(get_bitvec(v, 0, "no error check"), i);
    EXPECT_EQ(get_bitvec(v, 64), i);
    EXPECT_EQ(get_bitvec(v, 64, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 48), bitvec(0xAAAAAAAAAAAA));
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_bitvec(v, 48, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

TEST(asm_types, get_bitvec_bigi_0) {
    value_t v{tBIGINT, 0, 0};
    VECTOR_init1(v.bigi, 0);
    auto i = bitvec(0);
    CaptureStderr();
    EXPECT_EQ(get_bitvec(v), i);
    EXPECT_EQ(get_bitvec(v, 0), i);
    EXPECT_EQ(get_bitvec(v, 0, "no error check"), i);
    EXPECT_EQ(get_bitvec(v, 1), i);
    EXPECT_EQ(get_bitvec(v, 1, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 64), i);
    EXPECT_EQ(get_bitvec(v, 64, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 128), i);
    EXPECT_EQ(get_bitvec(v, 128, "no error"), i);
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
}

TEST(asm_types, get_bitvec_bigi_32bit) {
    value_t v{tBIGINT, 0, 0};
    VECTOR_init1(v.bigi, 0xAAAAAAAA);
    auto i = bitvec(0xAAAAAAAA);
    CaptureStderr();
    EXPECT_EQ(get_bitvec(v), i);
    EXPECT_EQ(get_bitvec(v, 0), i);
    EXPECT_EQ(get_bitvec(v, 0, "no error check"), i);
    EXPECT_EQ(get_bitvec(v, 32), i);
    EXPECT_EQ(get_bitvec(v, 32, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 16), bitvec(0xAAAA));
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_bitvec(v, 16, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

TEST(asm_types, get_bitvec_bigi_64bit) {
    value_t v{tBIGINT, 0, 0};
    if (sizeof(uintptr_t) == sizeof(uint32_t))
        VECTOR_init2(v.bigi, 0xAAAAAAAA, 0xAAAAAAAA);
    else
        VECTOR_init1(v.bigi, 0xAAAAAAAAAAAAAAAA);
    auto i = bitvec(0xAAAAAAAAAAAAAAAA);
    CaptureStderr();
    EXPECT_EQ(get_bitvec(v), i);
    EXPECT_EQ(get_bitvec(v, 0), i);
    EXPECT_EQ(get_bitvec(v, 0, "no error check"), i);
    EXPECT_EQ(get_bitvec(v, 64), i);
    EXPECT_EQ(get_bitvec(v, 64, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 48), bitvec(0xAAAAAAAAAAAA));
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_bitvec(v, 48, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

TEST(asm_types, get_bitvec_bigi_128bit) {
    value_t v{tBIGINT, 0, 0};
    if (sizeof(uintptr_t) == sizeof(uint32_t))
        VECTOR_init4(v.bigi, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA);
    else
        VECTOR_init2(v.bigi, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA);
    bitvec i;
    for (int j = 0; j < 4; ++j) i.putrange(j * 32, 32, 0xAAAAAAAA);
    CaptureStderr();
    EXPECT_EQ(get_bitvec(v), i);
    EXPECT_EQ(get_bitvec(v, 0), i);
    EXPECT_EQ(get_bitvec(v, 0, "no error check"), i);
    EXPECT_EQ(get_bitvec(v, 128), i);
    EXPECT_EQ(get_bitvec(v, 128, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 192), i);
    EXPECT_EQ(get_bitvec(v, 192, "no error"), i);
    EXPECT_EQ(get_bitvec(v, 48), bitvec(0xAAAAAAAAAAAA));
    EXPECT_TRUE(Stderr().find("error") == std::string::npos);
    CaptureStderr();
    get_bitvec(v, 48, "my error");
    EXPECT_TRUE(Stderr().find("error: my error") != std::string::npos);
}

}  // namespace
