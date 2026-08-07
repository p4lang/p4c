#include "backends/p4fmt/p4fmt.h"

#include "backends/p4fmt/attach.h"
#include "backends/p4fmt/p4formatter.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/parsers/parserDriver.h"
#include "ir/ir.h"
#include "lib/compile_context.h"
#include "lib/error.h"
#include "lib/source_file.h"
#include "options.h"

namespace P4::P4Fmt {

std::optional<std::pair<const IR::P4Program *, const Util::InputSources *>> parseProgram(
    const ParserOptions &options) {
    if (!std::filesystem::exists(options.file)) {
        ::P4::error(ErrorType::ERR_NOT_FOUND, "%1%: No such file found.", options.file);
        return std::nullopt;
    }
#ifdef SUPPORT_P4_14
    if (options.isv1()) {
        ::P4::error(ErrorType::ERR_UNKNOWN, "p4fmt cannot deal with p4-14 programs.");
        return std::nullopt;
    }
#endif
    auto preprocessorResult = options.preprocess();
    auto result =
        P4ParserDriver::parseProgramSources(preprocessorResult.value().get(), options.file.c_str());

    if (::P4::errorCount() > 0) {
        ::P4::error(ErrorType::ERR_OVERLIMIT, "%1% errors encountered, aborting compilation",
                    ::P4::errorCount());
        return std::nullopt;
    }

    BUG_CHECK(result.first != nullptr, "Parsing failed, but no error was reported");

    return result;
}

std::stringstream getFormattedOutput(std::filesystem::path inputFile) {
    AutoCompileContext autoP4FmtContext(new P4Fmt::P4FmtContext);
    auto &options = P4Fmt::P4FmtContext::get().options();

    options.file = std::move(inputFile);

    std::stringstream formattedOutput;

    auto parseResult = parseProgram(options);
    if (!parseResult) {
        if (::P4::errorCount() > 0) {
            ::P4::error("Failed to parse P4 file.");
        }
        return formattedOutput;
    }
    const auto &[program, sources] = *parseResult;

    std::unordered_map<const Util::Comment *, bool> globalCommentsMap;

    // Initialize the global comments map from the list of comments in the program.
    if (sources != nullptr) {
        for (const auto *comment : sources->getAllComments()) {
            globalCommentsMap[comment] =
                false;  // Initialize all comments as not yet attached to nodes
        }
    }

    auto top4 = P4Fmt::P4Formatter(&formattedOutput);
    auto attach = P4::P4Fmt::Attach(globalCommentsMap);
    program->apply(attach);
    // Print the program before running front end passes.
    program->apply(top4);

    if (::P4::errorCount() > 0) {
        ::P4::error("Failed to format p4 program.");
        return formattedOutput;
    }

    return formattedOutput;
}

}  // namespace P4::P4Fmt
