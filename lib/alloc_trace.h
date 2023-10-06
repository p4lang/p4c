/*
Copyright 2023-present Intel

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

#ifndef LIB_ALLOC_TRACE_H_
#define LIB_ALLOC_TRACE_H_

#include <map>
#include <ostream>

#include "exceptions.h"
#include "gc.h"

class AllocTrace {
    struct backtrace {
        void *trace[ALLOC_TRACE_DEPTH];

        backtrace(const backtrace &) = default;
        explicit backtrace(void **bt) { memcpy(trace, bt, sizeof(trace)); }
        bool operator<(const backtrace &a) const {
            for (int i = 0; i < ALLOC_TRACE_DEPTH; ++i)
                if (trace[i] != a.trace[i]) return trace[i] < a.trace[i];
            return false;
        }
        bool operator==(const backtrace &a) const {
            for (int i = 0; i < ALLOC_TRACE_DEPTH; ++i)
                if (trace[i] != a.trace[i]) return false;
            return true;
        }
    };
    std::map<backtrace, std::map<size_t, int>> data;
    void count(void **, size_t);
    static void callback(void *t, void **bt, size_t sz) {
        static_cast<AllocTrace *>(t)->count(bt, sz);
    }

 public:
    void clear() { data.clear(); }
#if HAVE_LIBGC
    alloc_trace_cb_t start() { return set_alloc_trace(callback, this); }
    void stop(alloc_trace_cb_t old) {
        auto tmp = set_alloc_trace(old);
        BUG_CHECK(tmp.fn == callback && tmp.arg == this, "AllocTrace stopped when not running");
    }
#else
    alloc_trace_cb_t start() {
        BUG("Can't trace allocations without garbage collection");
        return alloc_trace_cb_t{};
    }
    void stop(alloc_trace_cb_t) {}
#endif
    friend std::ostream &operator<<(std::ostream &, const AllocTrace &);
};

class PauseTrace {
#if HAVE_LIBGC
    alloc_trace_cb_t hold;
    PauseTrace(const PauseTrace &) = delete;

 public:
    PauseTrace() { hold = set_alloc_trace(nullptr, nullptr); }
    ~PauseTrace() { set_alloc_trace(hold); }
#endif
};

#endif /* LIB_ALLOC_TRACE_H_ */
