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

#ifndef EXTENSIONS_BF_P4C_COMMON_PRAGMA_COLLECT_GLOBAL_PRAGMA_H_
#define EXTENSIONS_BF_P4C_COMMON_PRAGMA_COLLECT_GLOBAL_PRAGMA_H_

#include <vector>
#include "ir/ir.h"

using namespace P4;

class CollectGlobalPragma : public Inspector {
    /// Vector of all global pragmas that need to communicated to the backend.
    std::vector<const IR::Annotation*> global_pragmas_;

    /// Given an IR::StructField* node, extract the header name associated with that node. @returns
    /// an empty string if the name annotation does not actually have an associated name.
    cstring getStructFieldName(const IR::StructField*) const;

    /// For all PHV related pragmas that have identifying information associated with them, add
    /// those annotations directly to the global_pragmas_ vector.
    bool preorder(const IR::Annotation *) override;

    /// Preorder function that extracts pragmas associated with the StructField node but which
    /// themselves do not include identifying characteristics (such as field names).
    /// E.g. \@pragma pa_not_parsed ingress
    ///      header header_type_t hdr1;
    /// The pragma pa_parsed is associated with the declaration for hdr1 (StructField node) but does
    /// not contain the name of the header. We need to extract the header name from the StructField
    /// node with which the annotation is associated.
    bool preorder(const IR::StructField*) override;

 public:
    const std::vector<const IR::Annotation*>& global_pragmas() const { return global_pragmas_; }

    /// Vector of all PHV pragmas recognized by the backend.
    static const std::vector<std::string> *g_global_pragma_names;

    /// Check if pragma exists
    const IR::Annotation *exists(const char *pragma_name) const;
};

#endif /* EXTENSIONS_BF_P4C_COMMON_PRAGMA_COLLECT_GLOBAL_PRAGMA_H_ */
