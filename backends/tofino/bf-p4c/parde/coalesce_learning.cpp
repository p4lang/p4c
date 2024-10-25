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

#include "coalesce_learning.h"

bool CoalesceLearning::preorder(IR::BFN::LearningTableEntry *lte) {
    int orig_offset = 0, new_offset = 0;
    std::map<int, int> adjust;
    for (unsigned i = 0; i + 1 < lte->sources.size(); ++i) {
        int size = lte->sources[i]->container.size() / 8U;  // size in bytes
        orig_offset += size;
        if (lte->sources[i]->container == lte->sources[i + 1]->container) {
            auto *combine = lte->sources[i]->clone();
            if (combine->range) {
                if (lte->sources[i + 1]->range) {
                    combine->range = combine->range->unionWith(*lte->sources[i + 1]->range);
                } else {
                    combine->range = std::nullopt;
                }
            }
            combine->debug.mergeWith(lte->sources[i + 1]->debug);
            lte->sources[i] = combine;
            adjust[orig_offset] = orig_offset - new_offset;
            lte->sources.erase(lte->sources.begin() + i + 1);
            --i;  // retry i with next container
            continue;
        }
        new_offset += size;
    }
    if (!adjust.empty()) {
        // FIXME -- DigestPacking uses #emit hacks to get around the normal ir-generator
        // FIXME -- checks/fixes, making it very fragile and unsafe.
        auto *mod = new IR::BFN::LearningTableEntry::DigestPacking(*lte->controlPlaneFormat);
        for (IR::BFN::DigestField &df : *mod) {
            auto ad = adjust.upper_bound(df.startByte);
            if (ad != adjust.begin()) {
                --ad;
                df.startByte -= ad->second;
            }
        }
        lte->controlPlaneFormat = mod;
    }
    return false;
}
