/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */
#include "backends/p4fmt/preprocessor.h"

namespace P4::P4Fmt {

std::vector<Directive> scanDirectives(const FormatTokens &source) {
    std::vector<Directive> result;
    bool lineStart = true;
    size_t directiveEnd = 0;
    for (size_t i = 0; i < source.tokens.size(); ++i) {
        const auto &token = source.tokens[i];
        if (token.end <= directiveEnd) continue;
        const auto text = source.logicalSpelling(token);
        if (token.kind != FormatTokens::Kind::Token) {
            if (text.find('\n') != std::string_view::npos) lineStart = true;
            continue;
        }
        // P4-14 pragmas, like directives, end at the logical line boundary.
        if ((lineStart && text == "#") || text == "@pragma") {
            auto end = source.logicalSource.find('\n', token.logicalEnd);
            if (end == std::string::npos) end = source.logicalSource.size();
            // P4 permits multiline strings in pragmas. Never cut a token at a
            // physical newline (the same rule preserves multiline comments).
            for (size_t j = i + 1; j < source.tokens.size(); ++j) {
                const auto &next = source.tokens[j];
                if (next.logicalBegin >= end) break;
                if (next.kind != FormatTokens::Kind::Whitespace && next.logicalEnd > end) {
                    end = source.logicalSource.find('\n', next.logicalEnd);
                    if (end == std::string::npos) end = source.logicalSource.size();
                }
            }
            directiveEnd = source.physicalOffsets[end];
            auto kind = Directive::Kind::Other;
            bool hasElse = false;
            if (text == "#") {
                size_t next = i + 1;
                while (next < source.tokens.size() && source.tokens[next].begin < directiveEnd &&
                       source.tokens[next].kind != FormatTokens::Kind::Token)
                    ++next;
                if (next < source.tokens.size() && source.tokens[next].begin < directiveEnd) {
                    const auto name = source.logicalSpelling(source.tokens[next]);
                    if (name == "if" || name == "ifdef" || name == "ifndef")
                        kind = Directive::Kind::If;
                    if (name == "else" || name == "elif" || name == "elifdef" || name == "elifndef")
                        kind = Directive::Kind::Else;
                    if (name == "endif") kind = Directive::Kind::Endif;
                    hasElse = name == "else";
                }
            }
            result.push_back({token.begin, directiveEnd, kind, hasElse});
        }
        lineStart = false;
    }
    return result;
}

std::optional<size_t> macroArgumentEnd(const FormatTokens &source, size_t tokenIndex) {
    if (source.logicalSpelling(source.tokens[tokenIndex]) != "(") return std::nullopt;
    size_t previous = tokenIndex;
    while (previous > 0 && source.tokens[--previous].kind != FormatTokens::Kind::Token) {
    }
    const auto name = source.logicalSpelling(source.tokens[previous]);
    const bool identifier = !name.empty() && ((name[0] >= 'a' && name[0] <= 'z') ||
                                              (name[0] >= 'A' && name[0] <= 'Z') || name[0] == '_');
    // Type-specialized calls also carry argument lists, including tuple literals.
    if ((!identifier && name != ">" && name != ">>") || name == "if" || name == "switch" ||
        name == "select" || name == "for" || name == "while")
        return std::nullopt;
    unsigned depth = 1;
    for (size_t j = tokenIndex + 1; j < source.tokens.size(); ++j) {
        if (source.tokens[j].kind != FormatTokens::Kind::Token) continue;
        const auto text = source.logicalSpelling(source.tokens[j]);
        if (text == "#" || text == "@pragma") return std::nullopt;
        if (text == "(") ++depth;
        if (text == ")" && --depth == 0) return source.tokens[j].end;
    }
    return std::nullopt;
}

}  // namespace P4::P4Fmt
