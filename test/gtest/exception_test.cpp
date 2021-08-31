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

#include <unistd.h>
#include <exception>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace Util {

TEST(UtilException, Messages) {
    // Check that exception message formatting works as expected.
    try {
        throw CompilerBug("test");
    } catch (std::exception &ex) {
        cstring err(ex.what());
        cstring redir_msg("Compiler Bug:\ntest\n");
        cstring no_redir_msg = cstring(ANSI_RED) + "Compiler Bug" + ANSI_CLR +":\ntest\n";
        cstring opts[] = {redir_msg, no_redir_msg};
        // The error message might or might not be colorized based on if the test are redirected 
        // or not, to make sure both options are valid an array is used.
        EXPECT_THAT(opts, ::testing::Contains(err));
    }

    try {
        throw CompilationError("Testing error %1%", 1);
    } catch (std::exception &ex) {
        cstring err(ex.what());
        EXPECT_EQ(err, "Testing error 1\n");
    }
}

}  // namespace Util
