/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_VALIDATEPROPERTIES_H_
#define MIDEND_VALIDATEPROPERTIES_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

/**
 * Checks to see if there are any unknown properties.
 *
 * @pre none
 * @post raise an error if there are invalid table properties in P4 program.
 */
class ValidateTableProperties : public Inspector {
    std::set<cstring> legalProperties;

 public:
    ValidateTableProperties(const std::initializer_list<cstring> legal) {
        setName("ValidateTableProperties");
        legalProperties.emplace("actions");
        legalProperties.emplace("default_action");
        legalProperties.emplace("key");
        legalProperties.emplace("entries");
        for (auto l : legal) {
            legalProperties.emplace(l);
        }
    }
    void postorder(const IR::Property *property) override;
    // don't check properties in externs (Declaration_Instances)
    bool preorder(const IR::Declaration_Instance * /*instance*/) override { return false; }
};

}  // namespace P4

#endif /* MIDEND_VALIDATEPROPERTIES_H_ */
