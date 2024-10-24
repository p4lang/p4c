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

#include "bf-p4c/phv/v2/tx_score.h"

#include <sstream>

#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/utils/utils.h"
#include "lib/exceptions.h"

namespace PHV {
namespace v2 {

TxScore *MinPackTxScoreMaker::make(const Transaction &tx) const {
    int n_packed = 0;
    bool use_mocha_or_dark = false;
    using ContainerAllocStatus = PHV::Allocation::ContainerAllocStatus;
    const auto *parent = tx.getParent();
    for (const auto &kv : tx.get_actual_diff()) {
        const auto &c = kv.first;
        use_mocha_or_dark |= (c.type().kind() == Kind::mocha || c.type().kind() == Kind::dark);
        auto parent_status = parent->alloc_status(c);
        if (parent_status != ContainerAllocStatus::EMPTY && !kv.second.slices.empty()) {
            n_packed++;
        }
    }
    return new MinPackTxScore(n_packed, use_mocha_or_dark);
}

bool MinPackTxScore::better_than(const TxScore *other_score) const {
    if (other_score == nullptr) return true;
    const MinPackTxScore *other = dynamic_cast<const MinPackTxScore *>(other_score);
    BUG_CHECK(other, "comparing MinPackTxScore with score of different type: %1%",
              other_score->str());
    if (use_mocha_or_dark != other->use_mocha_or_dark) {
        return use_mocha_or_dark < other->use_mocha_or_dark;
    }
    return n_packed < other->n_packed;
}

std::string MinPackTxScore::str() const {
    std::stringstream ss;
    ss << "minpack{ mocha_dark:" << use_mocha_or_dark << ", n_packed:" << n_packed << " }";
    return ss.str();
}

}  // namespace v2
}  // namespace PHV
