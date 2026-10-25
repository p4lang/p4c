/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_TABLEAPPLY_H_
#define FRONTENDS_P4_TABLEAPPLY_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

// helps resolve complex expressions involving a table apply
// such as table.apply().action_run
// and table.apply().hit

namespace P4 {

// These are used to figure out whether an expression has the form:
// table.apply().hit,
// table.apply().miss,
// or
// table.apply().action_run
class TableApplySolver {
 public:
    static const IR::P4Table *isHit(const IR::Expression *expression, DeclarationLookup *refMap,
                                    TypeMap *typeMap);
    static const IR::P4Table *isMiss(const IR::Expression *expression, DeclarationLookup *refMap,
                                     TypeMap *typeMap);
    static const IR::P4Table *isActionRun(const IR::Expression *expression,
                                          DeclarationLookup *refMap, TypeMap *typeMap);
};

}  // namespace P4

#endif /* FRONTENDS_P4_TABLEAPPLY_H_ */
