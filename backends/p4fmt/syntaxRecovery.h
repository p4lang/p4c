/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_P4FMT_SYNTAXRECOVERY_H_
#define BACKENDS_P4FMT_SYNTAXRECOVERY_H_

#include <optional>
#include <string>
#include <string_view>

#include "backends/p4fmt/formatTokens.h"

namespace P4::P4Fmt {

/// Whole-file recovery policy for the initial source-preserving formatter.
/// Directive interpretation and recovery of smaller regions can be added here
/// without teaching the compiler grammar or printer about opaque syntax.
class SyntaxRecovery {
 public:
    static std::optional<std::string_view> preservationReason(const FormatTokens &source);
    static bool preserveWholeFile(const FormatTokens &source) {
        return preservationReason(source).has_value();
    }

    /// Accept only whitespace edits between identical spelled tokens. Preserve
    /// comment-containing gaps exactly, including file headers and EOF comments.
    /// A mismatch means the caller must retain the entire original source.
    static std::optional<std::string> reconcile(const FormatTokens &source,
                                                std::string_view formatted);
};

}  // namespace P4::P4Fmt

#endif /* BACKENDS_P4FMT_SYNTAXRECOVERY_H_ */
