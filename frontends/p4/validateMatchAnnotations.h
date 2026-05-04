/*
 * Copyright 2019 VMware, Inc.
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_VALIDATEMATCHANNOTATIONS_H_
#define FRONTENDS_P4_VALIDATEMATCHANNOTATIONS_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/error.h"

namespace P4 {

/**
 * Checks that match annotations only have 1 argument which is of type match_kind.
 */
class ValidateMatchAnnotations final : public Inspector {
    TypeMap *typeMap;

 public:
    explicit ValidateMatchAnnotations(TypeMap *typeMap) : typeMap(typeMap) {
        setName("ValidateMatchAnnotations");
    }
    void postorder(const IR::Annotation *annotation) override {
        if (annotation->name != IR::Annotation::matchAnnotation) return;
        if (!isInContext<IR::StructField>()) return;
        // FIXME: Check annotation kind
        const auto &expr = annotation->getExpr();
        if (expr.size() != 1)
            ::P4::error(ErrorType::ERR_INVALID, "%1%: annotation must have exactly 1 argument",
                        annotation);
        auto e0 = expr.at(0);
        auto type = typeMap->getType(e0, true);
        if (type == nullptr) return;
        if (!type->is<IR::Type_MatchKind>())
            ::P4::error(ErrorType::ERR_TYPE_ERROR, "%1%: value must be a match_kind", e0);
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_VALIDATEMATCHANNOTATIONS_H_ */
