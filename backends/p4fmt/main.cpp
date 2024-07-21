#include <cstdlib>
#include <filesystem>
#include <sstream>

#include "lib/nullstream.h"
#include "options.h"
#include "p4fmt.h"

using namespace ::p4c;

int main(int argc, char *const argv[]) {
    AutoCompileContext autoP4FmtContext(new P4Fmt::P4FmtContext);
    auto &options = P4Fmt::P4FmtContext::get().options();
    if (options.process(argc, argv) == nullptr) {
        return EXIT_FAILURE;
    }
    options.setInputFile();

    std::stringstream formattedOutput = getFormattedOutput(options.file);
    if (formattedOutput.str().empty()) {
        return EXIT_FAILURE;
    };

    std::ostream *out = nullptr;
    // Write to stdout in absence of an output file.
    if (options.outputFile().empty()) {
        out = &std::cout;
    } else {
        out = openFile(options.outputFile(), false);
        if ((out == nullptr) || !(*out)) {
            ::error(ErrorType::ERR_NOT_FOUND, "%2%: No such file or directory.",
                    options.outputFile().string());
            options.usage();
            return EXIT_FAILURE;
        }
    }

    (*out) << formattedOutput.str();
    out->flush();
    if (!(*out)) {
        ::error(ErrorType::ERR_IO, "Failed to write to output file.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
