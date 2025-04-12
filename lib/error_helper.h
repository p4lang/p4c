/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LIB_ERROR_HELPER_H_
#define LIB_ERROR_HELPER_H_

#include <utility>

#include "lib/error_message.h"
#include "lib/format.h"
#include "lib/source_file.h"

namespace P4 {
// Invoked from ErrorReporter
template <class... Args>
ErrorMessage error_helper(const char *format, ErrorMessage msg, const Args &...args) {
    if constexpr (sizeof...(Args) != 0) {
        auto collect = [&](const Util::SourceInfo &info) { msg.locations.push_back(info); };
        (detail::visitSourceInfo(args, collect), ...);
    }
    msg.message = createFormattedMessage(format, args...);
    return msg;
}

// This overload exists for backwards compatibility
template <class... Args>
ErrorMessage error_helper(const char *format, const std::string &prefix,
                          const Util::SourceInfo &info, const std::string &suffix,
                          const Args &...args) {
    return error_helper(format, ErrorMessage(prefix, info, suffix), args...);
}

// Most direct invocations of error_helper usually only reduce arguments.
template <class... Args>
ErrorMessage error_helper(const char *format, const Args &...args) {
    return error_helper(format, ErrorMessage{}, args...);
}

}  // namespace P4

#endif /* LIB_ERROR_HELPER_H_ */
