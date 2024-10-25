/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_TABLE_MUTEX_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_TABLE_MUTEX_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

// FIXME -- move this to open source P4.org code?

/** Inspector to determine which tables in the midend are mutually exclusive
 * after running this pass, the operator() method can be used to query iff
 * two tables arte mutalluy exclusive */
class TableMutex : public Inspector, public ControlFlowVisitor, public P4::ResolutionContext {
    TableMutex *clone() const { return new TableMutex(*this); }
    void flow_merge(Visitor &) override;
    void flow_copy(ControlFlowVisitor &) override;
    struct Shared;
    Shared *data;  // shared between clones
    bitvec seen_tables;

    bool preorder(const IR::P4Control *) override;
    bool preorder(const IR::P4Parser *) override { return false; }  // ignore parsers
    bool preorder(const IR::MethodCallExpression *) override;
    void end_apply() override;
    friend std::ostream &operator<<(std::ostream &, const TableMutex &);

 public:
    TableMutex();
    bool operator()(const IR::P4Table *, const IR::P4Table *);
    bool operator()(const IR::P4Action *, const IR::P4Action *);
    bool operator()(const IR::Declaration *, const IR::Declaration *);
};

#endif /* BACKENDS_TOFINO_BF_P4C_MIDEND_TABLE_MUTEX_H_ */
