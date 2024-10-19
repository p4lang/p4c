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

#include "constraints.h"

namespace Constraints {

std::ostream &operator<<(std::ostream &out, const SolitaryConstraint &cons) {
    out << "(";
    if (cons.isALU()) out << "alu ";
    if (cons.isChecksum()) out << "checksum ";
    if (cons.isArch()) out << "arch ";
    if (cons.isDigest()) out << "digest ";
    if (cons.isPragmaSolitary()) out << "pragma_solitary ";
    if (cons.isPragmaContainerSize()) out << "pragma_container_size ";
    if (cons.isClearOnWrite()) out << "clear_on_write ";
    out << ")";
    return out;
}

}  // namespace Constraints
