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

#include "bf-p4c/logging/container_size_extractor.h"

#include <algorithm>
#include <numeric>

void ContainerSizeExtractor::extract(const PragmaContainerSize &pragma, ConstrainedFieldMap &dst) {
    for (auto &kv : pragma.field_to_layout()) {
        auto fieldName = kv.first->name;
        BUG_CHECK(dst.find(fieldName) != dst.end(), "Field %s not present in ConstrainedFieldMap",
                  fieldName);

        applyConstraintToField(dst[fieldName], kv.second);
    }
}

void ContainerSizeExtractor::applyConstraintToField(ConstrainedField &field,
                                                    const std::vector<int> &layout) {
    using ContainerSizeReason = Constraints::ContainerSizeConstraint::ContainerSizeReason;

    if (isLoggableOnField(layout)) {
        Constraints::ContainerSizeConstraint c;
        c.addConstraint(ContainerSizeReason::PRAGMA, layout[0]);
        field.setContainerSize(c);
        return;
    }

    unsigned fieldSize = getFieldSize(field);
    auto slicing = computeSlicing(fieldSize, layout);
    updateFieldSlicesWithSlicing(slicing, field);
    auto sortedSlicePointers = getSortedSlicePointers(slicing, field);

    applyConstraintToSlices(sortedSlicePointers, layout);
}

bool ContainerSizeExtractor::isLoggableOnField(const std::vector<int> &layout) {
    /* Constraint is loggable on field (rather than on slices) iff layout
       all items in layout have the same value. */
    for (unsigned i = 0; i < layout.size() - 1; i++) {
        if (layout[i] != layout[i + 1]) return false;
    }

    return true;
}

unsigned ContainerSizeExtractor::getFieldSize(const ConstrainedField &field) {
    le_bitrange aggregated;
    auto &slices = field.getSlices();
    for (const auto &slice : slices) {
        aggregated |= slice.getRange();
    }
    return aggregated.size();
}

std::vector<le_bitrange> ContainerSizeExtractor::computeSlicing(unsigned fieldSize,
                                                                const std::vector<int> &layout) {
    unsigned layoutSum = static_cast<unsigned>(std::accumulate(layout.begin(), layout.end(), 0));
    BUG_CHECK(fieldSize <= layoutSum,
              "Invalid container size constraint"
              ", sum of layout is %d, field size is %d",
              layoutSum, fieldSize);

    std::vector<le_bitrange> result;
    result.reserve(layout.size());

    bool upcasted = fieldSize < layoutSum;
    bool triviallySplittable = (!upcasted || (fieldSize > layoutSum - layout.back()));

    // Create splitting
    unsigned bgn = 0;
    float map = fieldSize / float(layoutSum);
    for (auto n : layout) {
        unsigned end = std::ceil((bgn + n) * (triviallySplittable ? 1.f : map));
        result.push_back(le_bitrange(bgn, end - 1));
        bgn = end;
    }

    if (upcasted) {  // Fix end of last slice if constraint is upcasted
        le_bitrange aux = result.back();
        result.back() = le_bitrange(aux.lo, fieldSize - 1);
    }

    return result;
}

void ContainerSizeExtractor::updateFieldSlicesWithSlicing(const std::vector<le_bitrange> &slicing,
                                                          ConstrainedField &field) {
    auto &slices = field.getSlices();

    // First add non-duplicate slices to field
    for (auto range : slicing) {
        // If range is not represented by anything in slices
        if (std::none_of(slices.begin(), slices.end(), [range](const ConstrainedSlice &slice) {
                return slice.getRange() == range;
            })) {
            field.addSlice(ConstrainedSlice(field, range));
        }
    }
}

std::vector<ConstrainedSlice *> ContainerSizeExtractor::getSortedSlicePointers(
    const std::vector<le_bitrange> &slicing, ConstrainedField &field) {
    auto &slices = field.getSlices();
    std::vector<ConstrainedSlice *> result;
    result.reserve(slicing.size());

    for (auto range : slicing) {
        for (auto itr = slices.begin(); itr != slices.end(); itr++) {
            if (itr->getRange() == range) {
                result.push_back(&(*itr));
            }
        }
    }

    return result;
}

void ContainerSizeExtractor::applyConstraintToSlices(std::vector<ConstrainedSlice *> &slices,
                                                     const std::vector<int> &layout) {
    using ContainerSizeReason = Constraints::ContainerSizeConstraint::ContainerSizeReason;

    BUG_CHECK(slices.size() == layout.size(),
              "Container Size constraint layout size %d"
              " mismatches number of slices %d.",
              layout.size(), slices.size());

    for (unsigned i = 0; i < slices.size(); i++) {
        Constraints::ContainerSizeConstraint c;
        c.addConstraint(ContainerSizeReason::PRAGMA, layout[i]);
        slices[i]->setContainerSize(c);
    }
}
