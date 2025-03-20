/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_ASM_BFAS_H_
#define BACKENDS_TOFINO_BF_ASM_BFAS_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <memory>
#include <string>

enum config_version_t { CONFIG_OLD = 1, CONFIG_NEW = 2, CONFIG_BOTH = 3 };
enum target_t {
    NO_TARGET = 0,
    TOFINO,
    TOFINO2,
    JBAY = TOFINO2,
    TOFINO2H,
    TOFINO2U,
    TOFINO2M,
    TOFINO2A0,
    TARGET_INDEX_LIMIT
};
enum binary_type_t {
    NO_BINARY = -3,
    FOUR_PIPE = -2,  // binary replicating to all 4 pipes
    ONE_PIPE = -1,   // binary for one pipe with pipe offset addresses
    PIPE0 = 0,       // binary with data just in pipe 0
    PIPE1,           // binary with data just in pipe 1
    PIPE2,           // binary with data just in pipe 2
    PIPE3,           // binary with data just in pipe 3
    MAX_PIPE_COUNT,  // Maximum number of pipes which bfas can create binary for
};

extern struct option_t {
    binary_type_t binary;
    bool condense_json;
    bool debug_info;
    bool disable_egress_latency_padding;
    bool disable_gfm_parity;
    bool disable_long_branch;
    bool disable_power_gating;
    bool gen_json;
    bool high_availability_enabled;
    bool match_compiler;
    bool multi_parsers;
    bool partial_input;
    bool singlewrite;
    std::string stage_dependency_pattern;
    target_t target;
    bool tof2lab44_workaround;
    config_version_t version;
    bool werror;
    bool nowarn;
    bool log_hashes;
    std::string output_dir;
    int num_stages_override;
    bool tof1_egr_parse_depth_checks_disabled;
    const char *fill_noop_slot;
} options;

extern unsigned unique_action_handle;
struct value_t;

extern std::string asmfile_name;
extern std::string asmfile_dir;
extern std::unique_ptr<std::ostream> gfm_out;

class BaseAsmParser;
extern BaseAsmParser *asm_parser;
void createSingleAsmParser();

std::string toString(target_t target);
std::ostream &operator<<(std::ostream &out, target_t target);

int asm_parse_file(const char *name, FILE *in);
int asm_parse_string(const char *in);

void no_sections_error_exit();
bool no_section_error(const char *name);

extern int error_count, warn_count;
extern void error(int lineno, const char *fmt, va_list);
void error(int lineno, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
inline void error(int lineno, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    error(lineno, fmt, args);
    va_end(args);
}
extern void warning(int lineno, const char *fmt, va_list);
void warning(int lineno, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
inline void warning(int lineno, const char *fmt, ...) {
#ifdef BAREFOOT_INTERNAL
    if (!options.nowarn) {
        va_list args;
        va_start(args, fmt);
        warning(lineno, fmt, args);
        va_end(args);
    }
#endif /* BAREFOOT_INTERNAL */
}

// FIXME: Replace with library bug macros.
inline const char *strip_prefix(const char *str, const char *pfx) {
    if (const char *p = strstr(str, pfx)) return p + strlen(pfx);
    return str;
}
void bug(const char *, int, const char * = 0, ...) __attribute__((format(printf, 3, 4)))
__attribute__((noreturn));
inline void bug(const char *fname, int lineno, const char *fmt, ...) {
#ifdef NDEBUG
    fprintf(stderr, "Assembler BUG");
#else
    fprintf(stderr, "%s:%d: Assembler BUG: ", fname, lineno);
    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
#endif /* !NDEBUG */
    fprintf(stderr, "\n");
    fflush(stderr);
    std::terminate();
}

extern std::unique_ptr<std::ostream> open_output(const char *, ...)
    __attribute__((format(printf, 1, 2)));

// FIXME: Replace with library bug macros.
#define SRCFILE strip_prefix(__FILE__, "bf-asm/")
#define BUG(...)                               \
    do {                                       \
        bug(SRCFILE, __LINE__, ##__VA_ARGS__); \
    } while (0)
#define BUG_CHECK(e, ...)                          \
    do {                                           \
        if (!(e)) BUG("Check failed" __VA_ARGS__); \
                                                   \
    } while (0)

class VersionIter {
    unsigned left, bit;
    void check() {
        while (left && !(left & 1)) {
            ++bit;
            left >>= 1;
        }
    }
    VersionIter() : left(0), bit(0) {}

 public:
    explicit VersionIter(config_version_t v) : left(v), bit(0) { check(); }
    VersionIter begin() { return *this; }
    VersionIter end() { return VersionIter(); }
    int operator*() const { return bit; }
    bool operator==(VersionIter &a) { return (left << bit) == (a.left << a.bit); }
    VersionIter &operator++() {
        left &= ~1;
        check();
        return *this;
    }
};

extern unsigned unique_table_offset;

#endif /* BACKENDS_TOFINO_BF_ASM_BFAS_H_ */
