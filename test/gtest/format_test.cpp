// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/stringify.h"

namespace P4::Util {

TEST(Util, Format) {
    auto &context = BaseCompileContext::get();
    cstring message = context.errorReporter().format_message("%1%", 5u);
    EXPECT_EQ("5\n", message);

    message = context.errorReporter().format_message("Number=%1%", 5);
    EXPECT_EQ("Number=5\n", message);

    message = context.errorReporter().format_message("Double=%1% String=%2%", 2.3, "short");
    EXPECT_EQ("Double=2.3 String=short\n", message);

    struct NiceFormat {
        int a, b, c;

        cstring toString() const {
            return "("_cs + Util::toString(this->a) + ","_cs + Util::toString(this->b) + ","_cs +
                   Util::toString(this->c) + ")"_cs;
        }
    };

    NiceFormat nf{1, 2, 3};
    message = context.errorReporter().format_message("Nice=%1%", nf);
    EXPECT_EQ("Nice=(1,2,3)\n", message);

    cstring x = "x"_cs;
    cstring y = "y"_cs;
    message = context.errorReporter().format_message("%1% %2%", x, y);
    EXPECT_EQ("x y\n", message);

    message = context.errorReporter().format_message("%1% %2%", x, 5);
    EXPECT_EQ("x 5\n", message);
}

}  // namespace P4::Util
