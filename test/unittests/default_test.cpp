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


