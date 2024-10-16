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

#ifndef BF_P4C_MIDEND_ACTION_SYNTHESIS_POLICY_H_
#define BF_P4C_MIDEND_ACTION_SYNTHESIS_POLICY_H_

#include "midend/actionSynthesis.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/midend/register_read_write.h"

namespace BFN {

/**
This class implements a policy suitable for the SynthesizeActions pass:
  - do not synthesize actions for the controls whose names are in the specified set.
    For example, we expect that the code in the deparser will not use any
    tables or actions.
  - do not combine statements with data dependencies (except on Hash.get) into a single action
  - do not combine uses of externs (other than Hash and Register write-after-read) into one action
  The hash exceptions above are because a common idiom is to use a hash function to hash some
  data and use the result as the index into a table execute method.  We prefer keeping that in
  one action as we can do it directly; if split it requires extra PHV (to hold the hash value)
  and an extra stage.

  It would probably be better to allow ActionSynthesis to combine stuff as much as possible and
  later split actions that don't work in a sinlge cycle.  We don't yet have a general action
  splitting/rewriting pass, however, and this is simpler for now.
*/
class ActionSynthesisPolicy : public P4::ActionSynthesisPolicy {
    P4::ReferenceMap    *refMap;
    P4::TypeMap         *typeMap;
    // set of controls where actions are not synthesized
    const std::set<cstring> *skip;

    bool convert(const Visitor::Context *, const IR::P4Control* control) override {
        if (control->is<IR::BFN::TnaDeparser>()) {
            return false;
        }
        for (auto c : *skip)
            if (control->name == c)
                return false;
        return true;
    }

    static const IR::Type_Extern *externType(const IR::Type *type) {
        if (auto *spec = type->to<IR::Type_SpecializedCanonical>())
            type = spec->baseType;
        return type->to<IR::Type_Extern>(); }

    class FindPathsWritten : public Inspector, TofinoWriteContext {
        std::set<cstring>       &writes;
        bool preorder(const IR::PathExpression *pe) {
            if (isWrite()) writes.insert(pe->toString());
            return false; }
        bool preorder(const IR::Member *m) {
            if (isWrite()) writes.insert(m->toString());
            return false; }
        bool preorder(const IR::AssignmentStatement *assign) {
            // special case -- ignore writing the result of a 'hash.get' call to a var,
            // as we can use that directly in the same action (hash is computed in ixbar hash)
            if (auto *mc = assign->right->to<IR::MethodCallExpression>()) {
                if (auto *m = mc->method->to<IR::Member>()) {
                    if (auto *et = externType(m->expr->type)) {
                        if (et->name == "Hash" && m->member == "get") return false; } } }
            return true; }

     public:
        explicit FindPathsWritten(std::set<cstring> &w) : writes(w) {} };

    class DependsOnPaths : public Inspector {
        ActionSynthesisPolicy   &self;
        std::set<cstring>       &paths;
        bool                    rv = false;
        bool preorder(const IR::PathExpression *pe) {
            if (paths.count(pe->toString())) rv = true;
            return !rv; }
        bool preorder(const IR::Member *m) {
            if (paths.count(m->toString())) rv = true;
            return !rv; }
        bool preorder(const IR::Node *) { return !rv; }
        void postorder(const IR::MethodCallExpression *mc) {
            auto *mi = P4::MethodInstance::resolve(mc, self.refMap, self.typeMap, true);
            if (auto *em = mi ? mi->to<P4::ExternMethod>() : nullptr) {
                for (auto *n : em->mayCall()) {
                    if (auto *fn = n->to<IR::Function>()) {
                        visit(fn->body, "body");
                    }
                }
            }
        }

     public:
        explicit operator bool() { return rv; }
        DependsOnPaths(ActionSynthesisPolicy &self, const IR::Node *n, std::set<cstring> &p)
        : self(self), paths(p), rv(false) {
            n->apply(*this); } };

    class ReferencesExtern : public Inspector {
        bool                    rv = false;
        bool preorder(const IR::PathExpression *pe) {
            if (rv) return false;
            auto *et = externType(pe->type);
            if (et && et->name != "Hash") {
                rv = true; }
            return !rv; }
        bool preorder(const IR::Node *) { return !rv; }

     public:
        explicit operator bool() { return rv; }
        explicit ReferencesExtern(const IR::Node *n) { n->apply(*this); } };

    /**
     * Detects write-after-read on the same register in two consecutive statements.
     * It will allow to create a RegisterAction with both read and write statement
     * in the RegisterReadWrite pass.
     */
    bool isRegisterWriteAfterRead(const IR::BlockStatement *blk,
                    const IR::StatOrDecl *stmt_decl) {
        auto *blk_stmt = blk->components.back()->to<IR::Statement>();
        if (!blk_stmt) return false;
        auto read_rv = BFN::RegisterReadWrite::extractRegisterReadWrite(blk_stmt);
        auto *read_mce = read_rv.first;
        if (!read_mce) return false;

        auto *stmt = stmt_decl->to<IR::Statement>();
        if (!stmt) return false;
        auto write_rv = BFN::RegisterReadWrite::extractRegisterReadWrite(stmt);
        auto *write_mce = write_rv.first;
        if (!write_mce) return false;

        auto *read_inst = P4::MethodInstance::resolve(read_mce, refMap, typeMap);
        // Check also object becuase of plain functions
        if (!read_inst || !read_inst->object) return false;
        auto *read_decl = read_inst->object->to<IR::Declaration_Instance>();
        if (!read_decl) return false;

        auto *read_member = read_mce->method->to<IR::Member>();
        if (!read_member) return false;
        auto *read_extern_type = externType(read_member->expr->type);
        if (!read_extern_type) return false;
        if (read_extern_type->name != "Register" || read_member->member != "read")
            return false;

        auto *write_inst = P4::MethodInstance::resolve(write_mce, refMap, typeMap);
        // Check also object becuase of plain functions
        if (!write_inst || !write_inst->object) return false;
        auto *write_decl = write_inst->object->to<IR::Declaration_Instance>();
        if (!write_decl) return false;

        auto *write_member = write_mce->method->to<IR::Member>();
        if (!write_member) return false;
        auto *write_extern_type = externType(write_member->expr->type);
        if (!write_extern_type) return false;
        if (write_extern_type->name != "Register" || write_member->member != "write")
            return false;

        if (read_decl->declid != write_decl->declid)
            return false;

        return true;
    }

    bool can_combine(const Visitor::Context *, const IR::BlockStatement *blk,
                     const IR::StatOrDecl *stmt) override {
        if (isRegisterWriteAfterRead(blk, stmt))
            return true;
        std::set<cstring>       writes;
        if (ReferencesExtern(blk) && ReferencesExtern(stmt)) return false;
        blk->apply(FindPathsWritten(writes));
        return !DependsOnPaths(*this, stmt, writes); }

 public:
    ActionSynthesisPolicy(const std::set<cstring> *skip, P4::ReferenceMap *refMap,
                          P4::TypeMap *typeMap)
    : refMap(refMap), typeMap(typeMap), skip(skip) { CHECK_NULL(skip); }
};

}  // namespace BFN

#endif  /* BF_P4C_MIDEND_ACTION_SYNTHESIS_POLICY_H_ */
