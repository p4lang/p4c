/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_ASM_RVALUE_REFERENCE_WRAPPER_H_
#define BACKENDS_TOFINO_BF_ASM_RVALUE_REFERENCE_WRAPPER_H_

template <class T>
class rvalue_reference_wrapper {
    T *ref;

 public:
    typedef T type;
    rvalue_reference_wrapper(T &&r) : ref(&r) {}  // NOLINT(runtime/explicit)
    template <class U>
    rvalue_reference_wrapper(U &&r) : ref(&r) {}  // NOLINT(runtime/explicit)
    T &&get() { return std::move(*ref); }
};

#endif /* BACKENDS_TOFINO_BF_ASM_RVALUE_REFERENCE_WRAPPER_H_ */
