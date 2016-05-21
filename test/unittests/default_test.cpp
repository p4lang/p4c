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

#include "../../lib/default.h"
#include "test.h"

using namespace Util;

namespace Test {
class A {
 public:
    int x;
    A() : x(5) {}
};

class TestDefault : public TestBase {
 public:
    int run() {
        char* t = Default<char*>();
        ASSERT_EQ(t, (char*)nullptr);

        A a = Default<A>();
        ASSERT_EQ(a.x, 5);

        return SUCCESS;
    }
};
}  // namespace Test

int main(int, char* []) {
    Test::TestDefault test;
    return test.run();
}


