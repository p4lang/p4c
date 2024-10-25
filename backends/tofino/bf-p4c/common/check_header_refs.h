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

#ifndef BF_P4C_COMMON_CHECK_HEADER_REFS_H_
#define BF_P4C_COMMON_CHECK_HEADER_REFS_H_

#include "bf-p4c/common/utils.h"
#include "ir/ir.h"

using namespace P4;

/**
 * Once the CopyHeaderEliminator and HeaderPushPo passes have run, HeaderRef
 * objects should no longer exist in the IR as arguments, only as children of
 * Member nodes.  This pass checks and throws a BUG if this property does not
 * hold.
 */
class CheckForHeaders final : public Inspector {
    bool preorder(const IR::Member *) { return false; }
    bool preorder(const IR::HeaderRef *h) {
        if (h->toString() == "ghost::gh_intr_md") return false;
        BUG("Header present in IR not under Member: %s", h->toString());
    }
};

#endif /* BF_P4C_COMMON_CHECK_HEADER_REFS_H_ */
