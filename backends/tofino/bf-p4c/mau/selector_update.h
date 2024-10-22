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

#ifndef BF_P4C_MAU_SELECTOR_UPDATE_H_
#define BF_P4C_MAU_SELECTOR_UPDATE_H_

#include "ir/pass_manager.h"
#include "lib/ordered_map.h"
#include "mau_visitor.h"

using namespace P4;

/** In order to work around a hw bug?, ALL selector tables need a stateful table
 * that can be used by the driver to set and clear entries in the table.  We already
 * ensure that the necessary instructions exist in the statful alu if one addresses
 * the selector, so here we just need to create a stateful table for each selector
 * that does not already have one.  We attach it to the same match table as the
 * selector, just to have a place to put it. */

class AddSelectorSalu : public PassManager {
    // map associating selectors with the stateful alu that updates them
    ordered_map<const IR::MAU::Selector *, const IR::MAU::StatefulAlu *> sel2salu;

    class FindSelectorSalu : public MauInspector {
        AddSelectorSalu &self;
        bool preorder(const IR::MAU::StatefulAlu *salu) {
            if (salu->selector) self.sel2salu[salu->selector] = salu;
            return false;
        }
        bool preorder(const IR::Expression *) { return false; }  // prune all expressions
     public:
        explicit FindSelectorSalu(AddSelectorSalu &self) : self(self) {}
    };

    class AddSaluIfNeeded : public MauModifier {
        AddSelectorSalu &self;
        bool preorder(IR::MAU::Table *);
        bool preorder(IR::Expression *) { return false; }  // prune all expressions
     public:
        explicit AddSaluIfNeeded(AddSelectorSalu &self) : self(self) {}
    };

 public:
    AddSelectorSalu() : PassManager({new FindSelectorSalu(*this), new AddSaluIfNeeded(*this)}) {}
};

#endif /* BF_P4C_MAU_SELECTOR_UPDATE_H_ */
