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
IR::BFN::Pipe *extractChecksumFromDeparser(const IR::BFN::TnaDeparser *deparser,
                                           IR::BFN::Pipe *pipe);
}  // namespace BFN
#endif /* BF_P4C_PARDE_DEPARSER_CHECKSUM_UPDATE_H_ */
