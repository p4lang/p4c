/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "infer_payload_offset.h"

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/device.h"
#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/parde/dump_parser.h"
#include "bf-p4c/parde/parde_utils.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"

static bool is_mutable_field(const FieldDefUse &defuse, const PHV::Field *f) {
    auto all_defs = defuse.getAllDefs(f->id);

    for (auto def : all_defs) {
        auto unit = def.first;
        if (unit->is<IR::MAU::Table>()) return true;
    }
    // Bridged metadata
    if (f->parsed() && !f->deparsed() && !f->pov && !f->is_intrinsic()) return true;
    return false;
}

class GetAllChecksumDest : public Inspector {
 public:
    std::set<cstring> checksumDest;
    bool is_checksum_dest(const PHV::Field *f) const { return checksumDest.count(f->name); }
    bool preorder(const IR::BFN::EmitChecksum *ec) override {
        checksumDest.insert(ec->dest->toString());
        return false;
    }
};

using StateSet = ordered_set<const IR::BFN::ParserState *>;

class FindParsingFrontier : public ParserInspector {
 public:
    struct CutSet {
        ordered_map<const IR::BFN::ParserState *, StateSet> transitions;
        StateSet transitions_to_pipe;

        StateSet get_all_srcs() const {
            StateSet cutset_srcs;

            for (auto &kv : transitions) cutset_srcs.insert(kv.first);

            for (auto s : transitions_to_pipe) cutset_srcs.insert(s);

            return cutset_srcs;
        }

        StateSet get_all_dsts() const {
            StateSet cutset_dsts;

            for (auto &kv : transitions) {
                for (auto s : kv.second) cutset_dsts.insert(s);
            }

            return cutset_dsts;
        }

        StateSet get_dsts_for(const IR::BFN::ParserState *state) const {
            if (!transitions.count(state)) return StateSet();
            return transitions.at(state);
        }

        StateSet get_srcs_for(const IR::BFN::ParserState *state) const {
            StateSet rv;

            for (auto &kv : transitions) {
                if (kv.second.count(state)) rv.insert(kv.first);
            }

            return rv;
        }

        void print() const {
            std::clog << "cutset:" << std::endl;

            for (auto &kv : transitions) {
                for (auto s : kv.second)
                    std::clog << "  " << kv.first->name << " -> " << s->name << std::endl;
            }

            for (auto s : transitions_to_pipe)
                std::clog << "  " << s->name << " -> (pipe)" << std::endl;
        }
    };

    struct Frontier {
        StateSet states;
        CutSet cutset;

        void print() {
            std::clog << "frontier:" << std::endl;

            for (auto s : states) std::clog << "  " << s->name << std::endl;

            cutset.print();
        }
    };

 private:
    const PhvInfo &phv;
    const FieldDefUse &defuse;
    const CollectParserInfo &parserInfo;
    const GetAllChecksumDest &checksumDest;

    ordered_map<const IR::BFN::Parser *, StateSet> mutable_field_states;

    bool preorder(const IR::BFN::Parser *parser) override {
        mutable_field_states[parser] = {};
        return true;
    }

    bool preorder(const IR::BFN::Extract *extract) override {
        auto parser = findContext<IR::BFN::Parser>();
        auto state = findContext<IR::BFN::ParserState>();
        auto f = phv.field(extract->dest->field);
        if (!f) return false;
        if (is_mutable_field(defuse, f) || checksumDest.is_checksum_dest(f)) {
            mutable_field_states[parser].insert(state);
        }
        return false;
    }

    struct BiPartition {
        StateSet mutable_field_descendants;
        StateSet non_mutable_field_descendants;

        void print() {
            std::clog << "bipartition:" << std::endl;

            std::clog << "non_mutable_field_descendants:" << std::endl;
            for (auto s : non_mutable_field_descendants) std::clog << "  " << s->name << std::endl;

            std::clog << "mutable_field_descendants:" << std::endl;
            for (auto s : mutable_field_descendants) std::clog << "  " << s->name << std::endl;
        }
    };

    BiPartition compute_bipartition(const IR::BFN::ParserGraph &graph,
                                    const StateSet &mutable_field_states) {
        BiPartition bipartition;

        for (auto s : graph.states()) {
            if (mutable_field_states.count(s)) continue;

            bool is_mutable_field_descendant = true;
            for (auto m : mutable_field_states) {
                if (graph.is_ancestor(s, m)) {
                    is_mutable_field_descendant = false;
                    break;
                }
            }
            if (is_mutable_field_descendant) bipartition.mutable_field_descendants.insert(s);
        }

        for (auto s : graph.states()) {
            if (!bipartition.mutable_field_descendants.count(s))
                bipartition.non_mutable_field_descendants.insert(s);
        }

        if (LOGGING(5)) bipartition.print();

        return bipartition;
    }

    CutSet compute_bipartition_cutset(const IR::BFN::ParserGraph &graph,
                                      const BiPartition &bipartition) {
        CutSet cutset;

        for (auto s : bipartition.mutable_field_descendants) {
            if (graph.predecessors().count(s)) {
                for (auto p : graph.predecessors().at(s)) {
                    if (bipartition.non_mutable_field_descendants.count(p))
                        cutset.transitions[p].insert(s);
                }
            }
        }

        for (auto s : bipartition.non_mutable_field_descendants) {
            if (graph.to_pipe().count(s)) cutset.transitions_to_pipe.insert(s);
        }

        if (LOGGING(5)) cutset.print();

        return cutset;
    }

    // See if we can absorb the transitions in cutset to their originating states.
    // s is a frontier state, if cutset originates from s == all outgoing transitions of s.
    // Otherwise, need to insert frontier state on all cutset originate from s.
    Frontier compute_frontier_by_push_up(const IR::BFN::ParserGraph &graph, const CutSet &cutset) {
        Frontier frontier;

        auto cutset_srcs = cutset.get_all_srcs();

        for (auto s : cutset_srcs) {
            bool is_frontier = true;

            if (graph.successors().count(s)) {
                if (cutset.get_dsts_for(s) != graph.successors().at(s)) is_frontier = false;
            }

            if (is_frontier) {
                frontier.states.insert(s);
            } else {
                if (cutset.transitions.count(s))
                    frontier.cutset.transitions[s] = cutset.transitions.at(s);

                if (cutset.transitions_to_pipe.count(s))
                    frontier.cutset.transitions_to_pipe.insert(s);
            }
        }

        return frontier;
    }

    Frontier compute_frontier(const IR::BFN::ParserGraph &graph, const CutSet &cutset) {
        auto push_up = compute_frontier_by_push_up(graph, cutset);

        if (LOGGING(5)) {
            std::clog << "push up frontier:" << std::endl;
            push_up.print();
        }

        return push_up;
    }

    Frontier find_frontier_by_bipartition(const IR::BFN::Parser *parser,
                                          const StateSet &mutable_field_states) {
        if (LOGGING(4)) {
            std::clog << parser->gress << " mutable_field_states:" << std::endl;
            for (auto s : mutable_field_states) std::clog << "  " << s->name << std::endl;
        }

        auto &graph = parserInfo.graph(parser);

        auto bipartition = compute_bipartition(graph, mutable_field_states);

        auto cutset = compute_bipartition_cutset(graph, bipartition);

        auto frontier = compute_frontier(graph, cutset);

        return frontier;
    }

    void find_frontier(const IR::BFN::Parser *parser, const StateSet &mutable_field_states) {
        if (mutable_field_states.empty()) {
            LOG4(parser->gress << " mutable_field_states: <none>");
            Frontier frontier;
            frontier.states.insert(parser->start);  // trivial case, start state is frontier
            parser_to_frontier[parser->gress] = frontier;
        } else {
            parser_to_frontier[parser->gress] =
                find_frontier_by_bipartition(parser, mutable_field_states);
        }

        if (LOGGING(4)) {
            std::clog << parser->gress << " ";
            parser_to_frontier[parser->gress].print();
        }

        create_color_groups_for_visualization(parser);
    }

    void create_color_groups_for_visualization(const IR::BFN::Parser *parser) {
        auto &graph = parserInfo.graph(parser);
        auto &front = parser_to_frontier[parser->gress];

        assoc::set<void *> cg;
        for (auto s : front.states) cg.insert((void *)s);  // NOLINT

        for (auto &st : front.cutset.transitions) {
            for (auto d : st.second) {
                for (auto t : graph.transitions(st.first, d)) cg.insert((void *)t);  // NOLINT
            }
        }

        for (auto s : front.cutset.transitions_to_pipe) {
            for (auto t : graph.to_pipe(s)) cg.insert((void *)t);  // NOLINT
        }

        color_groups.push_back(cg);
    }

    profile_t init_apply(const IR::Node *node) override {
        mutable_field_states.clear();
        parser_to_frontier.clear();
        return ParserInspector::init_apply(node);
    }

    void end_apply() override {
        for (auto &kv : mutable_field_states) find_frontier(kv.first, kv.second);
    }

 public:
    assoc::map<gress_t, Frontier> parser_to_frontier;
    std::vector<assoc::set<void *>> color_groups;  // for DumpParser

    FindParsingFrontier(const PhvInfo &phv, const FieldDefUse &defuse,
                        const CollectParserInfo &parserInfo, const GetAllChecksumDest &checksumDest)
        : phv(phv), defuse(defuse), parserInfo(parserInfo), checksumDest(checksumDest) {}
};

class InsertFrontierStates : public ParserTransform {
    bool need_to_insert_dummy(const IR::BFN::ParserState *state, IR::BFN::Transition *transition,
                              const FindParsingFrontier::Frontier &front) {
        if (transition->next) {
            for (auto &kv : front.cutset.transitions) {
                if (kv.first->name == state->name) {
                    for (auto s : kv.second) {
                        if (s->name == transition->next->name) {
                            return true;
                        }
                    }
                }
            }
        } else {
            for (auto s : front.cutset.transitions_to_pipe) {
                if (s->name == state->name) {
                    return true;
                }
            }
        }

        return false;
    }

    const IR::Node *preorder(IR::BFN::ParserState *state) override {
        auto parser = findContext<IR::BFN::Parser>();
        auto &front = frontier.parser_to_frontier.at(parser->gress);

        for (auto s : front.states) {
            if (s->name == state->name) {
                parser_to_frontier_state_names[parser->gress].insert(state->name);
                break;
            }
        }

        return state;
    }

    const IR::Node *preorder(IR::BFN::Transition *transition) override {
        auto parser = findContext<IR::BFN::Parser>();
        auto &front = frontier.parser_to_frontier.at(parser->gress);

        auto state = findContext<IR::BFN::ParserState>();
        bool need_to_insert = need_to_insert_dummy(state, transition, front);

        if (!need_to_insert) return transition;

        int id = parser_to_frontier_state_names[parser->gress].size();

        auto to_state = new IR::BFN::Transition(match_t(), 0, transition->next);
        cstring stopper_name = "$hdr_len_inc_stop_"_cs + cstring::to_cstring(id);
        auto stopper = new IR::BFN::ParserState(stopper_name, parser->gress, {}, {}, {to_state});

        LOG3("insert parser state "
             << stopper->name << " on " << state->name << " -> "
             << (transition->next ? transition->next->name : cstring("(pipe)")));

        transition->next = stopper;

        parser_to_frontier_state_names[parser->gress].insert(stopper->name);

        return transition;
    }

    profile_t init_apply(const IR::Node *root) override {
        parser_to_frontier_state_names.clear();
        return ParserTransform::init_apply(root);
    }

    const FindParsingFrontier &frontier;

 public:
    assoc::map<gress_t, std::set<cstring>> parser_to_frontier_state_names;

    explicit InsertFrontierStates(const FindParsingFrontier &frontier) : frontier(frontier) {}
};

class RewriteParde : public PardeTransform {
    // Determines the exact location to add hdr_len_inc_stop statement
    IR::Vector<IR::BFN::ParserPrimitive>::iterator find_vector_position_for_stopper(
        const IR::BFN::PacketRVal *rval, IR::Vector<IR::BFN::ParserPrimitive> &rv) {
        for (auto it = rv.begin(); it != rv.end(); it++) {
            if (auto source = get_packet_range(*it)) {
                if (!source->range.overlaps(rval->range)) {
                    return it;
                }
            }
        }
        return rv.begin();
    }

    void insert_hdr_len_inc_stop(IR::BFN::ParserState *state) {
        IR::Vector<IR::BFN::ParserPrimitive> rv;

        bool inserted = false;

        // assumes primitives have been sorted by packet rval
        for (auto stmt : boost::adaptors::reverse(state->statements)) {
            if (auto extract = stmt->to<IR::BFN::Extract>()) {
                auto f = phv.field(extract->dest->field);

                if (auto rval = extract->source->to<IR::BFN::PacketRVal>()) {
                    if (!inserted &&
                        (is_mutable_field(defuse, f) || checksumDest.is_checksum_dest(f))) {
                        auto stopper = new IR::BFN::HdrLenIncStop(rval);
                        auto it = find_vector_position_for_stopper(rval, rv);
                        rv.insert(it, stopper);
                        rv.insert(rv.begin(), extract);
                        inserted = true;

                        LOG4("inserted " << stopper << " in " << state->name);
                        state_to_hdr_len_inc_stop[state->name] = stopper;
                        continue;
                    }
                }
            }

            rv.insert(rv.begin(), stmt);
        }
        // state has no write-able fields, insert stopper at the beginning
        if (!inserted) {
            auto stopper =
                new IR::BFN::HdrLenIncStop(new IR::BFN::PacketRVal(nw_bitrange(0, 0), false));
            rv.insert(rv.begin(), stopper);

            LOG4("insert hdr_len_inc_stop (null-extract) in " << state->name);
        }

        state->statements = rv;
    }

    void elim_unused_field_in_frontier(IR::BFN::ParserState *state) {
        IR::Vector<IR::BFN::ParserPrimitive> rv;

        bool seen_hdr_len_inc_stop = false;
        // assumes primitives have been sorted by packet rval
        for (auto stmt : state->statements) {
            auto stopper = stmt->to<IR::BFN::HdrLenIncStop>();
            auto extract = stmt->to<IR::BFN::Extract>();
            const PHV::Field *field = nullptr;
            if (extract) {
                field = phv.field(extract->dest->field);
            }

            if (stopper) {
                seen_hdr_len_inc_stop = true;
                state_to_hdr_len_inc_stop[state->name] = stopper;

                rv.push_back(stmt);
                continue;
            }
            if (seen_hdr_len_inc_stop) {
                // Statements below hdr_len_inc_stop
                if (!extract || !field->deparsed() || uses.is_used_mau(field) ||
                    field->is_checksummed()) {
                    if (orig_state_to_new_state_stmts.count(state->name)) {
                        orig_state_to_new_state_stmts[state->name].push_back(stmt);
                    } else {
                        rv.push_back(stmt);
                    }
                } else {
                    LOG4("elim " << stmt << " in " << state->name);
                }
                if (field && !fields_above_frontier.count(field)) {
                    fields_below_frontier.insert(field);
                }
            } else {
                // Statements above hdr_len_inc_stop
                rv.push_back(stmt);
                if (field) {
                    fields_above_frontier.insert(field);
                }
            }
        }
        state->statements = rv;
    }

    void elim_unused_fields_below_frontier(IR::BFN::ParserState *state) {
        IR::Vector<IR::BFN::ParserPrimitive> rv;

        // assumes primitives have been sorted by packet rval
        for (auto stmt : state->statements) {
            auto extract = stmt->to<IR::BFN::Extract>();
            const PHV::Field *field = nullptr;
            if (extract) {
                field = phv.field(extract->dest->field);
            }
            if (!extract || !field->deparsed() || uses.is_used_mau(field) ||
                field->is_checksummed()) {
                rv.push_back(stmt);
            } else {
                LOG4("elim " << stmt << " in " << state->name);
            }
            if (field && !fields_above_frontier.count(field)) {
                fields_below_frontier.insert(field);
            }
        }
        state->statements = rv;
    }

    bool is_above_frontier(const IR::BFN::Parser *p, const IR::BFN::ParserState *s) {
        auto &front = parser_to_frontier_states.at(p->gress);
        auto &graph = parserInfo.graph(p);

        for (auto f : front) {
            if (graph.is_ancestor(s, f)) return true;
        }

        return false;
    }

    bool is_below_frontier(const IR::BFN::Parser *p, const IR::BFN::ParserState *s) {
        auto &front = parser_to_frontier_states.at(p->gress);
        auto &graph = parserInfo.graph(p);

        for (auto f : front) {
            if (graph.is_descendant(s, f)) return true;
        }

        return false;
    }

    IR::Node *preorder(IR::BFN::Parser *parser) override {
        orig_parser = getOriginal<IR::BFN::Parser>();
        return parser;
    }

    IR::Node *preorder(IR::BFN::ParserState *state) override {
        auto orig_state = getOriginal<IR::BFN::ParserState>();

        IR::BFN::ParserState *rv = nullptr;

        if (is_above_frontier(orig_parser, orig_state)) {
            // Some headers can be extracted is more than one states.
            // At this stage, it is possile that a header is extracted
            // above and below the frontier. For such fields, make sure that
            // none of the fields above tje frontier is below the frontier.
            for (auto stmt : state->statements) {
                if (auto extract = stmt->to<IR::BFN::Extract>()) {
                    if (extract->source->is<IR::BFN::PacketRVal>()) {
                        auto f = phv.field(extract->dest->field);
                        if (fields_below_frontier.count(f)) {
                            fields_below_frontier.erase(f);
                        }
                        fields_above_frontier.insert(f);
                    }
                }
            }
            rv = state;
        } else if (is_below_frontier(orig_parser, orig_state)) {
            rv = state->clone();
            SortExtracts sort(rv);
            elim_unused_fields_below_frontier(rv);
        } else {
            auto &front = parser_to_frontier_states.at(orig_parser->gress);
            BUG_CHECK(front.count(orig_state), "Incorrect parsing frontier computation at %1%",
                      state->name);

            rv = state->clone();
            SortExtracts sort(rv);
            insert_hdr_len_inc_stop(rv);
            elim_unused_field_in_frontier(rv);
        }

        return rv;
    }

    IR::Node *preorder(IR::BFN::EmitField *emit) override {
        prune();

        auto f = phv.field(emit->source->field);
        if (fields_below_frontier.count(f)) {
            LOG4("elim " << emit << " (below parsing frontier)");
            return nullptr;
        }
        return emit;
    }

    struct MapNameToState : Inspector {
        assoc::map<cstring, const IR::BFN::ParserState *> &name_to_state;

        explicit MapNameToState(assoc::map<cstring, const IR::BFN::ParserState *> &n2s)
            : name_to_state(n2s) {}

        bool preorder(const IR::BFN::ParserState *state) override {
            name_to_state[state->name] = state;
            return true;
        }
    };

    profile_t init_apply(const IR::Node *root) override {
        state_to_hdr_len_inc_stop.clear();
        orig_state_to_new_state_stmts.clear();
        parser_to_frontier_states.clear();
        name_to_state.clear();
        fields_above_frontier.clear();
        fields_below_frontier.clear();
        root->apply(MapNameToState(name_to_state));

        for (auto &kv : frontier.parser_to_frontier_state_names) {
            for (auto s : kv.second)
                parser_to_frontier_states[kv.first].insert(name_to_state.at(s));
        }

        if (LOGGING(3)) {
            std::vector<assoc::set<void *>> color_groups;

            for (auto &kv : parser_to_frontier_states) {
                assoc::set<void *> cg;

                std::clog << kv.first << " frontier states:" << std::endl;
                for (auto s : kv.second) {
                    std::clog << "  " << s->name << std::endl;
                    cg.insert((void *)s);  // NOLINT
                }
                color_groups.push_back(cg);
            }

            root->apply(DumpParser("after_insert_frontier_states", color_groups));
        }

        return PardeTransform::init_apply(root);
    }

    assoc::map<gress_t, StateSet> parser_to_frontier_states;
    assoc::map<cstring, const IR::BFN::ParserState *> name_to_state;
    assoc::set<const PHV::Field *> fields_above_frontier;
    assoc::set<const PHV::Field *> fields_below_frontier;

    const IR::BFN::Parser *orig_parser = nullptr;

    const PhvInfo &phv;
    const PhvUse &uses;
    const FieldDefUse &defuse;
    const CollectParserInfo &parserInfo;
    const InsertFrontierStates &frontier;
    const GetAllChecksumDest &checksumDest;

 public:
    // Maps state name to hdr_len_inc_stop statement
    assoc::map<cstring, const IR::BFN::HdrLenIncStop *> state_to_hdr_len_inc_stop;

    // Unused
    assoc::map<cstring, IR::Vector<IR::BFN::ParserPrimitive>> orig_state_to_new_state_stmts;

    RewriteParde(const PhvInfo &phv, const PhvUse &uses, const FieldDefUse &defuse,
                 const CollectParserInfo &parserInfo, const InsertFrontierStates &frontier,
                 const GetAllChecksumDest &checksumDest)
        : phv(phv),
          uses(uses),
          defuse(defuse),
          parserInfo(parserInfo),
          frontier(frontier),
          checksumDest(checksumDest) {}
};

class InsertStallState : public ParserTransform {
 public:
    const RewriteParde &rewriteParde;
    explicit InsertStallState(const RewriteParde &rewriteParde) : rewriteParde(rewriteParde) {}

    IR::Node *preorder(IR::BFN::ParserState *state) override {
        if (rewriteParde.orig_state_to_new_state_stmts.count(state->name)) {
            BUG_CHECK(rewriteParde.state_to_hdr_len_inc_stop.count(state->name),
                      "%1% does not contains hdr_len_inc_stop? ", state->name);
            auto stopper = rewriteParde.state_to_hdr_len_inc_stop.at(state->name);
            auto stopper_range = stopper->source->range.toUnit<RangeUnit::Byte>();
            // Get final header amt
            unsigned hdr_stop = stopper_range.hi ? stopper_range.hi + 1 : 0;
            auto transition = *(state->transitions).begin();
            //  make a new state

            unsigned new_shift = transition->shift - hdr_stop;
            cstring new_state_name = state->name + ".$hdr_len_stop_stall"_cs;
            auto new_state =
                new IR::BFN::ParserState(state->p4States, new_state_name, state->gress);
            auto orig_state_new_transition =
                new IR::BFN::Transition(match_t(), hdr_stop, new_state);

            auto statements = rewriteParde.orig_state_to_new_state_stmts.at(state->name);
            statements = *statements.apply(ShiftPacketRVal(hdr_stop * 8, true));
            IR::Vector<IR::BFN::Transition> transitions;
            IR::Vector<IR::BFN::Select> selects;

            for (auto orig_transition : state->transitions) {
                auto t = new IR::BFN::Transition(orig_transition->value, new_shift,
                                                 orig_transition->next);
                transitions.push_back(t);
            }
            new_state->transitions = transitions;
            new_state->selects = *(state->selects).apply(ShiftPacketRVal(hdr_stop * 8, true));
            new_state->statements = statements;
            state->transitions = {orig_state_new_transition};
            LOG3("Adding " << new_state->name);
        }
        return state;
    }
};

InferPayloadOffset::InferPayloadOffset(const PhvInfo &phv, const FieldDefUse &defuse) {
    auto uses = new PhvUse(phv);
    auto checksumDest = new GetAllChecksumDest;
    auto parserInfo = new CollectParserInfo;
    auto findFrontier = new FindParsingFrontier(phv, defuse, *parserInfo, *checksumDest);
    auto insertFrontier = new InsertFrontierStates(*findFrontier);
    auto rewriteParde =
        new RewriteParde(phv, *uses, defuse, *parserInfo, *insertFrontier, *checksumDest);

    addPasses({
        LOGGING(4) ? new DumpParser("before_infer_payload_offset") : nullptr,
        uses,
        checksumDest,
        parserInfo,
        findFrontier,
        LOGGING(5) ? new DumpParser("after_find_parsing_frontier", findFrontier->color_groups)
                   : nullptr,
        insertFrontier,
        parserInfo,
        rewriteParde,
        new InsertStallState(*rewriteParde),
        LOGGING(4) ? new DumpParser("after_infer_payload_offset") : nullptr,
    });
}
