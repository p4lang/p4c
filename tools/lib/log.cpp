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

#include "config.h"
#if HAVE_LIBGC
#include <gc/gc_cpp.h>
#define NOGC_ARGS (NoGC, 0, 0)
#else
#define NOGC_ARGS
#endif /* HAVE_LIBGC */

#include "log.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
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

struct LevelAndOutput {
    int                 level = -1;
    std::ostream        *out = nullptr;
};

// The first level cache for fileLogLevel() - the most recent result returned.
static const char* mostRecentFile = nullptr;
static LevelAndOutput *mostRecentInfo = nullptr;

// The second level cache for fileLogLevel(), mapping filenames to log levels.
static std::unordered_map<const void*, LevelAndOutput> logLevelCache;

// All log levels manually specified by the user.
static std::vector<std::string> debugSpecs;

// Each logfile will only be opened once, and will close when we exit.
static std::unordered_map<std::string, std::unique_ptr<std::ostream>> logfiles;

int OutputLogPrefix::ostream_xalloc = -1;
void OutputLogPrefix::setup_ostream_xalloc(std::ostream &out) {
    if (ostream_xalloc < 0) {
#ifdef MULTITHREAD
        static std::mutex lock;
        std::lock_guard<std::mutex> acquire(lock);
        if (ostream_xalloc < 0)
#endif  // MULTITHREAD
            ostream_xalloc = out.xalloc();
    }
}

#ifdef MULTITHREAD
struct OutputLogPrefix::lock_t {
    int         refcnt = 1;
    std::mutex  theMutex;

    void lock() { theMutex.lock(); }
    void unlock() { theMutex.unlock(); }
    static void cleanup(std::ios_base::event event, std::ios_base &out, int index) {
        if (event == std::ios_base::erase_event) {
            auto *p = static_cast<lock_t *>(out.pword(index));
            if (p && --p->refcnt <= 0) delete p;
        } else if (event == std::ios_base::copyfmt_event) {
            auto *p = static_cast<lock_t *>(out.pword(index));
            if (p) p->refcnt++;
        }
    }
};
#endif  // MULTITHREAD

OutputLogPrefix::~OutputLogPrefix() {
#ifdef MULTITHREAD
    if (lock) lock->unlock();
#endif  // MULTITHREAD
}

void OutputLogPrefix::indent(std::ostream &out) {
    setup_ostream_xalloc(out);
    if (int pfx = out.iword(ostream_xalloc))
        out << std::setw(pfx) << ':';
    out << indent_t::getindent(out);
}

std::ostream& operator<<(std::ostream& out, const OutputLogPrefix& pfx) {
    std::stringstream tmp;
#ifdef CLOCK_MONOTONIC
    if (LOGGING(2)) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t t = ts.tv_sec*1000000000UL + ts.tv_nsec - Log::Detail::initTime;
        t /= 1000000UL;  // millisec
        tmp << t/1000 << '.' << std::setw(3) << std::setfill('0') << t%1000 << ':'
            << std::setfill(' '); }
#endif
    if (LOGGING(1)) {
        const char *s = strrchr(pfx.fn, '/');
        const char *e = strrchr(pfx.fn, '.');
        s = s ? s + 1 : pfx.fn;
        if (e && e > s)
            tmp.write(s, e-s);
        else
            tmp << s;
        tmp << ':' << pfx.level << ':'; }
    pfx.setup_ostream_xalloc(out);
#ifdef MULTITHREAD
    if (!(pfx.lock = static_cast<OutputLogPrefix::lock_t *>(out.pword(pfx.ostream_xalloc)))) {
        static std::mutex lock;
        std::lock_guard<std::mutex> acquire(lock);
        if (!(pfx.lock = static_cast<OutputLogPrefix::lock_t *>(out.pword(pfx.ostream_xalloc)))) {
            out.pword(pfx.ostream_xalloc) = pfx.lock = new NOGC_ARGS OutputLogPrefix::lock_t;
            out.register_callback(OutputLogPrefix::lock_t::cleanup, pfx.ostream_xalloc); } }
    pfx.lock->lock();
#endif  // MULTITHREAD
    if (tmp.str().size() > 0) {
        out.iword(OutputLogPrefix::ostream_xalloc) = tmp.str().size();
        out << tmp.str(); }
    out << indent_t::getindent(out);
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
            if (!strcmp(name, ".cpp") || !strcmp(name, ".h")) return true;
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
        if (!pbackup && *pattern != '*') return false;
        while (*pattern == '*') {
            ++pattern;
            pbackup = nullptr; }
        if (pattern == pend) return true;
        // FIXME -- does not work for * followed by [ -- matches a literal [ instead.
        while (*name && *name != *pattern) {
            if (pbackup && *name == *pbackup) {
                pattern = pbackup;
                break; }
            name++; }
        if (!*name) return false;
        pbackup = pattern;
    }
}

const char *uncachedFileLogSpec(const char* file) {
    if (auto* startOfFilename = strrchr(file, '/'))
        file = startOfFilename + 1;

    for (auto& spec : debugSpecs)
        for (auto* pattern = spec.c_str(); pattern; pattern = strchr(pattern, ',')) {
            while (*pattern == ',') pattern++;
            if (match(pattern, file))
                if (auto* level = strchr(pattern, ':'))
                    return level + 1; }
    return nullptr;
}

int uncachedFileLogLevel(const char* file) {
    if (auto spec = uncachedFileLogSpec(file))
        return atoi(spec);
    // If there's no matching spec, compute a default from the global verbosity level,
    // except for THIS file
    if (!strcmp(file, __FILE__)) return 0;
    return verbosity > 0 ? verbosity - 1 : 0;
}

LevelAndOutput *cachedFileLogInfo(const char* file) {
#ifdef MULTITHREAD
    static std::mutex lock;
    std::lock_guard<std::mutex> acquire(lock);
#endif  // MULTITHREAD

    // There are two layers of caching here. First, we cache the most recent
    // result we returned, to minimize expensive lookups in tight loops.
    if (mostRecentFile == file)
        return mostRecentInfo;

    // Second, we look up @file in a hash table mapping from pointers to log
    // info. We expect to hit in this cache virtually all the time.
    mostRecentFile = file;
    return mostRecentInfo = &logLevelCache[file];
}

int fileLogLevel(const char* file) {
    auto *info = cachedFileLogInfo(file);
    if (info->level == -1) {
        // This is the slow path. We have to walk @debugSpecs to see if there are any
        // specs that match @file.  There's a race here in that two threads could do this
        // at the same time, but they should get the same result.
        info->level = uncachedFileLogLevel(file); }
    return info->level;
}

std::ostream &uncachedFileLogOutput(const char* file) {
    if (auto spec = uncachedFileLogSpec(file)) {
        while (isdigit(*spec)) ++spec;
        if (*spec == '>') {
            std::ios_base::openmode mode = std::ios_base::out;
            if (*++spec == '>') {
                mode |= std::ios_base::app;
                ++spec; }
            const char *end = strchr(spec, ',');
            if (!end) end = spec + strlen(spec);
            std::string logname(spec, end-spec);
            if (!logfiles.count(logname)) {
                // FIXME: can't emplace a unique_ptr in some versions of gcc -- need
                // explicit reset call.
                logfiles[logname].reset(new std::ofstream(logname, mode)); }
            return *logfiles.at(logname); } }
    return std::clog;
}

std::ostream &fileLogOutput(const char* file) {
    auto *info = cachedFileLogInfo(file);
    if (!info->out) {
#ifdef MULTITHREAD
        static std::mutex lock;
        std::lock_guard<std::mutex> acquire(lock);
#endif  // MULTITHREAD
        if (!info->out) {
            info->out = &uncachedFileLogOutput(file); } }
    return *info->out;
}

void invalidateCaches(int possibleNewMaxLogLevel) {
    mostRecentFile = nullptr;
    mostRecentInfo = nullptr;
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
        if (*pattern && *pattern != ',' && *pattern != '>') {
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
#endif  // MULTITHREAD

    Detail::debugSpecs.push_back(spec);
    Detail::invalidateCaches(maxLogLevelInSpec);
}

void increaseVerbosity() {
#ifdef MULTITHREAD
    static std::mutex lock;
    std::lock_guard<std::mutex> acquire(lock);
#endif  // MULTITHREAD
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
