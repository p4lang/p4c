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

#ifndef EXTENSIONS_BF_P4C_PARDE_COLLECT_PARSER_USEDEF_H_
#define EXTENSIONS_BF_P4C_PARDE_COLLECT_PARSER_USEDEF_H_

#include <optional>
#include "bf-p4c/common/utils.h"
#include "bf-p4c/parde/dump_parser.h"
#include "bf-p4c/parde/parde_utils.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_fields.h"

/** @addtogroup ParserCopyProp
 *  @{
 */

namespace Parser {

struct Def {
    const IR::BFN::ParserState* state = nullptr;
    const IR::BFN::InputBufferRVal* rval = nullptr;

    Def(const IR::BFN::ParserState* s,
        const IR::BFN::InputBufferRVal* r) : state(s), rval(r) { }

    bool equiv(const Def* other) const {
        if (this == other) return true;
        return (state->gress == other->state->gress) && (state->name == other->state->name) &&
               (rval->equiv(*other->rval));
    }

    std::string print() const {
        std::stringstream ss;
        ss << " ( " << state->name << " : " << rval->range << " )";
        return ss.str();
    }
};

struct Use {
    const IR::BFN::ParserState* state = nullptr;
    const IR::BFN::SavedRVal* save = nullptr;
    const IR::Expression* p4Source = nullptr;

    Use(const IR::BFN::ParserState* s,
        const IR::BFN::SavedRVal* save) : state(s), save(save) { }

    bool equiv(const Use* other) const {
        if (this == other) return true;
        return (state->gress == other->state->gress) && (state->name == other->state->name) &&
               (save->equiv(*other->save));
    }

    std::string print() const {
        std::stringstream ss;
        ss << " [ " << state->name << " : " << save->source;
        if (p4Source) ss << " " << p4Source;
        ss << " ]";
        return ss.str();
    }
};

struct UseDef {
    ordered_map<const Use*, std::vector<const Def*>> use_to_defs;

    void add_def(const PhvInfo& phv, const Use* use, const Def* def) {
        for (auto d : use_to_defs[use])
            if (d->equiv(def)) return;

        if (auto buf = use->save->source->to<IR::BFN::InputBufferRVal>()) {
            BUG_CHECK(buf->range.size() == def->rval->range.size(),
                      "parser def and use size mismatch");
        } else {
            le_bitrange bits;
            phv.field(use->save->source, &bits);
            BUG_CHECK(bits.size() == def->rval->range.size(),
                      "parser def and use size mismatch");
        }

        use_to_defs[use].push_back(def);
    }

    const Use*
    get_use(const IR::BFN::ParserState* state,
            const IR::BFN::SavedRVal* save,
            const IR::BFN::Select* select) const {
        auto temp = new Use(state, save);
        if (select) temp->p4Source = select->p4Source;
        for (auto& kv : use_to_defs)
            if (kv.first->equiv(temp))
                return kv.first;
        return temp;
    }

    const Def*
    get_def(const Use* use, const IR::BFN::ParserState* def_state) const {
        for (auto def : use_to_defs.at(use)) {
            if (def_state == def->state)
                return def;
        }
        return nullptr;
    }

    std::string print(const Use* use) const {
        std::stringstream ss;
        ss << "use: " << use->print() << " -- defs: ";
        for (auto def : use_to_defs.at(use))
            ss << def->print() << " ";
        return ss.str();
    }

    std::string print() const {
        std::stringstream ss;
        ss << "parser def use: " << std::endl;
        for (auto& kv : use_to_defs)
            ss << print(kv.first) << std::endl;
        return ss.str();
    }
};

}  // namespace Parser

struct ParserUseDef : std::map<const IR::BFN::Parser*, Parser::UseDef> { };

struct ResolveExtractSaves : Modifier {
    const PhvInfo& phv;

    explicit ResolveExtractSaves(const PhvInfo& phv) : phv(phv) { }

    struct FindRVal : Inspector {
        const PhvInfo& phv;
        const IR::Expression* expr;
        const IR::BFN::InputBufferRVal* rv = nullptr;

        FindRVal(const PhvInfo& phv, const IR::Expression* f) : phv(phv), expr(f) { }

        bool preorder(const IR::BFN::Extract* extract) override {
            le_bitrange f_bits, x_bits;

            auto f = phv.field(extract->dest->field, &f_bits);
            auto x = phv.field(expr, &x_bits);

            LOG4("Searching in extract: " << extract << ", f: " << f << ", x: " << x);
            if (f == x) {
                if (auto rval = extract->source->to<IR::BFN::InputBufferRVal>()) {
                    auto clone = rval->clone();

                    // Lines up source and dest (both can be field slices)
                    // The destination field, f, is in little endian, whereas the extract source
                    // in the input buffer is in big endian.
                    clone->range.lo -= f_bits.lo;
                    clone->range.hi += f->size - f_bits.hi - 1;

                    clone->range.lo = clone->range.hi - x_bits.hi;
                    clone->range.hi -= x_bits.lo;

                    rv = clone;
                }
            }

            return false;
        }
    };

    bool preorder(IR::BFN::Extract* extract) override {
        auto state = findOrigCtxt<IR::BFN::ParserState>();
        LOG4("state: " << state->name << ", extract: " << extract);
        if (auto save = extract->source->to<IR::BFN::SavedRVal>()) {
            if (auto rval = save->source->to<IR::BFN::InputBufferRVal>()) {
                extract->source = rval;
            } else {
                FindRVal find_rval(phv, save->source);
                state->apply(find_rval);

                if (find_rval.rv)
                    extract->source = find_rval.rv;

                LOG4("Found rval: " << find_rval.rv);
            }

            if (extract->source->is<IR::BFN::SavedRVal>())
                LOG4("cannot resolve?");
            else
                LOG4("as: " << extract->source);
        }

        return false;
    }
};

/**
 * @brief Run ResolveExtractConst pass until every IR::BFN::ConstantRVal which
 * can be propagated is propagated as far as it can.
 */
class PropagateExtractConst : public PassRepeated {
    /**
     * @brief For each extract 'extract' which has IR::BFN::SavedRVal source,
     * check if the source can be substituted by a constant and if yes,
     * substitute the source in the given 'extract' by IR::BFN::ConstantRVal.
     *
     * Source of 'extract' can be substituted by a constant if following
     * conditions are met:
     * 1.) previous extract in a parser graph which has an expression in dest equal
     *     to the expression in source of 'extract' has a constant
     *     (IR::BFN::ConstantRVal) as a source
     * 2.) there is only 1 such extract as described in 1.) or if there are
     *     multiple such extracts, they all have equal source constants
     *
     * Example:
     * --------
     *
     * before ResolveExtractConst pass:
     *
     * ingress parser state ingress::s1:
     *   extract constant 1 to ingress::hdr.h1.$valid;
     *   match const: *: (shift=0)
     *   goto ingress::s2
     * ingress parser state ingress::s2:
     *   extract [ ingress::hdr.h1.$valid; ] to ingress::hdr_0.$valid;
     *   ...
     *
     * after ResolveExtractConst pass:
     *
     * ingress parser state ingress::s1:
     *   extract constant 1 to ingress::hdr.h1.$valid;
     *   match const: *: (shift=0)
     *   goto ingress::s2
     * ingress parser state ingress::s2:
     *   extract constant 1 to ingress::hdr_0.$valid;
     *   ...
     */
    struct ResolveExtractConst : Modifier {
        struct AmbiguousPropagation {
            std::string message;
            explicit AmbiguousPropagation(const char* msg) : message(msg) {}
        };

        const CollectParserInfo& parserInfo;

        explicit ResolveExtractConst(const CollectParserInfo* pi) : parserInfo{*pi} {}

        const IR::BFN::ConstantRVal* find_constant(const IR::BFN::Extract* extract,
                const IR::BFN::ParserGraph& graph,
                const IR::BFN::ParserState* state,
                const bool start) {
            std::vector<const IR::BFN::Extract*> defExtracts;
            auto source = extract->source->to<IR::BFN::SavedRVal>()->source;

            for (const auto& statement : state->statements) {
                LOG4("  statement: " << statement);
                if (auto ext = statement->to<IR::BFN::Extract>()) {
                    LOG4("    ext: " << ext);
                    if (start && *ext == *extract) {
                        LOG4("    ext: " << ext << " and extract: " << extract <<
                                " are equal, breaking the loop!");
                        break;
                    }
                    if (source->equiv(*(ext->dest->field))) {
                        LOG4("source: " << source <<
                                " and dest: " << ext->dest->field <<
                                " from ext: " << ext <<
                                " in state: " << state->name <<
                                " are equal!");
                        defExtracts.push_back(ext);
                    }
                }
            }

            if (!defExtracts.empty()) {
                auto ext = defExtracts.back();
                if (ext->source->is<IR::BFN::ConstantRVal>())
                    return ext->source->to<IR::BFN::ConstantRVal>();
                else
                    throw AmbiguousPropagation("source is not a constant");
            }

            const IR::BFN::ConstantRVal* foundConstant = nullptr;

            if (graph.predecessors().count(state)) {
                for (const auto& predState : graph.predecessors().at(state)) {
                    LOG4("predState: " << predState);
                    auto ret = find_constant(extract, graph, predState, false);
                    if (ret) {
                        if (foundConstant) {
                            if (!foundConstant->equiv(*ret)) {
                                throw AmbiguousPropagation("multiple possible definitions");
                            }
                        } else {
                            // foundConstant == nullptr && ret->source->is<IR::BFN::ConstantRVal>()
                            foundConstant = ret;
                        }
                    }
                }
            }

            return foundConstant;
        }

        bool preorder(IR::BFN::Extract* extract) override {
            if (!extract->source->is<IR::BFN::SavedRVal>())
                return false;

            auto state = findOrigCtxt<IR::BFN::ParserState>();

            LOG4("ResolveExtractConst extract: " << extract <<
                    ", in state: " << std::endl << state);

            const auto& graph = parserInfo.graph(state);

            try {
                auto foundConst = find_constant(extract, graph, state, true);
                if (foundConst) {
                    LOG4("Propagating constant: " << foundConst << " to extract: " << extract);
                    extract->source = foundConst->clone();
                    LOG4("New extract is: " << extract);
                } else {
                    LOG4("No constant to propagate found for extract: " << extract);
                }
            } catch (AmbiguousPropagation &e) {
                LOG4("Can not propagate constant to extract: " << extract <<
                        " in state: " << state->name <<
                        ", because of: " << e.message << "!");
            }

            return false;
        }
    };

 public:
    explicit PropagateExtractConst(CollectParserInfo* pi) :
            PassManager({new ResolveExtractConst(pi), pi}) {}
};

/// Collect Use-Def for all select fields
///   Def: In which states are the select fields extracted?
///   Use: In which state are the select fields matched on?
struct CollectParserUseDef : PassManager {
    struct CollectDefs : ParserInspector {
        const PhvInfo& phv;

        explicit CollectDefs(const PhvInfo& phv) :
            phv(phv) { }

        std::map<const IR::BFN::ParserState*,
                 ordered_set<const IR::BFN::InputBufferRVal*>> state_to_rvals;

        std::map<const IR::BFN::InputBufferRVal*,
                 ordered_set<const IR::Expression*>> rval_to_lvals;

        // Maps each state to a set of lvalues that are extracted in it from
        // a input packet buffer
        std::map<const IR::BFN::ParserState*,
                 ordered_set<const PHV::Field*>> state_to_inp_buff_exts;

        // Maps each state to a set of lvalues that are extracted in it from
        // a constant
        std::map<const IR::BFN::ParserState*,
                 ordered_set<const PHV::Field*>> state_to_const_exts;

        // Maps each state to a set of lvalues that are extracted in it from
        // somewhere else (possibly SavedRVal)
        std::map<const IR::BFN::ParserState*,
                 ordered_set<const PHV::Field*>> state_to_other_exts;

        Visitor::profile_t init_apply(const IR::Node* root) override {
            state_to_rvals.clear();
            rval_to_lvals.clear();
            state_to_inp_buff_exts.clear();
            state_to_const_exts.clear();
            state_to_other_exts.clear();
            return ParserInspector::init_apply(root);
        }

        // TODO what if extract gets dead code eliminated?
        // TODO this won't work if the extract is out of order
        bool preorder(const IR::BFN::Extract* extract) override {
            auto state = findContext<IR::BFN::ParserState>();
            le_bitrange bits;
            auto f = phv.field(extract->dest->field, &bits);

            if (auto rval = extract->source->to<IR::BFN::InputBufferRVal>()) {
                state_to_rvals[state].insert(rval);
                rval_to_lvals[rval].insert(extract->dest->field);
                state_to_inp_buff_exts[state].insert(f);

                LOG4(state->name << " " << rval << " -> " << extract->dest->field);
            } else if (extract->source->is<IR::BFN::ConstantRVal>()) {
                state_to_const_exts[state].insert(f);
                LOG4(state->name << " constant -> " << extract->dest->field);
            } else {
                state_to_other_exts[state].insert(f);
                LOG4(state->name << " other -> " << extract->dest->field);
            }

            return false;
        }
    };

    struct MapToUse : ParserInspector {
        const PhvInfo& phv;
        const CollectParserInfo& parser_info;
        const CollectDefs& defs;

        ParserUseDef& parser_use_def;

        MapToUse(const PhvInfo& phv,
                 const CollectParserInfo& pi,
                 const CollectDefs& d,
                 ParserUseDef& parser_use_def) :
            phv(phv), parser_info(pi), defs(d), parser_use_def(parser_use_def) { }

        Visitor::profile_t init_apply(const IR::Node* root) override {
            parser_use_def.clear();
            return ParserInspector::init_apply(root);
        }

        // Checks if on each path to given state the value for expression saved
        // is defined (this means that for a given state it needs to be defined
        // in itself or each of its possible predecessors)
        bool is_source_extracted_on_each_path_to_state(const IR::BFN::ParserGraph& graph,
                                                       const IR::BFN::ParserState* state,
                                                       const PHV::Field* saved) {
            // The state sets the value to a value from input buffer
            // => the value is defined after this point
            if ((defs.state_to_inp_buff_exts.count(state) &&
                 defs.state_to_inp_buff_exts.at(state).count(saved)) ||
                (defs.state_to_other_exts.count(state) &&
                 defs.state_to_other_exts.at(state).count(saved))) {
                LOG6("State " << state->name << " extracts "
                     << saved->name << " from input buffer or other field");
                return true;
            // The state sets the value to a constant
            // => the value is not defined after this point
            } else if (defs.state_to_const_exts.count(state) &&
                       defs.state_to_const_exts.at(state).count(saved)) {
                LOG6("State " << state->name << " extracts " << saved->name
                     << " from a constant");
                return false;
            // This state does not change the value
            // => check all of its predecessors
            } else {
                if (!graph.predecessors().count(state)) {
                    // FIXME (MichalKekely): This should return false! Since we
                    // have not found any extract on this path yet and we cannot go
                    // any further back in the graph.
                    // However, currently this breaks egress metadata and extra parse
                    // states that are generated by the compiler (for example for mirrored
                    // packets some of the metadata are not "extracted").
                    LOG6("State " << state->name << " is the start state/entry point");
                    return true;
                }
                LOG6("State " << state->name << " does not extract "
                     << saved->name << ", checking predecessors");
                for (auto const pred : graph.predecessors().at(state)) {
                    if (!is_source_extracted_on_each_path_to_state(graph, pred, saved)) {
                        LOG6("State's " << state->name << " predecessor " << pred->name
                             << " does not have " << saved->name << " defined on all paths");
                        return false;
                    }
                }
                LOG6("State " << state->name << " has " << saved->name
                     << " defined on all paths");
                return true;
            }
        }

        void add_def(ordered_set<Parser::Def*>& rv,
                     const IR::BFN::ParserState* state,
                     const IR::BFN::InputBufferRVal* rval) {
            if (rval->range.lo >= 0) {
                auto def = new Parser::Def(state, rval);
                rv.insert(def);
            }
        }

        std::vector<std::pair<const IR::BFN::ParserState*, const IR::BFN::InputBufferRVal*>>
        defer_defs_to_children(const IR::BFN::InputBufferRVal* rval,
                               const IR::BFN::ParserGraph& graph,
                               const IR::BFN::ParserState* use_state,
                               const IR::BFN::ParserState* def_state,
                               const PHV::Field* field, const PHV::Field* saved,
                               const le_bitrange& field_bits, const le_bitrange& saved_bits) {
            if (def_state == use_state) return {};

            // Adjust the rval
            auto def_rval = rval->clone();
            if (saved_bits.size() != field_bits.size()) {
                auto nw_f_bits = field_bits.toOrder<Endian::Network>(field->size);
                auto nw_s_bits = saved_bits.toOrder<Endian::Network>(saved->size);

                auto full_rval = rval->clone();
                full_rval->range.lo -= nw_f_bits.lo;
                full_rval->range.hi += field->size - nw_f_bits.hi + 1;

                def_rval = full_rval->clone();
                def_rval->range.lo += nw_s_bits.lo;
                def_rval->range.hi -= saved->size - nw_s_bits.hi + 1;
            }

            // Check whether we can defer to the children
            bool def_deferred = false;
            bool def_invalid = false;
            std::vector<std::pair<const IR::BFN::ParserState *, const IR::BFN::InputBufferRVal *>>
                new_defs;
            for (const auto *trans : def_state->transitions) {
                if (trans->loop) {
                    def_invalid = true;
                    break;
                }

                if (!trans->next) continue;
                if (!graph.is_reachable(trans->next, use_state)) continue;

                auto *shifted_rval =
                    def_rval->apply(ShiftPacketRVal(trans->shift * 8, true))
                        ->to<IR::BFN::InputBufferRVal>();
                def_deferred = true;
                def_invalid |= shifted_rval->range.lo < 0;
                new_defs.push_back(std::make_pair(trans->next, shifted_rval));
            }

            if (def_deferred && !def_invalid) return new_defs;

            return {};
        }

        // defs have absolute offsets from current state
        ordered_set<Parser::Def*>
        find_defs(const IR::BFN::InputBufferRVal* rval,
                  const IR::BFN::ParserGraph& graph,
                  const IR::BFN::ParserState* state) {
            ordered_set<Parser::Def*> rv;

            if (rval->range.lo < 0) {  // def is in an earlier state
                if (graph.predecessors().count(state)) {
                    for (auto pred : graph.predecessors().at(state)) {
                        for (auto t : graph.transitions(pred, state)) {
                            auto shift = t->shift;

                            auto shifted_rval = rval->apply(ShiftPacketRVal(-(shift * 8), true));
                            auto defs = find_defs(shifted_rval->to<IR::BFN::InputBufferRVal>(),
                                                  graph, pred);

                            for (auto def : defs)
                                rv.insert(def);
                        }
                    }
                }

                return rv;
            } else if (rval->range.lo >= 0) {  // def is in this state
                add_def(rv, state, rval);
            }

            return rv;
        }

        // multiple defs in earlier states with no absolute offsets from current state
        ordered_set<Parser::Def*>
        find_defs(const IR::Expression* saved,
                  const IR::BFN::ParserGraph& graph,
                  const IR::BFN::ParserState* state) {
            ordered_set<Parser::Def*> rv;

            for (auto& kv : defs.state_to_rvals) {
                auto def_state = kv.first;

                if (def_state == state || graph.is_ancestor(def_state, state)) {
                    for (auto rval : kv.second) {
                        for (auto lval : defs.rval_to_lvals.at(rval)) {
                            le_bitrange s_bits, f_bits;
                            auto f = phv.field(lval, &f_bits);
                            auto s = phv.field(saved, &s_bits);

                            if (f == s) {
                                auto child_rvals = defer_defs_to_children(
                                    rval, graph, state, def_state, f, s, f_bits, s_bits);

                                if (child_rvals.size()) {
                                    for (auto& [def_state, rval] : child_rvals)
                                        add_def(rv, def_state, rval);
                                } else {
                                    if (s_bits.size() == f_bits.size()) {
                                        add_def(rv, def_state, rval);
                                    } else {
                                        auto nw_f_bits = f_bits.toOrder<Endian::Network>(f->size);
                                        auto nw_s_bits = s_bits.toOrder<Endian::Network>(s->size);

                                        auto full_rval = rval->clone();
                                        full_rval->range.lo -= nw_f_bits.lo;
                                        full_rval->range.hi += f->size - nw_f_bits.hi + 1;

                                        auto slice_rval = full_rval->clone();
                                        slice_rval->range.lo += nw_s_bits.lo;
                                        slice_rval->range.hi -= s->size - nw_s_bits.hi + 1;

                                        add_def(rv, def_state, slice_rval);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return rv;
        }

        bool preorder(const IR::BFN::SavedRVal* save) override {
            auto parser = findOrigCtxt<IR::BFN::Parser>();
            auto state = findOrigCtxt<IR::BFN::ParserState>();
            auto select = findOrigCtxt<IR::BFN::Select>();

            auto& graph = parser_info.graph(parser);

            auto use = parser_use_def[parser].get_use(state, save, select);

            ordered_set<Parser::Def*> defs;

            if (auto buf = save->source->to<IR::BFN::InputBufferRVal>())
                defs = find_defs(buf, graph, state);
            else
                defs = find_defs(save->source, graph, state);

            for (auto def : defs)
                parser_use_def[parser].add_def(phv, use, def);

            if (!select)
                return false;

            le_bitrange bits;
            auto f = phv.field(save->source, &bits);
            // Check if the value used in select is defined on each path to this state
            // (defined in this case means that it is assigned a value from a input packet
            // buffer, which is not rewritten by any other value since Tofino only
            // supports loading a value from packet input buffer to match register)
            if (!save->source->is<IR::BFN::InputBufferRVal>() &&
                !is_source_extracted_on_each_path_to_state(graph, state, f)) {
                ::fatal_error("Value used in select statement needs to be set "
                              "from input packet (not a constant) for every possible path "
                              "through the parse graph. This does not hold true for: %1%",
                              use->print());
            }

            if (!parser_use_def[parser].use_to_defs.count(use))
                ::fatal_error("Use of uninitialized parser value in a select statement %1%. ",
                              use->print());


            LOG4(parser_use_def[parser].print(use));

            return false;
        }

        bool preorder(const IR::BFN::Parser* parser) override {
            parser_use_def[parser] = {};

            std::stringstream ss;
            ss << "MapToUse parser:" << std::endl;
            parser->dbprint(ss);
            ss << std::endl << std::endl;
            LOG4(ss.str());

            return true;
        }
    };

    void end_apply() override {
        for (auto& kv : parser_use_def) {
            LOG3(kv.first->gress << " parser:");
            LOG3(parser_use_def[kv.first].print());
        }
    }

    explicit CollectParserUseDef(const PhvInfo& phv, const CollectParserInfo& parser_info) {
        auto collect_defs = new CollectDefs(phv);
        addPasses({
            collect_defs,
            new MapToUse(phv, parser_info, *collect_defs, parser_use_def),
        });
    }

    ParserUseDef parser_use_def;
};

struct CopyPropParserDef : public ParserModifier {
    const CollectParserInfo& parser_info;
    const ParserUseDef& parser_use_def;

    CopyPropParserDef(const CollectParserInfo& pi, const ParserUseDef& ud) :
        parser_info(pi), parser_use_def(ud) { }

    const IR::BFN::InputBufferRVal*
    get_absolute_def(const IR::BFN::SavedRVal* save) {
        auto parser = findOrigCtxt<IR::BFN::Parser>();
        auto state = findOrigCtxt<IR::BFN::ParserState>();
        auto select = findOrigCtxt<IR::BFN::Select>();

        auto use = parser_use_def.at(parser).get_use(state, save, select);

        if (parser_use_def.at(parser).use_to_defs.count(use) == 0) {
            LOG4("No defs for use: " << use->print());
            return nullptr;
        }

        auto defs = parser_use_def.at(parser).use_to_defs.at(use);

        LOG4("defs.size(): " << defs.size() << ", usedef: " <<
                parser_use_def.at(parser).print(use));

        if (defs.size() == 1) {
            auto def = *defs.begin();
            auto shifts = parser_info.get_all_shift_amounts(def->state, state);

            LOG4("shifts->size(): " << shifts->size());
            // if has single def of absolute offset, propagate to use
            if (shifts->size() == 1) {
                auto rval = def->rval->clone();

                if (rval->is<IR::BFN::PacketRVal>()) {
                    auto shift = *shifts->begin();
                    rval->range = rval->range.shiftedByBits(-shift);
                }

                return rval;
            }
        }

        return nullptr;
    }

    bool preorder(IR::BFN::SavedRVal* save) override {
        if (save->source->is<IR::BFN::InputBufferRVal>())
            return false;

        auto orig = getOriginal<IR::BFN::SavedRVal>();

        if (auto def = get_absolute_def(orig)) {
            LOG4("propagate " << def << " to " << save->source);
            save->source = def;
        }

        return false;
    }
};

struct CheckUnresolvedExtractSource : public ParserInspector {
    bool preorder(const IR::BFN::Extract* extract) override {
        if (extract->source->is<IR::BFN::SavedRVal>()) {
            auto state = findContext<IR::BFN::ParserState>();

            ::fatal_error("Unable to resolve extraction source."
                " This is likely due to the source having no absolute offset from the"
                " state %1%. %2%", state->name, extract);
        }

        return false;
    }
};

/** @} */  // end of group ParserCopyProp

/**
 * @defgroup ParserCopyProp ParserCopyProp
 * @ingroup parde
 * @ingroup LowerParserIR
 * @brief PassManager that governs parser copy propagation.
 */
struct ParserCopyProp : public PassManager {
    explicit ParserCopyProp(const PhvInfo& phv) {
        auto* parserInfo = new CollectParserInfo;
        auto* collectUseDef = new CollectParserUseDef(phv, *parserInfo);
        auto* copyPropDef = new CopyPropParserDef(*parserInfo, collectUseDef->parser_use_def);

        addPasses({
            LOGGING(4) ? new DumpParser("before_parser_copy_prop") : nullptr,
            new ResolveExtractSaves(phv),
            parserInfo,
            new PropagateExtractConst(parserInfo),
            collectUseDef,
            copyPropDef,
            new ResolveExtractSaves(phv),
            new CheckUnresolvedExtractSource,
            LOGGING(4) ? new DumpParser("after_parser_copy_prop") : nullptr
        });
    }
};

#endif  /* EXTENSIONS_BF_P4C_PARDE_COLLECT_PARSER_USEDEF_H_ */
