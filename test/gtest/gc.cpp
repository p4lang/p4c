// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>

#ifdef __GLIBC__
#include <malloc.h>

namespace P4::Test {

TEST(Allocator, UsableSize) {
    EXPECT_EQ(malloc_usable_size(nullptr), 0U);
    for (size_t size : {1U, 16U, 31U, 4096U, 65536U}) {
        void *ptr = malloc(size);
        ASSERT_NE(ptr, nullptr);
        EXPECT_GE(malloc_usable_size(ptr), size);
        memset(ptr, 0x5a, size);
        void *larger = realloc(ptr, size * 2);
        ASSERT_NE(larger, nullptr);
        EXPECT_GE(malloc_usable_size(larger), size * 2);
        for (size_t idx = 0; idx < size; ++idx) {
            EXPECT_EQ(static_cast<unsigned char *>(larger)[idx], 0x5a);
        }
        free(larger);
    }
}

TEST(Allocator, AlignedUsableSize) {
    for (size_t alignment : {16U, 64U, 4096U}) {
        void *ptr = nullptr;
        ASSERT_EQ(posix_memalign(&ptr, alignment, 37), 0);
        ASSERT_NE(ptr, nullptr);
        const auto size = malloc_usable_size(ptr);
        ASSERT_GE(size, 37U);
        // The entire reported range must be usable, including padding.
        memset(ptr, 0x5a, size);
        free(ptr);
    }
}

}  // namespace P4::Test
#endif
