/*
Copyright 2013-present Barefoot Networks, Inc. 

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

#include "log.h"
#include <string.h>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#ifdef MULTITHREAD
#include <mutex>
#define MTONLY(...)     __VA_ARGS__
#else
#define MTONLY(...)
#endif  // MULTITHREAD

int verbose = 0;
static std::vector<std::string> debug_specs;
static std::unordered_set<int *> *log_vars;

static bool match(const char *pattern, const char *name) {
    const char *pend = pattern + strcspn(pattern, ",:");
    const char *pbackup = 0;
    while (1) {
        while (pattern < pend && *pattern == *name) {
            pattern++;
            name++; }
        if (pattern == pend) {
            if (!strcmp(name, ".cpp")) return true;
            return *name == 0; }
        if (*pattern++ != '*') return false;
        if (pattern == pend) return true;
        while (*name && *name != *pattern) {
            if (pbackup && *name == *pbackup) {
                pattern = pbackup;
                break; }
            name++; }
        pbackup = pattern;
    }
}

int get_file_log_level(const char *file, int *level) {
    static std::unordered_set<int *> log_vars_;
    log_vars = &log_vars_;
    if (!log_vars->count(level)) {
        static bool loop_guard = false;
        if (loop_guard) return -1;
        MTONLY(
            static std::mutex lock;
            std::lock_guard<std::mutex> acquire(lock); )
        loop_guard = true;
        log_vars->emplace(level);
        loop_guard = false; }
    if (auto *p = strrchr(file, '/'))
        file = p+1;
    for (auto &s : debug_specs)
        for (auto *p = s.c_str(); p; p = strchr(p, ',')) {
            while (*p == ',') p++;
            if (match(p, file))
                if (auto *l = strchr(p, ':'))
                    return *level = atoi(l+1); }
    return *level = verbose > 0 ? verbose - 1 : 0;
}

void add_debug_spec(const char *spec) {
    bool ok = false;
    for (const char *p = strchr(spec, ':'); p; p = strchr(p, ':')) {
        ok = true;
        strtol(p+1, const_cast<char **>(&p), 10);
        if (*p && *p != ',') {
            ok = false;
            break; } }
    if (!ok)
        std::cerr << "Invalid debug trace spec '" << spec << "'" << std::endl;
    else
        debug_specs.push_back(spec);
    if (log_vars)
        for (auto p : *log_vars)
            *p = -1;
}

std::ostream &operator<<(std::ostream &out, const output_log_prefix &pfx) {
#if 1
    const char *s = strrchr(pfx.fn, '/');
    const char *e = strrchr(pfx.fn, '.');
    s = s ? s + 1 : pfx.fn;
    if (e && e > s)
        out.write(s, e-s);
    else
        out << s;
    out << ':' << pfx.level << ':';
#endif
    return out;
}
