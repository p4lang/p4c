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

#ifndef PARDE_MARSHAL_H_
#define PARDE_MARSHAL_H_

#include <iostream>
#include "lib/cstring.h"
#include "bf-p4c/ir/gress.h"

namespace P4 {

class JSONGenerator;
class JSONLoader;

struct MarshaledFrom {
    // use those two to uniquely identify a field.
    gress_t gress;
    cstring field_name;
    /// Here the `pre` and `post` is in network order, which means, on the wire
    /// the oreder is [pre_padding, field, post_padding].
    /// TODO: currently, we do not have post_padding. When we create padding for
    /// those marshalable fields, we append paddings before the field.
    size_t pre_padding;
    // size_t post_padding;

    std::string toString() const;

    bool operator==(const MarshaledFrom& other) const {
        return gress == other.gress && field_name == other.field_name
               && pre_padding == other.pre_padding;
    }

    /// JSON serialization/deserialization.
    void toJSON(JSONGenerator& json) const;
    static MarshaledFrom fromJSON(JSONLoader& json);

    friend std::ostream& operator<<(std::ostream& s, const MarshaledFrom& m);
    friend P4::JSONGenerator& operator<<(P4::JSONGenerator& out, const MarshaledFrom& c);

    MarshaledFrom()
        : gress(INGRESS), field_name(""), pre_padding(0) { }
    MarshaledFrom(gress_t gress, cstring name)
        : gress(gress), field_name(name), pre_padding(0) { }
    MarshaledFrom(gress_t gress, cstring name, size_t pre_padding)
        : gress(gress), field_name(name), pre_padding(pre_padding) { }
};

inline std::ostream& operator<<(std::ostream& s, const MarshaledFrom& m) {
    s << "(" << m.gress << ", " << m.field_name << ", " << m.pre_padding << ")";
    return s;
}

inline JSONGenerator& operator<<(JSONGenerator& out, const MarshaledFrom& c) {
    return out << c;
}

}  // namespace P4

#endif /* PARDE_MARSHAL_H_ */
