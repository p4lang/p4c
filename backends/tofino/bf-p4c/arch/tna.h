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
 * \defgroup TnaArchTranslation BFN::TnaArchTranslation
 * \ingroup ArchTranslation
 * \brief Set of passes that translate TNA architecture.
 */
#ifndef BF_P4C_ARCH_TNA_H_
#define BF_P4C_ARCH_TNA_H_

#include "arch.h"
#include "ir/ir.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

class BFN_Options;

namespace BFN {

/** \ingroup TnaArchTranslation */
const cstring ExternPortMetadataUnpackString = "port_metadata_unpack"_cs;
/** \ingroup TnaArchTranslation */
const cstring ExternDynamicHashString = "hash"_cs;

/**
 * \ingroup TnaArchTranslation
 * Normalize a TNA program's control and package instantiations so that later
 * passes don't need to account for minor variations in the architecture.
 *
 * The TNA architecture includes a number of `@optional` parameters to controls.
 * Users who don't need the metadata in these parameters can avoid some
 * boilerplate by leaving them out of the parameter list of the controls they
 * instantiate the TNA package with. This makes life easier for the user, but it
 * means that the compiler must contend with TNA programs which may or may not
 * include these parameters.
 *
 * To make things simpler for the rest of the midend and backend, this pass
 * reintroduces all the optional parameters.
 *
 * @pre The IR represents a valid TNA program, and all midend passes have run.
 * @post The IR represents a valid TNA program with all `@optional` parameters
 * specified by the architecture present.
 */
struct NormalizeNativeProgram : public PassManager {
    NormalizeNativeProgram(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, BFN_Options &options);
};

/**
 * \ingroup TnaSwitchTranslation
 * \brief PassManager that governs normalization of TNA architecture.
 */
struct TnaArchTranslation : public PassManager {
    TnaArchTranslation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, BFN_Options &options);

    ProgramPipelines threads;
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_TNA_H_ */
