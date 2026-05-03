/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 * Copyright 2022-present VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_COMMON_COPYSRCINFO_H_
#define FRONTENDS_COMMON_COPYSRCINFO_H_

#include "ir/ir.h"

namespace P4 {

/// This simple visitor copies the specified source information
/// to the root node of the IR tree that it is invoked on.
class CopySrcInfo : public Transform {
    const Util::SourceInfo &srcInfo;

 public:
    explicit CopySrcInfo(const Util::SourceInfo &srcInfo) : srcInfo(srcInfo) {}
    /// Only visit first node.
    const IR::Node *preorder(IR::Node *node) {
        auto result = node->clone();
        result->srcInfo = srcInfo;
        prune();
        return result;
    }
};

}  // namespace P4

#endif  // FRONTENDS_COMMON_COPYSRCINFO_H_
