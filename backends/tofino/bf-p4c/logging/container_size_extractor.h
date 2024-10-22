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

#ifndef BF_P4C_LOGGING_CONTAINER_SIZE_EXTRACTOR_H_
#define BF_P4C_LOGGING_CONTAINER_SIZE_EXTRACTOR_H_

#include "bf-p4c/logging/constrained_fields.h"
#include "bf-p4c/phv/pragma/pa_container_size.h"

/**
 *  Class for extracting information about container sizes from Container Size pragma
 */
class ContainerSizeExtractor {
 public:
    /**
     *  Extracts information from pragma, updates constraints on appropriate ConstrainedFields
     *  in dst.
     */
    static void extract(const PragmaContainerSize &pragma, ConstrainedFieldMap &dst);

 protected:
    static void applyConstraintToField(ConstrainedField &field, const std::vector<int> &layout);

    static bool isLoggableOnField(const std::vector<int> &layout);

    static unsigned getFieldSize(const ConstrainedField &field);

    static std::vector<le_bitrange> computeSlicing(unsigned fieldSize,
                                                   const std::vector<int> &layout);

    static void updateFieldSlicesWithSlicing(const std::vector<le_bitrange> &slicing,
                                             ConstrainedField &field);

    static std::vector<ConstrainedSlice *> getSortedSlicePointers(
        const std::vector<le_bitrange> &slicing, ConstrainedField &field);

    static void applyConstraintToSlices(std::vector<ConstrainedSlice *> &slices,
                                        const std::vector<int> &layout);
};

#endif  // BF_P4C_LOGGING_CONTAINER_SIZE_EXTRACTOR_H_
