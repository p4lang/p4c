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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_PARDE_VISITOR_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_PARDE_VISITOR_H_

#include "ir/ir.h"

/**
 * \ingroup parde
 */
class PardeInspector : public Inspector {
    bool preorder(const IR::MAU::Table *) override { return false; }
    bool preorder(const IR::MAU::TableSeq *) override { return false; }
    /// for traversing midend IR
    bool preorder(const IR::BFN::TnaControl *) override { return false; }
};

/**
 * \ingroup parde
 */
class PardeModifier : public Modifier {
    bool preorder(IR::MAU::Table *) override { return false; }
    bool preorder(IR::MAU::TableSeq *) override { return false; }
    /// for traversing midend IR
    bool preorder(IR::BFN::TnaControl *) override { return false; }
};

/**
 * \ingroup parde
 */
class PardeTransform : public Transform {
    IR::MAU::Table *preorder(IR::MAU::Table *t) override {
        prune();
        return t;
    }
    IR::MAU::TableSeq *preorder(IR::MAU::TableSeq *s) override {
        prune();
        return s;
    }
    /// for traversing midend IR
    IR::Node *preorder(IR::BFN::TnaControl *c) override {
        prune();
        return c;
    }
};

/**
 * \ingroup parde
 */
class ParserInspector : public Inspector {
    bool preorder(const IR::BFN::Deparser *) override { return false; }
    bool preorder(const IR::MAU::Table *) override { return false; }
    bool preorder(const IR::MAU::TableSeq *) override { return false; }
    /// for traversing midend IR
    bool preorder(const IR::BFN::TnaDeparser *) override { return false; }
    bool preorder(const IR::BFN::TnaControl *) override { return false; }
};

/**
 * \ingroup parde
 */
class ParserModifier : public Modifier {
    bool preorder(IR::BFN::Deparser *) override { return false; }
    bool preorder(IR::MAU::Table *) override { return false; }
    bool preorder(IR::MAU::TableSeq *) override { return false; }
    /// for traversing midend IR
    bool preorder(IR::BFN::TnaDeparser *) override { return false; }
    bool preorder(IR::BFN::TnaControl *) override { return false; }
};

/**
 * \ingroup parde
 */
class ParserTransform : public Transform {
    IR::BFN::Deparser *preorder(IR::BFN::Deparser *d) override {
        prune();
        return d;
    }
    IR::MAU::Table *preorder(IR::MAU::Table *t) override {
        prune();
        return t;
    }
    IR::MAU::TableSeq *preorder(IR::MAU::TableSeq *s) override {
        prune();
        return s;
    }
    /// for traversing midend IR
    IR::Node *preorder(IR::BFN::TnaDeparser *d) override {
        prune();
        return d;
    }
    IR::Node *preorder(IR::BFN::TnaControl *c) override {
        prune();
        return c;
    }
};

/**
 * \ingroup parde
 */
class DeparserInspector : public Inspector {
    bool preorder(const IR::BFN::AbstractParser *) override { return false; }
    bool preorder(const IR::MAU::Table *) override { return false; }
    bool preorder(const IR::MAU::TableSeq *) override { return false; }
    /// for traversing midend IR
    bool preorder(const IR::BFN::TnaParser *) override { return false; }
    bool preorder(const IR::BFN::TnaControl *) override { return false; }
};

/**
 * \ingroup parde
 */
class DeparserModifier : public Modifier {
    bool preorder(IR::BFN::AbstractParser *) override { return false; }
    bool preorder(IR::MAU::Table *) override { return false; }
    bool preorder(IR::MAU::TableSeq *) override { return false; }
    /// for traversing midend IR
    bool preorder(IR::BFN::TnaParser *) override { return false; }
    bool preorder(IR::BFN::TnaControl *) override { return false; }
};

/**
 * \ingroup parde
 */
class DeparserTransform : public Transform {
    IR::BFN::AbstractParser *preorder(IR::BFN::AbstractParser *p) override {
        prune();
        return p;
    }
    IR::MAU::Table *preorder(IR::MAU::Table *t) override {
        prune();
        return t;
    }
    IR::MAU::TableSeq *preorder(IR::MAU::TableSeq *s) override {
        prune();
        return s;
    }
    /// for traversing midend IR
    IR::Node *preorder(IR::BFN::TnaParser *p) override {
        prune();
        return p;
    }
    IR::Node *preorder(IR::BFN::TnaControl *c) override {
        prune();
        return c;
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_PARDE_VISITOR_H_ */
