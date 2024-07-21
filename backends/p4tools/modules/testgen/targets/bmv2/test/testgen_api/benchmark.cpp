#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "backends/p4tools/common/lib/logging.h"
#include "test/gtest/helpers.h"

#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace p4c::Test {

using namespace p4c::P4::literals;

TEST(P4TestgenBenchmark, SuccessfullyGenerate1000Tests) {
    auto compilerOptions = P4CContextWithOptions<CompilerOptions>::get().options();
    compilerOptions.target = "bmv2"_cs;
    compilerOptions.arch = "v1model"_cs;
    auto includePath = P4CTestEnvironment::getProjectRoot() / "p4include";
    compilerOptions.preprocessor_options = "-I" + includePath.string();
    auto fabricFile =
        P4CTestEnvironment::getProjectRoot() / "testdata/p4_16_samples/fabric_20190420/fabric.p4";
    compilerOptions.file = fabricFile.string();
    auto &testgenOptions = P4Tools::P4Testgen::TestgenOptions::get();
    testgenOptions.testBackend = "PROTOBUF_IR"_cs;
    testgenOptions.testBaseName = "dummy"_cs;
    testgenOptions.seed = 1;
    // Fix the packet size.
    testgenOptions.minPktSize = 512;
    testgenOptions.maxPktSize = 512;
    // Select a random path for each test.
    testgenOptions.pathSelectionPolicy = P4Tools::P4Testgen::PathSelectionPolicy::RandomBacktrack;
    // Generate 2000 tests.
    testgenOptions.maxTests = 2000;
    // This enables performance printing.
    P4Tools::enablePerformanceLogging();

    P4Tools::P4Testgen::Testgen::generateTests(compilerOptions, testgenOptions);

    // Print the report.
    P4Tools::printPerformanceReport();
}
}  // namespace p4c::Test
