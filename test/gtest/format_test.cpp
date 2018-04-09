/*
Copyright 2013-present Barefoot Networks, Inc. 

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
#include "lib/error.h"
#include "lib/cstring.h"
#include "lib/stringify.h"

namespace Util {

TEST(Util, Format) {
    auto& context = BaseCompileContext::get();
    cstring message = context.errorReporter().format_message("%1%", 5u);
    EXPECT_EQ("5\n", message);

    message = context.errorReporter().format_message("Number=%1%", 5);
    EXPECT_EQ("Number=5\n", message);

    message = context.errorReporter().format_message("Double=%1% String=%2%", 2.3, "short");
    EXPECT_EQ("Double=2.3 String=short\n", message);

    struct NiceFormat {
        int a, b, c;

        cstring toString() const {
            return cstring("(") +
                    Util::toString(this->a) + "," +
                    Util::toString(this->b) + "," +
                    Util::toString(this->c) + ")";
        }
    };

    NiceFormat nf{1, 2, 3};
    message = context.errorReporter().format_message("Nice=%1%", nf);
    EXPECT_EQ("Nice=(1,2,3)\n", message);

    cstring x = "x";
    cstring y = "y";
    message = context.errorReporter().format_message("%1% %2%", x, y);
    EXPECT_EQ("x y\n", message);

    message = context.errorReporter().format_message("%1% %2%", x, 5);
    EXPECT_EQ("x 5\n", message);

    message = Util::printf_format("Number=%d, String=%s", 5, "short");
    EXPECT_EQ("Number=5, String=short", message);
}

}  // namespace Util
