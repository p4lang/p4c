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

#include "defuse.h"

#include "frontends/p4/methodInstance.h"

const ordered_set<const ComputeDefUse::loc_t *> ComputeDefUse::empty = {};

void ComputeDefUse::flow_merge(Visitor &a_) {
    ComputeDefUse &a = dynamic_cast<ComputeDefUse &>(a_);
    for (auto &di : a.def_info) def_info[di.first].flow_merge(di.second);
}
void ComputeDefUse::flow_copy(ControlFlowVisitor &a_) {
    ComputeDefUse &a = dynamic_cast<ComputeDefUse &>(a_);
    BUG_CHECK(state == a.state, "inconsistent state in ComputeDefUse::flow_copy");
    def_info = a.def_info;
}

ComputeDefUse::def_info_t::def_info_t(const def_info_t &a)
    : defs(a.defs),
      live(a.live),
      parent(a.parent),
      valid_bit_defs(a.valid_bit_defs),
      fields(a.fields),
      slices(a.slices) {
    for (auto &v : Values(fields)) v.parent = this;
    for (auto &v : Values(slices)) v.parent = this;
}

ComputeDefUse::def_info_t::def_info_t(def_info_t &&a)
    : defs(std::move(a.defs)),
      live(std::move(a.live)),
      parent(std::move(a.parent)),
      valid_bit_defs(std::move(a.valid_bit_defs)),
      fields(std::move(a.fields)),
      slices(std::move(a.slices)) {
    for (auto &v : Values(fields)) v.parent = this;
    for (auto &v : Values(slices)) v.parent = this;
}

void ComputeDefUse::def_info_t::flow_merge(def_info_t &a) {
    defs.insert(a.defs.begin(), a.defs.end());
    live |= a.live;
    valid_bit_defs.insert(a.valid_bit_defs.begin(), a.valid_bit_defs.end());
    for (auto &f : a.fields) fields[f.first].flow_merge(f.second);
    for (auto &s : a.slices) {
        split_slice(s.first);
        for (auto it = slices_overlap_begin(s.first);
             it != slices.end() && it->first.overlaps(s.first); ++it) {
            BUG_CHECK(s.first.contains(it->first), "slice_split failed to work");
            it->second.flow_merge(s.second);
        }
    }
    slices_sanity();
}

class ComputeDefUse::SetupJoinPoints : public ControlFlowVisitor::SetupJoinPoints,
                                       public P4::ResolutionContext {
    bool preorder(const IR::ParserState *n) override {
        LOG6("SetupJoinPoints(ParserState " << n->name << ")" << Log::indent);
        return true;
    }
    void revisit(const IR::ParserState *n) override {
        if (n->isBuiltin() && n->components.empty() && !n->selectExpression) {
            // FIXME -- P4-14->16 conversion uses a single accept and reject state for
            // both ingress and egress, which will cause problems, so we avoid it.
            // Perhaps we should ignore/not revisit all states with components.empty()
            // as they by definition don't do anything?
            return;
        }
        ++join_points[n].count;
        LOG6("SetupJoinPoints::revisit(ParserState " << n->name << ") [" << join_points[n].count
                                                     << "]");
    }
    void loop_revisit(const IR::ParserState *n) override { LOG6("  * loop into " << n->name); }
    void postorder(const IR::ParserState *) override { LOG6_UNINDENT; }
    void revisit(const IR::Node *) override {}
    bool preorder(const IR::PathExpression *pe) override {
        if (pe->type->is<IR::Type_State>()) {
            auto *d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
            BUG_CHECK(d, "failed to resolve %s", pe);
            auto ps = d->to<IR::ParserState>();
            BUG_CHECK(ps, "%s is not a parser state", d);
            visit(ps, "transition");
        }
        return false;
    }
    bool preorder(const IR::P4Parser *p) override {
        IndentCtl::TempIndent indent;
        LOG6("SetupJoinPoints(P4Parser " << p->name << ")" << indent);
        LOG8("    " << Log::indent << Log::indent << *p << Log::unindent << Log::unindent);
        if (auto start = p->states.getDeclaration<IR::ParserState>("start"_cs))
            visit(start, "start"_cs);
        return false;
    }
    bool preorder(const IR::P4Control *) override { return false; }
    bool preorder(const IR::Type *) override { return false; }

 public:
    explicit SetupJoinPoints(decltype(join_points) &fjp)
        : ControlFlowVisitor::SetupJoinPoints(fjp) {}
};

void ComputeDefUse::applySetupJoinPoints(const IR::Node *root) {
    root->apply(SetupJoinPoints(*flow_join_points));
}

bool ComputeDefUse::filter_join_point(const IR::Node *n) {
    LOG6("init_join_flows " << n->to<IR::ParserState>()->name << " = "
                            << flow_join_points->at(n).count);
    return false;
}

const ComputeDefUse::loc_t *ComputeDefUse::getLoc(const Visitor::Context *ctxt) {
    if (!ctxt) return nullptr;
    loc_t tmp{ctxt->node, getLoc(ctxt->parent)};
    return &*cached_locs.insert(tmp).first;
}

const ComputeDefUse::loc_t *ComputeDefUse::getLoc(const IR::Node *n, const Visitor::Context *ctxt) {
    for (auto *p = ctxt; p; p = p->parent)
        if (p->node == n) return getLoc(p);
    auto rv = getLoc(ctxt);
    loc_t tmp{n, rv};
    return &*cached_locs.insert(tmp).first;
}

/* sanity check on slices -- make sure keys do not overlap */
void ComputeDefUse::def_info_t::slices_sanity() {
    auto prev = slices.end();
    for (auto it = slices.begin(); it != slices.end(); ++it) {
        if (prev != slices.end()) BUG_CHECK(!prev->first.overlaps(it->first), "Overlapping slices");
        prev = it;
    }
    BUG_CHECK(fields.empty() || slices.empty(), "Both fields and slices present");
}

/// return an iterator to the first element of 'slices' that overlaps the given range,
/// or slices.end() if none do
std::map<le_bitrange, ComputeDefUse::def_info_t>::iterator
ComputeDefUse::def_info_t::slices_overlap_begin(le_bitrange range) {
    auto rv = slices.lower_bound(range);
    if (rv != slices.begin()) {
        auto p = std::prev(rv);
        if (range.overlaps(p->first)) rv = p;
    }
    return rv;
}

/* erase parts of the slices that overlap the specified range.  So if we have [7:0] and [15:8],
 * erase_slice([11:4]) will leave [3:0] and [15:12]
 */
void ComputeDefUse::def_info_t::erase_slice(le_bitrange range) {
    for (auto it = slices_overlap_begin(range); it != slices.end() && it->first.overlaps(range);) {
        if (!range.contains(it->first)) {
            if (it->first.lo < range.lo) {
                bool i = slices.emplace(le_bitrange(it->first.lo, range.lo - 1), it->second).second;
                BUG_CHECK(i, "inserting already present range?");
            }
            if (it->first.hi > range.hi) {
                bool i =
                    slices.emplace(le_bitrange(range.hi + 1, it->first.hi), std::move(it->second))
                        .second;
                BUG_CHECK(i, "inserting already present range?");
            }
        }
        it = slices.erase(it);
    }
    slices_sanity();
}

/* split slices such that any slice that overlaps with the give range is completely contained
 * with that range.  So if we have the slices [7:0] and [15:8] split_slice([11:4]) will
 * split the slices into [3:0], [7:4], [11:8], and [15:12]
 */
void ComputeDefUse::def_info_t::split_slice(le_bitrange range) {
    for (auto it = slices_overlap_begin(range); it != slices.end() && it->first.overlaps(range);) {
        if (!range.contains(it->first)) {
            // first, insert the pieces of it->first that do not overlap range
            if (it->first.lo < range.lo) {
                bool i = slices.emplace(le_bitrange(it->first.lo, range.lo - 1), it->second).second;
                BUG_CHECK(i, "inserting already present range?");
            }
            if (it->first.hi > range.hi) {
                bool i = slices.emplace(le_bitrange(range.hi + 1, it->first.hi), it->second).second;
                BUG_CHECK(i, "inserting already present range?");
            }
            // then insert the intersecion of range and it->first
            bool i = slices.emplace(range.intersectWith(it->first), std::move(it->second)).second;
            BUG_CHECK(i, "inserting already present range?");
            it = slices.erase(it);
        } else {
            ++it;
        }
    }
    slices_sanity();
}

bool ComputeDefUse::preorder(const IR::P4Control *c) {
    BUG_CHECK(state == SKIPPING, "Nested %s not supported in ComputeDefUse", c);
    IndentCtl::TempIndent indent;
    LOG5("ComputeDefUse(P4Control " << c->name << ")" << indent);
    for (auto *p : c->getApplyParameters()->parameters)
        if (p->direction == IR::Direction::In || p->direction == IR::Direction::InOut)
            def_info[p].defs.insert(getLoc(p));
    state = NORMAL;
    visit(c->body, "body"_cs);  // just visit the body; tables/actions will be visited when applied
    for (auto *p : c->getApplyParameters()->parameters)
        if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut)
            add_uses(getLoc(p), def_info[p]);
    def_info.clear();
    state = SKIPPING;
    return false;
}

bool ComputeDefUse::preorder(const IR::P4Table *tbl) {
    if (state == SKIPPING) return false;
    IndentCtl::TempIndent indent;
    LOG5("ComputeDefUse(P4Table " << tbl->name << ")" << indent);
    if (auto key = tbl->getKey()) visit(key, "key"_cs);
    if (auto actions = tbl->getActionList()) {
        parallel_visit(actions->actionList, "actions");
    } else {
        BUG("No actions in %s", tbl);
    }
    return false;
}

bool ComputeDefUse::preorder(const IR::P4Action *act) {
    if (state == SKIPPING) return false;
    for (auto *p : *act->parameters) def_info[p].defs.insert(getLoc(p));
    IndentCtl::TempIndent indent;
    LOG5("ComputeDefUse(P4Action " << act->name << ")" << indent);
    visit(act->body, "body"_cs);
    return false;
}

bool ComputeDefUse::preorder(const IR::P4Parser *p) {
    BUG_CHECK(state == SKIPPING, "Nested %s not supported in ComputeDefUse", p);
    IndentCtl::TempIndent indent;
    LOG5("ComputeDefUse(P4Parser " << p->name << ")" << indent);
    for (auto *a : p->getApplyParameters()->parameters)
        if (a->direction == IR::Direction::In || a->direction == IR::Direction::InOut)
            def_info[a].defs.insert(getLoc(a));
    state = NORMAL;
    if (auto start = p->states.getDeclaration<IR::ParserState>("start"_cs)) {
        visit(start, "start"_cs);
    } else {
        BUG("No start state in %s", p);
    }
    for (auto *a : p->getApplyParameters()->parameters)
        if (a->direction == IR::Direction::Out || a->direction == IR::Direction::InOut)
            add_uses(getLoc(a), def_info[a]);
    def_info.clear();
    state = SKIPPING;
    return false;
}
bool ComputeDefUse::preorder(const IR::ParserState *p) {
    LOG5("ComputeDefUse(ParserState " << p->name << ")" << Log::indent);
    return true;
}
void ComputeDefUse::revisit(const IR::ParserState *p) { LOG5("  * revisit " << p->name); }
void ComputeDefUse::loop_revisit(const IR::ParserState *p) { LOG5("  * loop into " << p->name); }
void ComputeDefUse::postorder(const IR::ParserState *) { LOG5_UNINDENT; }

bool ComputeDefUse::preorder(const IR::KeyElement *ke) {
    visit(ke->expression, "expression");
    return false;
}

// Add all definitions in the given def_info_t (whole and partial) as defs that reach
// a use at the specified location
void ComputeDefUse::add_uses(const loc_t *loc, def_info_t &di) {
    for (auto *l : di.defs) {
        defuse.uses[l->node].insert(loc);
        defuse.defs[loc->node].insert(l);
    }
    for (auto &f : Values(di.fields)) add_uses(loc, f);
    for (auto &sl : Values(di.slices)) add_uses(loc, sl);
}

static const IR::Expression *get_primary(const IR::Expression *e, const Visitor::Context *ctxt) {
    if (ctxt && (ctxt->node->is<IR::Member>() || ctxt->node->is<IR::Slice>() ||
                 ctxt->node->is<IR::ArrayIndex>())) {
        return get_primary(ctxt->node->to<IR::Expression>(), ctxt->parent);
    } else {
        return e;
    }
}

static const IR::Expression *isValid(const IR::Member *m, const Visitor::Context *ctxt) {
    if (m->member.name == "$valid") return m;
    if (!ctxt || !ctxt->node->is<IR::MethodCallExpression>()) return nullptr;
    if (m->member.name == "isValid" || m->member.name == "setValid" ||
        m->member.name == "setInvalid")
        return ctxt->node->to<IR::Expression>();
    return nullptr;
}

const IR::Expression *ComputeDefUse::do_read(def_info_t &di, const IR::Expression *e,
                                             const Context *ctxt) {
    if (!ctxt) {
    } else if (auto *m = ctxt->node->to<IR::Member>()) {
        if (auto *t = isValid(m, ctxt->parent)) {
            auto loc = getLoc(t);
            for (auto *l : di.valid_bit_defs) {
                defuse.uses[l->node].insert(loc);
                defuse.defs[loc->node].insert(l);
            }
            return t;
        } else if (auto *str = m->expr->type->to<IR::Type_StructLike>()) {
            int fi = str->getFieldIndex(m->member.name);
            BUG_CHECK(fi >= 0, "%s: no field %s found", m, m->member.name);
            if (di.fields.count(m->member.name))
                e = do_read(di.fields.at(m->member.name), m, ctxt->parent);
            else
                e = get_primary(m, ctxt->parent);
            if (!di.live[fi]) return e;
        } else if (m->expr->type->to<IR::Type_Stack>()) {
            if (m->member.name == "lastIndex") {
                // this depends on how much has been written to the header stack, but not
                // on any data that has been written.  So what should the defs be?  For now,
                // amything that writes to the stack as a whole is considered
                e = m;
            } else if (m->member.name == "next" || m->member.name == "last") {
                for (auto &el : di.slices) add_uses(getLoc(m), el.second);
                e = m;
            } else {
                BUG("invalid read of header stack: %s", m);
            }
        } else {
            BUG("%s: Member of unexpected type %s", m, m->expr->type);
        }
    } else if (auto *sl = ctxt->node->to<IR::Slice>()) {
        le_bitrange range(sl->getL(), sl->getH());
        for (auto it = di.slices_overlap_begin(range);
             it != di.slices.end() && range.overlaps(it->first); ++it) {
            e = do_read(it->second, sl, ctxt->parent);
            BUG_CHECK(e == sl, "slice %s is not primary in ComputeDefUse::do_read", sl);
        }
        e = sl;
        if (!di.live.getrange(range.lo, range.size())) return e;
    } else if (auto *ai = ctxt->node->to<IR::ArrayIndex>()) {
        if (auto idx = ai->right->to<IR::Constant>()) {
            int i = idx->asInt();
            le_bitrange range(i, i);
            if (di.slices.count(range))
                e = do_read(di.slices.at(range), ai, ctxt->parent);
            else
                e = get_primary(ai, ctxt->parent);
            if (!di.live[i]) return e;
        } else {
            for (auto &sl : Values(di.slices)) do_read(sl, ai, ctxt->parent);
            e = get_primary(ai, ctxt->parent);
        }
    }
    auto loc = getLoc(e);
    for (auto *l : di.defs) {
        defuse.uses[l->node].insert(loc);
        defuse.defs[loc->node].insert(l);
    }
    return e;
}

// get the single constant integer argument of a push_front or pop_front call
static int constIntMethodArg(const Visitor::Context *ctxt) {
    BUG_CHECK(ctxt, "null context");
    auto *mc = ctxt->node->to<IR::MethodCallExpression>();
    BUG_CHECK(mc && mc->arguments->size() == 1, "%s wrong number of arguments", mc);
    auto *arg = mc->arguments->at(0)->expression->to<IR::Constant>();
    BUG_CHECK(arg && arg->value > 0, "%s argument is not a positive constant", mc);
    return arg->asInt();
}

const IR::Expression *ComputeDefUse::do_write(def_info_t &di, const IR::Expression *e,
                                              const Context *ctxt) {
    if (!ctxt) {
    } else if (auto *m = ctxt->node->to<IR::Member>()) {
        if (auto *t = isValid(m, ctxt->parent)) {
            di.valid_bit_defs.clear();
            di.valid_bit_defs.insert(getLoc(t));
            return t;
        } else if (auto *str = m->expr->type->to<IR::Type_StructLike>()) {
            int fi = str->getFieldIndex(m->member.name);
            BUG_CHECK(fi >= 0, "%s: no field %s found", m, m->member.name);
            e = do_write(di.fields[m->member.name], m, ctxt->parent);
            di.live[fi] = 0;
            if (!di.live) di.defs.clear();
            return e;
        } else if (auto *ts = m->expr->type->to<IR::Type_Stack>()) {
            if (m->member.name == "next" || m->member.name == "last") {
                di.defs.insert(getLoc(m));
                di.live.setrange(0, ts->getSize());
            } else if (m->member.name == "push_front" || m->member.name == "pop_front") {
                int cnt = constIntMethodArg(ctxt->parent);
                if (m->member.name == "push_front") {
                    di.live <<= cnt;
                    di.live.clrrange(ts->getSize(), cnt);
                } else {
                    di.live >>= cnt;
                    cnt = -cnt;
                }
                if (!di.live) di.defs.clear();
                decltype(di.slices) tmp;
                for (auto &sl : di.slices) {
                    int ni = sl.first.lo + cnt;
                    if (size_t(ni) < ts->getSize())
                        tmp.emplace(le_bitrange(ni, ni), std::move(sl.second));
                }
                di.slices = std::move(tmp);
            } else {
                BUG("invalid write to header stack: %s", m);
            }
        } else {
            BUG("%s: Member of unexpected type %s", m, m->expr->type);
        }
    } else if (auto *sl = ctxt->node->to<IR::Slice>()) {
        le_bitrange range(sl->getL(), sl->getH());
        di.live.clrrange(range.lo, range.size());
        if (!di.live) di.defs.clear();
        di.erase_slice(range);
        e = do_write(di.slices[range], sl, ctxt->parent);
        return e;
    } else if (auto *ai = ctxt->node->to<IR::ArrayIndex>()) {
        if (auto idx = ai->right->to<IR::Constant>()) {
            int i = idx->asInt();
            di.live[i] = 0;
            if (!di.live) di.defs.clear();
            e = do_write(di.slices[le_bitrange(i, i)], ai, ctxt->parent);
        } else {
            di.defs.insert(getLoc(ai));
            di.live.setrange(0, ai->left->type->to<IR::Type_Indexed>()->getSize());
            e = ai;
        }
        return e;
    }
    di.defs.clear();
    di.defs.insert(getLoc(e));
    di.fields.clear();
    di.slices.clear();
    if (auto *s = e->type->to<IR::Type_StructLike>()) {
        di.live.setrange(0, s->fields.size());
    } else if (auto *i = e->type->to<IR::Type_Indexed>()) {
        di.live.setrange(0, i->getSize());
    } else if (auto *b = e->type->to<IR::Type::Bits>()) {
        di.live.setrange(0, b->width_bits());
    } else if (auto *b = e->type->to<IR::Type::Varbits>()) {
        di.live.setrange(0, b->size);
    } else if (e->type->is<IR::Type::Boolean>()) {
        di.live.setrange(0, 1);
    } else if (e->type->is<IR::Type_Enum>() || e->type->is<IR::Type_Error>()) {
        di.live.setrange(0, 1);
    } else if (auto *se = e->type->to<IR::Type_SerEnum>()) {
        di.live.setrange(0, se->type->width_bits());
    } else {
        BUG("Unexpected type %s in ComputeDefUse::do_expr", e->type);
    }
    return e;
}

bool ComputeDefUse::preorder(const IR::PathExpression *pe) {
    LOG7("ComputeDefUse(PathExpression " << *pe << ")");
    if (pe->type->is<IR::Type_State>()) {
        auto *d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
        BUG_CHECK(d, "failed to resolve %s", pe);
        auto ps = d->to<IR::ParserState>();
        BUG_CHECK(ps, "%s is not a parser state", d);
        visit(ps, "transition");
        return false;
    }
    if (state == SKIPPING) return false;
    auto *d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
    BUG_CHECK(d, "failed to resolve %s", pe);
    if (isRead() && state != WRITE_ONLY) do_read(def_info[d], pe, getContext());
    if (isWrite() && state != READ_ONLY) do_write(def_info[d], pe, getContext());
    return false;
}
void ComputeDefUse::loop_revisit(const IR::PathExpression *pe) {
    LOG5("  * not visiting PathExpresion " << pe->path->name << " to avoid loop!");
}

bool ComputeDefUse::preorder(const IR::MethodCallExpression *mc) {
    auto *mi = P4::MethodInstance::resolve(mc, this);
    if (state == WRITE_ONLY) {
        BUG_CHECK(!isWrite(), "Method call in out or inout arg should have failed typechecking");
        return false;
    } else if (state == READ_ONLY) {
        if (!isRead()) return false;
    }
    auto saved_state = state;
    state = READ_ONLY;
    visit(mc->arguments, "arguments");
    state = NORMAL;
    if (auto *ac = mi->to<P4::ActionCall>()) {
        visit(ac->action, "action");
    } else if (auto *bi = mi->to<P4::BuiltInMethod>()) {
        if (bi->name == "isValid") {
            state = READ_ONLY;
        } else if (bi->name == "setValid" || bi->name == "setInvalid") {
            state = WRITE_ONLY;
        } else if (bi->name == "push_front" || bi->name == "pop_front") {
            // push/pop we deal with as writes (in do_write)
            state = WRITE_ONLY;
        } else {
            BUG("unknown BuiltInMethod: %s", mc);
        }
        visit(mc->method, "method");
    } else {
        if (mi->object) {
            auto obj = mi->object->getNode();  // FIXME -- should be able to visit an INode
            if (!isInContext(obj)) visit(obj, "object");
        }
    }
    state = WRITE_ONLY;
    visit(mc->arguments, "arguments");
    state = saved_state;
    return false;
}

void ComputeDefUse::end_apply() { LOG5(defuse); }

// Debugging
#if BAREFOOT_INTERNAL

std::ostream &operator<<(std::ostream &out, const ComputeDefUse::loc_t &loc) {
    out << '<' << loc.node->id << '>';
    if (loc.node->srcInfo) {
        unsigned line, col;
        out << '(' << loc.node->srcInfo.toSourcePositionData(&line, &col);
        out << ':' << line << ':' << (col + 1) << ')';
    }
    return out;
}

std::ostream &operator<<(
    std::ostream &out,
    const std::pair<const IR::Node *, const ordered_set<const ComputeDefUse::loc_t *>> &p) {
    out << Log::endl;
    out << DBPrint::setprec(DBPrint::Prec_Low);
    out << p.first << '<' << p.first->id << '>';
    if (p.first->srcInfo) {
        unsigned line, col;
        out << '(' << p.first->srcInfo.toSourcePositionData(&line, &col);
        out << ':' << line << ':' << (col + 1) << ')';
    }
    out << ": {";
    const char *sep = " ";
    for (auto *l : p.second) {
        out << sep << *l;
        sep = ", ";
    }
    out << (sep + 1) << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ComputeDefUse::defuse_t &du) {
    out << "defs:" << Log::indent;
    for (auto &p : du.defs) out << p;
    out << Log::unindent << Log::endl << "uses:" << Log::indent;
    for (auto &p : du.uses) out << p;
    out << Log::unindent;
    return out;
}

void dump(const ComputeDefUse::def_info_t &di) {
    if (!di.defs.empty()) {
        const char *sep = "";
        std::cout << "{ ";
        for (auto *d : di.defs) {
            std::cout << sep << *d;
            sep = ", ";
        }
        std::cout << " }";
    }
    for (auto &f : di.fields) {
        std::cout << Log::endl << '.' << f.first << ": " << Log::indent;
        dump(f.second);
        std::cout << Log::unindent;
    }
    for (auto &sl : di.slices) {
        std::cout << Log::endl << "[" << sl.first.hi << ":" << sl.first.lo << "]: " << Log::indent;
        dump(sl.second);
        std::cout << Log::unindent;
    }
}

void dump(const ordered_map<const IR::IDeclaration *, ComputeDefUse::def_info_t> &def_info) {
    for (auto &d : def_info) {
        std::cout << d.first << ": " << Log::indent;
        dump(d.second);
        std::cout << Log::unindent << Log::endl;
    }
}

void dump(const ComputeDefUse &cdu) { dump(cdu.def_info); }
void dump(const ComputeDefUse *cdu) { dump(*cdu); }

#else
std::ostream &operator<<(std::ostream &out, const ComputeDefUse::defuse_t &) { return out; }
#endif /* BAREFOOT_INTERNAL */
