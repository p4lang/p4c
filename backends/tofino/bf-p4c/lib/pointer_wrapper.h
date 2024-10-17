/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_LIB_POINTER_WRAPPER_H_
#define BF_P4C_LIB_POINTER_WRAPPER_H_

/* a pointer that inherits const-ness from its context (so is a const T * if it is itself
 * const and is a (non-const) T * if it is not const.  It is incidentally non-copyable or movable,
 * as that would result in losing the const-ness. */
template<class T> class pointer_wrap {
    T *ptr;
 public:
    pointer_wrap() : ptr(nullptr) {}
    pointer_wrap(pointer_wrap &a) : ptr(a.ptr) {}       // NOLINT(runtime/explicit)
    pointer_wrap(T *p) : ptr(p) {}                      // NOLINT(runtime/explicit)
    pointer_wrap(const pointer_wrap &) = delete;
    pointer_wrap(pointer_wrap &&) = delete;
    explicit operator bool() { return ptr != nullptr; }
    T *operator=(T *p) { return ptr = p; }
    T *operator->() { return ptr; }
    const T *operator->() const { return ptr; }
    T &operator*() { return *ptr; }
    const T &operator*() const { return *ptr; }
    operator T *() { return ptr; }
    operator const T *() const { return ptr; }
};

#endif /* BF_P4C_LIB_POINTER_WRAPPER_H_ */
