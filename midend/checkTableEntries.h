/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MIDEND_CHECKTABLEENTRIES_H_
#define MIDEND_CHECKTABLEENTRIES_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

class CheckTableEntries : public Inspector {
    bool genError;  // if true, generate errors for duplicates rather than just warnings
    bool preorder(const IR::P4Table *);
    bool preorder(const IR::P4Parser *) { return false; }
    bool preorder(const IR::Statement *) { return false; }
    void get_mask_val(const IR::Expression *, big_int &mask, big_int &val);
    bool ternary_covers(const IR::Expression *k1, const IR::Expression *k2);

 public:
    explicit CheckTableEntries(bool err = false) : genError(err) {}
};

}  // namespace P4

#endif /* MIDEND_CHECKTABLEENTRIES_H_ */
