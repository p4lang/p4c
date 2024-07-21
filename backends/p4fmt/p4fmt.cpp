#include "backends/p4fmt/p4fmt.h"

#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/toP4/toP4.h"
#include "ir/ir.h"
#include "lib/compile_context.h"
#include "lib/error.h"
#include "options.h"

namespace p4c::P4Fmt {

std::stringstream getFormattedOutput(std::filesystem::path inputFile) {
    AutoCompileContext autoP4FmtContext(new P4Fmt::P4FmtContext);
    auto &options = P4Fmt::P4FmtContext::get().options();

    options.file = std::move(inputFile);

    std::stringstream formattedOutput;

    const IR::P4Program *program = P4::parseP4File(options);
    if (program == nullptr && ::p4c::errorCount() != 0) {
        ::p4c::error("Failed to parse P4 file.");
        return formattedOutput;
    }

    auto top4 = P4::ToP4(&formattedOutput, false);
    // Print the program before running front end passes.
    program->apply(top4);

    if (::p4c::errorCount() > 0) {
        ::p4c::error("Failed to format p4 program.");
        return formattedOutput;
    }

    return formattedOutput;
}

}  // namespace p4c::P4Fmt
