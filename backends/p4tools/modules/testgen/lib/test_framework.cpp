#include "backends/p4tools/modules/testgen/lib/test_framework.h"

namespace P4Tools::P4Testgen {

TestFramework::TestFramework(const TestBackendConfiguration &testBackendConfiguration)
    : testBackendConfiguration(testBackendConfiguration) {}

const TestBackendConfiguration &TestFramework::getTestBackendConfiguration() const {
    return testBackendConfiguration.get();
}
}  // namespace P4Tools::P4Testgen
