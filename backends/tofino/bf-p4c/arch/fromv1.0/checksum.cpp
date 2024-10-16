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

#include "bf-p4c/arch/fromv1.0/checksum.h"

namespace BFN {
namespace V1 {

TranslateParserChecksums::TranslateParserChecksums(ProgramStructure *structure,
                                                   P4::ReferenceMap *refMap,
                                                   P4::TypeMap *typeMap)
        : parserGraphs(refMap, cstring()) {
    auto collectParserChecksums = new BFN::V1::CollectParserChecksums(refMap, typeMap);
    auto insertParserChecksums = new BFN::V1::InsertParserChecksums(this,
                                                  collectParserChecksums,
                                                  &parserGraphs,
                                                  structure);
    addPasses({&parserGraphs,
               collectParserChecksums,
               insertParserChecksums
              });
}

}  // namespace V1
}  // namespace BFN
