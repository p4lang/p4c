/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
