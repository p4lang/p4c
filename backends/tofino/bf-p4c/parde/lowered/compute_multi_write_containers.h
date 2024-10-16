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

#ifndef EXTENSIONS_BF_P4C_PARDE_LOWERED_COMPUTE_MULTI_WRITE_CONTAINERS_H_
#define EXTENSIONS_BF_P4C_PARDE_LOWERED_COMPUTE_MULTI_WRITE_CONTAINERS_H_

#include "bf-p4c/parde/allocate_parser_checksum.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/clot/clot_info.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParser
 * @brief Collects all containers that are written more than once by the parser.
 */
class ComputeMultiWriteContainers : public ParserModifier {
    const PhvInfo& phv;
    const CollectLoweredParserInfo& parser_info;

    std::set<std::pair<const IR::BFN::LoweredParserMatch*, unsigned>> visited_matches;
    unsigned stack_offset = 0;

 public:
    ComputeMultiWriteContainers(const PhvInfo& ph,
                                const CollectLoweredParserInfo& pi)
        : phv(ph), parser_info(pi) { visitDagOnce = false; }

 private:
    bool preorder(IR::BFN::LoweredParserMatch* match) override;
    void postorder(IR::BFN::LoweredParserMatch* match) override;

    bool preorder(IR::BFN::LoweredParser*) override;

    PHV::Container offset_container(const PHV::Container& container);

    bool has_non_mutex_writes(const IR::BFN::LoweredParser* parser,
            const std::set<const IR::BFN::LoweredParserMatch*>& matches);

    void detect_multi_writes(const IR::BFN::LoweredParser* parser,
            const std::map<PHV::Container, std::set<const IR::BFN::LoweredParserMatch*>>& writes,
            std::set<PHV::Container>& write_containers, const char* which);

    void postorder(IR::BFN::LoweredParser* parser) override;

    std::map<PHV::Container,
             std::set<const IR::BFN::LoweredParserMatch*>> bitwise_or, clear_on_write;
    std::set<PHV::Container> bitwise_or_containers, clear_on_write_containers;
};

}  // namespace Parde::Lowered

#endif /* EXTENSIONS_BF_P4C_PARDE_LOWERED_COMPUTE_MULTI_WRITE_CONTAINERS_H_ */
