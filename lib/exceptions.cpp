// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/exceptions.h"

#include "absl/strings/str_cat.h"

namespace P4::Util {

P4CExceptionBase::P4CExceptionBase(std::string formattedMessage, const char *heading,
                                   const char *color)
    : P4CExceptionBase(std::move(formattedMessage)) {
    message = absl::StrCat(cerr_colorize(color), heading, cerr_clear_colors(), ":\n", message);
}

P4CExceptionBase::P4CExceptionBase(std::source_location location, std::string formattedMessage,
                                   const char *heading, const char *color)
    : P4CExceptionBase(std::move(formattedMessage)) {
    message = absl::StrCat("In file: ", location.file_name(), ":", location.line(), "\n",
                           cerr_colorize(color), heading, cerr_clear_colors(), ": ", message);
}

}  // namespace P4::Util
