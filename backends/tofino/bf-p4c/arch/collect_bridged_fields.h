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

#ifndef BF_P4C_ARCH_COLLECT_BRIDGED_FIELDS_H_
#define BF_P4C_ARCH_COLLECT_BRIDGED_FIELDS_H_

#include <utility>
#include <optional>
#include "ir/ir.h"
#include "ir/control_flow_visitor.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace BFN {

using FieldRef = std::pair<cstring, cstring>;

struct BridgedFieldInfo {
    const IR::Type* type;
    const IR::Expression* refTemplate;
};

struct CollectBridgedFields : public Inspector,
                              protected BFN::ControlFlowVisitor,
                              protected P4WriteContext {
    using TnaParams = ordered_map<cstring, cstring>;
    using TnaContext = std::pair<gress_t, const TnaParams&>;

    CollectBridgedFields(P4::ReferenceMap* refMap, P4::TypeMap* typeMap);

    ordered_set<FieldRef> mayReadUninitialized[2];
    ordered_set<FieldRef> mayWrite[2];
    ordered_set<FieldRef> mustWrite[2];  // TODO: Not much use testing this...

    ordered_map<FieldRef, BridgedFieldInfo> fieldInfo;
    std::set<FieldRef> fieldsToBridge;  // using set here to avoid pathological case with
        // tofino/switch_generic_int_leaf that happens with ordered_set (visit order)
    ordered_set<cstring> doNotBridge;

 private:
    CollectBridgedFields* clone() const override;
    void flow_merge(Visitor& otherVisitor) override;
    void flow_copy(::ControlFlowVisitor& otherVisitor) override;

    std::optional<TnaContext> findTnaContext() const;
    bool analyzePathlikeExpression(const IR::Expression* expr);

    // skip non-TNA controls/parsers
    bool preorder(const IR::P4Parser *) override { return false; }
    bool preorder(const IR::P4Control *) override { return false; }
    bool preorder(const IR::BFN::TnaControl *) override;
    bool preorder(const IR::BFN::TnaParser *) override;
    bool preorder(const IR::BFN::TnaDeparser *) override;
    bool preorder(const IR::P4Table *) override;
    bool preorder(const IR::Annotation* annot) override;
    bool preorder(const IR::Member* member) override;
    bool preorder(const IR::PathExpression* path) override;
    bool preorder(const IR::MethodCallExpression *) override;
    void end_apply() override;

    // ignore parser and declaration loops for now
    void loop_revisit(const IR::ParserState *) override {}
    void loop_revisit(const IR::Declaration_Instance *) override {}

    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;

    CollectBridgedFields();
    CollectBridgedFields(const CollectBridgedFields &) = default;
    int uid;
    static int uid_counter;
};

}  // namespace BFN

#endif  /* BF_P4C_ARCH_COLLECT_BRIDGED_FIELDS_H_ */
