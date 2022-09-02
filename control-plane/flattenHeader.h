/*
Copyright 2019-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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
    P4::TypeMap* typeMap;
    IR::Type_Header* flattenedHeader;
    std::vector<cstring> nameSegments{};
    std::vector<const IR::Annotations*> allAnnotations{};
    bool needsFlattening{false};

    FlattenHeader(P4::TypeMap* typeMap, IR::Type_Header* flattenedHeader);

    void doFlatten(const IR::Type* type);

    cstring makeName(cstring sep) const;
    const IR::Annotations* mergeAnnotations() const;

 public:
    /// If the @headerType needs flattening, creates a clone of the IR node with
    /// a new flattened field list. Otherwise returns @headerType. This does not
    /// modify the IR.
    static const IR::Type_Header* flatten(
        P4::TypeMap* typeMap, const IR::Type_Header* headerType);
};

}  // namespace ControlPlaneAPI

}  // namespace P4

#endif  // CONTROL_PLANE_FLATTENHEADER_H_
