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

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_MERGE_POV_BITS_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_MERGE_POV_BITS_H_

#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"

namespace BFN {

using namespace P4;

/**
 * @ingroup common
 * @brief Merge POV bits where a single POV bit could be shared between multiple headers
 *
 * Identify situations where a single POV bit could be shared across multiple headers. "Merge" the
 * POV bits by replacing with the single POV bit target in the deparser.
 *
 * This pass is currently fairly simple: it only replaces references in the deparser. Ideally it
 * would replace all references as this would save some POV bits and could help with PHV fitting.
 *
 * I had experimented with more complete POV replacement throughout the pipe but halted those
 * changes. There are several places where more extensive changes are required for a more full
 * replacement:
 *  - Header mutex analysis: Logic doesn't account for aliased POV bits, so a manipulation of an
 *    alias source doesn't register as updating the alias destination and vice-versa.
 *  - Usedef: Logic doesn't account for aliased POV bits.
 *  - IXBar + hash: If two POV bits that are merged are used in a hash, the bits still need to be
 *    included twice. Currently the compiler will duplicate the byte in the IXBar (once for source
 *    and once for dest), but the assembler doesn't currently handle this correctly. Can either
 *    allow the byte duplication (consuming more IXBar resources) and update the assembler, or
 *    eliminate the duplication in the compiler but this requires allowing duplicate bit refences in
 *    IXBar::Use::Byte objects.
 *  - IXBar + match: Requires either the byte duplication on the ixbar, or duplication of bytes on
 *    the match bus. (Either choice consumes additional resources of one type or another.)
 *
 * The current approach of only replacing the POV in the deparser avoids the problems listed above.
 * However, it must be run after code elimination, otherwise set valid/set invalid calls may be
 * eliminated and then header mutex analysis breaks.
 */
class MergePovBits : public PassManager {
 protected:
    ordered_map<const PHV::Field *, const PHV::Field *> merge_pov;

 public:
    explicit MergePovBits(const PhvInfo &phv);
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_MERGE_POV_BITS_H_ */
