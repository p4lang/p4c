/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backends/p4fmt/syntaxRecovery.h"

#include "backends/p4fmt/preprocessor.h"

namespace P4::P4Fmt {

namespace {

// Normalize only printer-generated whitespace. Comments and string literals
// are copied from the source and must never pass through this function.
std::string cleanWhitespace(std::string_view whitespace) {
    std::string result;
    for (char c : whitespace) {
        if (c == '\n') {
            while (!result.empty() &&
                   (result.back() == ' ' || result.back() == '\t' || result.back() == '\r')) {
                result.pop_back();
            }
        }
        result += c;
    }
    return result;
}

}  // namespace

std::optional<std::string_view> SyntaxRecovery::preservationReason(const FormatTokens &source) {
    if (!source.complete) return "unterminated string/comment or unsupported token";
    // Line splicing precedes comment recognition in the C preprocessor. Until
    // logical lines are modeled, preserve even splices inside comments/strings.
    if (source.source.find("\\\n") != std::string_view::npos ||
        source.source.find("\\\r\n") != std::string_view::npos) {
        return "line splicing is not supported";
    }
    for (const auto &token : source.tokens) {
        if (token.kind != FormatTokens::Kind::Token) continue;
        const auto text = source.spelling(token);
        if (text == "#" || text == "##" || text == "\\") {
            return "preprocessor syntax is not supported";
        }
    }
    return std::nullopt;
}

std::optional<std::string> SyntaxRecovery::reconcile(const FormatTokens &source,
                                                     std::string_view formatted) {
    const FormatTokens proposal(formatted);
    if (preserveWholeFile(source) || preserveWholeFile(proposal)) return std::nullopt;

    std::string result;
    size_t next = 0;
    size_t sourceEnd = 0;
    size_t proposalEnd = 0;
    bool hasComment = false;
    size_t argumentEnd = 0;
    for (size_t index = 0; index < source.tokens.size(); ++index) {
        const auto &token = source.tokens[index];
        if (token.kind == FormatTokens::Kind::Comment) hasComment = true;
        if (token.kind != FormatTokens::Kind::Token) continue;

        while (next < proposal.tokens.size() &&
               proposal.tokens[next].kind == FormatTokens::Kind::Whitespace) {
            ++next;
        }
        if (next == proposal.tokens.size()) return std::nullopt;
        const auto &replacement = proposal.tokens[next++];
        if (replacement.kind != FormatTokens::Kind::Token ||
            source.spelling(token) != proposal.spelling(replacement)) {
            return std::nullopt;
        }

        if (hasComment || token.begin < argumentEnd) {
            result += source.source.substr(sourceEnd, token.begin - sourceEnd);
        } else {
            result +=
                cleanWhitespace(formatted.substr(proposalEnd, replacement.begin - proposalEnd));
        }
        result += source.spelling(token);
        if (token.begin >= argumentEnd) {
            if (auto end = macroArgumentEnd(source, index)) argumentEnd = *end;
        }
        sourceEnd = token.end;
        proposalEnd = replacement.end;
        hasComment = false;
    }
    while (next < proposal.tokens.size()) {
        if (proposal.tokens[next++].kind != FormatTokens::Kind::Whitespace) return std::nullopt;
    }
    if (hasComment) {
        result += source.source.substr(sourceEnd);
    } else if (sourceEnd != 0) {
        result += '\n';
    }
    return result;
}

}  // namespace P4::P4Fmt
