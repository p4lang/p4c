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

#ifndef _FIELD_DEFUSE_H_
#define _FIELD_DEFUSE_H_

#include <iostream>
#include "ir/ir.h"
#include "lib/symbitmatrix.h"
#include "lib/ltbitmatrix.h"
#include "lib/ordered_set.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/ir/control_flow_visitor.h"
#include "bf-p4c/ir/tofino_write_context.h"

using namespace P4;

/** @addtogroup parde
 *  @{
 */

/** Represent the parser initialization that sets all fields to zero.
 *  This is actually a dummy subclass of IR::Expression to work with
 *  the locpair setup. NEVER insert this class to IR, because it is not
 *  a IR node, because it is not registered in visiter classes.
 */
class ImplicitParserInit : public IR::Expression {
 private:
    IR::Expression* clone() const override {
        auto* new_expr = new ImplicitParserInit(*this);
        return new_expr; }

 public:
    explicit ImplicitParserInit(const PHV::Field* f)
        : field(f) { }
    const PHV::Field* field;
    void dbprint(std::ostream & out) const override {
        out << "ImplicitParserInit of " << field->id << ":" << field->name; }

    DECLARE_TYPEINFO(ImplicitParserInit);
};

/** Represent a parser error write. TODO: move to actual IR.
 */
class WriteParserError : public IR::Expression {
 private:
    IR::Expression *clone() const override {
        auto *new_expr = new WriteParserError(*this);
        return new_expr;
    }

 public:
    explicit WriteParserError(const PHV::Field *f) : field(f) {}
    const PHV::Field *field;
    void dbprint(std::ostream &out) const override { out << "write parser error to " << field; }

    DECLARE_TYPEINFO(WriteParserError);
};

class FieldDefUse : public BFN::ControlFlowVisitor, public Inspector, TofinoWriteContext {
 public:
    /** A given expression for a field might appear multiple places in the IR dag (eg, an
     * action used by mulitple tables), so we use a pair<Unit,Expr> to denote a particular
     * use or definition in the code */
    typedef std::pair<const IR::BFN::Unit *, const IR::Expression*>  locpair;
    typedef ordered_set<locpair> LocPairSet;

    enum VisitMode {
      VisitJustReads,
      VisitJustWrites,
      VisitAll
    };

    VisitMode mode = VisitAll;

    /// TODO: move them to IR and remove this hack.
    /// These constant variables are fields with the bug that hardware behaviors are not
    /// correctly captured by our IR. For example, parser_err might be written in parser, but
    /// our IR does not have any node to represent it.
    static const std::unordered_set<cstring> write_by_parser;

 private:
    const PhvInfo               &phv;
    SymBitMatrix                &conflict;

    /// Maps uses to defs and vice versa.
    ordered_map<locpair, LocPairSet>  &uses, &defs;
    ordered_map<locpair, bool> &ixbar_refs;

    /// All uses and all defs for each field.
    ordered_map<int, LocPairSet>      &located_uses, &located_defs;

    /// Maps each def to the set of defs that it may overwrite.
    ordered_map<locpair, LocPairSet>  &output_deps;

    /// All implicit parser zero initialization for each field.
    LocPairSet                        &parser_zero_inits;

    // All fields that rely on parser zero initialization.
    ordered_set<const PHV::Field*>    &uninitialized_fields;

    // All fields used as alias destinations.
    ordered_set<const PHV::Field*>    alias_destinations;
    static const LocPairSet emptyset;

    /// Intermediate data structure for computing def/use sets.
    struct info {
        const PHV::Field    *field = nullptr;
        LocPairSet           def, use;
        ordered_map<locpair, ordered_set<le_bitrange>> def_covered_ranges_map;
        // The reason to have a set of bit range is that there is a case that new def's range only
        // shadows a segment of a previous range, e.g., [3,5] shadows [0,7]. In this case, the
        // unshadowed range will be cut into two ranges. In addition, as shown in
        // bf-p4c/test/gtest/field_defuse.cpp test case FieldDefUseTest.ComplexSliceTest3, a def may
        // be duplicated since a def may go through multiple branchs and each branch may have
        // different covered def bit range.
    };
    /// Intermediate data structure for computing def/use sets.
    std::vector<info> defuse;
    class ClearBeforeEgress;
    class CollectAliasDestinations;

    profile_t init_apply(const IR::Node *root) override;
    void end_apply(const IR::Node *root) override;
    void end_apply() override;
    void collect_uninitalized();
    void check_conflicts(const info &read, int when);
    void read(const PHV::Field *, std::optional<le_bitrange>, const IR::BFN::Unit *,
              const IR::Expression *, bool);
    void read(const IR::HeaderRef *, const IR::BFN::Unit *, const IR::Expression *, bool);
    void write(const PHV::Field *, std::optional<le_bitrange>, const IR::BFN::Unit *,
               const IR::Expression *, bool);
    void write(const IR::HeaderRef *, const IR::BFN::Unit *, const IR::Expression *, bool);
    info &field(const PHV::Field *);
    info &field(int id) { return field(phv.field(id)); }
    void access_field(const PHV::Field *);
    bool preorder(const IR::BFN::Pipe *p) override;
    bool preorder(const IR::BFN::Parser *p) override;
    void postorder(const IR::BFN::Parser *p) override;
    bool preorder(const IR::BFN::ParserState *ps) override;
    void postorder(const IR::BFN::ParserState *ps) override;
    bool preorder(const IR::BFN::LoweredParser *p) override;
    bool preorder(const IR::MAU::Table *t) override;
    void postorder(const IR::MAU::Table *t) override;
    bool preorder(const IR::MAU::Primitive* prim) override;
    bool preorder(const IR::MAU::Action *p) override;
    bool preorder(const IR::MAU::StatefulAlu* prim) override;
    bool preorder(const IR::Expression *e) override;
    bool preorder(const IR::BFN::AbstractDeparser *d) override;
    void postorder(const IR::BFN::AbstractDeparser *d) override;
    FieldDefUse *clone() const override { return new FieldDefUse(*this); }
    void flow_merge(Visitor &) override;
    void flow_copy(::ControlFlowVisitor &) override;
    FieldDefUse(const FieldDefUse &) = default;
    FieldDefUse(FieldDefUse &&) = default;
    void shadow_previous_ranges(FieldDefUse::info &, le_bitrange &);
    friend std::ostream &operator<<(std::ostream &, const FieldDefUse::info &);
    friend void dump(const FieldDefUse::info &);
    friend std::ostream &operator<<(std::ostream &, const FieldDefUse &);
    std::ostream &dotgraph(std::ostream &) const;

 public:
    explicit FieldDefUse(const PhvInfo &p)
    : phv(p), conflict(*new SymBitMatrix),
      uses(*new std::remove_reference<decltype(uses)>::type),
      defs(*new std::remove_reference<decltype(defs)>::type),
      ixbar_refs(*new std::remove_reference<decltype(ixbar_refs)>::type),
      located_uses(*new std::remove_reference<decltype(located_uses)>::type),
      located_defs(*new std::remove_reference<decltype(located_defs)>::type),
      output_deps(*new std::remove_reference<decltype(output_deps)>::type),
      parser_zero_inits(*new std::remove_reference<decltype(parser_zero_inits)>::type),
      uninitialized_fields(*new std::remove_reference<decltype(uninitialized_fields)>::type)
      { joinFlows = true; visitDagOnce = false; BackwardsCompatibleBroken = true; }

    // TODO: unused?
    // const SymBitMatrix &conflicts() { return conflict; }

    const LocPairSet &getDefs(locpair use) const {
        return defs.count(use) ? defs.at(use) : emptyset; }
    const LocPairSet &getDefs(const IR::BFN::Unit *u, const IR::Expression *e) const {
        return getDefs(locpair(u, e)); }
    const LocPairSet &getDefs(const Visitor *v, const IR::Expression *e) const {
        return getDefs(locpair(v->findOrigCtxt<IR::BFN::Unit>(), e)); }

    /** Get all defs and uses of the PHV::Field with ID @p fid. */
    const LocPairSet getAllDefsAndUses(int fid) const {
        LocPairSet defsnuses;
        if (located_defs.count(fid)) defsnuses |= located_defs.at(fid);
        if (located_uses.count(fid)) defsnuses |= located_uses.at(fid);
        return defsnuses;
    }

    /** Get all defs and uses of the PHV::Field */
    const LocPairSet getAllDefsAndUses(const PHV::Field *f) const {
        if (!f) return {};
        return getAllDefsAndUses(f->id);
    }

    /** Get all defs of the PHV::Field with ID @p fid. */
    const LocPairSet &getAllDefs(int fid) const {
        return located_defs.count(fid) ? located_defs.at(fid) : emptyset; }

    const ordered_map<int, LocPairSet> &getAllDefs() const { return located_defs; }

    LocPairSet getParserDefs(const PHV::Field* f, std::optional<le_bitrange> bits) const;
    LocPairSet getParserDefs(const PHV::FieldSlice& fs) const {
        return getParserDefs(fs.field(), fs.range());
    }

    const LocPairSet &getUses(locpair def) const {
        return uses.count(def) ? uses.at(def) : emptyset; }
    const LocPairSet &getUses(const IR::BFN::Unit *u, const IR::Expression *e) const {
        return getUses(locpair(u, e)); }
    const LocPairSet &getUses(const Visitor *v, const IR::Expression *e) const {
        return getUses(locpair(v->findOrigCtxt<IR::BFN::Unit>(), e)); }
    /** Get all uses of the PHV::Field with ID @p fid. */
    const LocPairSet &getAllUses(int fid) const {
        return located_uses.count(fid) ? located_uses.at(fid) : emptyset; }
    const ordered_map<int, LocPairSet> &getAllUses() const { return located_uses; }

    /// @return all defs that the given def may overwrite.
    const LocPairSet &getOutputDeps(locpair def) const {
        return output_deps.count(def) ? output_deps.at(def) : emptyset;
    }
    const LocPairSet &getOutputDeps(const IR::BFN::Unit *u, const IR::Expression *e) const {
        return getOutputDeps(locpair(u, e));
    }
    const LocPairSet &getOutputDeps(const Visitor *v, const IR::Expression *e) const {
        return getOutputDeps(locpair(v->findOrigCtxt<IR::BFN::Unit>(), e));
    }

    const ordered_set<const PHV::Field*> &getUninitializedFields() const {
        return uninitialized_fields; }

    bool hasNonDarkContext(locpair info) const {
        if (ixbar_refs.count(info)) {
            LOG4("\t ixbar_refs:" << ixbar_refs.size());
        }

        if (!ixbar_refs.count(info)) {
            LOG5("\t\t ixbar_refs for expression " << info.second << " not found for unit " <<
                 info.first << ", but assuming it is non-dark");
            return true;
        }

        return ixbar_refs.at(info);
    }

    bool hasUninitializedRead(int fid) const {
        for (const auto& def : getAllDefs(fid)) {
            auto* expr = def.second;
            if (dynamic_cast<const ImplicitParserInit*>(expr) != nullptr)
                if (getUses(def).size() > 0)
                    return true; }
        return false;
    }

    /// @returns true if the field @p f is used in the parser.
    bool isUsedInParser(const PHV::Field* f) const;
    bool hasUseAt(const PHV::Field* f, const IR::BFN::Unit* u) const;
    bool hasDefAt(const PHV::Field* f, const IR::BFN::Unit* u) const;
    bool hasDefInParser(const PHV::Field* f, std::optional<le_bitrange> bits) const;
    bool hasDefInParser(const PHV::FieldSlice& fs) const {
        return hasDefInParser(fs.field(), fs.range());
    }
};

/** @} */  // end of group parde

#endif /* _FIELD_DEFUSE_H_ */
