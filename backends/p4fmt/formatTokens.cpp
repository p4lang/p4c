/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backends/p4fmt/formatTokens.h"

namespace P4::P4Fmt {
namespace {

bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

bool isWord(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// Keep these compound spellings in sync with frontends/parsers/p4/p4lexer.ll.
// Longest match first. Keep adjacent operators distinct from spaced operators,
// including >> (whose interpretation in the compiler lexer is contextual).
// @pragma is also whitespace-sensitive: @ pragma is an ordinary annotation.
constexpr std::string_view operators[] = {
    "@pragma", "|+|=", "|-|=", "&&&", "...", "<<=", ">>=", "|+|", "|-|", "{#}",
    "..",      "<<",   ">>",   "&&",  "||",  "==",  "!=",  ">=",  "<=",  "++",
    "+=",      "-=",   "*=",   "/=",  "%=",  "&=",  "|=",  "^=",  "+:",  "##"};

}  // namespace

FormatTokens::FormatTokens(std::string_view source) : source(source) {
    auto &offsets = physicalOffsets;
    for (size_t i = 0; i < source.size();) {
        if (source.substr(i, 2) == "\\\n") {
            i += 2;
        } else if (source.substr(i, 3) == "\\\r\n") {
            i += 3;
        } else {
            offsets.push_back(i);
            logicalSource += source[i++];
        }
    }
    offsets.push_back(source.size());
    const std::string_view text(logicalSource);
    size_t physicalEnd = 0;
    size_t pos = 0;
    while (pos < text.size()) {
        const auto begin = pos;
        auto kind = Kind::Token;
        if (isSpace(text[pos])) {
            kind = Kind::Whitespace;
            while (pos < text.size() && isSpace(text[pos])) ++pos;
        } else if (text.substr(pos, 2) == "//") {
            kind = Kind::Comment;
            pos += 2;
            while (pos < text.size() && text[pos] != '\n' && text[pos] != '\r') ++pos;
        } else if (text.substr(pos, 2) == "/*") {
            kind = Kind::Comment;
            const auto end = text.find("*/", pos + 2);
            if (end == std::string_view::npos) {
                complete = false;
                pos = text.size();
            } else {
                pos = end + 2;
            }
        } else if (text[pos] == '"') {
            ++pos;
            bool closed = false;
            while (pos < text.size()) {
                const auto c = text[pos++];
                if (c == '\\' && pos < text.size()) {
                    ++pos;
                } else if (c == '"') {
                    closed = true;
                    break;
                }
            }
            complete &= closed;
        } else if (isWord(text[pos])) {
            while (pos < text.size() && isWord(text[pos])) ++pos;
        } else {
            for (auto op : operators) {
                if (text.substr(pos, op.size()) == op) {
                    pos += op.size();
                    break;
                }
            }
            if (pos == begin) {
                const auto c = static_cast<unsigned char>(text[pos++]);
                if (c < 32 || c >= 127) complete = false;
            }
        }
        if (physicalEnd < offsets[begin]) {
            // A splice between tokens is trivia, but must be copied verbatim.
            tokens.push_back({Kind::Whitespace, physicalEnd, offsets[begin], begin, begin});
        }
        physicalEnd = offsets[pos - 1] + 1;
        tokens.push_back({kind, offsets[begin], physicalEnd, begin, pos});
    }
    if (physicalEnd < source.size()) {
        tokens.push_back({Kind::Whitespace, physicalEnd, source.size(), pos, pos});
    }
}

}  // namespace P4::P4Fmt
