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

#ifndef BF_P4C_PARDE_EXTRACT_DEPARSER_H_
#define BF_P4C_PARDE_EXTRACT_DEPARSER_H_

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/device.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/parde/parde_visitor.h"

namespace P4 {
namespace IR {

namespace BFN {
class Pipe;
}  // namespace BFN

class P4Control;

}  // namespace IR
}  // namespace P4

namespace BFN {

using namespace P4;
/**
 * @ingroup parde
 * @brief Transforms midend deparser IR::BFN::TnaDeparser into backend deparser IR::BFN::Deparser
 *
 * @pre Assume the nested if statements are already canonicalized to a
 *      single if statement enclosing any emit/pack method calls.
 */
class ExtractDeparser : public DeparserInspector {
    P4::TypeMap                                 *typeMap;
    P4::ReferenceMap                            *refMap;
    IR::BFN::Pipe                               *rv;
    IR::BFN::Deparser                           *dprsr = nullptr;
    ordered_map<cstring, IR::BFN::Digest *>     digests;

    ordered_map<cstring, std::vector<const IR::BFN::EmitField*>> headerToEmits;

    std::set<ordered_set<cstring>*>             userEnforcedHeaderOrdering;

    void generateEmits(const IR::MethodCallExpression* mc);
    void generateDigest(IR::BFN::Digest *&digest, cstring name, const IR::Expression *list,
                        const IR::Expression* select, int digest_index,
                        cstring controlPlaneName = nullptr);
    void convertConcatToList(std::vector<const IR::Expression*>& slices, const IR::Concat* expr);
    void processConcat(IR::Vector<IR::BFN::FieldLVal>& vec, const IR::Concat* expr);

    std::tuple<int, const IR::Expression*>
        getDigestIndex(const IR::IfStatement*, cstring name, bool singleEntry = false);
    int getDigestIndex(const IR::Declaration_Instance*);
    void processMirrorEmit(const IR::MethodCallExpression*, const IR::Expression*, int idx);
    void processMirrorEmit(const IR::MethodCallExpression*, int idx);
    void processResubmitEmit(const IR::MethodCallExpression*, const IR::Expression*, int idx);
    void processResubmitEmit(const IR::MethodCallExpression*, int idx);
    void processDigestPack(const IR::MethodCallExpression*, int, cstring);
    void enforceHeaderOrdering();

    /// Get the standard TNA parameter name corresponding to the paramater name used in the P4
    IR::ID getTnaParamName(const IR::BFN::TnaDeparser *deparser, IR::ID orig_name);

    bool preorder(const IR::Annotation* annot) override;
    bool preorder(const IR::AssignmentStatement* stmt) override;
    void postorder(const IR::MethodCallExpression* mc) override;
    void end_apply() override;

 public:
    explicit ExtractDeparser(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                            IR::BFN::Pipe *rv) :
            typeMap(typeMap), refMap(refMap), rv(rv) {
        setName("ExtractDeparser");
    }

    bool preorder(const IR::BFN::TnaDeparser* deparser) override {
        gress_t thread = deparser->thread;
        dprsr = new IR::BFN::Deparser(thread);
        digests.clear();
        return true;
    }

    void postorder(const IR::BFN::TnaDeparser* deparser) override {
        for (const auto& kv : digests) {
            auto name = kv.first;
            auto digest = kv.second;
            if (!digest)
                continue;
            for (auto fieldList : digest->fieldLists) {
                if (fieldList->idx < 0 ||
                    fieldList->idx > static_cast<int>(Device::maxCloneId(deparser->thread))) {
                    error("Invalid %1% index %2% in %3%", name, fieldList->idx,
                            int(deparser->thread));
                }
            }
        }
        rv->thread[deparser->thread].deparser = dprsr; }
};
//// check if the LHS of the assignment statements are being used
// in mirror, digest or resubmit.
struct AssignmentStmtErrorCheck : public DeparserInspector {
    const IR::Type* left = nullptr;
    bool stmtOk = false;
    explicit AssignmentStmtErrorCheck(const IR::Type* left) : left(left) { }

    void postorder(const IR::MethodCallExpression* methodCall) override {
        auto member = methodCall->method->to<IR::Member>();
        auto expr = member->expr->to<IR::PathExpression>();
        if (!expr) return;
        const IR::Type_Extern* type = nullptr;
        if (auto spType = expr->type->to<IR::Type_SpecializedCanonical>()) {
            type = spType->baseType->to<IR::Type_Extern>();
        } else {
            type = expr->type->to<IR::Type_Extern>();
        }
        if (!type) return;
        if (type->name != "Mirror" && type->name != "Digest" && type->name != "Resubmit") {
            return;
        }
        auto arguments = *methodCall->arguments;
        for (auto argument : arguments) {
            if (argument->expression->type->equiv(*left)) {
                stmtOk = true;
                return;
            }
        }
        return;
    }
};

}  // namespace BFN

#endif /* BF_P4C_PARDE_EXTRACT_DEPARSER_H_ */
