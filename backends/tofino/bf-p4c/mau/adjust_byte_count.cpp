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

#include "bf-p4c/mau/adjust_byte_count.h"

namespace BFN {

AdjustByteCountSetup::AdjustByteCountSetup() { addPasses({new Scan(*this), new Update(*this)}); }

int AdjustByteCountSetup::Update::get_bytecount(IR::MAU::AttachedMemory * /*unused*/) {
    auto *orig_am = getOriginal()->to<IR::MAU::AttachedMemory>();
    if (!orig_am) return 0;
    if (self.adjust_byte_counts.count(orig_am) > 0) {
        return -1 * self.adjust_byte_counts[orig_am];
    }
    return 0;
}

const IR::MAU::Counter *AdjustByteCountSetup::Update::preorder(IR::MAU::Counter *counter) {
    LOG1("Counter : " << counter);
    auto bytecount_adjust = get_bytecount(counter);
    if (bytecount_adjust != 0) {
        if (counter->bytecount_adjust != 0 && counter->bytecount_adjust != bytecount_adjust) {
            warning(
                "Overwriting previously set byte adjust value on counter %1%"
                " from %2% to %3%. This value was possibly also set using a"
                " pragma. Please remove any '@adjust_byte_count' pragma on the"
                " counter. This value should only be set through the 'count'"
                " method argument",
                counter, counter->bytecount_adjust, bytecount_adjust);
        }
        counter->bytecount_adjust = bytecount_adjust;
        LOG3(" Setting bytecount_adjust on " << counter << " to " << counter->bytecount_adjust);
    }
    return counter;
}

const IR::MAU::Meter *AdjustByteCountSetup::Update::preorder(IR::MAU::Meter *meter) {
    LOG1("Meter : " << meter);
    auto bytecount_adjust = get_bytecount(meter);
    if (bytecount_adjust != 0) {
        if (meter->bytecount_adjust != 0 && meter->bytecount_adjust != bytecount_adjust) {
            warning(
                "Overwriting previously set byte adjust value on meter %1%"
                " from %2% to %3%. This value was possibly also set using a"
                " pragma. Please remove any '@adjust_byte_count' pragma on the"
                " meter. This value should only be set through the 'execute'"
                " method argument",
                meter, meter->bytecount_adjust, bytecount_adjust);
        }
        meter->bytecount_adjust = bytecount_adjust;
        LOG3(" Setting bytecount_adjust on " << meter << " to " << meter->bytecount_adjust);
    }
    return meter;
}

bool AdjustByteCountSetup::Scan::preorder(const IR::MAU::Primitive *prim) {
    LOG1("Primitive : " << prim);

    const IR::MAU::AttachedMemory *obj = nullptr;
    auto dot = prim->name.find('.');
    cstring method = dot ? cstring(dot + 1) : prim->name;
    while (dot && dot > prim->name && std::isdigit(dot[-1])) --dot;
    auto objType = dot ? prim->name.before(dot) : cstring();

    if ((((objType == "DirectCounter") || (objType == "Counter")) && (method == "count")) ||
        (((objType == "DirectMeter") || (objType == "Meter")) && (method == "execute"))) {
        if (prim->operands.size() == 0) return true;

        auto gref = prim->operands.at(0)->to<IR::GlobalRef>();
        if (!gref) return true;

        int idx = -1;
        if (auto tprim = prim->to<IR::MAU::TypedPrimitive>()) {
            for (auto o : tprim->op_names) {
                if (o.second == "adjust_byte_count") idx = o.first;
            }
        }

        if (idx < 0) return false;

        auto adjust_byte_count_op = prim->operands.at(idx)->to<IR::Constant>();
        if (!adjust_byte_count_op) {
            error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                  "Adjust byte count operand on primitive %1% "
                  "does not resolve to a constant value",
                  prim);
            return false;
        }
        auto adjust_byte_count = adjust_byte_count_op->asInt64();

        obj = gref->obj->to<IR::MAU::AttachedMemory>();
        if (self.adjust_byte_counts.count(obj) > 0) {
            auto exp_count = self.adjust_byte_counts[obj];
            if (adjust_byte_count != exp_count) {
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                      "All '%1%' method calls on '%2%' do not have the same"
                      "  value for argument (adjust_byte_count) %2%",
                      prim, obj);
                return false;
            }
        } else {
            auto elem = std::make_pair(obj, adjust_byte_count);
            self.adjust_byte_counts.insert(elem);
            LOG3(" Adding " << obj << " with adjust byte count value " << adjust_byte_count);
        }
    }

    return true;
}

}  // namespace BFN
