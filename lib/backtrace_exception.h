/*
 * SPDX-FileCopyrightText: 2019 Barefoot Networks, Inc.
 * Copyright 2019-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_BACKTRACE_EXCEPTION_H_
#define LIB_BACKTRACE_EXCEPTION_H_

#include <exception>
#include <string>

#include "absl/debugging/stacktrace.h"
#include "config.h"

namespace P4 {

void backtrace_fill_stacktrace(std::string &msg, void *const *backtrace, int size);

template <class E>
class backtrace_exception : public E {
    static constexpr int buffer_size = 64;
    void *backtrace_buffer[buffer_size];
    int backtrace_size;
    mutable std::string message;

 public:
    template <class... Args>
    explicit backtrace_exception(Args &&...args) : E(std::forward<Args>(args)...) {
        backtrace_size = absl::GetStackTrace(backtrace_buffer, buffer_size, 1);
    }

    const char *what() const noexcept {
        try {
            message = E::what();
            if (backtrace_size > 0)
                backtrace_fill_stacktrace(message, backtrace_buffer, backtrace_size);
            return message.c_str();
        } catch (...) {
        }
        return E::what();
    }
};

}  // namespace P4

#endif /* LIB_BACKTRACE_EXCEPTION_H_ */
