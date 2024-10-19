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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_LOWERED_PARSER_IR_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_LOWERED_PARSER_IR_H_

#include "bf-p4c/parde/allocate_parser_checksum.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/count_strided_header_refs.h"
#include "bf-p4c/parde/parde_visitor.h"

namespace Parde::Lowered {

/**
 * @ingroup LowerParserIR
 * @brief Combines the high-level parser IR and the results of PHV allocation to
 *        produce a low-level, target-specific representation of the parser program.
 *
 * Note that the new IR is just constructed here.
 * ReplaceParserIR is what actually replaces the high-level IR with the lowered version.
 */
struct ComputeLoweredParserIR : public ParserInspector {
    ComputeLoweredParserIR(
        const PhvInfo &phv, ClotInfo &clotInfo, const AllocateParserChecksums &checksumAlloc,
        std::map<gress_t, std::set<PHV::Container>> &origParserZeroInitContainers)
        : phv(phv),
          clotInfo(clotInfo),
          checksumAlloc(checksumAlloc),
          origParserZeroInitContainers(origParserZeroInitContainers) {
        // Initialize the map from high-level parser states to low-level parser
        // states so that null, which represents the end of the parser program
        // in the high-level IR, is mapped to null, which conveniently enough
        // means the same thing in the lowered IR.
        loweredStates[nullptr] = nullptr;
    }

    std::map<const IR::BFN::ParserState *, const IR::BFN::LoweredParserState *> loweredStates;
    std::set<const IR::BFN::LoweredParserState *> dontMergeStates;

    const IR::BFN::ContainerRef *igParserError = nullptr;
    const IR::BFN::ContainerRef *egParserError = nullptr;
    unsigned egressMetaOpt = 0;
    unsigned egressMetaSize = 0;  // in bytes

 private:
    profile_t init_apply(const IR::Node *node) override;

    bool preorder(const IR::Type_Header *type) override;

    IR::Vector<IR::BFN::LoweredParserChecksum> lowerParserChecksums(
        const IR::BFN::Parser *parser, const IR::BFN::ParserState *state,
        const std::vector<const IR::BFN::ParserChecksumPrimitive *> &checksums);

    unsigned int rangeToInt(const IR::BFN::PacketRVal *range);

    /// Create lowered HW checksum primitives that can be consumed by the assembler.
    /// Get the checksum unit allocation from checksumAlloc and destination container
    /// from PHV allocation.
    IR::BFN::LoweredParserChecksum *lowerParserChecksum(
        const IR::BFN::Parser *parser, const IR::BFN::ParserState *state, cstring name,
        std::vector<const IR::BFN::ParserChecksumPrimitive *> &checksums);

    unsigned getOffsetIncAmt(const IR::BFN::ParserState *state);

    void postorder(const IR::BFN::ParserState *state) override;

    void end_apply() override;

    const PhvInfo &phv;
    ClotInfo &clotInfo;
    const AllocateParserChecksums &checksumAlloc;
    std::map<gress_t, std::set<PHV::Container>> &origParserZeroInitContainers;
    // Maps clot tag to checksum unit whose output will be deposited in that CLOT for each gress
    std::map<gress_t, std::map<unsigned, unsigned>> clotTagToCsumUnit;
};

}  // namespace Parde::Lowered

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_LOWERED_COMPUTE_LOWERED_PARSER_IR_H_ */
