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

#ifndef BF_P4C_MIDEND_COPY_BLOCK_PRAGMAS_H_
#define BF_P4C_MIDEND_COPY_BLOCK_PRAGMAS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

/**
 * \ingroup midend
 * \brief Pass that copies block annotations to the block's tables.
 */
class CopyBlockPragmas : public PassManager {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    std::set<cstring> pragmas;
    std::map<const IR::P4Table *, ordered_map<cstring, const IR::Annotation *>>   toAdd;
    class FindPragmasFromApply;
    class CopyToTables;

 public:
    CopyBlockPragmas(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, P4::TypeChecking *typeChecking,
                     std::set<cstring> pragmas);
    CopyBlockPragmas(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, std::set<cstring> pragmas)
    : CopyBlockPragmas(refMap, typeMap, new P4::TypeChecking(refMap, typeMap), pragmas) {}
};

#endif /* BF_P4C_MIDEND_COPY_BLOCK_PRAGMAS_H_ */
