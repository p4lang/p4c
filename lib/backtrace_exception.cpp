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

#include "backtrace_exception.h"

#include <stdarg.h>
#if HAVE_LIBBACKTRACE
#include <backtrace.h>
extern struct backtrace_state *global_backtrace_state;
#endif

#include <functional>
#include <regex>
#include <stdexcept>
#include <system_error>
#include <typeinfo>

#include "crash.h"
#include "exename.h"
#include "hex.h"

#if HAVE_LIBBACKTRACE
int append_message(void *msg_, uintptr_t pc, const char *file, int line, const char *func) {
    std::string &msg = *static_cast<std::string *>(msg_);
    std::stringstream tmp;
    tmp << "\n  0x" << hex(pc) << "  " << (func ? func : "??");
    if (file) tmp << "\n    " << file << ":" << line;
    msg += tmp.str();
    return 0;
}
#endif

void backtrace_fill_stacktrace(std::string &msg, void *const *backtrace, int size) {
    // backtrace_symbols is only available with libexecinfo
#if HAVE_LIBBACKTRACE
    if (!global_backtrace_state)
        global_backtrace_state = backtrace_create_state(exename(), 1, nullptr, nullptr);
    backtrace_full(global_backtrace_state, 1, append_message, nullptr, &msg);
    (void)backtrace;
    (void)size;
#elif HAVE_EXECINFO_H
    char **strings = backtrace_symbols(backtrace, size);
    for (int i = 0; i < size; i++) {
        if (strings) {
            msg += "\n  ";
            msg += strings[i];
        }
        if (const char *line = addr2line(backtrace[i], strings ? strings[i] : 0)) {
            msg += "\n    ";
            msg += line;
        }
    }
    free(strings);
#else
    // unused
    (void)msg;
    (void)backtrace;
    (void)size;
#endif
}

#ifdef __GLIBC__
/* DANGER -- overrides for glibc++ exception throwers to include a stack trace.
 * correct functions depends on library internals, so may not work on some versions
 * and will fail with non-GNU libc++. These function are declared in bits/functexcept.h. */

namespace std {

void __throw_bad_alloc() { throw backtrace_exception<bad_alloc>(); }

void __throw_bad_cast() { throw backtrace_exception<bad_cast>(); }

void __throw_bad_function_call() { throw backtrace_exception<bad_function_call>(); }

void __throw_invalid_argument(char const *m) { throw backtrace_exception<invalid_argument>(m); }

void __throw_length_error(char const *m) { throw backtrace_exception<length_error>(m); }

void __throw_logic_error(char const *m) { throw backtrace_exception<logic_error>(m); }

void __throw_out_of_range(char const *m) { throw backtrace_exception<out_of_range>(m); }

void __throw_out_of_range_fmt(char const *fmt, ...) {
    char buffer[1024];  // should be large enough for all cases?
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    throw backtrace_exception<out_of_range>(buffer);
}

void __throw_regex_error(regex_constants::error_type err) {
    throw backtrace_exception<regex_error>(err);
}

void __throw_system_error(int err) {
    throw backtrace_exception<system_error>(error_code(err, generic_category()));
}

}  // namespace std

#endif /* __GLIBC__ */
