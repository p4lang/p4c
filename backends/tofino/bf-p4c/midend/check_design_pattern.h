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

/**
 * \defgroup CheckDesignPattern BFN::CheckDesignPattern
 * \ingroup midend
 * \brief Set of passes that check for design patterns.
 *
 * Sometimes, we would like to guide user to write P4 program in certain ways
 * that can be supported by our backend. This might be desirable for a couple
 * reasons:
 * 1. our implementation is not ready for any arbitrary p4 program.
 * 2. Some legal P4 program does not provide enough information for the
 * backend.
 *
 * To give an example for the second case. P4 frontend allows user to write
 * code for Mirror extern as follows:
 *
 *     Mirror() mirror;
 *     mirror.emit(session_id, { ... });
 *
 * Compiler frontend will infer type tuple for the field list, however, a list/tuple
 * does not provide enough information to the backend to optimize the layout,
 * a.k.a, flexible packing. The flexible packing algorithm assume there is a header
 * type to be repacked.
 *
 * Due to the limitation, we will have to guide the user to write P4 in way that
 * can be supported with the current backend implementation. This pass a place
 * that these rules can be reinforced, and error message are generated to steer
 * programmer in the right direction.
 */
#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_DESIGN_PATTERN_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_DESIGN_PATTERN_H_

#include "bf-p4c-options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace BFN {
/**
 * \ingroup CheckDesignPattern
 * \brief Checks if the emit function call is valid with the TNA constraints.
 *
 * As the name suggested, TNA requires the emit() function call
 * to provide the type of the field list. There are a few rules:
 *
 * 1. the type must be a header type, it cannot be struct or tuple;
 *
 * 2. if the type is header and the header does not contain field with a
 * flexible annotation. The compiler should encourage the programmer to pad the
 * header to byte-boundary. Otherwise, PHV allocation might fail due to
 * unimplemented constraint.
 */
class CheckExternValidity : public Inspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

 public:
    CheckExternValidity(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {}
    bool preorder(const IR::MethodCallExpression *) override;
};

/**
 * \typedef ActionExterns
 * \ingroup CheckDesignPattern
 */
typedef std::map<const IR::P4Action *, std::vector<const P4::ExternMethod *>> ActionExterns;
/**
 * \ingroup CheckDesignPattern
 */
class FindDirectExterns : public Inspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    ActionExterns &directExterns;

 public:
    FindDirectExterns(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, ActionExterns &directExterns)
        : refMap(refMap), typeMap(typeMap), directExterns(directExterns) {}
    bool preorder(const IR::MethodCallExpression *) override;
    profile_t init_apply(const IR::Node *root) override;
};

/**
 * \ingroup CheckDesignPattern
 */
class CheckDirectExternsOnTables : public Modifier {
    P4::ReferenceMap *refMap;
    ActionExterns &directExterns;

 public:
    CheckDirectExternsOnTables(P4::ReferenceMap *refMap, P4::TypeMap *,
                               ActionExterns &directExterns)
        : refMap(refMap), directExterns(directExterns) {}
    bool preorder(IR::P4Table *) override;
};

/**
 * \ingroup CheckDesignPattern
 *
 * Direct resources declared in the program and used in an action need to be
 * also specified on the table. Throw a compile time error if resources are
 * missing on the table. As otherwise, the appropriate control plane api's do
 * not get generated for that resource.
 */
class CheckDirectResourceInvocation : public PassManager {
    ActionExterns directExterns;

 public:
    static const std::map<cstring, cstring> externsToProperties;
    CheckDirectResourceInvocation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        passes.push_back(new FindDirectExterns(refMap, typeMap, directExterns));
        passes.push_back(new CheckDirectExternsOnTables(refMap, typeMap, directExterns));
    }
};

/**
 * \ingroup CheckTableConstEntries
 * It does not make any sense to have an empty const entry list when defining a table. If const
 * entries is defined, table's entries size is based on the size of const entries and cannot be
 * modified during runtime. Based on P4_16 language spec v1.2.2, there is only definition of const
 * entries, but in the future, it is possible that both mutable and immutable enteris can be
 * defined. Therefore, if P4_16 spec changes, this pass should be changed accordingly.
 */

class CheckTableConstEntries : public Inspector {
 public:
    CheckTableConstEntries() {}
    bool preorder(const IR::P4Table *) override;
};

/**
 * \ingroup CheckDesignPattern
 * \brief Top level PassManager that governs checking for design patterns.
 */
class CheckDesignPattern : public PassManager {
 public:
    CheckDesignPattern(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        passes.push_back(new CheckExternValidity(refMap, typeMap));
        if (BackendOptions().arch != "v1model")
            passes.push_back(new CheckDirectResourceInvocation(refMap, typeMap));
    }
};

}  // namespace BFN

#endif  // BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_DESIGN_PATTERN_H_
