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

#include "bf-p4c/arch/fromv1.0/checksum.h"

namespace BFN {
namespace V1 {

TranslateParserChecksums::TranslateParserChecksums(ProgramStructure *structure,
                                                   P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
    : parserGraphs(refMap, cstring()) {
    auto collectParserChecksums = new BFN::V1::CollectParserChecksums(refMap, typeMap);
    auto insertParserChecksums =
        new BFN::V1::InsertParserChecksums(this, collectParserChecksums, &parserGraphs, structure);
    addPasses({&parserGraphs, collectParserChecksums, insertParserChecksums});
}

}  // namespace V1
}  // namespace BFN
