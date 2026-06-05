/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_MIDENDLAST_H_
#define MIDEND_MIDENDLAST_H_

#include "ir/ir.h"

namespace P4 {

class MidEndLast : public Inspector {
 public:
    MidEndLast() { setName("MidEndLast"); }
    bool preorder(const IR::P4Program *) override { return false; }
};

}  // namespace P4

#endif /* MIDEND_MIDENDLAST_H_ */
