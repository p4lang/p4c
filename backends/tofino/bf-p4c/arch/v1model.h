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
 * \defgroup SimpleSwitchTranslation BFN::SimpleSwitchTranslation
 * \ingroup ArchTranslation
 * \brief Set of passes that translate v1model architecture.
 */
#ifndef BF_P4C_ARCH_V1MODEL_H_
#define BF_P4C_ARCH_V1MODEL_H_

#include <boost/algorithm/string.hpp>

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/arch/fromv1.0/v1_converters.h"
#include "bf-p4c/arch/fromv1.0/v1_program_structure.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/ir/gress.h"
#include "frontends/common/options.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/sideEffects.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/uniqueNames.h"
#include "ir/ir.h"
#include "ir/namemap.h"

namespace BFN {

/** \ingroup SimpleSwitchTranslation */
class AddAdjustByteCount : public Modifier {
    V1::ProgramStructure *structure;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    bool preorder(IR::Declaration_Instance *decl) override;

 public:
    AddAdjustByteCount(V1::ProgramStructure *structure, P4::ReferenceMap *refMap,
                       P4::TypeMap *typeMap)
        : structure(structure), refMap(refMap), typeMap(typeMap) {}
};

/**
 * \ingroup SimpleSwitchTranslation
 * \brief PassManager that governs normalization of v1model architecture.
 */
class SimpleSwitchTranslation : public PassManager {
 public:
    V1::ProgramStructure structure;
    const IR::ToplevelBlock *toplevel = nullptr;

    SimpleSwitchTranslation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, BFN_Options &options);

    const IR::ToplevelBlock *getToplevelBlock() {
        CHECK_NULL(toplevel);
        return toplevel;
    }
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_V1MODEL_H_ */
