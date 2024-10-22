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
