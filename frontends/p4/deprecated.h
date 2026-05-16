/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_DEPRECATED_H_
#define FRONTENDS_P4_DEPRECATED_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Checks for the use of symbols that are marked as @deprecated and
 * gives warnings.
 */
class Deprecated : public Inspector, public ResolutionContext {
 public:
    Deprecated() { setName("Deprecated"); }

    void warnIfDeprecated(const IR::IAnnotated *declaration, const IR::Node *errorNode);

    bool preorder(const IR::PathExpression *path) override;
    bool preorder(const IR::Type_Name *name) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_DEPRECATED_H_ */
