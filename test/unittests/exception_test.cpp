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
            ASSERT_EQ(err, "COMPILER BUG:\ntest\n");
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
