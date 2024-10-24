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

#ifndef BF_P4C_MIDEND_SIMPLIFY_KEY_POLICY_H_
#define BF_P4C_MIDEND_SIMPLIFY_KEY_POLICY_H_

#include "midend/simplifyKey.h"

namespace BFN {

class IsPhase0 : public P4::KeyIsSimple {
 public:
    IsPhase0() {}

    bool isSimple(const IR::Expression *, const Visitor::Context *ctxt) override {
        while (true) {
            if (ctxt == nullptr) return false;
            auto *n = ctxt->node;
            if (n->is<IR::P4Program>()) return false;
            if (auto table = n->to<IR::P4Table>()) {
                auto annot = table->getAnnotations();
                if (annot->getSingle("phase0"_cs)) {
                    return true;
                }
                return false;
            }
            ctxt = ctxt->parent;
        }
        return false;
    }
};

class IsSlice : public P4::KeyIsSimple {
 public:
    IsSlice() {}

    bool isSimple(const IR::Expression *expr, const Visitor::Context *) override {
        auto slice = expr->to<IR::Slice>();
        if (!slice) return false;
        auto *e = slice->e0;
        while (e->is<IR::Member>()) e = e->to<IR::Member>()->expr;
        return e->to<IR::PathExpression>() != nullptr;
    }
};

// Extend P4::IsMask to handle Slices
// E.g. hdr.ethernet.smac[15:0] & 0x1f1f
// This is a valid key which contains a field slice and a mask
// Frontend IsMask only handles non sliced fields on masks.
class IsSliceMask : public P4::IsMask {
    void postorder(const IR::Slice *) override {}
    void postorder(const IR::Constant *) override {
        // Only skip constants under a Slice
        if (!getParent<IR::Slice>()) simple = false;
    }

 public:
    IsSliceMask() { setName("IsSlicMask"); }

    bool isSimple(const IR::Expression *expr, const Visitor::Context *ctxt) override {
        return P4::IsMask::isSimple(expr, ctxt);
    }
};

class KeyIsSimple {
 public:
    static P4::KeyIsSimple *getPolicy(P4::TypeMap &typeMap) {
        return new P4::OrPolicy(
            new P4::OrPolicy(new P4::OrPolicy(new P4::IsValid(&typeMap), new P4::IsMask()),
                             new BFN::IsPhase0()),
            new P4::OrPolicy(new BFN::IsSlice(), new BFN::IsSliceMask()));
    }
};

}  // namespace BFN

#endif /* BF_P4C_MIDEND_SIMPLIFY_KEY_POLICY_H_ */
