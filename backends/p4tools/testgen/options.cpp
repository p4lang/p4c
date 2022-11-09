#include "backends/p4tools/testgen/options.h"

#include <cstdlib>
#include <iostream>
#include <string>

#include "lib/exceptions.h"
#include "p4tools/common/options.h"

#include "backends/p4tools/testgen/lib/logging.h"

namespace P4Tools {

TestgenOptions& TestgenOptions::get() {
    static TestgenOptions INSTANCE;
    return INSTANCE;
}

const char* TestgenOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for P4Testgen.");
}

TestgenOptions::TestgenOptions()
    : AbstractP4cToolOptions("Generate packet tests for a P4 program") {
    registerOption(
        "--strict", nullptr,
        [this](const char*) {
            strict = true;
            return true;
        },
        "Fail on unimplemented features instead of trying the next branch.");
    registerOption(
        "--input-packet-only", nullptr,
        [this](const char*) {
            inputPacketOnly = true;
            return true;
        },
        "Only produce the input packet for each test");

    registerOption(
        "--max-tests", "maxTests",
        [this](const char* arg) {
            maxTests = std::atoi(arg);
            return true;
        },
        "Sets the maximum number of tests to be generated");

    registerOption(
        "--pop-level", "popLevel",
        [this](const char* arg) {
            popLevel = std::atoi(arg);
            return true;
        },
        "Sets the fraction for multiPop exploration; default is 3 when meaningful strategy is "
        "activated.");

    registerOption(
        "--out-dir", "outputDir",
        [this](const char* arg) {
            outputDir = arg;
            return true;
        },
        "Directory for generated tests\n");

    registerOption(
        "--test-backend", "testBackend",
        [this](const char* arg) {
            testBackend = arg;
            testBackend = testBackend.toUpper();
            return true;
        },
        "Select the test back end. P4Testgen will produce tests that correspond to the input "
        "format of this test back end.");

    registerOption(
        "--packet-size", "packetSize",
        [this](const char* arg) {
            packetSize = std::atoi(arg);
            return true;
        },
        "If enabled, sets all input packets to a fixed size in bits (from 1 to 12000 bits). 0 "
        "implies no packet sizing.");

    registerOption(
        "--input-branches", "selectedBranches",
        [this](const char* arg) {
            selectedBranches = arg;
            // These options are mutually exclusive.
            if (trackBranches) {
                std::cerr << "--input-branches and --track-branches are mutually exclusive. Choose "
                             "one or the other."
                          << std::endl;
                exit(1);
            }
            return true;
        },
        "List of the selected branches which should be chosen for selection.");

    registerOption(
        "--track-branches", nullptr,
        [this](const char*) {
            trackBranches = true;
            // These options are mutually exclusive.
            if (!selectedBranches.empty()) {
                std::cerr << "--input-branches and --track-branches are mutually exclusive. Choose "
                             "one or the other."
                          << std::endl;
                exit(1);
            }
            return true;
        },
        "Track the branches that are chosen in the symbolic executor. This can be used for "
        "deterministic replay.");

    registerOption(
        "--exploration-strategy", "explorationStrategy",
        [this](const char* arg) {
            explorationStrategy = arg;
            return true;
        },
        "Selects a specific exploration strategy for test generation. Options are: "
        "randomAccessStack, linearEnumeration, maxCoverage. Defaults to incrementalStack.");

    registerOption(
        "--linear-enumeration", "linearEnumeration",
        [this](const char* arg) {
            linearEnumeration = std::atoi(arg);
            return true;
        },
        "Max bound for vector size in linearEnumeration; defaults to 2.");

    registerOption(
        "--saddle-point", "saddlePoint",
        [this](const char* arg) {
            saddlePoint = std::atoi(arg);
            return true;
        },
        "Threshold to invoke multiPop on randomAccessMaxCoverage.");

    registerOption(
        "--print-traces", nullptr,
        [](const char*) {
            P4Testgen::enableTraceLogging();
            return true;
        },
        "Print the associated traces and test information for each generated test.");

    registerOption(
        "--print-steps", nullptr,
        [](const char*) {
            P4Testgen::enableStepLogging();
            return true;
        },
        "Print the representation of each program node while the stepper steps through the "
        "program.");

    registerOption(
        "--print-coverage", nullptr,
        [](const char*) {
            P4Testgen::enableCoverageLogging();
            return true;
        },
        "Print detailed statement coverage statistics the interpreter collects while stepping "
        "through the program.");

    registerOption(
        "--print-performance-report", nullptr,
        [](const char*) {
            P4Testgen::enablePerformanceLogging();
            return true;
        },
        "Print timing report summary at the end of the program.");

    registerOption(
        "--dcg", "DCG",
        [this](const char*) {
            dcg = true;
            return true;
        },
        R"(Build a DCG for input graph. This control flow graph directed cyclic graph can be used
        for statement reachability analysis.)");

    registerOption(
        "--pattern", "pattern",
        [this](const char* arg) {
            pattern = arg;
            return true;
        },
        "List of the selected branches which should be chosen for selection.");
}

}  // namespace P4Tools
