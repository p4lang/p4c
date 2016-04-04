#include "test.h"

int TestBase::isSuccess(TestBase::TestResult result) {
    return result == TestBase::TestResult::Success;
}
