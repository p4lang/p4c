/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_MAU_EMPTY_CONTROLS_H_
#define BF_P4C_MAU_EMPTY_CONTROLS_H_

#include "mau_visitor.h"

using namespace P4;

class RemoveEmptyControls : public MauTransform {
    const IR::MAU::Table *postorder(IR::MAU::Table *tbl) override {
        if (tbl->next.count("$default"_cs)) {
            auto def = tbl->next.at("$default"_cs);
            if (def->tables.empty())
                tbl->next.erase("$default"_cs);
        } else {
            for (auto it = tbl->next.begin(); it != tbl->next.end();) {
                if (it->second->tables.empty())
                    it = tbl->next.erase(it);
                else
                    ++it; } }
        if (tbl->conditional_gateway_only() && tbl->next.empty())
            return nullptr;
        return tbl; }
};

#endif /* BF_P4C_MAU_EMPTY_CONTROLS_H_ */
