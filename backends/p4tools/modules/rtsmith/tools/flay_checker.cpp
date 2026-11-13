#include <cstdlib>
#include <random>

#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/flay/flay.h"
#include "backends/p4tools/modules/flay/register.h"
#include "backends/p4tools/modules/rtsmith/core/util.h"
#include "backends/p4tools/modules/rtsmith/options.h"
#include "backends/p4tools/modules/rtsmith/register.h"
#include "backends/p4tools/modules/rtsmith/rtsmith.h"
#include "frontends/common/parser_options.h"
#include "lib/compile_context.h"
#include "lib/error.h"
#include "lib/options.h"

namespace P4::P4Tools::RtSmith {

namespace {

class FlayCheckerOptions : public RtSmithOptions {
    /// Write a performance report.
    bool _writePerformanceReport = false;
    /// Skip parser processing in Flay.
    bool _skipParsers = false;

 public:
    FlayCheckerOptions() {
        registerOption(
            "--file", "inputFile",
            [this](const char *arg) {
                file = arg;
                if (!std::filesystem::exists(file)) {
                    error("The input P4 program '%s' does not exist.", file.c_str());
                    return false;
                }
                return true;
            },
            "The input file to process.");
        registerOption(
            "--enable-info-logging", nullptr,
            [](const char *) {
                enableInformationLogging();
                return true;
            },
            "Print verbose messages.");
        registerOption(
            "--write-performance-report", nullptr,
            [this](const char *) {
                enablePerformanceLogging();
                _writePerformanceReport = true;
                return true;
            },
            "Write a performance report for the file. The report will be written to either the "
            "location of the reference file or the location of the folder.");
        registerOption(
            "--skip-parsers", nullptr,
            [this](const char *) {
                _skipParsers = true;
                return true;
            },
            "Skip parser processing in Flay.");
    }

    ~FlayCheckerOptions() override = default;

    // Process options; return list of remaining options.
    // Returns EXIT_FAILURE if an error occurred.
    int processOptions(int argc, char *const argv[]) {
        auto *unprocessedOptions = process(argc, argv);
        if (unprocessedOptions == nullptr) {
            return EXIT_FAILURE;
        }
        if (unprocessedOptions != nullptr && !unprocessedOptions->empty()) {
            for (const auto &option : *unprocessedOptions) {
                error("Unprocessed input: %s", option);
            }
            return EXIT_FAILURE;
        }
        if (file.empty()) {
            error("No input file specified.");
            return EXIT_FAILURE;
        }
        if (_outputDir.empty()) {
            _outputDir = std::tmpnam(nullptr);
            printInfo("Using temporary directory: %s", _outputDir.c_str());
        }
        // If the environment variable P4FLAY_INFO or RTSMITH_INFO is set, enable information
        // logging.
        if (std::getenv("P4FLAY_INFO") != nullptr || std::getenv("RTSMITH_INFO") != nullptr) {
            enableInformationLogging();
        }
        return EXIT_SUCCESS;
    }

    [[nodiscard]] const std::filesystem::path &getInputFile() const { return file; }

    [[nodiscard]] const RtSmithOptions &toRtSmithOptions() const { return *this; }

    [[nodiscard]] bool writePerformanceReport() const { return _writePerformanceReport; }

    [[nodiscard]] bool skipParsers() const { return _skipParsers; }
};

}  // namespace

int run(const FlayCheckerOptions &options, const RtSmithOptions &rtSmithOptions) {
    printInfo("Generating RtSmith configuration for program...");
    if (!RtSmith::generateConfig(rtSmithOptions)) {
        return EXIT_FAILURE;
    }

    printInfo("RtSmith configuration complete.");
    printInfo("Starting Flay optimization...");
    {
        auto *flayContext = new CompileContext<Flay::FlayOptions>();
        AutoCompileContext autoContext(flayContext);
        Flay::FlayOptions &flayOptions = flayContext->options();
        flayOptions.target = options.target;
        flayOptions.arch = options.arch;
        flayOptions.file = options.file;
        if (options.skipParsers()) {
            flayOptions.setSkipParsers();
        }
        if (rtSmithOptions.userP4Info().has_value()) {
            flayOptions.setUserP4Info(rtSmithOptions.userP4Info().value());
        }
        flayOptions.setControlPlaneApi(std::string(rtSmithOptions.controlPlaneApi()));
        flayOptions.preprocessor_options = rtSmithOptions.preprocessor_options;
        flayOptions.setControlPlaneConfig(rtSmithOptions.outputDir() / "initial_config.txtpb");
        flayOptions.setConfigurationUpdatePattern(rtSmithOptions.outputDir() / "*update_*.txtpb");
        ASSIGN_OR_RETURN(auto flayServiceStatistics, Flay::Flay::optimizeProgram(flayOptions),
                         EXIT_FAILURE);
        printInfo("Flay optimization complete.");
        printInfo("Statistics:");
        for (const auto &[analysisName, statistic] : flayServiceStatistics) {
            printInfo("#####\n%1%:\n%2%#####", analysisName, statistic->toFormattedString());
        }
    }

    return EXIT_SUCCESS;
}

}  // namespace P4::P4Tools::RtSmith

int main(int argc, char *argv[]) {
    P4::P4Tools::Flay::registerFlayTargets();
    P4::P4Tools::RtSmith::registerRtSmithTargets();

    // Set up the options.
    auto *compileContext =
        new P4::P4Tools::CompileContext<P4::P4Tools::RtSmith::FlayCheckerOptions>();
    P4::AutoCompileContext autoContext(
        new P4::P4CContextWithOptions<P4::P4Tools::RtSmith::FlayCheckerOptions>());
    // Process command-line options.
    if (compileContext->options().processOptions(argc, argv) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    auto *rtSmithContext =
        new P4::P4Tools::CompileContext<P4::P4Tools::RtSmith::RtSmithOptions>(*compileContext);
    P4::AutoCompileContext autoContext2(rtSmithContext);
    // Run the reference checker.
    auto rtSmithOptions = rtSmithContext->options();
    if (rtSmithOptions.seed.has_value()) {
        P4::P4Tools::printInfo("Using provided seed");
    } else {
        P4::P4Tools::printInfo("Generating seed...");
        // No seed provided, we generate our own.
        std::random_device r;
        rtSmithOptions.seed = r();
        P4::P4Tools::Utils::setRandomSeed(*rtSmithOptions.seed);
    }

    auto result = P4::P4Tools::RtSmith::run(compileContext->options(), rtSmithContext->options());
    if (result == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    P4::P4Tools::printPerformanceReport();
    return P4::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
