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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_INIT_ZERO_CONTAINERS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_INIT_ZERO_CONTAINERS_H_

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/parde/parde_visitor.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParser
 * @brief Computes containers that have fields relying on parser zero initialization.
 *
 * These containers will be marked as valid coming out of the parser (Tofino only).
 * In Tofino2, all containers are valid coming out of the parser.
 */
class ComputeInitZeroContainers : public ParserModifier {
    void postorder(IR::BFN::LoweredParser *parser) override;

 public:
    ComputeInitZeroContainers(
        const PhvInfo &phv, const FieldDefUse &defuse,
        const ordered_set<const PHV::Field *> &no_init,
        const std::map<gress_t, std::set<PHV::Container>> &origParserZeroInitContainers)
        : phv(phv),
          defuse(defuse),
          no_init_fields(no_init),
          origParserZeroInitContainers(origParserZeroInitContainers) {}

    const PhvInfo &phv;
    const FieldDefUse &defuse;
    const ordered_set<const PHV::Field *> &no_init_fields;
    const std::map<gress_t, std::set<PHV::Container>> &origParserZeroInitContainers;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_INIT_ZERO_CONTAINERS_H_ */
