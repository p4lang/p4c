/*
 * Copyright 2020 Barefoot Networks, Inc.
 * SPDX-FileCopyrightText: 2020 Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_SWITCHADDDEFAULT_H_
#define FRONTENDS_P4_SWITCHADDDEFAULT_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

/*
 * find switch statements that do not cover all possible cases in their case list (no
 * default: case and at least one action tag with no case) and add a default: {}
 * case that does nothing.
 */
class SwitchAddDefault : public Modifier {
 public:
    void postorder(IR::SwitchStatement *) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_SWITCHADDDEFAULT_H_ */
