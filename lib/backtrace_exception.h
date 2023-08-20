/*
Copyright 2019-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef LIB_BACKTRACE_EXCEPTION_H_
#define LIB_BACKTRACE_EXCEPTION_H_

#include <exception>
#include <string>

#include "config.h"

#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

void backtrace_fill_stacktrace(std::string &msg, void *const *backtrace, int size);

template <class E>
class backtrace_exception : public E {
    static constexpr int buffer_size = 64;
    void *backtrace_buffer[buffer_size];
    int backtrace_size;
    mutable std::string message;

 public:
    template <class... Args>
    explicit backtrace_exception(Args... args) : E(std::forward<Args>(args)...) {
#if HAVE_EXECINFO_H
        backtrace_size = backtrace(backtrace_buffer, buffer_size);
#else
        backtrace_size = 0;
#endif
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

#endif /* LIB_BACKTRACE_EXCEPTION_H_ */
