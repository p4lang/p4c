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
