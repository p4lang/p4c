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

#include "remove_set_metadata.h"

#include "bf-p4c/midend/path_linearizer.h"
#include "bf-p4c/midend/type_categories.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace BFN {

RemoveSetMetadata::RemoveSetMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
    : refMap(refMap), typeMap(typeMap) {}

bool isEgressIntrinsicHeader(const IR::Type *type) {
    if (auto header = type->to<IR::Type_Header>()) {
        if (header->name == "egress_intrinsic_metadata_t") return true;
    }

    return false;
}

const IR::AssignmentStatement *RemoveSetMetadata::preorder(IR::AssignmentStatement *assignment) {
    prune();

    auto *parser = findContext<IR::BFN::TnaParser>();
    if (!parser) return assignment;
    if (parser->thread != EGRESS) return assignment;

    PathLinearizer linearizer;
    assignment->left->apply(linearizer);

    // If the destination of the write isn't a path-like expression, or if it's
    // too complex to analyze, err on the side of caution and don't remove it.
    if (!linearizer.linearPath) {
        LOG4("Won't remove egress parser assignment to complex object: " << assignment);
        return assignment;
    }

    // We only want to remove writes to metadata.
    auto &path = *linearizer.linearPath;
    if (!isMetadataReference(path, typeMap)) {
        LOG4(
            "Won't remove egress parser assignment to non-metadata "
            "object: "
            << assignment);
        return assignment;
    }

    // Even a write to metadata shouldn't be removed if the metadata is local to
    // the parser.
    auto *param = getContainingParameter(path, refMap);
    if (!param) {
        LOG4("Won't remove egress parser assignment to local object: " << assignment);
        return assignment;
    }

    // Don't remove writes to intrinsic metadata or compiler-generated metadata.
    auto *paramType = typeMap->getType(param);
    BUG_CHECK(paramType, "No type for param: %1%", param);

    // "egress_intrinsic_metadata_t" is generated in egress packet buffer, it doesn't
    // make sense to re-set these in egress parser
    if (!isEgressIntrinsicHeader(paramType)) {
        if (isIntrinsicMetadataType(paramType) || isCompilerGeneratedType(paramType)) {
            LOG4("Won't remove egress parser assignment to intrinsic metadata: " << assignment);
            return assignment;
        }
    }

    // This is a write to non-local metadata; remove it.
    LOG4("Removing egress parser assignment to metadata: " << assignment);
    return nullptr;
}

}  // namespace BFN
