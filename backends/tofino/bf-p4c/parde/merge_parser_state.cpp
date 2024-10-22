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

#include "bf-p4c/parde/merge_parser_state.h"

#include <map>
#include <vector>

#include "bf-p4c/parde/dump_parser.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "lib/ordered_map.h"

namespace {

struct CollectStateUses : public ParserInspector {
    using State = IR::BFN::ParserState;
    bool preorder(const IR::BFN::ParserState *state) {
        if (!n_transition_to.count(state)) n_transition_to[state] = 0;

        for (const auto *transition : state->transitions) {
            auto *next_state = transition->next;
            if (!next_state) continue;

            n_transition_to[next_state]++;
        }

        return true;
    }

    Visitor::profile_t init_apply(const IR::Node *root) {
        n_transition_to.clear();
        return ParserInspector::init_apply(root);
    }

    std::map<const State *, int> n_transition_to;
};

/// Shift all input packet extracts to the right by the given
/// amount. Works for SaveToRegister and Extract.
/// The coordinate system:
/// [0............31]
/// left..........right
struct RightShiftPacketRVal : public Modifier {
    int byteDelta = 0;
    bool allowNegative = false;
    explicit RightShiftPacketRVal(int byteDelta, bool allowNegative = false)
        : byteDelta(byteDelta), allowNegative(allowNegative) {}
    bool preorder(IR::BFN::PacketRVal *rval) override {
        rval->range = rval->range.shiftedByBytes(byteDelta);
        BUG_CHECK(rval->range.lo >= 0 || allowNegative, "Shifting extract to negative position.");
        return true;
    }
};

class ComputeMergeableState : public ParserInspector {
    using State = IR::BFN::ParserState;
    std::set<cstring> loop_header;

 public:
    explicit ComputeMergeableState(const CollectStateUses &uses)
        : n_transition_to(uses.n_transition_to) {}

 private:
    void postorder(const IR::BFN::ParserState *state) {
        LOG1("ComputeMergeableState postorder on state: " << state->name);
        // If branching, cannot fold in any children node.
        if (state->dontMerge) return;
        for (auto t : state->transitions) {
            if (t->loop.size()) {
                loop_header.insert(t->loop);
                return;
            }
        }
        if (state->transitions.size() > 1) return;

        // Termination State
        if (state->transitions.size() == 0) return;

        auto *transition = *state->transitions.begin();
        auto *next_state = transition->next;
        if (!next_state) return;

        // Dont merge loop header with its previous state
        if (loop_header.count(next_state->name)) {
            return;
        }
        if (next_state->dontMerge) return;

        if (!is_dont_care(transition->value)) return;

        if (is_merge_point(next_state)) return;

        // removed check that prevented merging with a state containing a select

        // TODO more thoughts can be given to this
        for (auto stmt : next_state->statements) {
            if (stmt->is<IR::BFN::ParserCounterLoadPkt>()) return;
        }
        std::vector<const State *> state_chain = getStateChain(next_state);

        LOG4("Add " << state->name << " to merge chain");
        state_chain.push_back(state);
        state_chains[state] = state_chain;
        state_chains.erase(next_state);
    }

    void end_apply() {
        // forall merging vector, create the new state, save it to result
        for (const auto &kv : state_chains) {
            if (kv.second.size() >= 2) {
                auto *merged_state = createMergedState(kv.second);
                results[kv.first] = merged_state;
            }
        }
    }

    bool is_compiler_generated(cstring name) { return name.startsWith("$"); }

    State *createMergedState(const std::vector<const State *> states) {
        if (LOGGING(3)) {
            std::clog << "Creating merged state for:" << std::endl;
            for (auto s : states) std::clog << s->name << std::endl;
        }

        int shifted = 0;
        const State *tail = states.front();
        cstring name = ""_cs;

        IR::Vector<IR::BFN::ParserPrimitive> extractions;
        IR::Vector<IR::BFN::SaveToRegister> saves;
        std::set<const IR::ParserState *> p4_states;

        // Merge all except the tail state.
        bool is_first = true;
        for (auto itr = states.rbegin(); itr != states.rend(); ++itr) {
            auto &st = *itr;
            BUG_CHECK(st->transitions.size() == 1 || itr == states.rend() - 1,
                      "branching state can not be merged, unless the last");
            auto *transition = *st->transitions.begin();

            for (const auto *stmt : st->statements) {
                auto s = stmt->apply(RightShiftPacketRVal(shifted));
                extractions.push_back(s->to<IR::BFN::ParserPrimitive>());
            }
            for (const auto *save : transition->saves) {
                saves.push_back(
                    (save->apply(RightShiftPacketRVal(shifted)))->to<IR::BFN::SaveToRegister>());
            }

            cstring state_name = stripThreadPrefix(st->name);

            if (!is_compiler_generated(name) || !is_compiler_generated(state_name)) {
                if (!is_first) name += ".";
                name += state_name;
                is_first = false;
            }
            if (itr != states.rend() - 1) {
                for (const auto *save : transition->saves) {
                    saves.push_back((save->apply(RightShiftPacketRVal(shifted)))
                                        ->to<IR::BFN::SaveToRegister>());
                }
                shifted += transition->shift;
            }

            p4_states.insert(st->p4States.begin(), st->p4States.end());
        }

        IR::Vector<IR::BFN::Select> selects;
        for (const auto *select : tail->selects)
            selects.push_back(
                (select->apply(RightShiftPacketRVal(shifted, true)))->to<IR::BFN::Select>());

        auto *merged_state = new IR::BFN::ParserState(nullptr, name, tail->gress);
        merged_state->selects = selects;
        merged_state->statements = extractions;
        for (const auto *transition : tail->transitions) {
            auto *new_transition = createMergedTransition(shifted, transition, saves);
            merged_state->transitions.push_back(new_transition);
        }
        merged_state->stride = tail->stride;
        merged_state->p4States.insert(p4_states.begin(), p4_states.end());

        LOG3("Created " << merged_state->name);

        return merged_state;
    }

    IR::BFN::Transition *createMergedTransition(
        int shifted, const IR::BFN::Transition *prev_transition,
        const IR::Vector<IR::BFN::SaveToRegister> &prev_saves) {
        IR::Vector<IR::BFN::SaveToRegister> saves = prev_saves;
        for (const auto *save : prev_transition->saves) {
            saves.push_back(
                (save->apply(RightShiftPacketRVal(shifted)))->to<IR::BFN::SaveToRegister>());
        }
        auto *rst = new IR::BFN::Transition(
            prev_transition->value, prev_transition->shift + shifted, prev_transition->next);
        rst->saves = saves;
        return rst;
    }

    std::vector<const State *> getStateChain(const State *state) {
        if (!state_chains.count(state)) {
            state_chains[state] = {state};
            LOG4("Add " << state->name << " to merge chain");
        }
        return state_chains.at(state);
    }

    bool is_merge_point(const State *state) { return n_transition_to.at(state) > 1; }

    bool is_dont_care(const IR::BFN::ParserMatchValue *value) {
        if (auto *const_value = value->to<IR::BFN::ParserConstMatchValue>()) {
            return (const_value->value.word0 ^ const_value->value.word1) == 0;
        }
        return false;
    }

    Visitor::profile_t init_apply(const IR::Node *root) {
        state_chains.clear();
        results.clear();
        return ParserInspector::init_apply(root);
    }

    const std::map<const State *, int> &n_transition_to;
    ordered_map<const State *, std::vector<const State *>> state_chains;

 public:
    // A serie of states starting from `key` can be merged into one `value`.
    std::map<const State *, State *> results;
};

struct WriteBackMergedState : public ParserModifier {
    explicit WriteBackMergedState(const ComputeMergeableState &cms) : replace_map(cms.results) {}

    bool preorder(IR::BFN::Parser *parser) override {
        auto *original_parser = getOriginal<IR::BFN::Parser>();
        if (replace_map.count(original_parser->start)) {
            LOG4("Replacing " << parser->gress << " parser start " << parser->start->name
                              << " with " << replace_map.at(original_parser->start)->name);
            parser->start = replace_map.at(original_parser->start);
        }
        return true;
    }

    bool preorder(IR::BFN::Transition *transition) override {
        auto *original_transition = getOriginal<IR::BFN::Transition>();
        auto *next = original_transition->next;
        if (replace_map.count(next)) {
            LOG4("Replacing " << transition->next->name << " with " << replace_map.at(next)->name);
            transition->next = replace_map.at(next);
        }
        return true;
    }

    const std::map<const IR::BFN::ParserState *, IR::BFN::ParserState *> &replace_map;
};

}  // namespace

MergeParserStates::MergeParserStates() {
    auto *collectStateUses = new CollectStateUses();
    auto *computeMergeableState = new ComputeMergeableState(*collectStateUses);
    addPasses({LOGGING(4) ? new DumpParser("before_merge_parser_states") : nullptr,
               collectStateUses, computeMergeableState,
               new WriteBackMergedState(*computeMergeableState),
               LOGGING(4) ? new DumpParser("after_merge_parser_states") : nullptr});
}
