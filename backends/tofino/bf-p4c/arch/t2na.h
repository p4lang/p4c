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
 * \defgroup T2naArchTranslation BFN::T2naArchTranslation
 * \ingroup ArchTranslation
 * \brief Set of passes that translate T2NA architecture.
 */
#ifndef BF_P4C_ARCH_T2NA_H_
#define BF_P4C_ARCH_T2NA_H_

#include "arch.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

class BFN_Options;

namespace BFN {

/**
 * \ingroup T2naSwitchTranslation
 * \brief PassManager that governs normalization of T2NA architecture.
 */
struct T2naArchTranslation : public PassManager {
    T2naArchTranslation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, BFN_Options &options);

    ProgramPipelines pipelines;
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_T2NA_H_ */
