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

#ifndef BF_P4C_MIDEND_CHECK_HEADER_ALIGNMENT_H_
#define BF_P4C_MIDEND_CHECK_HEADER_ALIGNMENT_H_

#include "ir/ir.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "bf-p4c/midend/type_checker.h"

namespace P4 {
class TypeMap;
class ReferenceMap;
}  // namespace P4

namespace BFN {

/**
 * Check for padding field with explicit assignment and report a warning if found.
 *
 * Setting a padding field add some unnecessary constraints. If a field must be set
 * to a specific value, it is probably because it is not a padding field and must
 * not be classify as such.
 */
class CheckPadAssignment final : public Inspector {
 private:
    bool preorder(const IR::AssignmentStatement* statement) override;

 public:
    CheckPadAssignment() {}
};

/**
 * Check for non-byte-aligned header types and report an error if any are found.
 *
 * On Tofino, we can only parse and deparse byte-aligned headers, so
 * non-byte-aligned headers aren't useful.
 *
 * TODO: We could theoretically allow non-byte-aligned headers if they're
 * only used in the MAU, but for now I've avoided that since it seems less
 * confusing for the P4 programmer to have a simple and clear rule.
 *
 * TODO: We do not enforce this rule on headers that is used in Mirror
 * Resubmit, Digest, as the fields in these headers are going to be reordered.
 *
 * By default, all fields in the headers are candidates for reordering, except
 * the 'tag' fields that are used by parser to differentiate different mirror
 * packet headers, etc. These tagged fields are marked with the \@tag annotation.
 */
class CheckHeaderAlignment final : public Inspector {
    P4::TypeMap* typeMap;

 private:
    bool preorder(const IR::Type_Header* header) override;

 public:
    explicit CheckHeaderAlignment(P4::ReferenceMap*, P4::TypeMap* typeMap) :
        typeMap(typeMap) {}
};

using HeaderTypeMap = ordered_map<cstring, const IR::Type_Header*>;
using HeaderToPad = std::unordered_set<cstring>;
using ResubmitHeaders = std::unordered_set<cstring>;

class FindPaddingCandidate : public Inspector {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    HeaderToPad* headers_to_pad;
    ResubmitHeaders* resubmit_headers;
    HeaderTypeMap* all_header_types;

 public:
    std::vector<cstring> find_headers_to_pad(P4::MethodInstance* mi);
    std::vector<const IR::Type_StructLike*> find_all_headers(P4::MethodInstance* mi);
    void check_mirror(P4::MethodInstance* mi);
    void check_digest(P4::MethodInstance* mi);
    void check_resubmit(P4::MethodInstance* mi);
    bool preorder(const IR::MethodCallExpression* expr) override;
    bool preorder(const IR::Type_Header*) override;
    explicit FindPaddingCandidate(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
            HeaderToPad* headers_to_pad,
            ResubmitHeaders* resubmit_headers,
            HeaderTypeMap* all_header_types) :
        refMap(refMap), typeMap(typeMap), headers_to_pad(headers_to_pad),
        resubmit_headers(resubmit_headers),
        all_header_types(all_header_types) {}
};

class AddPaddingFields : public Transform {
    P4::TypeMap* typeMap;
    HeaderToPad* headers_to_pad;
    ResubmitHeaders* resubmit_headers;
    HeaderTypeMap* all_header_types;

 public:
    explicit AddPaddingFields(P4::ReferenceMap*, P4::TypeMap* typeMap,
            HeaderToPad* headers_to_pad,
            ResubmitHeaders* resubmit_headers,
            HeaderTypeMap* all_header_types) :
        typeMap(typeMap), headers_to_pad(headers_to_pad),
        resubmit_headers(resubmit_headers),
        all_header_types(all_header_types) {}

    const IR::Node* preorder(IR::Type_Header* st) override;
    const IR::Node* preorder(IR::StructExpression *) override;
};

// This pass transforms all resubmit headers from Type_Header to Type_FixedSizeHeader
class TransformResubmitHeaders : public Transform {
    ResubmitHeaders* resubmit_headers;
 public:
    explicit TransformResubmitHeaders(ResubmitHeaders* resubmit_headers) :
        resubmit_headers(resubmit_headers) {}
    const IR::Node* preorder(IR::Type_Header *) override;
};

class PadFlexibleField : public PassManager {
    // headers used in mirror, resubmit and digest
    HeaderToPad headers_to_pad;
    // all header types before padding is added.
    HeaderTypeMap all_header_types;
    ResubmitHeaders resubmit_headers;

 public:
    PadFlexibleField(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
        addPasses({
            new FindPaddingCandidate(refMap, typeMap,
                    &headers_to_pad, &resubmit_headers, &all_header_types),
            new AddPaddingFields(refMap, typeMap,
                    &headers_to_pad, &resubmit_headers, &all_header_types),
            new TransformResubmitHeaders(&resubmit_headers),
            // After padding the TypeInference might
            // change new ListExpressions to StructExpressions
            new P4::ClearTypeMap(typeMap),
            new P4::ResolveReferences(refMap),
            new BFN::TypeInference(typeMap, false),
            new P4::ClearTypeMap(typeMap),
            new BFN::TypeChecking(refMap, typeMap, true),
            new CheckHeaderAlignment(refMap, typeMap),
            new CheckPadAssignment(),
            });
        setName("PadFlexibleField");
    }
};

}  // namespace BFN

#endif  /* BF_P4C_MIDEND_CHECK_HEADER_ALIGNMENT_H_ */
