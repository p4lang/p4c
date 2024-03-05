#include "backends/p4tools/modules/testgen/options.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/options.h"
#include "lib/error.h"

#include "backends/p4tools/modules/testgen/lib/logging.h"

namespace P4Tools::P4Testgen {

TestgenOptions &TestgenOptions::get() {
    static TestgenOptions INSTANCE;
    return INSTANCE;
}

const char *TestgenOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for P4Testgen.");
}

const std::set<cstring> TestgenOptions::SUPPORTED_STOP_METRICS = {"MAX_NODE_COVERAGE"};

TestgenOptions::TestgenOptions()
    : AbstractP4cToolOptions("Generate packet tests for a P4 program.") {
    registerOption(
        "--strict", nullptr,
        [this](const char *) {
            strict = true;
            return true;
        },
        "Fail on unimplemented features instead of trying the next branch.");

    registerOption(
        "--max-tests", "maxTests",
        [this](const char *arg) {
            try {
                // Unfortunately, we can not use std::stoul because negative inputs are okay
                // according to the C++ standard.
                maxTests = std::stoll(arg);
                if (maxTests < 0) {
                    throw std::invalid_argument("Invalid input.");
                }
            } catch (std::invalid_argument &) {
                ::error("Invalid input value %1% for --max-tests. Expected positive integer.", arg);
                return false;
            }
            return true;
        },
        "Sets the maximum number of tests to be generated [default: 1]. Setting the value to 0 "
        "will generate tests until no more paths can be found.");

    registerOption(
        "--stop-metric", "stopMetric",
        [this](const char *arg) {
            stopMetric = cstring(arg).toUpper();
            if (SUPPORTED_STOP_METRICS.count(stopMetric) == 0) {
                ::error(
                    "Stop metric %1% not supported. Supported stop metrics are "
                    "%2%.",
                    stopMetric, Utils::containerToString(SUPPORTED_STOP_METRICS));
                return false;
            }
            return true;
        },
        "Stops generating tests when a particular metric is satisifed. Currently supported options "
        "are:\n\"MAX_NODE_COVERAGE\".");

    registerOption(
        "--packet-size-range", "packetSizeRange",
        [this](const char *arg) {
            auto rangeStr = std::string(arg);
            size_t packetLenStr = rangeStr.find_first_of(':');
            try {
                auto minPacketLenStr = rangeStr.substr(0, packetLenStr);
                minPktSize = std::stoi(minPacketLenStr);
                if (minPktSize < 0) {
                    ::error(
                        "Invalid minimum packet size %1%. Minimum packet size must be at least 0.",
                        minPktSize);
                }
                auto maxPacketLenStr = rangeStr.substr(packetLenStr + 1);
                maxPktSize = std::stoi(maxPacketLenStr);
                if (maxPktSize < minPktSize) {
                    ::error(
                        "Invalid packet size range %1%:%2%.  The maximum packet size must be at "
                        "least the size of the minimum packet size.",
                        minPktSize, maxPktSize);
                }
            } catch (std::invalid_argument &) {
                ::error(
                    "Invalid packet size range %1%. Expected format is [min]:[max], where [min] "
                    "and [max] are integers.",
                    arg);
                return false;
            }
            return true;
        },
        "Specify the possible range of the input packet size in bits. The format is [min]:[max] "
        "inclusive. "
        "The default values are \"0:72000\". The maximum is set to jumbo frame size (9000 bytes).");

    registerOption(
        "--port-ranges", "portRanges",
        [this](const char *arg) {
            // Convert the input into a StringStream and split by comma (',').
            // Each element is then again split by colon (':').
            std::stringstream argStream(arg);
            while (argStream.good()) {
                std::string rangeStr;
                std::getline(argStream, rangeStr, ',');
                size_t portStr = rangeStr.find_first_of(':');
                try {
                    auto loPortStr = rangeStr.substr(0, portStr);
                    auto loPortRange = std::stoi(loPortStr);
                    if (loPortRange < 0) {
                        ::error(
                            "Invalid low port value %1%. low port value must be at "
                            "least "
                            "0.",
                            loPortRange);
                    }
                    auto hiPortStr = rangeStr.substr(portStr + 1);
                    auto hiPortRange = std::stoi(hiPortStr);
                    if (hiPortRange < loPortRange) {
                        ::error(
                            "Invalid permitted port range %1%:%2%.  The high port value must "
                            "be "
                            "at "
                            "least the size of the low port value.",
                            loPortRange, hiPortRange);
                    }
                    permittedPortRanges.emplace_back(loPortRange, hiPortRange);
                } catch (std::invalid_argument &) {
                    ::error(
                        "Invalid permitted port range %1%. Expected format is [lo]:[hi], where "
                        "[lo] "
                        "and [hi] are integers.",
                        arg);
                    return false;
                }
            }
            return true;
        },
        "Specify the possible input/output ports. The format is [lo]:[hi],[lo]:[hi],... inclusive. "
        "Ranges can overlap, but lo must always be below hi. If the value is too large for the "
        "target-specific port variable, it will overflow. Default behavior is delegated to the "
        "test back end. Some test back ends may restrict the available port ranges.");

    registerOption(
        "--skip-control-plane-entities", "skippedControlPlaneEntities",
        [this](const char *arg) {
            // Convert the input into a StringStream and split by comma (',').
            std::stringstream argStream(arg);
            while (argStream.good()) {
                std::string substr;
                std::getline(argStream, substr, ',');
                skippedControlPlaneEntities.emplace(substr);
            }
            return true;
        },
        "Specify the set of control plane entities for which P4Testgen should not generate a "
        "configuration for. For example, if a particular table is in the set, P4Testgen will "
        "assume that only the default action or constant entries can be executed. ");

    registerOption(
        "--out-dir", "outputDir",
        [this](const char *arg) {
            outputDir = arg;
            return true;
        },
        "The output directory for the generated tests. The directory will be created, if it does "
        "not exist.");

    registerOption(
        "--test-backend", "testBackend",
        [this](const char *arg) {
            testBackend = arg;
            testBackend = testBackend.toUpper();
            return true;
        },
        "Select the test back end. P4Testgen will produce tests that correspond to the input "
        "format of this test back end.");

    registerOption(
        "--input-branches", "selectedBranches",
        [this](const char *arg) {
            selectedBranches = arg;
            // These options are mutually exclusive.
            if (trackBranches) {
                ::error(
                    "--input-branches and --track-branches are mutually exclusive. Choose "
                    "one or the other.");
                return false;
            }
            return true;
        },
        "[EXPERIMENTAL] List of the selected branches which should be chosen for selection.");

    registerOption(
        "--track-branches", nullptr,
        [this](const char *) {
            trackBranches = true;
            // These options are mutually exclusive.
            if (!selectedBranches.empty()) {
                ::error(
                    "--input-branches and --track-branches are mutually exclusive. Choose "
                    "one or the other.");
                return false;
            }
            return true;
        },
        "[EXPERIMENTAL] Track the branches that are chosen in the symbolic executor. This can be "
        "used for deterministic replay.");

    registerOption(
        "--output-packet-only", nullptr,
        [this](const char *) {
            outputPacketOnly = true;
            if (!selectedBranches.empty()) {
                ::error(
                    "--input-branches cannot guarantee --output-packet-only."
                    " Aborting.");
                return false;
            }
            return true;
        },
        "Produced tests must have an output packet as outcome.");

    registerOption(
        "--dropped-packet-only", nullptr,
        [this](const char *) {
            droppedPacketOnly = true;
            if (!selectedBranches.empty()) {
                ::error(
                    "--input-branches cannot guarantee --dropped-packet-only."
                    " Aborting.");
                return false;
            }
            return true;
        },
        "Produced tests must have a dropped packet as outcome.");

    registerOption(
        "--path-selection", "pathSelectionPolicy",
        [this](const char *arg) {
            using P4Testgen::PathSelectionPolicy;

            static std::map<cstring, PathSelectionPolicy> const PATH_SELECTION_OPTIONS = {
                {"DEPTH_FIRST", PathSelectionPolicy::DepthFirst},
                {"RANDOM_BACKTRACK", PathSelectionPolicy::RandomBacktrack},
                {"GREEDY_STATEMENT_SEARCH", PathSelectionPolicy::GreedyStmtCoverage},
            };
            auto selectionString = cstring(arg).toUpper();
            auto it = PATH_SELECTION_OPTIONS.find(selectionString);
            if (it != PATH_SELECTION_OPTIONS.end()) {
                pathSelectionPolicy = it->second;
                return true;
            }
            std::set<cstring> printSet;
            std::transform(PATH_SELECTION_OPTIONS.cbegin(), PATH_SELECTION_OPTIONS.cend(),
                           std::inserter(printSet, printSet.begin()),
                           [](const std::pair<cstring, PathSelectionPolicy> &mapTuple) {
                               return mapTuple.first;
                           });
            ::error(
                "Path selection policy %1% not supported. Supported path selection policies are "
                "%2%.",
                selectionString, Utils::containerToString(printSet));
            return false;
        },
        "Selects a specific path selection strategy for test generation. Options are: "
        "DEPTH_FIRST, RANDOM_BACKTRACK, and GREEDY_STATEMENT_SEARCH. "
        "Defaults to DEPTH_FIRST.");

    registerOption(
        "--track-coverage", "coverageItem",
        [this](const char *arg) {
            static std::set<cstring> const COVERAGE_OPTIONS = {"STATEMENTS", "TABLE_ENTRIES",
                                                               "ACTIONS"};
            hasCoverageTracking = true;
            auto selectionString = cstring(arg).toUpper();
            auto it = COVERAGE_OPTIONS.find(selectionString);
            if (it != COVERAGE_OPTIONS.end()) {
                if (selectionString == "STATEMENTS") {
                    coverageOptions.coverStatements = true;
                    return true;
                }
                if (selectionString == "TABLE_ENTRIES") {
                    coverageOptions.coverTableEntries = true;
                    return true;
                }
                if (selectionString == "ACTIONS") {
                    coverageOptions.coverActions = true;
                    return true;
                }
            }
            ::error(
                "Coverage tracking for label %1% not supported. Supported coverage tracking "
                "options are "
                "%2%.",
                selectionString, Utils::containerToString(COVERAGE_OPTIONS));
            return false;
        },
        "Specifies, which IR nodes to track for coverage in the targeted P4 program. Multiple "
        "options are possible: Currently supported: STATEMENTS, TABLE_ENTRIES (table rules encoded "
        "in the table entries in P4), ACTIONS (actions invoked, directly or by tables). "
        "Defaults to no coverage.");

    registerOption(
        "--only-covering-tests", nullptr,
        [this](const char *) {
            coverageOptions.onlyCoveringTests = true;
            return true;
        },
        "If coverage tracking is enabled only generate tests which update the total number of "
        "covered nodes.");

    registerOption(
        "--assert-min-coverage", "minCoverage",
        [this](const char *arg) {
            try {
                minCoverage = std::stof(arg);
                if (minCoverage < 0 || minCoverage > 1) {
                    throw std::invalid_argument("Invalid input.");
                }
            } catch (std::invalid_argument &) {
                ::error(
                    "Invalid input value %1% for --assert-min-coverage. "
                    "Expected float in range [0, 1].",
                    arg);
                return false;
            }
            return true;
        },
        "Specifies minimum coverage that needs to be achieved for P4Testgen to exit successfully. "
        "The input needs to be value in range [0, 1] (where 1 means the metric is fully covered). "
        "Defaults to 0 which means no checking.");

    registerOption(
        "--print-traces", nullptr,
        [](const char *) {
            P4Testgen::enableTraceLogging();
            return true;
        },
        "Print the associated traces and test information for each generated test.");

    registerOption(
        "--print-steps", nullptr,
        [](const char *) {
            P4Testgen::enableStepLogging();
            return true;
        },
        "Print the representation of each program node while the stepper steps through the "
        "program.");

    registerOption(
        "--print-coverage", nullptr,
        [](const char *) {
            P4Testgen::enableCoverageLogging();
            return true;
        },
        "Print detailed statement coverage statistics the interpreter collects while stepping "
        "through the program.");

    registerOption(
        "--print-performance-report", nullptr,
        [](const char *) {
            enablePerformanceLogging();
            return true;
        },
        "Print timing report summary at the end of the program.");

    registerOption(
        "--dcg", "DCG",
        [this](const char *) {
            dcg = true;
            return true;
        },
        "[EXPERIMENTAL] Build a DCG for input graph. This control flow graph directed cyclic graph "
        "can be used for statement reachability analysis.");

    registerOption(
        "--pattern", "pattern",
        [this](const char *arg) {
            pattern = arg;
            return true;
        },
        "List of the selected branches which should be chosen for selection.");

    registerOption(
        "--test-name", "testBaseName",
        [this](const char *arg) {
            testBaseName = arg;
            return true;
        },
        "The base name of the tests which are generated.");

    registerOption(
        "--disable-assumption-mode", nullptr,
        [this](const char * /*arg*/) {
            enforceAssumptions = false;
            return true;
        },
        "Do not apply the conditions defined within \"testgen_assume\" extern calls in P4 programs."
        "They will have no effect on P4Testgen's path exploration.");

    registerOption(
        "--assertion-mode", nullptr,
        [this](const char * /*arg*/) {
            assertionModeEnabled = true;
            return true;
        },
        "Produce only tests that violate the condition defined in assert calls. This will either "
        "produce no tests or only tests that contain counter examples.");
}

bool TestgenOptions::validateOptions() const {
    if (minCoverage > 0 && !hasCoverageTracking) {
        ::error(
            ErrorType::ERR_INVALID,
            "It is not allowed to have --assert-min-coverage set to non-zero without a coverage "
            "tracking enabled with --track-coverage option. Without coverage tracking, the "
            "--assert-min-coverage is meaningless.");
        return false;
    }
    return true;
}

}  // namespace P4Tools::P4Testgen
