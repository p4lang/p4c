/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_P4FMT_SOURCEPARSER_H_
#define BACKENDS_P4FMT_SOURCEPARSER_H_

#include <optional>

#include "backends/p4fmt/formatTokens.h"
#include "frontends/p4/toP4/sourceDocument.h"

namespace P4::P4Fmt {

/// Parse layout, not semantics. Unknown names and incomplete declarations are
/// valid source-formatting input; unlexable input is not.
std::optional<SourceDocument> parseSource(const FormatTokens &source);

/// Verify exact token/comment spellings, boundaries, and inter-token splices.
bool sameSourceTokens(const FormatTokens &source, const FormatTokens &proposal);

}  // namespace P4::P4Fmt
#endif /* BACKENDS_P4FMT_SOURCEPARSER_H_ */
