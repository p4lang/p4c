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

#ifndef EXTENSIONS_BF_P4C_PARDE_CLOT_FIELD_SLICE_SET_H_
#define EXTENSIONS_BF_P4C_PARDE_CLOT_FIELD_SLICE_SET_H_

#include "bf-p4c/phv/phv_fields.h"

/// Implements comparisons correctly.
class FieldSliceSet : public std::set<const PHV::FieldSlice*, PHV::FieldSlice::Less> {
 public:
    using std::set<const PHV::FieldSlice*, PHV::FieldSlice::Less>::set;

    bool operator==(const FieldSliceSet& other) const {
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

    bool operator<(const FieldSliceSet& other) const {
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

    bool operator>(const FieldSliceSet& other) const {
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

    bool operator!=(const FieldSliceSet& other) const {
        return !operator==(other);
    }

    bool operator<=(const FieldSliceSet& other) const {
      return !operator>(other);
    }

    bool operator>=(const FieldSliceSet& other) const {
      return !operator<(other);
    }
};

using PovBitSet = FieldSliceSet;

#endif /* EXTENSIONS_BF_P4C_PARDE_CLOT_FIELD_SLICE_SET_H_ */
