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

#ifndef _BF_P4C_LIB_AUTOCLONE_H_
#define _BF_P4C_LIB_AUTOCLONE_H_

#include <memory>

/** Adaptor/wrapper class for std::unique_ptr that allows copying by cloning the
 *  pointed at object.  An autoclone_ptr can be used when you want to make a pointer
 *  field act like a value field -- any copy of the containing object will deep-copy
 *  all the autoclone_ptr-referenced objects avoid any accidental shallow copy aliasing
 *  or slicing (particularly useful for dynamic classes).
 */
template <class T, class D = std::default_delete<T>>
class autoclone_ptr : public std::unique_ptr<T, D> {
 public:
    autoclone_ptr() = default;
    autoclone_ptr(autoclone_ptr &&) = default;
    autoclone_ptr &operator=(autoclone_ptr &&) = default;
    autoclone_ptr(const autoclone_ptr &a) : std::unique_ptr<T, D>(a ? a->clone() : nullptr) {}
    autoclone_ptr &operator=(const autoclone_ptr &a) {
        auto *t = a.get();
        this->reset(t ? t->clone() : nullptr);
        return *this;
    }
};

#endif /* _BF_P4C_LIB_AUTOCLONE_H_ */
