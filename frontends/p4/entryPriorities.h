/*
 * Copyright 2023 Mihai Budiu
 * SPDX-FileCopyrightText: 2023 Mihai Budiu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_ENTRYPRIORITIES_H_
#define FRONTENDS_P4_ENTRYPRIORITIES_H_

#include "coreLibrary.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {

/// Assigns priorities to table entries if they are not 'const'
class EntryPriorities : public Transform, public ResolutionContext {
    P4::P4CoreLibrary &corelib;

    bool requiresPriority(const IR::KeyElement *ke) const;

 public:
    EntryPriorities() : corelib(P4::P4CoreLibrary::instance()) { setName("EntryPriorities"); }
    const IR::Node *preorder(IR::EntriesList *entries) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_ENTRYPRIORITIES_H_ */
