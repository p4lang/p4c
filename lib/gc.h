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

#ifndef LIB_GC_H_
#define LIB_GC_H_

#include <cstddef>

#define ALLOC_TRACE_DEPTH 5

void setup_gc_logging();
size_t gc_mem_inuse(size_t *max = 0);  // trigger GC, return inuse after

struct alloc_trace_cb_t {
    void (*fn)(void *arg, void **pc, size_t sz);
    void *arg;
};
alloc_trace_cb_t set_alloc_trace(alloc_trace_cb_t cb);
alloc_trace_cb_t set_alloc_trace(void (*fn)(void *arg, void **pc, size_t sz), void *arg);

#endif /* LIB_GC_H_ */
