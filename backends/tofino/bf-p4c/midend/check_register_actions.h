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

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_REGISTER_ACTIONS_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_REGISTER_ACTIONS_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace BFN {

/**
 * \ingroup midend
 * \brief PassManager that checks if the RegisterActions work on proper Registers.
 *
 * Registers and their RegisterActions use don't care types that are unified. This
 * pass just checks if the RegisterAction works with register that has the same width.
 * If not we can still allow it, but only if the index width is appropriately increased/decreased.
 */
class CheckRegisterActions : public Inspector {
    P4::TypeMap *typeMap;

    bool preorder(const IR::Declaration_Instance *di) override;

 public:
    explicit CheckRegisterActions(P4::TypeMap *typeMap) : typeMap(typeMap) {}
};

}  // namespace BFN

#endif  // BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_REGISTER_ACTIONS_H_
