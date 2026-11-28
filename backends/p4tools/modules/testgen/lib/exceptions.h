/*
 * SPDX-FileCopyrightText: 2022 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_

#include "lib/exceptions.h"

namespace P4::P4Tools::P4Testgen {

/// This class indicates a feature that is not implemented in P4Testgen.
/// Paths with this unimplemented feature should be skipped
class TestgenUnimplemented final : public Util::P4CExceptionBase {
 public:
    template <typename... Args>
    explicit TestgenUnimplemented(const char *format, Args &&...args)
        : P4CExceptionBase(createBugMessage(format, args...), "Not yet implemented",
                           Util::ANSI_BLUE) {}

    template <typename... Args>
    TestgenUnimplemented(std::source_location location, const char *format, Args &&...args)
        : P4CExceptionBase(location, createBugMessage(format, args...),
                           "Unimplemented compiler support", Util::ANSI_BLUE) {}
};

template <typename... Args>
[[noreturn]] inline void TESTGEN_UNIMPLEMENTED(Util::FormatWithLocation fmt, Args &&...args) {
    throw TestgenUnimplemented(fmt.location, fmt.format, std::forward<Args>(args)...);
}

}  // namespace P4::P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_EXCEPTIONS_H_ */
