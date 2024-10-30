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

#ifndef BF_P4C_MAU_EMPTY_CONTROLS_H_
#define BF_P4C_MAU_EMPTY_CONTROLS_H_

#include "mau_visitor.h"

using namespace P4;

class RemoveEmptyControls : public MauTransform {
    const IR::MAU::Table *postorder(IR::MAU::Table *tbl) override {
        if (tbl->next.count("$default"_cs)) {
            auto def = tbl->next.at("$default"_cs);
            if (def->tables.empty()) tbl->next.erase("$default"_cs);
        } else {
            for (auto it = tbl->next.begin(); it != tbl->next.end();) {
                if (it->second->tables.empty())
                    it = tbl->next.erase(it);
                else
                    ++it;
            }
        }
        if (tbl->conditional_gateway_only() && tbl->next.empty()) return nullptr;
        return tbl;
    }
};

#endif /* BF_P4C_MAU_EMPTY_CONTROLS_H_ */
