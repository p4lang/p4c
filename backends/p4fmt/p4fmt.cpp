#include "backends/p4fmt/p4fmt.h"

#include <fstream>
#include <memory>
#include <sstream>

#include "backends/p4fmt/formatTokens.h"
#include "backends/p4fmt/sourceParser.h"
#include "backends/p4fmt/syntaxRecovery.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/parsers/parserDriver.h"
#include "lib/compile_context.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/nullstream.h"
#include "options.h"

namespace P4::P4Fmt {

namespace {

// Keep speculative parser/printer diagnostics isolated. The caller reports a
// fallback only after this context has been destroyed, so strict-mode errors
// are visible and counted in the caller's compilation context.
std::optional<std::string> tryFormat(const FormatTokens &source,
                                     const std::filesystem::path &inputFile,
                                     std::string_view &fallbackReason) {
    if (auto reason = SyntaxRecovery::preservationReason(source)) {
        fallbackReason = *reason;
        return std::nullopt;
    }

    nullstream diagnostics;
    AutoCompileContext context(std::make_unique<P4FmtContext>());
    P4FmtContext::get().errorReporter().setOutputStream(&diagnostics);
    try {
        std::istringstream stream(std::string{source.source});
        const auto *program = P4ParserDriver::parse(stream, inputFile.string());
        if (program == nullptr || ::P4::errorCount() > 0) {
            fallbackReason = "input could not be parsed as standalone P4";
            return std::nullopt;
        }

        Util::SourceCodeBuilder proposal;
        ToP4 formatter(proposal, false);
        formatter.setnoIncludesArg(true);
        program->apply(formatter);
        if (::P4::errorCount() > 0) {
            fallbackReason = "ToP4 could not print this input";
            return std::nullopt;
        }

        auto result = SyntaxRecovery::reconcile(source, proposal.toString());
        if (!result) fallbackReason = "ToP4 output changes source token spelling or order";
        return result;
    } catch (const Util::CompilationError &) {
        fallbackReason = "input is unsupported by the parser or printer";
    } catch (const Util::CompilerUnimplemented &) {
        fallbackReason = "input uses unimplemented parser or printer functionality";
    }
    return std::nullopt;
}

}  // namespace

std::optional<std::string> getFormattedOutput(const std::filesystem::path &inputFile,
                                              bool failOnFallback) {
    std::ifstream input(inputFile, std::ios::binary);
    if (!input) {
        ::P4::error(ErrorType::ERR_IO, "Could not read %1%", inputFile);
        return std::nullopt;
    }
    std::string original;
    char buffer[4096];
    while (input) {
        input.read(buffer, sizeof(buffer));
        original.append(buffer, static_cast<size_t>(input.gcount()));
    }
    if (input.bad()) {
        ::P4::error(ErrorType::ERR_IO, "Failed to read %1%", inputFile);
        return std::nullopt;
    }

    const FormatTokens source(original);
    std::string_view fallbackReason;
    auto formatted = tryFormat(source, inputFile, fallbackReason);
    if (formatted) return formatted;
    if (auto document = parseSource(source)) {
        AutoCompileContext context(std::make_unique<P4FmtContext>());
        Util::SourceCodeBuilder proposal;
        ToP4 printer(proposal, false);
        printer.printSource(*document);
        auto result = proposal.toString();
        if (sameSourceTokens(source, FormatTokens(result))) return result;
        fallbackReason = "source layout changes token spelling or boundaries";
    }
    if (failOnFallback) {
        ::P4::error(ErrorType::ERR_UNSUPPORTED, "%1%: formatting fallback: %2%", inputFile,
                    fallbackReason);
        return std::nullopt;
    }
    return original;
}

}  // namespace P4::P4Fmt
