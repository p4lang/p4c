/*
Copyright 2024 Intel Corp.

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

#ifndef MIDEND_DEF_USE_H_
#define MIDEND_DEF_USE_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "lib/bitrange.h"

namespace P4C::P4 {

/**
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
    static int uid_ctr;
    int uid = 0;
    ComputeDefUse *clone() const override {
        auto *rv = new ComputeDefUse(*this);
        rv->uid = ++uid_ctr;
        LOG8("ComputeDefUse::clone " << rv->uid);
        return rv;
    }
    void flow_merge(Visitor &) override;
    void flow_copy(ControlFlowVisitor &) override;
    bool operator==(const ControlFlowVisitor &) const override;

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
            if (!parent || !a.parent) return parent != nullptr;
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
    typedef ordered_set<const loc_t *> locset_t;

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
        locset_t defs;
        bitvec live;
        // FIXME -- this parent field is never used and is not set consistently, so
        // appears to be useless?
        def_info_t *parent = nullptr;
        // track valid bit access for headers separate from the rest of the header
        locset_t valid_bit_defs;
        // one of these maps will always be empty.
        std::map<cstring, def_info_t> fields;
        std::map<le_bitrange, def_info_t> slices;  // also used for arrays
        // keys in slices are always non-overlapping (checked by slices_sanity)
        void slices_sanity();
        std::map<le_bitrange, def_info_t>::iterator slices_overlap_begin(le_bitrange);
        void erase_slice(le_bitrange);
        void split_slice(le_bitrange);
        void flow_merge(def_info_t &);
        bool operator==(const def_info_t &) const;
        bool operator!=(const def_info_t &a) const { return !(*this == a); }
        def_info_t() = default;
        def_info_t(const def_info_t &);
        def_info_t(def_info_t &&);
    };
    ordered_map<const IR::IDeclaration *, def_info_t> def_info;
    void add_uses(const loc_t *, def_info_t &);
    void set_live_from_type(def_info_t &di, const IR::Type *type);

    // computed defuse info for all uses and defs in the program
    struct defuse_t {
        // defs maps from all uses to their definitions
        // uses maps from all definitions to their uses
        // uses/defs are lvalue expressions, or param declarations.
        ordered_map<const IR::Node *, locset_t> defs;
        ordered_map<const IR::Node *, locset_t> uses;
    } & defuse;
    static const locset_t empty;

    profile_t init_apply(const IR::Node *root) override {
        auto rv = Inspector::init_apply(root);
        LOG3("## Midend ComputeDefUse");
        uid_ctr = 0;
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
    bool preorder(const IR::AssignmentStatement *) override;
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
        visitDagOnce = false;
    }
    void clear() {
        cached_locs.clear();
        def_info.clear();
        defuse.defs.clear();
        defuse.uses.clear();
    }

    const locset_t &getDefs(const IR::Node *n) const {
        auto it = defuse.defs.find(n);
        return it == defuse.defs.end() ? empty : it->second;
    }
    const locset_t &getUses(const IR::Node *n) const {
        auto it = defuse.uses.find(n);
        return it == defuse.uses.end() ? empty : it->second;
    }

    // for debugging
    friend std::ostream &operator<<(std::ostream &, const loc_t &);
    friend std::ostream &operator<<(std::ostream &, const defuse_t &);
    friend std::ostream &operator<<(std::ostream &out, const ComputeDefUse &cdu) {
        return out << cdu.defuse;
    }
};

}  // namespace P4C::P4

#endif /* MIDEND_DEF_USE_H_ */
