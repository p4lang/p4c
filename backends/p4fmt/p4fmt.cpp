#include "backends/p4fmt/p4fmt.h"

#include "backends/p4fmt/attach.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "ir/ir.h"
#include "lib/compile_context.h"
#include "lib/error.h"
#include "options.h"
#include "p4formatter.h"

namespace P4::P4Fmt {

std::stringstream getFormattedOutput(std::filesystem::path inputFile) {
    AutoCompileContext autoP4FmtContext(new P4Fmt::P4FmtContext);
    auto &options = P4Fmt::P4FmtContext::get().options();

    options.file = std::move(inputFile);

    std::stringstream formattedOutput;

    const IR::P4Program *program = P4::parseP4File(options);
    if (program == nullptr && ::P4::errorCount() != 0) {
        ::P4::error("Failed to parse P4 file.");
        return formattedOutput;
    }

    std::unordered_map<const Util::Comment *, bool> globalCommentsMap;

    // Initialize the global comments map from the list of comments in the program.
    if (!program->objects.empty()) {
        const auto *firstNode = program->objects.front();
        if (firstNode->srcInfo.isValid()) {
            for (const auto *comment : firstNode->srcInfo.getAllFileComments()) {
                globalCommentsMap[comment] =
                    false;  // Initialize all comments as not yet attached to nodes
            }
        }
    }

    auto attach = P4::P4Fmt::Attach(globalCommentsMap);
    program = program->apply(attach);
    // Print the program before running front end passes.
    auto top4 = P4Fmt::P4Formatter(&formattedOutput, attach.getCommentsMap());
    program->apply(top4);

    if (::P4::errorCount() > 0) {
        ::P4::error("Failed to format p4 program.");
        return formattedOutput;
    }

    return formattedOutput;
}

}  // namespace P4::P4Fmt
