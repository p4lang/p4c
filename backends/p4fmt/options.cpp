#include "options.h"

namespace p4c::P4Fmt {

P4fmtOptions::P4fmtOptions() {
    registerOption(
        "-o", "outfile",
        [this](const char *arg) {
            outFile = arg;
            return true;
        },
        "Write formatted output to outfile");
}

const std::filesystem::path &P4fmtOptions::outputFile() const { return outFile; }

}  // namespace p4c::P4Fmt
