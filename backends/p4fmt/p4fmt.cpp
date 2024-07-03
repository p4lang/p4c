#include <cstdlib>
#include <iostream>

#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/toP4/toP4.h"
#include "ir/ir.h"
#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/nullstream.h"
#include "options.h"

int main(int argc, char *const argv[]) {
    AutoCompileContext autoP4FmtContext(new P4Fmt::P4FmtContext);
    auto &options = P4Fmt::P4FmtContext::get().options();

    if (options.process(argc, argv) == nullptr) {
        return EXIT_FAILURE;
    }

    options.setInputFile();

    std::ostream *out = nullptr;

    // Write to stdout in absence of an output file.
    if (options.outputFile().empty()) {
        out = &std::cout;
    } else {
        out = openFile(options.outputFile(), false);
        if (!(*out)) {
            ::error(ErrorType::ERR_NOT_FOUND, "%2%: No such file or directory.",
                    options.outputFile().string());
            options.usage();
            return EXIT_FAILURE;
        }
    }

    const IR::P4Program *program = P4::parseP4File(options);

    if (program == nullptr && ::errorCount() != 0) {
        return EXIT_FAILURE;
    }

    auto top4 = P4::ToP4(out, false);

    std::cerr << "\n############################## INITIAL ##############################\n";
    // Print the program before running front end passes.
    program->apply(top4);

    return ::errorCount() > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
