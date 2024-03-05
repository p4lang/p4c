#include "backends/p4tools/modules/testgen/lib/test_framework.h"

#include "backends/p4tools/modules/testgen/lib/exceptions.h"

namespace P4Tools::P4Testgen {

TestFramework::TestFramework(const TestBackendConfiguration &testBackendConfiguration)
    : testBackendConfiguration(testBackendConfiguration) {}

const TestBackendConfiguration &TestFramework::getTestBackendConfiguration() const {
    return testBackendConfiguration.get();
}

bool TestFramework::isInFileMode() const {
    return getTestBackendConfiguration().fileBasePath.has_value();
}

AbstractTestReferenceOrError TestFramework::produceTest(const TestSpec * /*spec*/,
                                                        cstring /*selectedBranches*/,
                                                        size_t /*testIdx*/,
                                                        float /*currentCoverage*/) {
    TESTGEN_UNIMPLEMENTED("produceTest() not implemented for this test framework.");
}

}  // namespace P4Tools::P4Testgen
