#include "options.h"

namespace P4::P4Fmt {

P4fmtOptions::P4fmtOptions() {
    registerOption(
        "--fail-on-fallback", nullptr,
        [this](const char *) {
            failOnFallback_ = true;
            return true;
        },
        "Fail instead of preserving input when formatting is unsupported");
    registerOption(
        "-o", "outfile",
        [this](const char *arg) {
            outFile = arg;
            return true;
        },
        "Write formatted output to outfile");
}

const std::filesystem::path &P4fmtOptions::outputFile() const { return outFile; }

}  // namespace P4::P4Fmt
