/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_COMMON_NAME_GATEWAYS_H_
#define FRONTENDS_COMMON_NAME_GATEWAYS_H_

#if SUPPORT_P4_14
#include "frontends/p4-14/ir-v1.def.h"
#endif
#include "ir/core.h"

namespace P4 {

class NameGateways : public Transform {
    const IR::Node *preorder(IR::If *n) override { return new IR::NamedCond(*n); }
    const IR::Node *preorder(IR::NamedCond *n) override { return n; }
};

}  // namespace P4

#endif /* FRONTENDS_COMMON_NAME_GATEWAYS_H_ */
