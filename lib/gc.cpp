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
#include <unistd.h>
#include <new>
#include "log.h"
#include "gc.h"
#include "cstring.h"
#include "n4.h"
#include "backtrace.h"

/* glibc++ requires defining global delete with this exception spec to avoid warnings.
 * If it's not defined, probably not using glibc++ and don't need anything */
#ifndef _GLIBCXX_USE_NOEXCEPT
#define _GLIBCXX_USE_NOEXCEPT _NOEXCEPT
#endif

static bool done_init, started_init;
// emergency pool to allow a few extra allocations after a bad_alloc is thrown so we
// can generate reasonable errors, a stack trace, etc
static char emergency_pool[16*1024];
static char *emergency_ptr;

// One can disable the GC, e.g., to run under Valgrind, by editing config.h
#if HAVE_LIBGC
void *operator new(std::size_t size) {
    /* DANGER -- on OSX, can't safely call the garbage collector allocation
     * routines from a static global constructor without manually initializing
     * it first.  Since we have global constructors that want to allocate
     * memory, we need to force initialization */
    if (!done_init) {
        started_init = true;
        GC_INIT();
        done_init = true; }
    auto *rv = ::operator new(size, UseGC, 0, 0);
    if (!rv && emergency_ptr && emergency_ptr + size < emergency_pool + sizeof(emergency_pool)) {
        rv = emergency_ptr;
        size += -size & 0xf;  // align to 16 bytes
        emergency_ptr += size; }
    if (!rv) {
        if (!emergency_ptr) emergency_ptr = emergency_pool;
        throw backtrace_exception<std::bad_alloc>(); }
    return rv;
}
void operator delete(void *p) _GLIBCXX_USE_NOEXCEPT {
    if (p >= emergency_pool && p < emergency_pool + sizeof(emergency_pool))
        return;
    gc::operator delete(p);
}

void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void *p) _GLIBCXX_USE_NOEXCEPT { ::operator delete(p); }

void *realloc(void *ptr, size_t size) {
    if (!done_init) {
        if (started_init) {
            // called from within GC_INIT, so we can't call it again.  Fall back to using
            // sbrk and let it leak.
            size = (size + 0xf) & ~0xf;
            void *rv = sbrk(size);
            if (ptr) {
                size_t max = reinterpret_cast<char *>(rv) - reinterpret_cast<char *>(ptr);
                memcpy(rv, ptr, max < size ? max : size); }
            return rv;
        } else {
            started_init = true;
            GC_INIT();
            done_init = true; } }
    if (ptr) {
        if (GC_is_heap_ptr(ptr))
            return GC_realloc(ptr, size);
        size_t max = reinterpret_cast<char *>(sbrk(0)) - reinterpret_cast<char *>(ptr);
        void *rv = GC_malloc(size);
        memcpy(rv, ptr, max < size ? max : size);
        return rv;
    } else {
        return GC_malloc(size);
    }
}
void *malloc(size_t size) { return realloc(nullptr, size); }
void free(void *ptr) {
    if (done_init && GC_is_heap_ptr(ptr))
        GC_free(ptr);
}
void *calloc(size_t size, size_t elsize) {
    size *= elsize;
    void *rv = malloc(size);
    if (rv) memset(rv, 0, size);
    return rv;
}

#if HAVE_GC_PRINT_STATS
/* GC_print_stats is not exported as an API symbol and cannot be used on some systems */
extern "C" int GC_print_stats;
#endif /* HAVE_GC_PRINT_STATS */

static int gc_logging_level;

static void gc_callback() {
    if (gc_logging_level >= 1) {
        std::clog << "****** GC called ****** (heap size " << n4(GC_get_heap_size()) << ")";
        size_t count, size = cstring::cache_size(count);
        std::clog << " cstring cache size " << n4(size) << " (count " << n4(count) << ")"
                  << std::endl;
    }
}

void silent(char *, GC_word) {}

void reset_gc_logging() {
    gc_logging_level = Log::Detail::fileLogLevel(__FILE__);
#if HAVE_GC_PRINT_STATS
    GC_print_stats = gc_logging_level >= 2;  // unfortunately goes directly to stderr!
#endif /* HAVE_GC_PRINT_STATS */
}

#endif  /* HAVE_LIBGC */

void setup_gc_logging() {
#if HAVE_LIBGC
    GC_set_start_callback(gc_callback);
    reset_gc_logging();
    Log::Detail::addInvalidateCallback(reset_gc_logging);
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
