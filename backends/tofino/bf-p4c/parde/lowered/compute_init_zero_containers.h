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

#ifndef EXTENSIONS_BF_P4C_PARDE_LOWERED_COMPUTE_INIT_ZERO_CONTAINERS_H_
#define EXTENSIONS_BF_P4C_PARDE_LOWERED_COMPUTE_INIT_ZERO_CONTAINERS_H_

#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/common/field_defuse.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParser
 * @brief Computes containers that have fields relying on parser zero initialization.
 *
 * These containers will be marked as valid coming out of the parser (Tofino only).
 * In Tofino2, all containers are valid coming out of the parser.
 */
class ComputeInitZeroContainers : public ParserModifier {
    void postorder(IR::BFN::LoweredParser* parser) override;

 public:
    ComputeInitZeroContainers(
            const PhvInfo& phv,
            const FieldDefUse& defuse,
            const ordered_set<const PHV::Field*>& no_init,
            const std::map<gress_t, std::set<PHV::Container>>& origParserZeroInitContainers)
        : phv(phv), defuse(defuse), no_init_fields(no_init),
          origParserZeroInitContainers(origParserZeroInitContainers) {}

    const PhvInfo& phv;
    const FieldDefUse& defuse;
    const ordered_set<const PHV::Field*>& no_init_fields;
    const std::map<gress_t, std::set<PHV::Container>>& origParserZeroInitContainers;
};

}  // namespace Parde::Lowered

#endif /* EXTENSIONS_BF_P4C_PARDE_LOWERED_COMPUTE_INIT_ZERO_CONTAINERS_H_ */
