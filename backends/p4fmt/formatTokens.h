/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_P4FMT_FORMATTOKENS_H_
#define BACKENDS_P4FMT_FORMATTOKENS_H_

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace P4::P4Fmt {

/// A lossless partition of source text. This is a spelling scanner, not a P4 parser:
/// it does not classify identifiers as types, expand macros, or interpret literals.
/// The caller must keep the source buffer alive while using this object.
class FormatTokens {
 public:
    enum class Kind { Whitespace, Comment, Token };
    struct Token {
        Kind kind;
        size_t begin;
        size_t end;
        size_t logicalBegin;
        size_t logicalEnd;
    };

    std::vector<size_t> physicalOffsets;

    explicit FormatTokens(std::string_view source);

    std::string_view source;
    std::vector<Token> tokens;
    /// C translation phase 2 precedes token/comment recognition.
    std::string logicalSource;
    /// False for an unterminated string/comment or a byte we cannot safely scan.
    bool complete = true;

    std::string_view logicalSpelling(const Token &token) const {
        return std::string_view(logicalSource)
            .substr(token.logicalBegin, token.logicalEnd - token.logicalBegin);
    }

    std::string_view spelling(const Token &token) const {
        return source.substr(token.begin, token.end - token.begin);
    }
};

}  // namespace P4::P4Fmt

#endif /* BACKENDS_P4FMT_FORMATTOKENS_H_ */
