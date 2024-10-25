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

#ifndef BF_P4C_MIDEND_COPY_BLOCK_PRAGMAS_H_
#define BF_P4C_MIDEND_COPY_BLOCK_PRAGMAS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

/**
 * \ingroup midend
 * \brief Pass that copies block annotations to the block's tables.
 */
class CopyBlockPragmas : public PassManager {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    std::set<cstring> pragmas;
    std::map<const IR::P4Table *, ordered_map<cstring, const IR::Annotation *>> toAdd;
    class FindPragmasFromApply;
    class CopyToTables;

 public:
    CopyBlockPragmas(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, P4::TypeChecking *typeChecking,
                     std::set<cstring> pragmas);
    CopyBlockPragmas(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, std::set<cstring> pragmas)
        : CopyBlockPragmas(refMap, typeMap, new P4::TypeChecking(refMap, typeMap), pragmas) {}
};

#endif /* BF_P4C_MIDEND_COPY_BLOCK_PRAGMAS_H_ */
