/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_LIB_POINTER_WRAPPER_H_
#define BF_P4C_LIB_POINTER_WRAPPER_H_

/* a pointer that inherits const-ness from its context (so is a const T * if it is itself
 * const and is a (non-const) T * if it is not const.  It is incidentally non-copyable or movable,
 * as that would result in losing the const-ness. */
template <class T>
class pointer_wrap {
    T *ptr;

 public:
    pointer_wrap() : ptr(nullptr) {}
    pointer_wrap(pointer_wrap &a) : ptr(a.ptr) {}  // NOLINT(runtime/explicit)
    pointer_wrap(T *p) : ptr(p) {}                 // NOLINT(runtime/explicit)
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
