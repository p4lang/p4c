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

#include "../../lib/exceptions.h"
#include "test.h"

using namespace Util;

namespace Test {
class TestException : public TestBase {
 public:
    int run() {
        try {
            throw CompilerBug("test");
        }
        catch (std::exception &ex) {
            cstring err(ex.what());
            cstring expected = cstring(ANSI_RED) + "Compiler Bug" + ANSI_CLR +":\ntest\n";
            ASSERT_EQ(err, expected);
        }

        try {
            throw CompilationError("Testing error %1%", 1);
        }
        catch (std::exception &ex) {
            cstring err(ex.what());
            ASSERT_EQ(err, "Testing error 1\n");
        }

        return SUCCESS;
    }
};
}  // namespace Test

int main(int /* argc*/, char* /*argv*/[]) {
    Test::TestException test;
    return test.run();
}
