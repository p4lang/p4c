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

#include "field_defuse.h"
#include <regex>
#include "lib/hex.h"
#include "lib/log.h"
#include "lib/map.h"
#include "lib/range.h"

const FieldDefUse::LocPairSet FieldDefUse::emptyset;

const std::unordered_set<cstring> FieldDefUse::write_by_parser = {
    "ingress::ig_intr_md_from_prsr.parser_err"_cs,
    "egress::eg_intr_md_from_prsr.parser_err"_cs
};

static std::ostream &operator<<(std::ostream &out, const FieldDefUse::locpair &loc) {
    return out << *loc.second << " in " << *loc.first;
}

class FieldDefUse::ClearBeforeEgress : public Inspector, TofinoWriteContext {
    FieldDefUse &self;
    bool preorder(const IR::Expression *e) override {
        auto *f = self.phv.field(e);
        if (f && isWrite()) {
            LOG4("CLEARING FIELD: " << e);
            self.defuse[f->id] = info();
            return false; }
        if (e->is<IR::Member>())
            return false;
        return true; }
    bool preorder(const IR::HeaderRef *hr) override {
        for (int id : self.phv.struct_info(hr).field_ids()) {
            if (isWrite()) {
                self.defuse[id] = info();
                LOG4("CLEARING HEADER REF: " << hr); } }
        return false; }
 public:
    explicit ClearBeforeEgress(FieldDefUse &self) : self(self) {}
};

class FieldDefUse::CollectAliasDestinations : public Inspector {
    FieldDefUse& self;
    bool preorder(const IR::BFN::AliasMember *e) override {
        const auto* f = self.phv.field(e);
        if (!f) return false;
        self.alias_destinations.insert(f);
        return false;
    }
    bool preorder(const IR::BFN::AliasSlice* sl) override {
        const auto* f = self.phv.field(sl);
        if (!f) return false;
        self.alias_destinations.insert(f);
        return false;
    }
 public:
    explicit CollectAliasDestinations(FieldDefUse &s) : self(s) {}
};

Visitor::profile_t FieldDefUse::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);
    static int callctr;
    LOG2("FieldDefUse starting #" << ++callctr << IndentCtl::indent);
    conflict.clear();
    defs.clear();
    uses.clear();
    defuse.clear();
    defuse.resize(phv.num_fields());
    located_uses.clear();
    located_defs.clear();
    output_deps.clear();
    parser_zero_inits.clear();
    alias_destinations.clear();
    uninitialized_fields.clear();
    ixbar_refs.clear();
    return rv;
}

void FieldDefUse::check_conflicts(const info &read, int when) {
    int firstdef = INT_MAX;
    for (auto def : read.def)
        firstdef = std::min(firstdef, def.first->stage());
    for (auto& other : defuse) {
        if (other.field == nullptr) continue;
        if (other.field == read.field) continue;
        for (auto use : other.use) {
            int use_when = use.first->stage();
            if (use_when >= firstdef && use_when <= when)
                conflict(read.field->id, other.field->id) = true; } }
}

FieldDefUse::info &FieldDefUse::field(const PHV::Field *f) {
    auto &info = defuse[f->id];
    assert(!info.field || info.field == f);
    info.field = f;
    return info;
}

void FieldDefUse::read(const PHV::Field *f, std::optional<le_bitrange> range,
    const IR::BFN::Unit *unit, const IR::Expression *e, bool needsIXBar) {
    // If range is std::nullopt, then it means the whole field is read.
    if (!f) return;
    auto &info = field(f);
    LOG3("defuse: " << DBPrint::Brief << *unit << " reading " << f->name);
    info.use.clear();
    locpair use(unit, e);
    info.use.emplace(use);
    located_uses[f->id].emplace(use);
    check_conflicts(info, unit->stage());
    for (auto &def : info.def) {
        if (!range) {
            LOG4("  " << use << " uses " << def);
            uses[def].emplace(use);
            defs[use].emplace(def);
        } else {
            const auto &covered_ranges = info.def_covered_ranges_map[def];
            for (const auto &covered_range : covered_ranges) {
                if (covered_range.overlaps(*range)) {
                    LOG4("  " << use << " uses " << def);
                    uses[def].emplace(use);
                    defs[use].emplace(def);
                }
            }
        }
    }
    ixbar_refs[use] |= needsIXBar;

    LOG5("\t Adding IXBar " << needsIXBar << " for use  " << use <<
         "  ixbar_refs:" << ixbar_refs.size());
}
void FieldDefUse::read(const IR::HeaderRef *hr, const IR::BFN::Unit *unit,
                       const IR::Expression *e, bool needsIXBar) {
    if (!hr) return;
    PhvInfo::StructInfo info = phv.struct_info(hr);
    for (int id : info.field_ids())
        read(phv.field(id),std::nullopt, unit, e, needsIXBar);
    if (!info.metadata)
        read(phv.field(hr->toString() + ".$valid"), std::nullopt, unit, e, needsIXBar);
}

void FieldDefUse::shadow_previous_ranges(FieldDefUse::info &info, le_bitrange &bit_range) {
    safe_vector<locpair> def_to_rm;
    for (auto &def_ranges : info.def_covered_ranges_map) {
        auto &def = def_ranges.first;
        auto &ranges = def_ranges.second;
        safe_vector<ordered_set<ordered_set<le_bitrange>>::iterator> range_to_rm;
        ordered_set<le_bitrange> new_ranges;
        for (auto range : ranges){
            if (!bit_range.overlaps(range)) {
                new_ranges.insert(range);
                continue;
            }
            if (!bit_range.contains(range)) {
                if (range.contains(bit_range)) {
                    if (range.hi == bit_range.hi) {
                        // e.g., bit_range = [5-7], range = [3-7], unshadowed range is [3-4]
                        new_ranges.insert(le_bitrange(range.lo, bit_range.lo - 1));
                    } else if (range.lo == bit_range.lo) {
                        // e.g., bit_range = [3-5], range = [3-7], unshadowed range is [6-7]
                        new_ranges.insert(le_bitrange(bit_range.hi + 1, range.hi));
                    } else {
                        // e.g., bit_range = [3-5], range = [0-7] unshadowed ranges are [0-2] and
                        // [6-7]
                        new_ranges.insert(le_bitrange(range.lo, bit_range.lo - 1));
                        new_ranges.insert(le_bitrange(bit_range.hi + 1, range.hi));
                    }
                } else {
                    if (range.hi > bit_range.hi) {
                        // e.g., bit_range = [0-5], range = [2-7] unshadowed range is [6-7]
                        new_ranges.insert(le_bitrange(bit_range.hi + 1, range.hi));
                    } else {
                        // e.g., bit_range = [2-7], range = [0-5] unshadowed range is [0-1]
                        new_ranges.insert(le_bitrange(range.lo, bit_range.lo - 1));
                    }
                }
            }  // else this range is shadowed.
        }
        if (new_ranges.empty()) {
            def_to_rm.push_back(def);
        } else {
            ranges.clear();
            for (auto range : new_ranges) ranges.insert(range);
        }
    }

    for (auto& def : def_to_rm) {
        BUG_CHECK(info.def_covered_ranges_map.erase(def), "def not find");
        BUG_CHECK(info.def.erase(def), "def not find");
    }
}

void FieldDefUse::write(const PHV::Field *f, std::optional<le_bitrange> range,
                        const IR::BFN::Unit *unit, const IR::Expression *e, bool needsIXBar) {
    // If range is std::nullopt, then it means that the whole field is written.
    if (!f) return;
    auto &info = field(f);
    LOG3("defuse: " << DBPrint::Brief << *unit <<
         " writing " << f->name << (range ? " (partial)" : ""));

    // Update output_deps with the new def.
    locpair def(unit, e);
    for (auto &old_def : info.def) {
        le_bitrange old_range;
        const PHV::Field *old_f = phv.field(old_def.second, &old_range);
        if (old_f && range && !range->overlaps(old_range)) continue;
        LOG4("  " << def << " overwrites " << old_def);
        output_deps[def].insert(old_def);
    }
    le_bitrange bit_range;
    if (!range) {
        bit_range.lo = 0;
        bit_range.hi = f->size - 1;
    } else {
        bit_range = *range;
    }
    if (unit->is<IR::BFN::ParserState>()) {
        // parser can't rewrite PHV (it ors), so need to treat it as a read for conflicts, but
        // we don't mark it as a use of previous writes, and don't clobber those previous writes.
        if (!range) {
            info.use.clear();
            // Though parser do OR value into phv, as long as it is not partial, the zero-init
            // def should be remove, because this def will override the 0.
            safe_vector<locpair> to_rm;
            for (const auto& v : info.def) {
                if (parser_zero_inits.count(v)) {
                    to_rm.push_back(v);
                }
            }

            for (const auto& v : to_rm) {
                info.def.erase(v);
                info.def_covered_ranges_map.erase(v);
            }
        }

        info.use.emplace(unit, e);
        check_conflicts(info, unit->stage());
        for (auto &def : info.def)
            LOG4("  " << e << " in " << *unit << " combines with " <<
                 def.second << " from " << *def.first);
    } else {
        if (!range) {
            info.def.clear();
            info.def_covered_ranges_map.clear();
        }
        shadow_previous_ranges(info, bit_range);
    }

    info.def_covered_ranges_map[locpair(unit, e)].insert(bit_range);
    info.def.emplace(unit, e);
    located_defs[f->id].emplace(unit, e);
    ixbar_refs[def] |= needsIXBar;
    LOG5("\t Adding IXBar " << needsIXBar << " for def " << def <<
         "  ixbar_refs:" << ixbar_refs.size());
}
void FieldDefUse::write(const IR::HeaderRef *hr, const IR::BFN::Unit *unit,
                        const IR::Expression *e, bool needsIXBar) {
    if (!hr) return;
    PhvInfo::StructInfo info = phv.struct_info(hr);
    for (int id : info.field_ids())
        write(phv.field(id), std::nullopt, unit, e, needsIXBar);
    if (!info.metadata)
        write(phv.field(hr->toString() + ".$valid"), std::nullopt, unit, e, needsIXBar);
}

bool FieldDefUse::preorder(const IR::BFN::Pipe *p) {
    p->apply(CollectAliasDestinations(*this));
    return true;
}

bool FieldDefUse::preorder(const IR::BFN::Parser *p) {
    LOG6("FieldDefUse " << p->gress << " Parser" << IndentCtl::indent);
    if (p->gress == EGRESS) {
        /* after processing the ingress pipe, before proceeding to the egress pipe, we
         * clear everything mentioned in the egress parser.  We want to ensure that nothing
         * that might be set by the egress parser is considered for bridging -- if it might
         * be set but isn't, it should be left unset in the egress pipe (not bridged) */
        p->apply(ClearBeforeEgress(*this));
    }

    for (const auto& f : phv) {
        if (f.gress != p->gress) {
            continue; }

        // Some DeparserParameters are not initialized by parser, i.e. fields that are
        // f.deparsed_to_tm() && f.is_solitary(). However, they should be treated as
        // implicitly initialized to an invalid value, instead of not initialized,
        // for the correctness of overlay analysis.

        // In the ingress, bridged_metadata is considered as a header field.
        // For ingress, all metadata are initialized in the beginning.
        // for egress, only non-bridged metadata are initializes at the beginning.
        if (!alias_destinations.count(&f)) {
            if (p->gress == INGRESS && (!f.metadata && !f.bridged)) continue;
            if (p->gress == EGRESS  && (!f.metadata || f.bridged)) continue;
        }

        if (write_by_parser.count(f.name)) {
            LOG2("Adding parser error write expr for " << f.name);
            auto *parser_begin = p->start;
            const PHV::Field *f_p = phv.field(f.id);
            BUG_CHECK(f_p != nullptr, "Dereferencing an invalid field id");
            IR::Expression *parser_err_expr = new WriteParserError(f_p);
            auto &info = field(f_p);
            info.def.emplace(parser_begin, parser_err_expr);
            info.def_covered_ranges_map[locpair(parser_begin, parser_err_expr)].insert(
                    StartLen(0, f.size));
            located_defs[f.id].emplace(parser_begin, parser_err_expr);
        } else {
            LOG2("Adding implicit parser initialization expr for " << f.name);
            auto *parser_begin = p->start;
            const PHV::Field *f_p = phv.field(f.id);
            BUG_CHECK(f_p != nullptr, "Dereferencing an invalid field id");
            IR::Expression *dummy_expr = new ImplicitParserInit(f_p);
            auto &info = field(f_p);
            parser_zero_inits.emplace(parser_begin, dummy_expr);
            info.def.emplace(parser_begin, dummy_expr);
            info.def_covered_ranges_map[locpair(parser_begin, dummy_expr)].insert(
                le_bitrange(0, f_p->size - 1));
            located_defs[f.id].emplace(parser_begin, dummy_expr);
        }
    }
    return true;
}
void FieldDefUse::postorder(const IR::BFN::Parser *) {
    LOG6_UNINDENT;
}

bool FieldDefUse::preorder(const IR::BFN::ParserState *ps) {
    LOG6("FieldDefUse " << ps->gress << " ParserState " << ps->name << IndentCtl::indent);
    return true;
}
void FieldDefUse::postorder(const IR::BFN::ParserState *) {
    LOG6_UNINDENT;
}

bool FieldDefUse::preorder(const IR::BFN::LoweredParser*) {
    BUG("Running FieldDefUse after the parser IR has been lowered; "
        "this will produce invalid results.");
    return false;
}

bool FieldDefUse::preorder(const IR::MAU::Table *t) {
    LOG6("FieldDefUse " << t->gress << " table " << t->name << IndentCtl::indent);
    return true;
}
void FieldDefUse::postorder(const IR::MAU::Table *) {
    LOG6_UNINDENT;
}

bool FieldDefUse::preorder(const IR::MAU::Action *act) {
    // stateful table arguments are read before the the action code runs which reads
    // them.  FIXME -- should only visit the SaluAction that is triggered by this action,
    // not all of them.
    // Only multistage_fifo.p4 needs visit stateful call before the action code runs.
    LOG6("FieldDefUse preorder Action : " << act << IndentCtl::indent);
    visit(act->stateful_calls, "stateful");
    if (act->parallel) {
        mode = VisitJustReads;
        visit(act->action, "action");
        mode = VisitJustWrites;
        visit(act->action, "action");
        mode = VisitAll;
    } else {
        visit(act->action, "action"); }
    LOG6_UNINDENT;
    return false;
}

bool FieldDefUse::preorder(const IR::MAU::Primitive* prim) {
    // TODO: consider h.f1 = h.f1 + 1; When we visit it,
    // we should first visit the source on the RHS, as how hardware does.
    // The h.f1 on RHS is an use of previous def, and the one on LHS is a
    // write that clears the previous defs from this point.
    // TODO: The long-term fix for this is to change the order of visiting when
    // visiting IR::MAU::Primitive to the evaluation order defined in spec,
    // to make control flow visit correct.
    LOG6("FieldDefUse preorder Primitive : " << prim << IndentCtl::indent);
    if (prim->operands.size() > 0) {
        if (mode != VisitJustWrites) {
            for (size_t i = 1; i < prim->operands.size(); ++i) {
                visit(prim->operands[i], "operands", i); } }
        if (mode != VisitJustReads) {
            visit(prim->operands[0], "dest", 0); }
    }
    LOG6_UNINDENT;
    return false;
}

/* StatefulALU can be defined in a global scope and have StatefulALU Actions in
 * different threads e.g. one in ingress and one in ghost. FieldDefUse must only
 * visit the actions relevant to the current thread.
 * - Make a list of all actions on the SALU
 * - Find the Table invoking the SALU
 * - Visit all actions on the table and create a list of the ones which map to
 *   the SALU actions
 * - Visit only the collected actions in the list
 * This ensures only the actions relevant to the current thread are visited.
 */
bool FieldDefUse::preorder(const IR::MAU::StatefulAlu *salu) {
    LOG6("FieldDefUse preorder Stateful : " << salu << IndentCtl::indent);
    visitAgain();
    std::set<cstring> salu_actions_to_visit;
    if (auto tbl = findContext<IR::MAU::Table>()) {
        for (auto act : tbl->actions) {
            auto tblActIdx = tbl->name + "-" + act.first;
            if (salu->action_map.count(tblActIdx)) {
                salu_actions_to_visit.insert(salu->action_map.at(tblActIdx));
            }
        }
        for (auto sv : salu_actions_to_visit) {
            LOG3("  visiting salu action " << sv);
            visit(salu->instruction.at(sv));
        }
    }
    LOG6_UNINDENT;
    return false;
}

bool FieldDefUse::preorder(const IR::Expression *e) {
    le_bitrange bits;
    auto *f = phv.field(e, &bits);
    auto *hr = e->to<IR::HeaderRef>();

    // Prevent visiting HeaderRefs in Members when PHV lookup fails, eg. for
    // $valid fields before allocatePOV.
    if (!f && e->is<IR::Member>()) return false;
    if (!f && !hr) return true;
    LOG6("FieldDefUse preorder : " << e << IndentCtl::indent);

    if (auto unit = findContext<IR::BFN::Unit>()) {
        bool needsIXBar = true, ok = false;
        for (auto c = getContext(); c; c = c->parent) {
            if (c->node->is<IR::MAU::IXBarExpression>() || c->node->is<IR::MAU::StatefulCall>() ||
                c->node->is<IR::MAU::StatefulAlu>() || c->node->is<IR::MAU::Table>() ||
                c->node->is<IR::MAU::Meter>()) {
                needsIXBar = true;
                ok = true;
                break;
            }
            if (c->node->is<IR::MAU::Action>() || c->node->is<IR::BFN::ParserState>() ||
                c->node->is<IR::BFN::Deparser>() || c->node->is<IR::BFN::GhostParser>()) {
                needsIXBar = false;
                ok = true;
                break;
            }
        }
        BUG_CHECK(ok, "FieldDefUse expression in unexpected context");
        bool partial = (f && (bits.lo != 0 || bits.hi != f->size-1));
        if (isWrite()) {
            if (partial) {
                write(f, bits, unit, e, needsIXBar);
            } else {
                write(f, std::nullopt, unit, e, needsIXBar);
            }
            write(hr, unit, e, needsIXBar);
            LOG3("  write at unit : " << unit);
        } else {
            if (partial) {
                read(f, bits, unit, e, needsIXBar);
            } else {
                read(f, std::nullopt, unit, e, needsIXBar);
            }
            read(hr, unit, e, needsIXBar);
            LOG3("  read at unit : " << unit);
        }
    } else {
        assert(0); }
    LOG6_UNINDENT;
    return false;
}

bool FieldDefUse::preorder(const IR::BFN::AbstractDeparser *d) {
    LOG6("FieldDefUse " << d->gress << " Deparser" << IndentCtl::indent);
    return true;
}
void FieldDefUse::postorder(const IR::BFN::AbstractDeparser *) {
    LOG6_UNINDENT;
}

void FieldDefUse::flow_merge(Visitor &a_) {
    FieldDefUse &a = dynamic_cast<FieldDefUse &>(a_);
    for (auto &i : a.defuse) {
        if (i.field == nullptr) continue;
        auto &info = field(i.field);
        BUG_CHECK(&info != &i, "same object in FieldDefUse::flow_merge");
        info.def.insert(i.def.begin(), i.def.end());
        info.use.insert(i.use.begin(), i.use.end());

        // consider a program like this:
        // bit<16> foo = 1;
        // if (bar == 1) {
        //     foo[15:9] = 2;
        // }
        //   <---------- flow merge point
        // if (foo == 3) {...}
        // At flow merge point, locpair for foo = 1 will have two copies. One copy will go through
        // true branch of if statement, another will go through false branch(but do nothing).
        // At the time of flow merging, for the true branch one, unshadowed range will be [8:0]
        // and for false branch one, unshadow range will be [15:0]. We will need to collect both
        // unshadow ranges.
        for (auto &def_ranges : i.def_covered_ranges_map) {
            for (auto &range : def_ranges.second) {
                // It is ok if two ranges are the same and are inserted into a set, since they are
                // effectively like one def.
                info.def_covered_ranges_map[def_ranges.first].insert(range);
            }
        }
    }
    for (auto ndr : a.ixbar_refs) {
        ixbar_refs[ndr.first] |= a.ixbar_refs[ndr.first];
    }
}
void FieldDefUse::flow_copy(::ControlFlowVisitor &a_) {
    FieldDefUse &a = dynamic_cast<FieldDefUse &>(a_);
    BUG_CHECK(mode == a.mode, "Inconsitent mode in FieldDefUse::flow_copy");
    defuse = a.defuse;
}

std::ostream &operator<<(std::ostream &out, const FieldDefUse::info &i) {
    out << DBPrint::Brief;
    out << i.field->name << ": def:";
    const char *sep = "";
    for (auto u : i.def) {
        out << sep << *u.first;
        sep = ","; }
    out << " use:";
    sep = "";
    for (auto u : i.use) {
        out << sep << *u.first;
        sep = ","; }
    out << std::endl;
    out << DBPrint::Reset;
    return out;
}
void dump(const FieldDefUse::info &a) { std::cout << a; }

std::ostream &operator<<(std::ostream &out, const FieldDefUse &a) {
    for (auto &i : a.defuse) {
        if (i.field == nullptr) continue;
        out << i;
    }
    out << DBPrint::Brief;
    unsigned maxw = 0;
    for (unsigned i = 0; i < a.phv.num_fields(); i++) {
        auto field = a.phv.field(i);
        CHECK_NULL(field);
        unsigned sz = field->name.size();
        if (a.defuse[i].field != nullptr)
            sz += 2;
        if (maxw < sz) maxw = sz; }
    for (unsigned i = 0; i < a.phv.num_fields(); i++) {
        auto field = a.phv.field(i);
        CHECK_NULL(field);
        auto name = field->name;
        if (a.defuse[i].field != nullptr)
            out << '[' << std::setw(maxw-2) << std::left << name << ']';
        else
            out << std::setw(maxw) << std::left << name;
        out << ' ';
        for (unsigned j = 0; j <= i; j++)
            out << (a.conflict[i][j] ? '1' : '0');
        out << std::endl; }
    out << DBPrint::Reset;
    return out;
}
void dump(const FieldDefUse &a) { std::cout << a; }

struct code { int id; };
std::ostream &operator<<(std::ostream &out, const code &c) {
    switch (c.id/26/26) {
    case 0:
        return out << char('a' + c.id/26) << char('a' + c.id%26);
    case 1:
        return out << char('A' + c.id/26 - 26) << char('a' + c.id%26);
    case 2:
        return out << char('a' + c.id/26 - 52) << char('A' + c.id%26);
    case 3:
        return out << char('A' + c.id/26 - 78) << char('A' + c.id%26);
    default:
        return out << "??"; }
}

std::string to_string(const code &a) {
    std::stringstream tmp;
    tmp << a;
    return tmp.str();
}

bool FieldDefUse::isUsedInParser(const PHV::Field* f) const {
    for (const FieldDefUse::locpair def : getAllDefs(f->id))
        if (def.first->is<IR::BFN::ParserState>() || def.first->is<IR::BFN::Parser>())
            return true;
    for (const FieldDefUse::locpair use : getAllUses(f->id))
        if (use.first->is<IR::BFN::ParserState>() || use.first->is<IR::BFN::Parser>())
            return true;
    return false;
}

bool FieldDefUse::hasUseAt(const PHV::Field* f, const IR::BFN::Unit* u) const {
    for (auto& use : getAllUses(f->id))
        if (u == use.first) return true;
    return false;
}

bool FieldDefUse::hasDefAt(const PHV::Field* f, const IR::BFN::Unit* u) const {
    for (auto& def : getAllDefs(f->id))
        if (def.first == u) return true;
    return false;
}

static inline auto
getParserRangeDefMatcher(const PhvInfo& phv, const PHV::Field* f,
                         const std::optional<le_bitrange> &bits) {
    le_bitrange range = bits ? *bits : StartLen(0, f->size);
    // note that we need to capture f (pointer) and range by value to avoid dangling reference,
    // but phv by reference (it is large and we got it by reference)
    return [&phv, range, f](const FieldDefUse::locpair& lp) {
            le_bitrange rng;
            if (!(lp.first->is<IR::BFN::ParserState>() || lp.first->is<IR::BFN::Parser>()))
                return false;

            // Cannot extract field - e.g. ImplicitParserInit
            if (!phv.field(lp.second, &rng)) return false;
            return range.overlaps(rng);
        };
}

FieldDefUse::LocPairSet
FieldDefUse::getParserDefs(const PHV::Field* f, std::optional<le_bitrange> bits) const {
    LocPairSet out;
    auto matcher = getParserRangeDefMatcher(phv, f, bits);
    for (auto &lp : getAllDefs(f->id)) {
        if (matcher(lp))
            out.insert(lp);
    }
    return out;
}

bool FieldDefUse::hasDefInParser(const PHV::Field* f, std::optional<le_bitrange> bits) const {
    return std::any_of(getAllDefs(f->id).begin(), getAllDefs(f->id).end(),
                       getParserRangeDefMatcher(phv, f, bits));
}

void FieldDefUse::collect_uninitalized() {
    // Get all uninitialized fields
    for (const auto& def : parser_zero_inits) {
        auto uses = getUses(def);
        if (uses.size()) {
            auto* init = def.second->to<ImplicitParserInit>();
            BUG_CHECK(init, "Defuse zero init IR type error.");
            uninitialized_fields.insert(init->field);
        }
    }
}

void FieldDefUse::end_apply(const IR::Node *) {
    collect_uninitalized();

    LOG_FEATURE("defuse_graph", 3, std::bind(&FieldDefUse::dotgraph, this, std::placeholders::_1));

    if (!LOGGING(2)) return;
    LOG2("FieldDefUse conflicts result:" << IndentCtl::indent);
    int count = phv.num_fields();
    if (count >= 40) {
        for (auto& f : phv)
            LOG2(code{f.id} << " " << f.name); }
    std::string tmp = "\\/ ";
    for (int i = 0; i < count; i++)
        tmp += char('a' + i/26);
    LOG2(tmp);
    tmp = "/\\";
    for (int i = 0; i < count; i++)
        tmp += char('a' + i%26);
    for (int i = 0; i < count; i++) {
        LOG2(tmp);
        tmp = to_string(code{i});
        for (int j = 0; j < count; j++)
            tmp += (conflict[i][j] ? '1' : '0');
        if (count < 40) {
            auto field = phv.field(i);
            CHECK_NULL(field);
            tmp += " ";
            tmp += field->name; } }
    LOG2(tmp << IndentCtl::unindent);

    LOG2("The number of uninitialized fields: " << uninitialized_fields.size());
    for (const auto* f : uninitialized_fields) {
        LOG2("  ---------------------------------");
        LOG2("  uninitialized field: " << f);
        LocPairSet defs = getAllDefs(f->id);
        LOG2("  .......Defs: ");
        for (const auto& kv : defs) {
            auto* unit = kv.first;
            auto* expr = kv.second;
            LOG2("    unit name, unit->name = " << DBPrint::Brief << *unit);
            LOG2("    in expression, expr = " << expr);
        }
        LocPairSet uses = getAllUses(f->id);
        LOG2("  ......Uses: ");
        for (const auto& kv : uses) {
            auto* unit = kv.first;
            auto* expr = kv.second;
            LOG2("    unit name, unit->name = " << DBPrint::Brief << *unit);
            LOG2("    in expression, expr = " << expr);
        }
    }
}

void FieldDefUse::end_apply() {
    LOG2_UNINDENT;  // indent from init_apply
}

std::ostream &FieldDefUse::dotgraph(std::ostream &out) const {
    struct graph {
        struct node {
            cstring unit, expr, line;
            node() = delete;
            node(cstring u, cstring e, cstring l) : unit(u), expr(e), line(l) {}
            bool operator<(const node &a) const {
                return std::tie(unit, expr, line) < std::tie(a.unit, a.expr, a.line); }
        };
        std::map<node, int> nodes;
        std::map<locpair, const node *> by_loc;
        std::regex *filter = nullptr;
        bool filter_cmpl = false;

        int get(locpair loc, std::ostream &out) {
            if (!by_loc.count(loc))
                by_loc.emplace(loc, find(loc, out));
            auto *p = by_loc.at(loc);
            return p ? nodes.at(*p) : -1; }
        const node *find(locpair loc, std::ostream &out) {
            std::stringstream unit, expr;
            cstring line;
            unit << DBPrint::Brief << *loc.first;
            expr << DBPrint::Brief << *loc.second;
            if (filter && (regex_search(expr.str(), *filter) ^ filter_cmpl))
                return nullptr;
            if (loc.second->srcInfo)
                line = loc.second->srcInfo.toPositionString();
            node tmp(unit.str(), expr.str(), line);
            if (!nodes.count(tmp)) {
                int id = nodes.size() + 1;
                nodes[tmp] = id;
                out << "n" << id << " [label=\"" << tmp.unit << "\\n" << tmp.expr;
                if (tmp.line) out << "\\n" << tmp.line;
                out << "\"]\n"; }
            return &nodes.find(tmp)->first; }
        ~graph() {
            nodes.clear();
            by_loc.clear();
            delete filter;
        }
    } dot;
    static int ctr = 0;
    if (auto *filt = getenv("GRAPH_IGNORE")) {
        if (*filt == '!') {
            ++filt;
            dot.filter_cmpl = true; }
        dot.filter = new std::regex(filt); }
    out << "digraph defuse_" << ++ctr << " {\n";
    out << "rankdir=LR\n";
    for (auto &du : uses) {
        int def = dot.get(du.first, out);
        if (def < 0) continue;
        for (auto &use : du.second) {
            int u = dot.get(use, out);
            if (u < 0) continue;
            out << "n" << def << " -> n" << u << "\n";
        }
    }
    out << "}\n";
    out << "// -- END --" << std::endl;
    return out;
}
