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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_FIELD_SLICE_SET_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_FIELD_SLICE_SET_H_

#include "bf-p4c/phv/phv_fields.h"

/// Implements comparisons correctly.
class FieldSliceSet : public std::set<const PHV::FieldSlice *, PHV::FieldSlice::Less> {
 public:
    using std::set<const PHV::FieldSlice *, PHV::FieldSlice::Less>::set;

    bool operator==(const FieldSliceSet &other) const {
        if (size() != other.size()) return false;

        auto it1 = begin();
        auto it2 = other.begin();
        while (it1 != end()) {
            if (!PHV::FieldSlice::equal(*it1, *it2)) return false;
            ++it1;
            ++it2;
        }

        return true;
    }

    bool operator<(const FieldSliceSet &other) const {
        if (size() != other.size()) return size() < other.size();

        auto it1 = begin();
        auto it2 = other.begin();
        while (it1 != end()) {
            if (!PHV::FieldSlice::equal(*it1, *it2)) return PHV::FieldSlice::less(*it1, *it2);
            ++it1;
            ++it2;
        }

        return false;
    }

    bool operator>(const FieldSliceSet &other) const {
        if (size() != other.size()) return size() > other.size();

        auto it1 = begin();
        auto it2 = other.begin();
        while (it1 != end()) {
            if (!PHV::FieldSlice::equal(*it1, *it2)) return PHV::FieldSlice::greater(*it1, *it2);
            ++it1;
            ++it2;
        }

        return false;
    }

    bool operator!=(const FieldSliceSet &other) const { return !operator==(other); }

    bool operator<=(const FieldSliceSet &other) const { return !operator>(other); }

    bool operator>=(const FieldSliceSet &other) const { return !operator<(other); }
};

using PovBitSet = FieldSliceSet;

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_FIELD_SLICE_SET_H_ */
