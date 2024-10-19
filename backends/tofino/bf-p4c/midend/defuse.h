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

#ifndef BF_P4C_MIDEND_DEFUSE_H_
#define BF_P4C_MIDEND_DEFUSE_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

/**
 * @ingroup midend
 * @ingroup parde
 * @brief Compute defuse info within P4Parser and P4Control blocks in the midend.
 *
 * This pass finds all uses and definitions of field/slice values in all controls
 * and parsers in the program and stores maps of which defs reach which uses.
 * After the pass runs, getDefs(use) will return all the definitions in the program
 * that reach the argument use while getUses(def) will return all the uses of the
 * given def.  The returned values are sets of ComputeUseDef::loc_t objects which
 * contain both the node that is def or use as well as the context path from the root
 * of the IR to that node -- actions that are used by mulitple tables or parser states
 * reachable via multiple paths may have mulitple entries as a result
 *
 * @pre Currently the code does not consider calls between controls or parsers, as it is
 * expected to run after inlining when all such calls have been flattened.
 * It could be extended to deal with the before inlining case.
 */
class ComputeDefUse : public Inspector,
                      public ControlFlowVisitor,
                      public P4WriteContext,
                      public P4::ResolutionContext {
    ComputeDefUse *clone() const { return new ComputeDefUse(*this); }
    void flow_merge(Visitor &) override;
    void flow_copy(ControlFlowVisitor &) override;
    enum { SKIPPING, NORMAL, READ_ONLY, WRITE_ONLY } state = SKIPPING;

 public:
    // a location in the program.  Includes the context from the visitor, which needs to
    // be copied out of the Visitor::Context objects, as they are allocated on the stack and
    // will become invalid as the IR traversal continues
    struct loc_t {
        const IR::Node *node;
        const loc_t *parent;
        bool operator<(const loc_t &a) const {
            if (node != a.node) return node->id < a.node->id;
            if (!a.parent) return parent != nullptr;
            return *parent < *a.parent;
        }
        template <class T>
        const T *find() const {
            for (auto *p = this; p; p = p->parent) {
                if (auto *t = p->node->to<T>()) return t;
            }
            return nullptr;
        }
    };

 private:
    std::set<loc_t> &cached_locs;
    const loc_t *getLoc(const Visitor::Context *ctxt);
    const loc_t *getLoc() { return getLoc(getChildContext()); }
    const loc_t *getLoc(const IR::Node *, const Visitor::Context *);
    const loc_t *getLoc(const IR::Node *n) { return getLoc(n, getChildContext()); }

    // flow tracking info about defs live at the point we are currently visiting
    struct def_info_t {
        // definitions of a symbol (or part of a symbol) visible at this point in the
        // program.  `defs` will be empty if `live` is; if not those defs are visible only
        // for those bits/elements/fields where live is set.
        ordered_set<const loc_t *> defs;
        bitvec live;
        def_info_t *parent = nullptr;
        // track valid bit access for headers separate from the rest of the header
        ordered_set<const loc_t *> valid_bit_defs;
        // one of these maps will always be empty.
        std::map<cstring, def_info_t> fields;
        std::map<le_bitrange, def_info_t> slices;  // also used for arrays
        // keys in slices are always non-overlapping (checked by slices_sanity)
        void slices_sanity();
        std::map<le_bitrange, def_info_t>::iterator slices_overlap_begin(le_bitrange);
        void erase_slice(le_bitrange);
        void split_slice(le_bitrange);
        void flow_merge(def_info_t &);
        def_info_t() = default;
        def_info_t(const def_info_t &);
        def_info_t(def_info_t &&);
    };
    ordered_map<const IR::IDeclaration *, def_info_t> def_info;
    void add_uses(const loc_t *, def_info_t &);

    // computed defuse info for all uses and defs in the program
    struct defuse_t {
        // defs maps from all uses to their definitions
        // uses maps from all definitions to their uses
        // uses/defs are lvalue expressions, or param declarations.
        ordered_map<const IR::Node *, ordered_set<const loc_t *>> defs;
        ordered_map<const IR::Node *, ordered_set<const loc_t *>> uses;
    } & defuse;
    static const ordered_set<const loc_t *> empty;

    profile_t init_apply(const IR::Node *root) override {
        auto rv = Inspector::init_apply(root);
        state = SKIPPING;
        clear();
        return rv;
    }
    bool preorder(const IR::P4Control *) override;
    bool preorder(const IR::P4Table *) override;
    bool preorder(const IR::P4Action *) override;
    bool preorder(const IR::P4Parser *) override;
    bool preorder(const IR::ParserState *) override;
    void revisit(const IR::ParserState *) override;
    void loop_revisit(const IR::ParserState *) override;
    void postorder(const IR::ParserState *) override;
    bool preorder(const IR::Type *) override { return false; }
    bool preorder(const IR::Annotations *) override { return false; }
    bool preorder(const IR::KeyElement *) override;
    const IR::Expression *do_read(def_info_t &, const IR::Expression *, const Context *);
    const IR::Expression *do_write(def_info_t &, const IR::Expression *, const Context *);
    bool preorder(const IR::PathExpression *) override;
    void loop_revisit(const IR::PathExpression *) override;
    bool preorder(const IR::MethodCallExpression *) override;
    void end_apply() override;

    class SetupJoinPoints;
    void applySetupJoinPoints(const IR::Node *root) override;
    bool filter_join_point(const IR::Node *) override;

 public:
    ComputeDefUse()
        : ResolutionContext(true), cached_locs(*new std::set<loc_t>), defuse(*new defuse_t) {
        joinFlows = true;
    }
    void clear() {
        cached_locs.clear();
        def_info.clear();
        defuse.defs.clear();
        defuse.uses.clear();
    }

    const ordered_set<const loc_t *> &getDefs(const IR::Node *n) const {
        auto it = defuse.defs.find(n);
        return it == defuse.defs.end() ? empty : it->second;
    }
    const ordered_set<const loc_t *> &getUses(const IR::Node *n) const {
        auto it = defuse.uses.find(n);
        return it == defuse.uses.end() ? empty : it->second;
    }

    // for debugging
    friend std::ostream &operator<<(std::ostream &, const defuse_t &);
    friend std::ostream &operator<<(std::ostream &out, const ComputeDefUse &cdu) {
        return out << cdu.defuse;
    }
#if BAREFOOT_INTERNAL
    friend void dump(const def_info_t &);
    friend void dump(const ordered_map<const IR::IDeclaration *, def_info_t> &);
    friend void dump(const ComputeDefUse &);
#endif /* BAREFOOT_INTERNAL */
};

#endif /* BF_P4C_MIDEND_DEFUSE_H_ */
