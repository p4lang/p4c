/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_UTILS_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_UTILS_H_

#include <iostream>

#include "ir/ir.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/gc.h"

#if defined(__GNUC__) && __GNUC__ < 7
#define BFN_FALLTHROUGH /* fall through */
#define BFN_MAYBE_UNUSED __attribute__((unused))
#else
#define BFN_FALLTHROUGH [[fallthrough]]
#define BFN_MAYBE_UNUSED [[maybe_unused]]
#endif

using namespace P4;

struct DumpPipe : public Inspector {
    const char *heading;
    DumpPipe() : heading(nullptr) {}
    explicit DumpPipe(const char *h) : heading(h) {}
#if BAREFOOT_INTERNAL
    bool preorder(const IR::Node *pipe) override;
#endif  // BAREFOOT_INTERNAL
};

/// Used to end the compiler after a fatal error is raised. Usually, this just trows an exception
/// \ref Util::CompilationError. In internal builds when a program contains "expect error" and all
/// the errors are expected (including the fatal one) then this function just exits (calls
/// std::exit(0)). This is to allow for expected errors even in builds that do not catch exceptions
/// in \ref p4c-barefoot.cpp (internal debug builds, etc.).
void end_fatal_error();

/// Report an error with the given message and exit.
template <typename... T>
inline void fatal_error(const char *format, T... args) {
    error(format, args...);
    end_fatal_error();
}

/// Report an error with the error type and given message and exit.
template <typename... T>
inline void fatal_error(int kind, const char *format, T... args) {
    error(kind, format, args...);
    end_fatal_error();
}

#ifdef BAREFOOT_INTERNAL
#define INTERNAL_WARNING(...) warning(ErrorType::WARN_UNSUPPORTED, __VA_ARGS__)
#else
#define INTERNAL_WARNING(...) BUG(__VA_ARGS__)
#endif

/// Check if ghost control is present on any pipes other than current pipe given
/// by pipe_id argument
bool ghost_only_on_other_pipes(int pipe_id);

/// Separate out key and mask from an input key string
/// Input: "vrf & 0xff0f", Output: std::pair<"vrf", "0xff0f">
std::pair<cstring, cstring> get_key_and_mask(const cstring &input);

/// Separate out key slice info from an input key slice string
/// Input: "vrf[15:0]", Output: std::pair<true, "vrf", 15, 0>
std::tuple<bool, cstring, int, int> get_key_slice_info(const cstring &input);

const IR::Vector<IR::Expression> *getListExprComponents(const IR::Node &node);

bool is_starter_pistol_table(const cstring &tableName);
#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_UTILS_H_ */
