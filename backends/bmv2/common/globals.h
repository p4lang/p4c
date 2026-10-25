/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_COMMON_GLOBALS_H_
#define BACKENDS_BMV2_COMMON_GLOBALS_H_

#include "backend.h"
#include "ir/ir.h"

namespace P4::BMV2 {

class ConvertGlobals : public Inspector {
    ConversionContext *ctxt;
    const bool emitExterns;

 public:
    explicit ConvertGlobals(ConversionContext *ctxt, const bool &emitExterns_)
        : ctxt(ctxt), emitExterns(emitExterns_) {
        setName("ConvertGlobals");
    }

    bool preorder(const IR::ExternBlock *block) override;
    bool preorder(const IR::ToplevelBlock *block) override {
        /// Blocks are not in IR tree, use a custom visitor to traverse
        for (auto it : block->constantValue) {
            if (it.second->is<IR::Block>()) visit(it.second->getNode());
        }
        return false;
    }
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_COMMON_GLOBALS_H_ */
