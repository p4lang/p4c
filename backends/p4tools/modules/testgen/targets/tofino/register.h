/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/tofino/target.h"
#include "backends/p4tools/modules/testgen/testgen.h"

namespace P4::P4Tools::P4Testgen {

/// Register the Tofino testgen target with the testgen framework.
inline void tofinoRegisterTestgenTarget() {
    Tofino::Tofino_TnaTestgenTarget::make();
    Tofino::JBay_T2naTestgenTarget::make();
}

}  // namespace P4::P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_REGISTER_H_ */
