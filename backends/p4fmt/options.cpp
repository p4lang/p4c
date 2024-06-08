#include "options.h"

#include "lib/path.h"

namespace P4Fmt {

P4fmtOptions::P4fmtOptions() {
    registerOption(
        "-o", "outfile",
        [this](const char *arg) {
            outFile = arg;
            return true;
        },
        "Write formatted output to outfile");
}

const Util::PathName &P4fmtOptions::outputFile() const { return outFile; }

}  // namespace P4Fmt
