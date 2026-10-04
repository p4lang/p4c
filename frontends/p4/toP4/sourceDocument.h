/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef FRONTENDS_P4_TOP4_SOURCEDOCUMENT_H_
#define FRONTENDS_P4_TOP4_SOURCEDOCUMENT_H_

#include <string_view>
#include <vector>

namespace P4 {

/// A source-backed layout for ToP4. Spelling and protected trivia borrow the
/// input buffer. Unlike compiler IR, this representation retains directives,
/// comments, macro arguments, and incomplete declarations without interpretation.
struct SourceFragment {
    enum class Break { Original, None, Space, Line, BlankLine };
    std::string_view spelling;
    std::string_view trivia;
    Break before = Break::Original;
    unsigned indent = 0;
};
using SourceDocument = std::vector<SourceFragment>;

}  // namespace P4
#endif /* FRONTENDS_P4_TOP4_SOURCEDOCUMENT_H_ */
