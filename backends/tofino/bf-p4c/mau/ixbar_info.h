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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_IXBAR_INFO_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_IXBAR_INFO_H_

#include <array>

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource.h"
#include "lib/safe_vector.h"

namespace BFN {

using namespace P4;

/**
 * Log IXBar allocation
 */
class CollectIXBarInfo : public MauInspector {
    const PhvInfo &phv;
    std::map<int, safe_vector<IXBar::Use::Byte>> _stage;
    std::map<IXBar::Use::Byte, const IR::MAU::Table *> _byteToTables;

    profile_t init_apply(const IR::Node *) override;

    void end_apply(const IR::Node *) override;

    void postorder(const IR::MAU::Table *) override;

    void sort_ixbar_byte();
    std::string print_ixbar_byte() const;

 public:
    explicit CollectIXBarInfo(const PhvInfo &phv) : phv(phv) {}
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_IXBAR_INFO_H_ */
