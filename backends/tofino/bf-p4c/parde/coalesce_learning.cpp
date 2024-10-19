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
