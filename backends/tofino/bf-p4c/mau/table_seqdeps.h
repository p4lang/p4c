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

#ifndef BF_P4C_MAU_TABLE_SEQDEPS_H_
#define BF_P4C_MAU_TABLE_SEQDEPS_H_

#include "field_use.h"
#include "mau_visitor.h"

using namespace P4;

class TableFindSeqDependencies : public MauModifier {
    FieldUse uses;
    profile_t init_apply(const IR::Node *root) override;
    void postorder(IR::MAU::TableSeq *) override;

 public:
    explicit TableFindSeqDependencies(const PhvInfo &p) : uses(p) {}
};

#endif /* BF_P4C_MAU_TABLE_SEQDEPS_H_ */
