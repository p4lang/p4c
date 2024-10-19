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
