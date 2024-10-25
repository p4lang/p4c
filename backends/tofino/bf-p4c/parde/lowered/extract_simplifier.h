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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_EXTRACT_SIMPLIFIER_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_EXTRACT_SIMPLIFIER_H_

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/lowered/helpers.h"
#include "bf-p4c/phv/phv_fields.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParserIR
 * @brief Helper class that splits extract operations into multiple smaller extracts,
 *        such that each extract writes to exactly one PHV container.
 */
struct ExtractSimplifier {
    const PhvInfo &phv;
    const ClotInfo &clot;

    ExtractSimplifier(const PhvInfo &phv, const ClotInfo &clot) : phv(phv), clot(clot) {}

    using ExtractSequence = std::vector<const IR::BFN::LoweredExtractPhv *>;

    /// Add a new extract operation to the sequence.
    void add(const IR::BFN::Extract *extract);
    void add(const IR::BFN::ExtractClot *extract);
    void add(const IR::BFN::ExtractPhv *extract);

    /// Create a new merged extract from a sequence of extracts.
    /// The buffer range must have been identified already.
    /// All extracts should have the same write mode.
    ///
    /// @param container   The container to create the merged extract for
    /// @param extracts    The extracts to merge
    /// @param bufferRange The calculated range of buffer bytes to extract
    template <typename InputBufferRValType>
    static const IR::BFN::LoweredExtractPhv *createMergedExtract(PHV::Container container,
                                                                 const ExtractSequence &extracts,
                                                                 nw_byteinterval bufferRange);
    template <typename InputBufferRValType>
    static const ExtractSequence mergeExtractsFor(PHV::Container container,
                                                  const ExtractSequence &extracts);

    void sortExtractPhvs(IR::Vector<IR::BFN::LoweredParserPrimitive> &loweredExtracts);

    const IR::BFN::LoweredExtractPhv *mergeExtractsForConstants(PHV::Container container,
                                                                const ExtractSequence &extracts);

    /// Convert the sequence of Extract operations that have been passed to
    /// `add()` so far into a sequence of LoweredExtract operations. Extracts
    /// that write to the same container are merged together.
    IR::Vector<IR::BFN::LoweredParserPrimitive> lowerExtracts(
        std::map<gress_t, std::map<unsigned, unsigned>> clotTagToCsumUnit);

    /// The sequence of extract operations to be simplified. They're organized
    /// by container so that multiple extracts to the same container can be
    /// merged.
    ordered_map<PHV::Container, ExtractSequence> extractFromPacketByContainer;
    ordered_map<PHV::Container, ExtractSequence> extractFromBufferByContainer;
    ordered_map<PHV::Container, ExtractSequence> extractConstantByContainer;

    ordered_map<Clot *, std::vector<const IR::BFN::ExtractClot *>> clotExtracts;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_EXTRACT_SIMPLIFIER_H_ */
