/*
 * Copyright 2022 VMware, Inc.
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_VALIDATEVALUESETS_H_
#define FRONTENDS_P4_VALIDATEVALUESETS_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/error.h"

namespace P4 {

/**
 * Checks that match annotations only have 1 argument which is of type match_kind.
 */
class ValidateValueSets final : public Inspector {
 public:
    ValidateValueSets() { setName("ValidateValueSets"); }
    void postorder(const IR::P4ValueSet *valueSet) override {
        if (!valueSet->size->is<IR::Constant>()) {
            ::P4::error(ErrorType::ERR_EXPECTED, "%1%: value_set size must be constant",
                        valueSet->size);
        }
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_VALIDATEVALUESETS_H_ */
