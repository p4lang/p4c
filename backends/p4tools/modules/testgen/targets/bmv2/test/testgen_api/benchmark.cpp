#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "backends/p4tools/common/lib/logging.h"

#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace Test {

TEST(P4TestgenBenchmark, SuccessfullyGenerate1000Tests) {
    auto compilerOptions = CompilerOptions();
    compilerOptions.target = "bmv2";
    compilerOptions.arch = "v1model";
    auto fabricFile = std::filesystem::path(__FILE__).replace_filename(
        "../../../../../../../../testdata/p4_16_samples/fabric_20190420/fabric.p4");
    compilerOptions.file = fabricFile.string();
    auto &testgenOptions = P4Tools::P4Testgen::TestgenOptions::get();
    testgenOptions.testBackend = "PROTOBUF_IR";
    testgenOptions.testBaseName = "dummy";
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
}  // namespace Test
