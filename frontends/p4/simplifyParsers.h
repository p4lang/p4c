/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_SIMPLIFYPARSERS_H_
#define FRONTENDS_P4_SIMPLIFYPARSERS_H_

#include "ir/ir.h"

namespace P4 {

/** @brief Remove unreachable parser states, and collapse simple chains of
 * states.
 *
 * Does not remove the "accept" state, even if it is not reachable.  A
 * transition between states ```s1``` and ```s2``` is part of a "simple" chain if:
 *  - there are no other outgoing edges from ```s1```,
 *  - there are no other incoming edges to ```s2```,
 *  - and ```s2``` does not have annotations.
 *
 * Note that UniqueNames must run before this pass, so that we won't end up
 * with same-named state-local variables in the same state.
 */
class SimplifyParsers : public Transform {
 public:
    SimplifyParsers() { setName("SimplifyParsers"); }

    const IR::Node *preorder(IR::P4Parser *parser) override;
    const IR::Node *preorder(IR::P4Control *control) override {
        prune();
        return control;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_SIMPLIFYPARSERS_H_ */
