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

#include "bf-p4c/parde/mirror/const_mirror_session_opt.h"
#include "ir/ir.h"

IR::Node* ConstMirrorSessionOpt::preorder(IR::BFN::Digest* d) {
    prune();
    if (d->name != "mirror")
        return d;
    for (auto fl : d->fieldLists) {
        ERROR_CHECK(fl->sources.size() != 0, "%1%: Expected one or more fields", fl);
        auto arg = fl->sources.at(0);
        if (auto cst = arg->to<IR::Constant>()) {
            if (cst->value != 0)
                continue;
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
IR::Node* ConstMirrorSessionOpt::preorder(IR::TempVar* tv) {
    if (!tv->deparsed_zero)
        return tv;
    if (tv->type != IR::Type_Bits::get(10))
        return tv;
    auto *new_tv = new IR::TempVar(IR::Type_Bits::get(8));
    new_tv->name = tv->name;
    new_tv->srcInfo = tv->srcInfo;
    new_tv->deparsed_zero = true;
    return new_tv;
}
