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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_MAU_VISITOR_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_MAU_VISITOR_H_

#include "ir/ir.h"

using namespace P4;

/* MAU-specific visitor subclasses that automatically prune off traversal of non-MAU
 * parts of the tree */

class MauInspector : public Inspector {
    /// for traversing backend IR
    bool preorder(const IR::BFN::AbstractParser *) override { return false; }
    bool preorder(const IR::BFN::AbstractDeparser *) override { return false; }
    /// for traversing midend IR
    bool preorder(const IR::P4Parser *) override { return false; }
    bool preorder(const IR::BFN::TnaParser *) override { return false; }
    bool preorder(const IR::BFN::TnaDeparser *) override { return false; }
};

class MauTableInspector : public MauInspector {
    // skip subtrees (of tables) that never contain tables
    bool preorder(const IR::MAU::Action *) override { return false; }
    bool preorder(const IR::Expression *) override { return false; }
};

class MauModifier : public Modifier {
    /// for traversing backend IR
    bool preorder(IR::BFN::AbstractParser *) override { return false; }
    bool preorder(IR::BFN::AbstractDeparser *) override { return false; }
    /// for traversing midend IR
    bool preorder(IR::P4Parser *) override { return false; }
    bool preorder(IR::BFN::TnaParser *) override { return false; }
    bool preorder(IR::BFN::TnaDeparser *) override { return false; }
};

class MauTransform : public Transform {
    /// for traversing backend IR
    IR::Node *preorder(IR::BFN::AbstractParser *p) override {
        prune();
        return p;
    }
    IR::Node *preorder(IR::BFN::AbstractDeparser *d) override {
        prune();
        return d;
    }
    /// for traversing midend IR
    IR::Node *preorder(IR::P4Parser *p) override {
        prune();
        return p;
    }
    IR::Node *preorder(IR::BFN::TnaParser *p) override {
        prune();
        return p;
    }
    IR::Node *preorder(IR::BFN::TnaDeparser *p) override {
        prune();
        return p;
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_MAU_VISITOR_H_ */
