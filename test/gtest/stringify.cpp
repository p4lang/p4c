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

TEST(stringify, simple) {
    cstring str = appendFormat("%s", "AAA");
    EXPECT_EQ("AAA", str);
}

TEST(stringify, overflow) {
    char test_str[129];
    for (int i = 0; i < 129; i++)
        test_str[0] = 'A';
    cstring str = appendFormat("%s", test_str);
    EXPECT_EQ(test_str, str);
}

TEST(stringify, empty) {
    cstring str = appendFormat("%s", "");
    EXPECT_EQ("", str);
}
}