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

#ifndef BF_P4C_ARCH_REMOVE_SET_METADATA_H_
#define BF_P4C_ARCH_REMOVE_SET_METADATA_H_

#include "ir/ir.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace BFN {

/**
 * Removes assignments to metadata fields in the egress parser.
 *
 * In v1model, the values metadata fields have at the end of the ingress thread
 * are serialized into a hidden header which is sent to the egress thread along
 * with the real packet. That hidden header is then deserialized to initialize
 * the same metadata fields in the egress thread. These metadata fields are
 * collectively called "bridged metadata".
 *
 * This process can introduce conflicts if the bridged fields are also written
 * to elsewhere in the egress parser. Since the user can't write their own
 * egress parser in v1model, the compiler has to be responsible for dealing with
 * this problem. We eliminate the potential for conflict by removing assignments
 * to metadata fields from the egress parser.
 *
 * There is one exception to the rule: writes to metadata which is declared
 * local to the parser isn't removed. Because such metadata doesn't escape the
 * parser, it couldn't be bridged in any case, and removing it would
 * unnecessarily restrict both the user and the transformations that the
 * compiler can perform.
 *
 * @pre All parsers are inlined, the program has been transformed to use the
 * TNA architecture, and the refMap is current.
 * @post Assignments to non-parser-local metadata fields are removed from the
 * egress parser.
 */
struct RemoveSetMetadata : public Transform {
    RemoveSetMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap);

    const IR::AssignmentStatement *preorder(IR::AssignmentStatement *assignment);

 private:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_REMOVE_SET_METADATA_H_ */
