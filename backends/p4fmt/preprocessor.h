/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_P4FMT_PREPROCESSOR_H_
#define BACKENDS_P4FMT_PREPROCESSOR_H_

#include <optional>
#include <vector>

#include "backends/p4fmt/formatTokens.h"

namespace P4::P4Fmt {

/// Preprocessing directives are opaque logical lines. We never expand macros,
/// evaluate conditions, or resolve includes: every branch remains in the source.
struct Directive {
    enum class Kind { Other, If, Else, Endif };
    size_t begin;
    size_t end;
    Kind kind;
    bool hasElse;
};

std::vector<Directive> scanDirectives(const FormatTokens &source);

/// Return the byte after a possibly macro-invoked argument list beginning at
/// tokenIndex. Unknown identifiers are conservatively considered macro names.
std::optional<size_t> macroArgumentEnd(const FormatTokens &source, size_t tokenIndex);

}  // namespace P4::P4Fmt
#endif /* BACKENDS_P4FMT_PREPROCESSOR_H_ */
