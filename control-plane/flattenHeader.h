/*
 * SPDX-FileCopyrightText: 2019 Barefoot Networks, Inc.
 * Copyright 2019-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTROL_PLANE_FLATTENHEADER_H_
#define CONTROL_PLANE_FLATTENHEADER_H_

#include <vector>

#include "ir/ir.h"

namespace P4 {

class TypeMap;

namespace ControlPlaneAPI {

/// Flattens a header type "locally", without modifying the IR.
class FlattenHeader {
 private:
    P4::TypeMap *typeMap;
    IR::Type_Header *flattenedHeader;
    std::vector<cstring> nameSegments;
    std::vector<const IR::Vector<IR::Annotation> *> allAnnotations;
    bool needsFlattening{false};

    FlattenHeader(P4::TypeMap *typeMap, IR::Type_Header *flattenedHeader);

    void doFlatten(const IR::Type *type);

    cstring makeName(std::string_view sep) const;
    IR::Vector<IR::Annotation> mergeAnnotations() const;

 public:
    /// If the @headerType needs flattening, creates a clone of the IR node with
    /// a new flattened field list. Otherwise returns @headerType. This does not
    /// modify the IR.
    static const IR::Type_Header *flatten(P4::TypeMap *typeMap, const IR::Type_Header *headerType);
};

}  // namespace ControlPlaneAPI

}  // namespace P4

#endif  // CONTROL_PLANE_FLATTENHEADER_H_
