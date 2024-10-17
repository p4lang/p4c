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

#ifndef BF_P4C_PARDE_DEPARSER_CHECKSUM_UPDATE_H_
#define BF_P4C_PARDE_DEPARSER_CHECKSUM_UPDATE_H_

#include "ir/ir.h"

/**
 * \defgroup ExtractChecksum ExtractChecksum
 * \ingroup parde
 * \brief Extracts checksum from translated checksum extern.
 */

namespace P4 {
namespace IR {
namespace BFN {
class Pipe;
}  // namespace BFN
class P4Control;
}  // namespace IR
}  // namespace P4

namespace BFN {

using namespace P4;
/**
 * \ingroup ExtractChecksum
 *
 * This function extracts checksum from the translated tofino.p4 checksum extern.
 * Error checking should be done during the translation, not in this function.
 *
 * Attempts to convert the %P4 code in the provided control into deparser
 * EmitChecksum primitives. The control is expected to follow the pattern of
 * tofino.p4's IngressDeparser and EgressDeparser control.
 *
 *     if (header.isValid()) {
 *       checksumExternInstance.update({
 *          header.sourceField1,
 *          header.sourceField2
 *       }, header.destField);
 *     }
 *
 */
IR::BFN::Pipe*
extractChecksumFromDeparser(const IR::BFN::TnaDeparser* deparser,
                            IR::BFN::Pipe* pipe);
}  // namespace BFN
#endif /* BF_P4C_PARDE_DEPARSER_CHECKSUM_UPDATE_H_ */
