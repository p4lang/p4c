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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_IR_UTILS_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_IR_UTILS_H_

#include "ir/ir.h"

using namespace P4;

IR::Member *gen_fieldref(const IR::HeaderOrMetadata *hdr, cstring field);

const IR::HeaderOrMetadata *getMetadataType(const IR::BFN::Pipe *pipe, cstring typeName);

bool isSigned(const IR::Type *);

// probably belongs in ir/ir.h or ir/node.h...
template <class T>
inline T *clone_update(const T *&ptr) {
    T *rv = ptr->clone();
    ptr = rv;
    return rv;
}

uint64_t bitMask(unsigned size);
big_int bigBitMask(int size);

// FIXME -- move to open source code (ir/pass_manager.h probably)
template <class BT>
class CatchBacktrack : public Backtrack {
    std::function<void(BT *)> fn;
    bool backtrack(trigger &trig) override {
        if (auto t = dynamic_cast<BT *>(&trig)) {
            fn(t);
            return true;
        } else {
            return false;
        }
    }
    // pass does nothing
    const IR::Node *apply_visitor(const IR::Node *n, const char * = 0) override { return n; }

 public:
    explicit CatchBacktrack(std::function<void(BT *)> f) : fn(f) {}
    explicit CatchBacktrack(std::function<void()> f) : fn([f](BT *) { f(); }) {}
};

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_IR_UTILS_H_ */
