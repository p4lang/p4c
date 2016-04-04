#include <gc/gc_cpp.h>
#include <new>
#include "log.h"
#include "gc.h"

/* glibc++ requires defining global delete with this exception spec to avoid warnings.
 * If it's not defined, probably not using glibc++ and don't need anything */
#ifndef _GLIBCXX_USE_NOEXCEPT
#define _GLIBCXX_USE_NOEXCEPT _NOEXCEPT
#endif

void *operator new(std::size_t size) { return ::operator new(size, UseGC, 0, 0); }
void *operator new[](std::size_t size) { return ::operator new(size, UseGC, 0, 0); }
void operator delete(void *p) _GLIBCXX_USE_NOEXCEPT { return gc::operator delete(p); }
void operator delete[](void *p) _GLIBCXX_USE_NOEXCEPT { return gc::operator delete(p); }

extern "C" void (*GC_start_call_back)(void);
extern "C" size_t GC_get_heap_size(void);
extern "C" int GC_print_stats;

static void gc_callback() {
    LOG1("****** GC called ****** (heap size " << GC_get_heap_size() << ")");
    GC_print_stats = LOGGING(2) ? 1 : 0;  // unfortunately goes directly to stderr!
}

void setup_gc_logging() {
    GC_print_stats = LOGGING(2) ? 1 : 0;  // unfortunately goes directly to stderr!
    GC_start_call_back = gc_callback;
}
