#include "options.h"

namespace P4Fmt {

P4fmtOptions::P4fmtOptions() {
    registerOption(
        "-o", "outfile",
        [this](const char *arg) {
            outFile = cstring(arg);
            return true;
        },
        "Write formatted output to outfile");
}

const cstring &P4fmtOptions::outputFile() const { return outFile; }

}  // namespace P4Fmt
