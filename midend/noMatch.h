/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_NOMATCH_H_
#define MIDEND_NOMATCH_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {

/// Convert
/// state s { transition select (e) { ... } }
/// into
/// state s { transition select (e) { ... default: noMatch; }}
/// state noMatch { verify(false, error.NoMatch); transition reject; }
class HandleNoMatch : public Transform {
    MinimalNameGenerator nameGen;

 public:
    const IR::ParserState *noMatch = nullptr;
    HandleNoMatch() { setName("HandleNoMatch"); }
    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = Transform::init_apply(node);
        node->apply(nameGen);

        return rv;
    }
    const IR::Node *postorder(IR::SelectExpression *expression) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *postorder(IR::P4Program *program) override;
};

}  // namespace P4

#endif /* MIDEND_NOMATCH_H_ */
