#include "backends/p4tools/testgen/options.h"

#include <cstdlib>
#include <iostream>
#include <string>

#include "lib/exceptions.h"

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
        "--permissive", nullptr,
        [this](const char*) {
            permissive = true;
            return true;
        },
        "Do not fail on unimplemented features. Instead, try the next branch.");
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
        "Sets the fraction for multiPop exploration; default is 0 for a single pop.");

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
        "--linear-enumeration", "linearEnumeration",
        [this](const char* arg) {
            linearEnumeration = std::atoi(arg);
            return true;
        },
        "Performs linear exploration of the program paths; expects max bound for vector size.");

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
}

}  // namespace P4Tools
