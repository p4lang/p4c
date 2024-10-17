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
 * \defgroup PortableSwitchTranslation BFN::PortableSwitchTranslation
 * \ingroup ArchTranslation
 * \brief Set of passes that translate PSA architecture.
 */
#ifndef EXTENSIONS_BF_P4C_ARCH_PSA_PSA_H_
#define EXTENSIONS_BF_P4C_ARCH_PSA_PSA_H_

#include <optional>
#include <boost/algorithm/string.hpp>
#include "ir/ir.h"
#include "ir/namemap.h"
#include "ir/pattern.h"
#include "frontends/common/options.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/sideEffects.h"
#include "frontends/p4/methodInstance.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/arch/arch.h"

namespace BFN {

/**
 * \ingroup PortableSwitchTranslation
 * \brief PassManager that governs normalization of PSA architecture.
 */
class PortableSwitchTranslation : public PassManager {
 public:
    const IR::ToplevelBlock   *toplevel = nullptr;
    PortableSwitchTranslation(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                              BFN_Options& options);
};

}  // namespace BFN

#endif /* EXTENSIONS_BF_P4C_ARCH_PSA_PSA_H_ */
