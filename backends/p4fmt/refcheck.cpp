// Ref: https://github.com/fruffy/flay/blob/master/tools/reference_checker.cpp

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>

#include "backends/p4fmt/p4fmt.h"
#include "backends/p4tools/common/lib/logging.h"
#include "options.h"
#include "p4fmt.h"

using namespace ::P4;

namespace P4::P4Fmt {

namespace {
class ReferenceCheckerOptions : protected P4fmtOptions {
    /// The input file to process
    std::optional<std::filesystem::path> inputFile;

    /// The reference file to compare against.
    std::optional<std::filesystem::path> referenceFile;

    /// Overwrite the reference file and do not compare against it.
    bool overwriteReferences = false;

 public:
    ReferenceCheckerOptions() {
        registerOption(
            "--file", "inputFile",
            [this](const char *arg) {
                inputFile = arg;
                if (!std::filesystem::exists(inputFile.value())) {
                    ::P4::error("The input P4 program '%s' does not exist.",
                                inputFile.value().c_str());
                    return false;
                }
                return true;
            },
            "The input file to process.");
        registerOption(
            "--overwrite", nullptr,
            [this](const char *) {
                overwriteReferences = true;
                return true;
            },
            "Do not check references, instead overwrite the reference.");
        registerOption(
            "--reference-file", "referenceFile",
            [this](const char *arg) {
                referenceFile = arg;
                return true;
            },
            "The reference file to compare against.");
    }
    ~ReferenceCheckerOptions() override = default;

    int processOptions(int argc, char *argv[]) {
        auto *unprocessedOptions = P4fmtOptions::process(argc, argv);

        if (unprocessedOptions != nullptr && !unprocessedOptions->empty()) {
            for (const auto &option : *unprocessedOptions) {
                ::P4::error("Unprocessed input: %s", option);
            }
            return EXIT_FAILURE;
        }

        if (!inputFile.has_value()) {
            ::P4::error("No input file specified.");
            return EXIT_FAILURE;
        }
        if (!referenceFile.has_value()) {
            ::P4::error(
                "Reference file has not been specified. Use "
                "--reference-file.");
            return EXIT_FAILURE;
        }
        // If the environment variable P4FMT_REPLACE is set, overwrite the reference file.
        if (std::getenv("P4FMT_REPLACE") != nullptr) {
            overwriteReferences = true;
        }
        return EXIT_SUCCESS;
    }

    [[nodiscard]] const std::filesystem::path &getInputFile() const { return inputFile.value(); }

    [[nodiscard]] std::optional<std::filesystem::path> getReferenceFile() const {
        return referenceFile;
    }

    [[nodiscard]] bool doOverwriteReferences() const { return overwriteReferences; }
};

int compareAgainstReference(const std::stringstream &formattedOutput,
                            const std::filesystem::path &referenceFile) {
    // Construct the command to invoke the diff utility
    std::stringstream command;
    command << "echo " << std::quoted(formattedOutput.str());
    command << "| diff --color -u " << referenceFile.c_str() << " -";

    // Open a pipe to capture the output of the diff command.
    P4Tools::printInfo("Running diff command: \"%s\"", command.str());
    FILE *pipe = popen(command.str().c_str(), "r");
    if (pipe == nullptr) {
        ::P4::error("Unable to create pipe to diff command.");
        return EXIT_FAILURE;
    }
    // Read and print the output of the diff command.
    std::stringstream result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result << buffer;
    }
    if (pclose(pipe) != 0) {
        ::P4::error("Diff command failed.\n%1%", result.str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

std::optional<std::filesystem::path> getFilePath(const ReferenceCheckerOptions &options,
                                                 const std::filesystem::path &basePath,
                                                 std::string_view suffix) {
    auto referenceFileOpt = options.getReferenceFile();
    auto referencePath = basePath;
    if (referenceFileOpt.has_value()) {
        // If a reference file is explicitly provided, just overwrite this file.
        referencePath = referenceFileOpt.value();
    } else {
        ::P4::error("Reference file has not been specified.");
        return std::nullopt;
    }
    return referencePath.replace_extension(suffix);
}

}  // namespace

int run(const ReferenceCheckerOptions &options) {
    auto referenceFileOpt = options.getReferenceFile();

    std::stringstream formattedOutput = getFormattedOutput(options.getInputFile());

    if (formattedOutput.str().empty()) {
        ::P4::error("Formatting Failed");
        return EXIT_FAILURE;
    }

    if (options.doOverwriteReferences()) {
        auto referencePath = getFilePath(options, options.getInputFile().stem(), ".ref");
        if (!referencePath.has_value()) {
            return EXIT_FAILURE;
        }

        std::ofstream ofs(referencePath.value());
        ofs << formattedOutput << std::endl;
        P4Tools::printInfo("Writing reference file %s", referencePath.value().c_str());
        ofs.close();
        return EXIT_SUCCESS;
    }
    if (referenceFileOpt.has_value()) {
        auto referenceFile = std::filesystem::absolute(referenceFileOpt.value());
        return compareAgainstReference(formattedOutput, referenceFile);
    }
    ::P4::error("Reference file has not been specified.");
    return EXIT_FAILURE;
}
}  // namespace P4::P4Fmt

class RefCheckContext : public BaseCompileContext {};

int main(int argc, char *argv[]) {
    AutoCompileContext autoP4RefCheckContext(new RefCheckContext);
    P4Fmt::ReferenceCheckerOptions options;

    if (options.processOptions(argc, argv) == EXIT_FAILURE || ::P4::errorCount() != 0) {
        return EXIT_FAILURE;
    }

    auto result = P4Fmt::run(options);
    if (result == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    return ::P4::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
