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
#include <gc/gc_mark.h>
#endif  /* HAVE_LIBGC */
#include <new>
#include "log.h"
#include "gc.h"
#include "cstring.h"

/* glibc++ requires defining global delete with this exception spec to avoid warnings.
 * If it's not defined, probably not using glibc++ and don't need anything */
#ifndef _GLIBCXX_USE_NOEXCEPT
#define _GLIBCXX_USE_NOEXCEPT _NOEXCEPT
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
/* GC_print_stats is not an official GC API and is not available on
 * Windows builds
 */
#define NO_GC_PRINT_STATS
#endif

// One can disable the GC, e.g., to run under Valgrind, by editing config.h
#if HAVE_LIBGC
static bool done_init;
void *operator new(std::size_t size) {
    /* DANGER -- on OSX, can't safely call the garbage collector allocation
     * routines from a static global constructor without manually initializing
     * it first.  Since we have global constructors that want to allocate
     * memory, we need to force initialization */
    if (!done_init) {
        GC_INIT();
        done_init = true; }
    return ::operator new(size, UseGC, 0, 0);
}
void *operator new[](std::size_t size) {
    if (!done_init) {
        GC_INIT();
        done_init = true; }
    return ::operator new(size, UseGC, 0, 0);
}
void operator delete(void *p) _GLIBCXX_USE_NOEXCEPT { return gc::operator delete(p); }
void operator delete[](void *p) _GLIBCXX_USE_NOEXCEPT { return gc::operator delete(p); }

#ifndef NO_GC_PRINT_STATS
extern "C" int GC_print_stats;
#endif

static void gc_callback() {
    size_t count;
    LOG1("****** GC called ****** (heap size " << GC_get_heap_size() << ")");
    LOG2("cstring cache size " << cstring::cache_size(count) << " (count=" << count << ")");
#ifndef NO_GC_PRINT_STATS
    /* GC_print_stats is not exported as an API symbol and cannot be set on windows builds */
    GC_print_stats = LOGGING(2) ? 1 : 0;  // unfortunately goes directly to stderr!
#endif /* ! NO_GC_PRINT_STATS */
}

void silent(char *, GC_word) {}
#endif  /* HAVE_LIBGC */

void setup_gc_logging() {
#if HAVE_LIBGC
#ifndef NO_GC_PRINT_STATS
    GC_print_stats = LOGGING(2) ? 1 : 0;
#endif /* ! NO_GC_PRINT_STATS */
    GC_set_start_callback(gc_callback);
    GC_set_warn_proc(&silent);
#endif  /* HAVE_LIBGC */
}

size_t gc_mem_inuse(size_t *max) {
#if HAVE_LIBGC
    GC_word heapsize, heapfree;
    GC_gcollect();
    GC_get_heap_usage_safe(&heapsize, &heapfree, 0, 0, 0);
    if (max) *max = heapsize;
    return heapsize - heapfree;
#else
    if (max) *max = 0;
    return 0;
#endif
}
