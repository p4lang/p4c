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

#ifndef BF_P4C_COMMON_EMPTY_TABLESEQ_H_
#define BF_P4C_COMMON_EMPTY_TABLESEQ_H_

#include "bf-p4c/mau/mau_visitor.h"

using namespace P4;

/// Adds empty table sequences to implicit fall-through paths in the program. For example, when
/// an 'if' statement has no 'else', this adds an empty table sequence to the 'else' branch.
class AddEmptyTableSeqs : public MauModifier {
    void postorder(IR::BFN::Pipe *pipe) override;
    void postorder(IR::MAU::Table *tbl) override;

 public:
    AddEmptyTableSeqs() {}
};

#endif /* BF_P4C_COMMON_EMPTY_TABLESEQ_H_ */
