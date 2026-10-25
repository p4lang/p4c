// SPDX-FileCopyrightText: 2016 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "callGraph.h"

namespace P4 {
using namespace literals;

cstring cgMakeString(cstring s) { return s; }
cstring cgMakeString(char c) { return ""_cs + c; }
cstring cgMakeString(const IR::Node *node) { return node->toString(); }
cstring cgMakeString(const IR::INode *node) { return node->toString(); }

}  // namespace P4
