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

#include "bf-p4c/parde/mirror/const_mirror_session_opt.h"

#include "ir/ir.h"

IR::Node *ConstMirrorSessionOpt::preorder(IR::BFN::Digest *d) {
    prune();
    if (d->name != "mirror") return d;
    for (auto fl : d->fieldLists) {
        ERROR_CHECK(fl->sources.size() != 0, "%1%: Expected one or more fields", fl);
        auto arg = fl->sources.at(0);
        if (auto cst = arg->to<IR::Constant>()) {
            if (cst->value != 0) continue;
            visit(arg);
        }
    }
    return d;
}

/* The optimization is as follows:
 * 1. If the digest is not a mirror digest, do nothing.
 * 2. If the digest has no field lists, do nothing.
 * 3. If the first element in the field list has a constant source, and the
 *    constant is 0, and the constant is 10b wide, then replace the constant
 *    with a constant 0 that is 8b wide.
 * Effectively, instead of emitting a 10b mirror_id of 0, we emit an 8b
 * mirror_id of 0. The hardware concatenates the 8b mirror_id with itself
 * to generate a 16b input to the mirror table and the overall effect is
 * the same. However, the 8b mirror_id is more efficient because we
 * can reuse B0, thus save a container.
 */
IR::Node *ConstMirrorSessionOpt::preorder(IR::TempVar *tv) {
    if (!tv->deparsed_zero) return tv;
    if (tv->type != IR::Type_Bits::get(10)) return tv;
    auto *new_tv = new IR::TempVar(IR::Type_Bits::get(8));
    new_tv->name = tv->name;
    new_tv->srcInfo = tv->srcInfo;
    new_tv->deparsed_zero = true;
    return new_tv;
}
