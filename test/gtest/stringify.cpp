/*
Copyright 2018-present VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <cstdarg>
#include "gtest/gtest.h"
#include "lib/stringify.h"
#include "lib/cstring.h"

namespace Test {
cstring appendFormat(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    cstring str = Util::vprintf_format(format, ap);
    va_end(ap);
    return str;
}

TEST(stringify, vprintf_simple) {
    cstring str = appendFormat("%s", "AAA");
    EXPECT_EQ(str, "AAA");
}

TEST(stringify, vprintf_overflow) {
    std::string c(129, 'A');
    cstring test_str = c;
    cstring str = appendFormat("%s", test_str);
    EXPECT_EQ(str.c_str(), test_str.c_str());
    EXPECT_EQ(str.size(), test_str.size());
}

TEST(stringify, vprintf_empty) {
    cstring str = appendFormat("%s", "");
    EXPECT_EQ(str, "");
}

TEST(stringify, printf_simple) {
    cstring str = Util::printf_format("%s", "AAA");
    EXPECT_EQ(str, "AAA");
}

TEST(stringify, printf_overflow) {
    std::string c(129, 'A');
    cstring test_str = c;
    cstring str = Util::printf_format("%s", test_str);
    EXPECT_EQ(str.c_str(), test_str.c_str());
    EXPECT_EQ(str.size(), test_str.size());
}

TEST(stringify, printf_empty) {
    cstring str = Util::printf_format("%s", "");
    EXPECT_EQ(str, "");
}

}  // namespace Test
