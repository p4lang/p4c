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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_MULTI_WRITE_CONTAINERS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_MULTI_WRITE_CONTAINERS_H_

#include "bf-p4c/parde/allocate_parser_checksum.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parde_visitor.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParser
 * @brief Collects all containers that are written more than once by the parser.
 */
class ComputeMultiWriteContainers : public ParserModifier {
    const PhvInfo &phv;
    const CollectLoweredParserInfo &parser_info;

    std::set<std::pair<const IR::BFN::LoweredParserMatch *, unsigned>> visited_matches;
    unsigned stack_offset = 0;

 public:
    ComputeMultiWriteContainers(const PhvInfo &ph, const CollectLoweredParserInfo &pi)
        : phv(ph), parser_info(pi) {
        visitDagOnce = false;
    }

 private:
    bool preorder(IR::BFN::LoweredParserMatch *match) override;
    void postorder(IR::BFN::LoweredParserMatch *match) override;

    bool preorder(IR::BFN::LoweredParser *) override;

    PHV::Container offset_container(const PHV::Container &container);

    bool has_non_mutex_writes(const IR::BFN::LoweredParser *parser,
                              const std::set<const IR::BFN::LoweredParserMatch *> &matches);

    void detect_multi_writes(
        const IR::BFN::LoweredParser *parser,
        const std::map<PHV::Container, std::set<const IR::BFN::LoweredParserMatch *>> &writes,
        std::set<PHV::Container> &write_containers, const char *which);

    void postorder(IR::BFN::LoweredParser *parser) override;

    std::map<PHV::Container, std::set<const IR::BFN::LoweredParserMatch *>> bitwise_or,
        clear_on_write;
    std::set<PHV::Container> bitwise_or_containers, clear_on_write_containers;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_MULTI_WRITE_CONTAINERS_H_ */
