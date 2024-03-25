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
#endif /* HAVE_LIBGC */
#include <sys/mman.h>
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include <cstddef>
#include <cstring>
#include <new>

#include "backtrace_exception.h"
#include "cstring.h"
#include "gc.h"
#include "log.h"
#include "n4.h"

// One can disable the GC, e.g., to run under Valgrind, by editing config.h or toggling
// -DENABLE_GC=OFF in CMake.
#if HAVE_LIBGC
static bool done_init, started_init;
// emergency pool to allow a few extra allocations after a bad_alloc is thrown so we
// can generate reasonable errors, a stack trace, etc
static char emergency_pool[16 * 1024];
static char *emergency_ptr;

static alloc_trace_cb_t trace_cb;
static bool tracing = false;
#if !HAVE_EXECINFO_H
#define backtrace(BUFFER, SIZE) memset(BUFFER, 0, (SIZE) * sizeof(void *))
#endif
#define TRACE_ALLOC(size)                        \
    if (trace_cb.fn && !tracing) {               \
        void *buffer[ALLOC_TRACE_DEPTH];         \
        tracing = true;                          \
        backtrace(buffer, ALLOC_TRACE_DEPTH);    \
        trace_cb.fn(trace_cb.arg, buffer, size); \
        tracing = false;                         \
    }

static void maybe_initialize_gc() {
    if (!done_init) {
        started_init = true;
        GC_INIT();
        done_init = true;
    }
}

void *operator new(std::size_t size) {
    TRACE_ALLOC(size)

    maybe_initialize_gc();
    auto *rv = ::operator new(size, UseGC, 0, 0);
    if (!rv && emergency_ptr && emergency_ptr + size < emergency_pool + sizeof(emergency_pool)) {
        rv = emergency_ptr;
        size = (size + 15) / 16 * 16;  // align to 16 bytes
        emergency_ptr += size;
    }
    if (!rv) {
        if (!emergency_ptr) emergency_ptr = emergency_pool;
        throw backtrace_exception<std::bad_alloc>();
    }
    return rv;
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
    TRACE_ALLOC(size)

    maybe_initialize_gc();
    // FIXME: Call nothrow operator new from libgc with suitable new libgc
    // versions
    auto *rv = ::operator new(size, UseGC, 0, 0);
    if (!rv && emergency_ptr && emergency_ptr + size < emergency_pool + sizeof(emergency_pool)) {
        rv = emergency_ptr;
        size = (size + 15) / 16 * 16;  // align to 16 bytes
        emergency_ptr += size;
    }
    if (!rv && !emergency_ptr) emergency_ptr = emergency_pool;
    return rv;
}

void *operator new(std::size_t size, std::align_val_t alignment) {
    TRACE_ALLOC(size)

    if (static_cast<size_t>(alignment) < sizeof(void *))
        alignment = std::align_val_t(sizeof(void *));

    maybe_initialize_gc();

    void *rv = nullptr;
    if (GC_posix_memalign(&rv, static_cast<size_t>(alignment), size) != 0)
        throw backtrace_exception<std::bad_alloc>();

    return rv;
}

void *operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept {
    TRACE_ALLOC(size)

    if (static_cast<size_t>(alignment) < sizeof(void *))
        alignment = std::align_val_t(sizeof(void *));

    maybe_initialize_gc();

    void *rv = nullptr;
    if (GC_posix_memalign(&rv, static_cast<size_t>(alignment), size) != 0) rv = nullptr;

    return rv;
}

alloc_trace_cb_t set_alloc_trace(alloc_trace_cb_t cb) {
    alloc_trace_cb_t old = trace_cb;
    trace_cb = cb;
    return old;
}

alloc_trace_cb_t set_alloc_trace(void (*fn)(void *, void **, size_t), void *arg) {
    alloc_trace_cb_t old = trace_cb;
    trace_cb.fn = fn;
    trace_cb.arg = arg;
    return old;
}

// clang-format off
void operator delete(void* p) noexcept {
    if (p >= emergency_pool && p < emergency_pool + sizeof(emergency_pool)) {
        return;
    }
    gc::operator delete(p);
}

void operator delete(void* p, std::size_t /*size*/) noexcept {
    if (p >= emergency_pool && p < emergency_pool + sizeof(emergency_pool)) {
        return;
    }
    gc::operator delete(p);
}

// FIXME: We can get rid of GC_base here with suitable new libgc
// See https://github.com/ivmai/bdwgc/issues/589 for more info
void operator delete(void *p, std::align_val_t) noexcept {
    gc::operator delete(GC_base(p));
}

void operator delete(void *p, std::size_t, std::align_val_t) noexcept {
    gc::operator delete(GC_base(p));
}
// clang-format on

void *operator new[](std::size_t size) { return ::operator new(size); }
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void *operator new[](std::size_t size, const std::nothrow_t &tag) noexcept {
    return ::operator new(size, tag);
}
void *operator new[](std::size_t size, std::align_val_t alignment,
                     const std::nothrow_t &tag) noexcept {
    return ::operator new(size, alignment, tag);
}

void operator delete[](void *p) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::align_val_t) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t, std::align_val_t) noexcept { ::operator delete(p); }

namespace {

constexpr size_t headerSize = 16;
using max_align_t = std::max_align_t;
static_assert(headerSize >= alignof(max_align_t), "mmap header size not large enough");
static_assert(headerSize >= sizeof(size_t), "mmap header size not large enough");

void *data_to_header(void *ptr) { return static_cast<char *>(ptr) - headerSize; }

void *header_to_data(void *ptr) { return static_cast<char *>(ptr) + headerSize; }

size_t raw_size(void *ptr) {
    size_t size;
    memcpy(static_cast<void *>(&size), data_to_header(ptr), sizeof(size));
    return size;
}

void *raw_alloc(size_t size) {
    void *ptr =
        mmap(0, size + headerSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(ptr, static_cast<void *>(&size), sizeof(size));
    return header_to_data(ptr);
}

void raw_free(void *ptr) {
    size_t size = raw_size(ptr);
    munmap(data_to_header(ptr), size + headerSize);
}

}  // namespace

void *realloc(void *ptr, size_t size) {
    if (!done_init) {
        if (started_init) {
            // called from within GC_INIT, so we can't call it again.  Fall back to using
            // mmap.
            void *rv = raw_alloc(size);
            if (ptr) {
                size_t max = raw_size(ptr);
                memcpy(rv, ptr, max < size ? max : size);
                raw_free(ptr);
            }
            return rv;
        } else {
            started_init = true;
            GC_INIT();
            done_init = true;
        }
    }
    TRACE_ALLOC(size)
    if (ptr) {
        if (GC_is_heap_ptr(ptr)) return GC_realloc(ptr, size);
        size_t max = raw_size(ptr);
        void *rv = GC_malloc(size);
        memcpy(rv, ptr, max < size ? max : size);
        raw_free(ptr);
        return rv;
    } else {
        return GC_malloc(size);
    }
}
// IMPORTANT: do not simplify this to realloc(nullptr, size)
// As it could be optimized to malloc(size) call causing
// infinite loops
void *malloc(size_t size) {
    if (!done_init) return realloc(nullptr, size);

    TRACE_ALLOC(size)
    return GC_malloc(size);
}
void free(void *ptr) {
    if (done_init && GC_is_heap_ptr(ptr)) GC_free(ptr);
}
void *calloc(size_t size, size_t elsize) {
    size *= elsize;
    void *rv = malloc(size);
    if (rv) memset(rv, 0, size);
    return rv;
}
int posix_memalign(void **memptr, size_t alignment, size_t size)
#ifndef __APPLE__
    noexcept
#endif
{
    maybe_initialize_gc();

    TRACE_ALLOC(size)
    return GC_posix_memalign(memptr, alignment, size);
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
#endif                                       /* HAVE_GC_PRINT_STATS */
}

#endif /* HAVE_LIBGC */

void setup_gc_logging() {
#if HAVE_LIBGC
    GC_set_start_callback(gc_callback);
    reset_gc_logging();
    Log::Detail::addInvalidateCallback(reset_gc_logging);
    GC_set_warn_proc(&silent);
#endif /* HAVE_LIBGC */
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
