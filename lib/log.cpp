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
#include <iomanip>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef MULTITHREAD
#include <mutex>
#endif  // MULTITHREAD

namespace Log {
namespace Detail {

int verbosity = 0;
int maximumLogLevel = 0;

// The time at which logging was initialized; used so that log messages can have
// relative rather than absolute timestamps.
static uint64_t initTime = 0;

// The first level cache for fileLogLevel() - the most recent result returned.
static const char* mostRecentFile = nullptr;
static int mostRecentLevel = -1;

// The second level cache for fileLogLevel(), mapping filenames to log levels.
static std::unordered_map<const void*, int> logLevelCache;

// All log levels manually specified by the user.
static std::vector<std::string> debugSpecs;

std::ostream& operator<<(std::ostream& out, const Log::Detail::OutputLogPrefix& pfx) {
#ifdef CLOCK_MONOTONIC
    if (LOGGING(2)) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t t = ts.tv_sec*1000000000UL + ts.tv_nsec - Log::Detail::initTime;
        t /= 1000000UL;  // millisec
        out << t/1000 << '.' << std::setw(3) << std::setfill('0') << t%1000 << ':'
            << std::setfill(' '); }
#endif
    if (LOGGING(1)) {
        const char *s = strrchr(pfx.fn, '/');
        const char *e = strrchr(pfx.fn, '.');
        s = s ? s + 1 : pfx.fn;
        if (e && e > s)
            out.write(s, e-s);
        else
            out << s;
        out << ':' << pfx.level << ':'; }
    return out;
}

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
        if (*pattern == '[') {
            bool negate = false;
            if (pattern[1] == '^') {
                negate = true;
                ++pattern; }
            while ((*++pattern != *name || pattern[1] == '-') && *pattern != ']' && *pattern) {
                if (pattern[1] == '-' && pattern[2] != ']') {
                    if (*name >= pattern[0] && *name <= pattern[2]) break;
                    pattern += 2; } }
            if ((*pattern == ']' || !*pattern) ^ negate) return false;
            while (*pattern && *pattern++ != ']') continue;
            if (pattern > pend) pend = pattern + strcspn(pattern, ",:");
            name++;
            continue; }
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

int uncachedFileLogLevel(const char* file) {
    if (auto* startOfFilename = strrchr(file, '/'))
        file = startOfFilename + 1;

    for (auto& spec : debugSpecs)
        for (auto* pattern = spec.c_str(); pattern; pattern = strchr(pattern, ',')) {
            while (*pattern == ',') pattern++;
            if (match(pattern, file))
                if (auto* level = strchr(pattern, ':'))
                    return atoi(level + 1); }

    // If there's no matching spec, compute a default from the global verbosity level,
    // except for THIS file
    if (!strcmp(file, "log.cpp")) return 0;
    return verbosity > 0 ? verbosity - 1 : 0;
}

int fileLogLevel(const char* file) {
#ifdef MULTITHREAD
    static std::mutex lock;
    std::lock_guard<std::mutex> acquire(lock);
#endif

    // There are two layers of caching here. First, we cache the most recent
    // result we returned, to minimize expensive lookups in tight loops.
    if (mostRecentFile == file)
      return mostRecentLevel;

    mostRecentFile = file;

    // Second, we look up @file in a hash table mapping from pointers to log
    // levels. We expect to hit in this cache virtually all the time.
    auto logLevelCacheIter = logLevelCache.find(file);
    if (logLevelCacheIter != logLevelCache.end())
      return mostRecentLevel = logLevelCacheIter->second;

    // This is the slow path. We have to walk @debugSpecs to see if there are any
    // specs that match @file.
    mostRecentLevel = uncachedFileLogLevel(file);
    logLevelCache[file] = mostRecentLevel;
    return mostRecentLevel;
}

void invalidateCaches(int possibleNewMaxLogLevel) {
    mostRecentFile = nullptr;
    mostRecentLevel = 0;
    logLevelCache.clear();
    maximumLogLevel = std::max(maximumLogLevel, possibleNewMaxLogLevel);
}

}  // namespace Detail

void addDebugSpec(const char* spec) {
#ifdef CLOCK_MONOTONIC
    if (!Detail::initTime) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        Detail::initTime = ts.tv_sec*1000000000UL + ts.tv_nsec; }
#endif

    // Validate @spec.
    bool ok = false;
    long maxLogLevelInSpec = 0;
    for (auto* pattern = strchr(spec, ':'); pattern; pattern = strchr(pattern, ':')) {
        ok = true;
        long level = strtol(pattern + 1, const_cast<char**>(&pattern), 10);
        if (*pattern && *pattern != ',') {
            ok = false;
            break; }
        maxLogLevelInSpec = std::max(maxLogLevelInSpec, level);
    }

    if (!ok) {
        std::cerr << "Invalid debug trace spec '" << spec << "'" << std::endl;
        return; }

#ifdef MULTITHREAD
    static std::mutex lock;
    std::lock_guard<std::mutex> acquire(lock);
#endif

    Detail::debugSpecs.push_back(spec);
    Detail::invalidateCaches(maxLogLevelInSpec);
}

void increaseVerbosity() {
#ifdef MULTITHREAD
    static std::mutex lock;
    std::lock_guard<std::mutex> acquire(lock);
#endif
#ifdef CLOCK_MONOTONIC
    if (!Detail::initTime) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        Detail::initTime = ts.tv_sec*1000000000UL + ts.tv_nsec; }
#endif

    Detail::verbosity++;
    Detail::invalidateCaches(Detail::verbosity - 1);
}

}  // namespace Log
