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

/**
 * \defgroup SimpleSwitchTranslation BFN::SimpleSwitchTranslation
 * \ingroup ArchTranslation
 * \brief Set of passes that translate v1model architecture.
 */
#ifndef BF_P4C_ARCH_V1MODEL_H_
#define BF_P4C_ARCH_V1MODEL_H_

#include <boost/algorithm/string.hpp>
#include "ir/ir.h"
#include "ir/namemap.h"
#include "frontends/common/options.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/sideEffects.h"
#include "frontends/p4/methodInstance.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/arch/fromv1.0/v1_program_structure.h"
#include "bf-p4c/arch/fromv1.0/v1_converters.h"
#include "bf-p4c/arch/arch.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"

namespace BFN {

/** \ingroup SimpleSwitchTranslation */
class AddAdjustByteCount : public Modifier {
    V1::ProgramStructure *structure;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    bool preorder(IR::Declaration_Instance* decl) override;
 public:
     AddAdjustByteCount(V1::ProgramStructure *structure,
             P4::ReferenceMap* refMap, P4::TypeMap *typeMap)
         : structure(structure), refMap(refMap), typeMap(typeMap) {}
};

/**
 * \ingroup SimpleSwitchTranslation
 * \brief PassManager that governs normalization of v1model architecture.
 */
class SimpleSwitchTranslation : public PassManager {
 public:
    V1::ProgramStructure structure;
    const IR::ToplevelBlock   *toplevel = nullptr;

    SimpleSwitchTranslation(P4::ReferenceMap* refMap, P4::TypeMap* typeMap, BFN_Options& options);

    const IR::ToplevelBlock* getToplevelBlock() { CHECK_NULL(toplevel); return toplevel; }
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_V1MODEL_H_ */
