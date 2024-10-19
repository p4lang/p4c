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

/* clang-format off */
#include <iterator>
#include <string>
#include <boost/range/adaptors.hpp>

#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/parde/allocate_parser_match_register.h"
#include "bf-p4c/parde/collect_parser_usedef.h"
#include "bf-p4c/parde/dump_parser.h"
#include "bf-p4c/parde/parser_info.h"
#include "clot/clot.h"
#include "device.h"
#include "match_register.h"

using namespace Parser;

// If the start state has selects, we need to insert a dummy state
// before it where we can insert the save instructions.
struct InsertInitSaveState : public ParserTransform {
    IR::BFN::Parser *preorder(IR::BFN::Parser *parser) override {
        auto init = new IR::BFN::ParserState(nullptr, "$init_match"_cs, parser->start->gress);
        auto next = new IR::BFN::Transition(match_t(), 0, parser->start);
        init->transitions = {next};
        parser->start = init;

        return parser;
    }
};

// Insert a stall state on transition whose next state requires
// branch word that is out of the input buffer of the current state.
struct ResolveOutOfBufferSaves : public ParserTransform {
    const ParserUseDef &parser_use_def;
    CollectParserInfo parser_info;

    explicit ResolveOutOfBufferSaves(const ParserUseDef &pud) : parser_use_def(pud) {}

    profile_t init_apply(const IR::Node *root) override {
        root->apply(parser_info);
        return ParserTransform::init_apply(root);
    }

    // keep track of number stall states from orig_state
    std::map<cstring, unsigned> orig_state_to_stall_cnt;

    void insert_stall_state_for_oob_save(IR::BFN::Transition *t) {
        auto orig = getOriginal<IR::BFN::Transition>();
        auto parser = findOrigCtxt<IR::BFN::Parser>();
        auto src = parser_info.graph(parser).get_src(orig);

        auto cnt = orig_state_to_stall_cnt[src->name]++;
        cstring name = src->name + ".$oob_stall_"_cs + cstring::to_cstring(cnt);
        auto stall = new IR::BFN::ParserState(src->p4States, name, src->gress);

        LOG2("created stall state for out of buffer select on " << src->name << " -> "
                                                                << t->next->name);

        auto to_dst = new IR::BFN::Transition(match_t(), 0, t->next);
        stall->transitions.push_back(to_dst);
        t->next = stall;

        // move loop back to stall state
        if (t->loop) {
            to_dst->loop = t->loop;
            t->loop = cstring();
        }
    }

    struct GetMaxSavedRVal : Inspector {
        int rv = -1;

        bool preorder(const IR::BFN::SavedRVal *save) override {
            if (auto buf = save->source->to<IR::BFN::PacketRVal>())
                rv = std::max(buf->range.hi, rv);
            return false;
        }
    };

    const IR::BFN::ParserState *get_next_state(const IR::BFN::Transition *t) {
        if (t->next) {
            return t->next;
        } else if (t->loop) {
            auto parser = findOrigCtxt<IR::BFN::Parser>();
            return parser_info.graph(parser).get_state(t->loop);
        } else {
            return nullptr;
        }
    }

    bool has_oob_save(const IR::BFN::Transition *t) {
        if (auto next = get_next_state(t)) {
            GetMaxSavedRVal max_save;

            next->selects.apply(max_save);
            next->statements.apply(max_save);

            if (max_save.rv >= 0) {
                return int(t->shift + max_save.rv / 8 + 1) >
                   Device::pardeSpec().byteInputBufferSize();
            }

            // If scratch regs are available, we may need the additional choice of
            // scratching at the predecessors. Therefore if the branch words are
            // more than 32 bytes away at the incoming transitions to the def states
            // we need to insert stall states.

            if (!Device::pardeSpec().scratchRegisters().empty()) {
                auto parser = findOrigCtxt<IR::BFN::Parser>();
                auto use_def = parser_use_def.at(parser);

                for (auto &ud : use_def.use_to_defs) {
                    for (auto d : ud.second) {
                        if (d->state->name == next->name) {
                            if (auto buf = d->rval->to<IR::BFN::PacketRVal>()) {
                                if (int(t->shift + buf->range.hi / 8 + 1) >
                                        Device::pardeSpec().byteInputBufferSize()) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    IR::BFN::Transition *postorder(IR::BFN::Transition *t) override {
        if (has_oob_save(t)) insert_stall_state_for_oob_save(t);

        return t;
    }
};

static bool equiv(const std::vector<std::pair<MatchRegister, nw_bitrange>> &a,
                  const std::vector<std::pair<MatchRegister, nw_bitrange>> &b) {
    if (a.size() != b.size()) return false;
    for (unsigned i = 0; i < a.size(); i++)
        if (a[i] != b[i]) return false;
    return true;
}

/// @brief Delay Defs used by select statements
///
/// P4 select statements require the data being matched on to be loaded into the parser match
/// registers. There are only a small number of match (plus scratch) registers, and so it's
/// possible that the allocator runs out of registers.
///
/// The select match data must be extracted into match registers before it is shifted out of
/// the parser's processing window. Allocation can sometimes be aided by delaying when data is
/// shifted out, allowing the match register(s) for that data to be allocated in a later state.
///
/// Consider this P4 parser fragment:
///
///     state s1 {
///         pkt.extract(h1);
///         transition select (h1.field_32b) {
///             val1: s2;
///             // ...
///         }
///     }
///     state s2 {
///         pkt.extract(h2);
///         transition select (h1.field_8b) {
///             // ...
///         }
///     }
///
/// In this fragment, header `h1` is extracted and fields `h1.field_32b` and `h1.field_8b` are used
/// in select statements. Normally, the compiler will choose to extract `h1` _and shift all of the
/// data out` in state `s1`, but this would require it to allocate 40b of match registers to the two
/// select fields. Tofino 1 only has 32b total of match registers and so allocation is impossible.
///
/// Only `h1.field_32b` is used in `s1`; `h1.field_8b` is not used until `s2`. If the compiler
/// delays the shift in `s1` so that `h1.field_8b` remains in the buffer, then the compiler can
/// delay the match allocation for this field until `s2`, allowing allocation to succeed.
///
/// The match register allocation pass identifies def points for each value used in a select
/// statement. Def points are chosen to be the last state in which a value still exists in the
/// parser buffer prior to being shifted out.
///
/// This pass delays one or more defs identified during allocation. It does so by delaying the
/// shifts that cover the def data, allowing the def to be delayed until a later state.
///
/// Currently defs are delayed by only a single transition: defs are not delayed across multiple
/// transitions. (This is an optimization to consider in the future.)
struct DelayDefs : public ParserModifier {
    explicit DelayDefs(
        const CollectParserInfo &parser_info,
        const std::map<gress_t, std::map<const Def *, std::set<const IR::BFN::ParserState *>>>
            &delay_defs)
        : parser_info(parser_info), delay_defs(delay_defs) {}

    std::map<std::pair<const IR::BFN::ParserState *, const IR::BFN::ParserState *>, int>
        delay_amount;
    std::map<const IR::BFN::ParserState *, int> extra_shift;

    profile_t init_apply(const IR::Node *root) override {
        delay_amount.clear();
        extra_shift.clear();

        // Identify the minimum bit index to keep for each transition
        std::map<std::pair<const IR::BFN::ParserState *, const IR::BFN::ParserState *>, int>
            min_bits;
        for (auto &[gress, delay_def_info] : delay_defs) {
            for (auto &[def, next_states] : delay_def_info) {
                auto *rval = def->rval->to<IR::BFN::InputBufferRVal>();
                BUG_CHECK(rval, "Expecting an InputBufferRVal");
                for (auto *next_state : next_states) {
                    auto transition = std::make_pair(def->state, next_state);
                    if (min_bits.count(transition)) {
                        min_bits[transition] = std::min(rval->range.lo, min_bits[transition]);
                    } else {
                        min_bits.emplace(transition, rval->range.lo);
                    }
                    LOG5("Min bit for " << def->state->name << " -> " << next_state->name << ": "
                                        << min_bits[transition]);
                }
            }
        }

        // For each bit position to keep, calculate how much to delay (subtract from) the transition
        // out of the state, and how much additional shift to add to the target state
        for (auto [transition, amt] : min_bits) {
            auto &[state, next_state] = transition;

            // Identify how much is being shifted
            bool seen = false;
            unsigned shift = 0;
            for (auto *trans : state->transitions) {
                if (!trans->next || trans->next != next_state) continue;

                BUG_CHECK(!seen || shift == trans->shift,
                          "Multiple transitions seen from %1% to %2% with different shift amounts: "
                          "%1% %2%",
                          state->name, trans->next->name, shift, trans->shift);
                shift = trans->shift;
                seen = true;
            }

            // Calculate the amount to delay the shift in the transition out of the state and how
            // much additional shift to add to the following state
            delay_amount[transition] = std::max(0, static_cast<int>(shift) - amt / 8);
            for (auto *trans : state->transitions) {
                if (!trans->next || trans->next != next_state) continue;
                extra_shift[trans->next] =
                    std::max(delay_amount[transition], extra_shift[trans->next]);
            }
        }

        if (LOGGING(5)) {
            LOG5("Shift delay:");
            for (auto [transition, amt] : delay_amount)
                LOG5("  " << transition.first->name << " -> " << transition.second->name << ": "
                          << amt);
            LOG5("Additional shift:");
            for (auto [state, amt] : extra_shift) LOG5("  " << state->name << ": " << amt);
        }

        return ParserModifier::init_apply(root);
    }

    bool preorder(IR::BFN::ParserState *state) override {
        const auto *orig = getOriginal<IR::BFN::ParserState>();

        // Add any additional shift amount to the extracts/select offsets
        if (extra_shift.count(orig)) {
            LOG3("Adjusting extracts/selects for " << state->name << " by " << extra_shift.at(orig)
                                                   << " bytes");
            LOG5("Original: " << state);

            // Adjust extract statements by the extra shift amount
            IR::Vector<IR::BFN::ParserPrimitive> new_statements;
            for (auto stmt : state->statements) {
                if (auto *extract = stmt->to<IR::BFN::Extract>()) {
                    if (auto *rval = extract->source->to<IR::BFN::InputBufferRVal>()) {
                        auto *new_rval = rval->clone();
                        new_rval->range = new_rval->range.shiftedByBytes(extra_shift.at(orig));
                        auto *new_extract = extract->clone();
                        new_extract->source = new_rval;
                        new_statements.push_back(new_extract);
                        continue;
                    }
                }
                new_statements.push_back(stmt);
            }
            state->statements = new_statements;

            // Adjust select statements by the extra shift amount
            IR::Vector<IR::BFN::Select> new_selects;
            for (auto select : state->selects) {
                if (auto *saved_rval = select->source->to<IR::BFN::SavedRVal>()) {
                    if (auto *buf_rval = saved_rval->source->to<IR::BFN::InputBufferRVal>()) {
                        auto *new_buf_rval = buf_rval->clone();
                        new_buf_rval->range =
                            new_buf_rval->range.shiftedByBytes(extra_shift.at(orig));
                        auto *new_saved_rval = saved_rval->clone();
                        new_saved_rval->source = new_buf_rval;
                        auto *new_select = select->clone();
                        new_select->source = new_saved_rval;
                        new_selects.push_back(new_select);
                        continue;
                    }
                }
                new_selects.push_back(select);
            }
            state->selects = new_selects;

            LOG5("Adjusted: " << state);
        }

        return true;
    }

    bool preorder(IR::BFN::Transition *trans) override {
        const auto *orig = getOriginal<IR::BFN::Transition>();
        const auto *state = findOrigCtxt<IR::BFN::ParserState>();

        // Adjust shifts on transitions, taking into account how much to delay the transition _from_
        // the state, and how much additional shift to add from delayed transitions _to_ the state.
        int adj = 0;
        auto transition = std::pair(state, orig->next);
        if (delay_amount.count(transition)) adj -= delay_amount.at(transition);
        if (extra_shift.count(state)) adj += extra_shift.at(state);

        if (adj) {
            auto orig_shift = trans->shift;
            trans->shift = static_cast<unsigned>(static_cast<int>(trans->shift) + adj);
            LOG3("Adjust shift on transition from "
                 << state->name << " to " << (trans->next ? trans->next->name : "DONE") << " by "
                 << adj << ": orig=" << orig_shift << " new=" << trans->shift);
        }

        return true;
    }

    /// Check whether a def can be delayed
    ///
    /// Current check(s):
    ///   - next state is only reachable from the current state
    ///
    /// Open questions:
    ///   - Do we need to restrict delays to cases where the new shift value never exceed the
    ///     maximum shift (32B)?
    ///   - If we allow cases where a new shift exceeds the max, do we need to rerun the split state
    ///     pass?
    static bool can_delay_def(const IR::BFN::ParserGraph &graph, const IR::BFN::ParserState *state,
                              const IR::BFN::Transition *transition) {
        for (auto t : graph.transitions_to(transition->next)) {
            if (graph.get_src(t) != state) return false;
        }
        return true;
    }

    const CollectParserInfo &parser_info;
    const std::map<gress_t, std::map<const Def *, std::set<const IR::BFN::ParserState *>>>
        &delay_defs;
};

typedef std::map<const IR::BFN::Transition *, const IR::BFN::ParserState *> DefSet;

struct AllocationResult {
    // The saves need to be executed on this transition.
    std::map<const IR::BFN::Transition *, std::vector<const IR::BFN::SaveToRegister *>>
        transition_saves;

    // Registers to scratches on this transition.
    std::map<const IR::BFN::Transition *, std::set<MatchRegister>> transition_scratches;

    // The register slices that this select should match against.
    std::map<const Use *, std::vector<std::pair<MatchRegister, nw_bitrange>>> save_reg_slices;
};

/// This is the match register allocation. We implements a greedy
/// graph coloring algorthim where two select's interfere if their
/// live range overlap.
class MatcherAllocator : public Visitor {
    const PhvInfo &phv;
    const CollectParserInfo &parser_info;
    const ParserUseDef &parser_use_def;

 public:
    MatcherAllocator(const PhvInfo &phv, const CollectParserInfo &pi,
                     const ParserUseDef &parser_use_def)
        : phv(phv), parser_info(pi), parser_use_def(parser_use_def) {}

 public:
    AllocationResult result;

    /// Defs to delay in an attempt to create a valid allocation.
    /// An empty set at the end of the pass indicates not to retry (no further
    /// potential defs to delay could be identified).
    std::map<gress_t, std::map<const Def *, std::set<const IR::BFN::ParserState *>>> delay_defs;

    /// Summary of delay defs from previous round. ParserState objects replaced by strings since
    /// the pointers change from round to round.
    std::map<std::pair<cstring, nw_bitrange>, std::set<cstring>> prev_delay_defs;

    /// Final iteration: return an error if allocation fails, even if delay_defs are identified
    bool final_iteration = false;

 private:
    /// Represents a group of select fields that can be coalesced into
    /// single allocation unit (see comment below for coalescing).
    struct UseGroup {
        const UseDef &use_def;

        std::vector<const Use *> members;

        std::vector<DefSet> def_transition_sets;

        unsigned bits_in_group = 0;

        explicit UseGroup(const UseDef &s) : use_def(s) {}

        const IR::BFN::ParserState *get_use_state() const {
            auto leader = members.front();
            return leader->state;
        }

        std::vector<const IR::BFN::ParserState *> get_def_states() const {
            auto leader = members.front();

            std::vector<const IR::BFN::ParserState *> rv;

            for (auto def : use_def.use_to_defs.at(leader)) rv.push_back(def->state);

            return rv;
        }

        const Def *get_def(const IR::BFN::ParserState *def_state) const {
            auto leader = members.front();
            return use_def.get_def(leader, def_state);
        }

        DefSet get_pred_def_set(const IR::BFN::ParserGraph &graph,
                                const std::vector<const IR::BFN::ParserState *> def_states) const {
            DefSet def_set;

            for (auto def_state : def_states) {
                for (auto pred : graph.predecessors().at(def_state)) {
                    for (auto t : graph.transitions(pred, def_state)) def_set[t] = def_state;
                }

                for (auto &kv : graph.loopbacks()) {
                    if (kv.first.second == def_state->name) {
                        for (auto t : kv.second) def_set[t] = def_state;
                    }
                }
            }

            return def_set;
        }

        /// For each select, computes a set of transitions where
        /// the branch word(s) need to be save into the match registers.
        void compute_def_transitions(const IR::BFN::ParserGraph &graph) {
            auto use_state = get_use_state();

            auto def_states = get_def_states();

            // case 1: def state == use state
            if (def_states.size() == 1 && def_states[0] == use_state) {
                BUG_CHECK(graph.predecessors().count(use_state),
                          "select state has no predecessors");

                DefSet def_set = get_pred_def_set(graph, def_states);

                def_transition_sets.push_back(def_set);
            } else {  // case 2: def states are ancestor states
                DefSet succ_def_set;

                for (auto def_state : def_states) {
                    for (auto succ : graph.successors().at(def_state)) {
                        if (succ == use_state || graph.is_ancestor(succ, use_state)) {
                            for (auto t : graph.transitions(def_state, succ))
                                succ_def_set[t] = def_state;
                        }
                    }
                }

                def_transition_sets.push_back(succ_def_set);

                // If scratch regs are available, we may need the additional choice of
                // scratching at the predecessors
                if (!Device::pardeSpec().scratchRegisters().empty()) {
                    DefSet pred_def_set = get_pred_def_set(graph, def_states);

                    def_transition_sets.push_back(pred_def_set);
                }
            }

            if (LOGGING(5)) {
                std::clog << "group: " << print() << " has " << def_transition_sets.size()
                          << " def sets:" << std::endl;

                unsigned id = 0;
                for (auto &def_set : def_transition_sets) {
                    std::clog << "def set " << id++ << ":" << std::endl;

                    for (auto &kv : def_set) {
                        std::clog << graph.get_src(kv.first)->name << " -> "
                                  << (kv.first->next ? kv.first->next->name : kv.first->loop)
                                  << " -- def state: " << kv.second->name << std::endl;
                    }
                }
            }

            BUG_CHECK(!def_transition_sets.empty(), "no def transition sets for select group?");
        }

        nw_bitrange get_range(const IR::BFN::ParserState *def_state) const {
            auto head = members.front();
            auto tail = members.back();

            int lo = use_def.get_def(head, def_state)->rval->range.lo;
            int hi = use_def.get_def(tail, def_state)->rval->range.hi;

            return nw_bitrange(lo, hi);
        }

        /// preconditions:
        ///   - assumes caller has checked candidate has same use/def states as group
        ///   - assumes defs are sorted already
        bool join(const Use *cand) {
            if (members.empty()) {
                members.push_back(cand);

                auto &defs = use_def.use_to_defs.at(cand);
                bits_in_group = defs.at(0)->rval->range.size();

                return true;
            }

            // check if candidate's rvals are in same byte as all defs in group
            // Note: They can even be in a following byte, which means that they are still
            // continuous, or rather any other match register load would start right
            // after; additionally this takes care of the possibility of using 16b
            // match registers that already loaded 2 bytes

            auto cand_defs = use_def.use_to_defs.at(cand);
            auto last_defs = use_def.use_to_defs.at(members.back());

            if (cand_defs.size() != last_defs.size()) return false;

            for (unsigned i = 0; i < cand_defs.size(); i++) {
                auto cd = cand_defs.at(i);
                auto ld = last_defs.at(i);

                if (ld->rval->range.hi + 1 == cd->rval->range.lo) continue;

                auto lo = cd->rval->range.lo / 8;
                auto hi = ld->rval->range.hi / 8;

                // Same byte
                if (lo == hi) continue;
                // Following byte
                if (lo == hi + 1) continue;

                // Not the same or following byte => cannot join
                return false;
            }

            members.push_back(cand);
            bits_in_group += cand_defs.at(0)->rval->range.size();

            return true;
        }

        unsigned bytes_in_group() const {
            auto head_defs = use_def.use_to_defs.at(members.front());
            auto tail_defs = use_def.use_to_defs.at(members.back());

            auto lo = head_defs.at(0)->rval->range.lo / 8;
            auto hi = tail_defs.at(0)->rval->range.hi / 8;

            auto bytes = hi - lo + 1;
            return bytes;
        }

        bool have_same_defs(const Use *a, const Use *b) const {
            auto a_defs = use_def.use_to_defs.at(a);
            auto b_defs = use_def.use_to_defs.at(b);

            if (a_defs.size() != b_defs.size()) return false;

            for (unsigned i = 0; i < a_defs.size(); i++) {
                if (!a_defs.at(i)->equiv(b_defs.at(i))) return false;
            }

            return true;
        }

        // a's defs are subset of b's defs
        bool have_subset_defs(const Use *a, const Use *b) const {
            if (!use_def.use_to_defs.count(a)) return false;
            if (!use_def.use_to_defs.count(b)) return false;
            auto a_defs = use_def.use_to_defs.at(a);
            auto b_defs = use_def.use_to_defs.at(b);

            for (unsigned i = 0; i < a_defs.size(); i++) {
                bool exists = false;

                for (unsigned j = 0; j < b_defs.size(); j++) {
                    if (a_defs.at(i)->equiv(b_defs.at(j))) {
                        exists = true;
                        break;
                    }
                }

                if (!exists) return false;
            }

            return true;
        }

        bool have_same_defs(const UseGroup *other) const {
            if (members.size() != other->members.size()) return false;
            return have_same_defs(members.at(0), other->members.at(0));
        }

        bool have_subset_defs(const UseGroup *other) const {
            return have_subset_defs(members.at(0), other->members.at(0));
        }

        /// Given the registers allocated to a group, compute the
        /// the fields' layout within each register (slices)
        std::vector<std::pair<MatchRegister, nw_bitrange>> calc_reg_slices_for_use(
            const Use *use, const std::vector<MatchRegister> &group_regs) const {
            std::vector<std::pair<MatchRegister, nw_bitrange>> reg_slices;

            auto def_states = get_def_states();
            auto state = def_states.at(0);  // pick first state as representative

            nw_bitrange group_range = get_range(state);
            nw_bitrange select_range = use_def.get_def(use, state)->rval->range;

            int group_start_bit_in_regs = group_range.lo % 8;
            int select_start_bit_in_regs =
                group_start_bit_in_regs + (select_range.lo - group_range.lo);

            nw_bitrange select_range_in_reg(
                StartLen(select_start_bit_in_regs, select_range.size()));

            int curr_reg_start = 0;
            for (auto &reg : group_regs) {
                int reg_size_in_bit = reg.size * 8;

                nw_bitrange curr_reg_range(StartLen(curr_reg_start, reg_size_in_bit));
                auto intersect = toClosedRange(select_range_in_reg.intersectWith(curr_reg_range));

                if (intersect) {
                    nw_bitrange range_in_reg = (*intersect).shiftedByBits(-curr_reg_start);
                    auto slice = std::make_pair(reg, range_in_reg);
                    reg_slices.push_back(slice);
                }

                curr_reg_start += reg_size_in_bit;
            }

            return reg_slices;
        }

        std::string print() const {
            std::stringstream ss;
            ss << "{ ";
            for (auto u : members) {
                ss << u->print() << " ";
            }
            ss << "}";
            return ss.str();
        }
    };

 private:
    profile_t init_apply(const IR::Node *root) override {
        // Record the delay defs before clearing
        // This data structure should be cleared externally before
        // running multiple allocations.
        for (auto &[_, delay_def_info] : delay_defs)
            for (auto &[def, next_states] : delay_def_info)
                for (auto *ns : next_states)
                    prev_delay_defs[std::make_pair(def->state->name, def->rval->range)].emplace(
                        ns ? ns->name : "NULL");

        result.transition_saves.clear();
        result.transition_scratches.clear();
        result.save_reg_slices.clear();
        delay_defs.clear();

        return Visitor::init_apply(root);
    }

    const IR::Node *apply_visitor(const IR::Node *root, const char *) override {
        for (auto &kv : parser_use_def) allocate_all(kv.first, kv.second);
        return root;
    }

    bool have_same_use_def_states(const UseDef &parser_use_def, const Use *a, const Use *b) {
        if (a->state != b->state) return false;

        auto a_defs = parser_use_def.use_to_defs.at(a);
        auto b_defs = parser_use_def.use_to_defs.at(b);

        if (a_defs.size() != b_defs.size()) return false;

        for (auto a_def : a_defs) {
            bool found = false;

            for (auto b_def : b_defs) {
                if (a_def->state->name == b_def->state->name) {
                    found = true;
                    break;
                }
            }

            if (!found) return false;
        }

        return true;
    }

    /// sort uses by their position in the input buffer
    void sort_use_group(std::vector<const Use *> &group, const UseDef &use_def) {
        std::stable_sort(group.begin(), group.end(), [&use_def](const Use *a, const Use *b) {
                int a_min, b_min;
                a_min = b_min = Device::pardeSpec().byteInputBufferSize() * 8;

            for (auto def : use_def.use_to_defs.at(a)) a_min = std::min(a_min, def->rval->range.lo);

            for (auto def : use_def.use_to_defs.at(b)) b_min = std::min(b_min, def->rval->range.lo);

                return a_min < b_min;
            });
    }

    std::vector<UseGroup *> coalesce_group(const std::vector<const Use *> &group,
                                           const IR::BFN::Parser *parser, const UseDef &use_def) {
        std::vector<UseGroup *> coalesced;

        for (auto select : group) {
            bool joined = false;

            for (auto coal : coalesced) {
                if (coal->join(select)) {
                    joined = true;
                    break;
                }
            }

            if (!joined) {
                auto coal = new UseGroup(use_def);
                coal->join(select);
                coalesced.push_back(coal);
            }
        }

        auto &graph = parser_info.graph(parser);

        for (auto coal : coalesced) coal->compute_def_transitions(graph);

        return coalesced;
    }

    // Before the register allocation, perform coalescing of select fields
    // so that each coalesced group will be considered as a single allocation unit.
    //
    // We can coalesce selects if they
    //   1. have the same use state
    //   2. have the same set of def states
    //   3. rvals can be coalesced (from contiguous packet bytes)
    std::vector<const UseGroup *> coalesce_uses(const IR::BFN::Parser *parser,
                                                const UseDef &use_def) {
        std::vector<std::vector<const Use *>> use_groups;

        // let's first group by condition 1 & 2

        for (auto &kv : use_def.use_to_defs) {
            bool joined = false;

            for (auto &group : use_groups) {
                auto leader = *group.begin();

                if (have_same_use_def_states(use_def, kv.first, leader)) {
                    group.push_back(kv.first);
                    joined = true;
                    break;
                }
            }

            if (!joined) use_groups.push_back({kv.first});
        }

        // now coalesce by condition 3

        std::vector<const UseGroup *> coalesced;

        for (auto &group : use_groups) {
            sort_use_group(group, use_def);

            auto coals = coalesce_group(group, parser, use_def);

            for (auto &coal : coals) coalesced.push_back(coal);
        }

        if (LOGGING(2)) {
            std::clog << "created " << coalesced.size() << " coalesced groups:" << std::endl;

            unsigned id = 0;

            for (auto coal : coalesced) {
                std::clog << "group " << id++ << ": ";
                std::clog << coal->print() << std::endl;
            }
        }

        return coalesced;
    }

    bool is_ancestor(const IR::BFN::ParserGraph &graph, const IR::BFN::Transition *x_def,
                     const IR::BFN::ParserState *y_use) {
        return x_def->next == y_use || graph.is_ancestor(x_def->next, y_use);
    }

    /// returns true if x's defs intersects y's (def,use)
    bool overlap(const IR::BFN::ParserGraph &graph, const UseGroup *x, const DefSet &x_defs,
                 const UseGroup *y, const DefSet &y_defs) {
        auto y_use = y->get_use_state();

        for (auto &kv : x_defs) {
            auto x_def = kv.first;
            auto x_def_state = kv.second;

            if (is_ancestor(graph, x_def, y_use)) {
                for (auto &xy : y_defs) {
                    auto y_def = xy.first;
                    auto y_def_state = xy.second;
                    auto x_def_src = graph.get_src(x_def);

                    auto xd = x->get_def(x_def_state);
                    auto yd = y->get_def(y_def_state);

                    if (xd->equiv(yd)) continue;

                    if (x_def == y_def || is_ancestor(graph, y_def, x_def_src)) return true;
                }
            }
        }

        return false;
    }

    /// returns true if a and b's live range intersection is not null
    bool interfere(const IR::BFN::ParserGraph &graph, const UseGroup *a, const DefSet &a_defs,
                   const UseGroup *b, const DefSet &b_defs) {
        if (overlap(graph, a, a_defs, b, b_defs)) return true;

        if (overlap(graph, b, b_defs, a, a_defs)) return true;

        return false;
    }

    /// Checks if Defset "a" and DefSet "b" have a common transition.
    /// Returns true if "a" and "b" have at least one transition
    /// in common.  Returns false otherwise.
    bool have_common_transition(const DefSet &a, const DefSet &b) {
        for (auto &x : a) {
            if (b.count(x.first)) return true;
        }
        return false;
    }

    struct Allocation {
        std::map<const UseGroup *, std::vector<MatchRegister>> group_to_alloc_regs;
        std::map<const UseGroup *, DefSet> group_to_def_set;
        std::map<const UseGroup *, std::map<cstring, MatchRegister>> group_to_scratch_reg_map;
    };

    /// Contains the match and scratch registers available for a given UseGroup DefSet,
    /// classified as constrained or unconstrained.
    ///
    /// Available "regular" match registers are never constrained, since they need to be unused
    /// in the specified DefSet to be considered available.
    ///
    /// Available scratch registers can be constrained or unconstrained:
    ///     - The constraint referred here appears when a scratch register is being
    ///       allocated in a DefSet that already has some registers allocated by
    ///       another UseGroup.  In that case:
    ///         - If the same scratch register is already allocated by the other UseGroup,
    ///           the scratch register can allocated but it has to deal with the same byte
    ///           in the header as the scratch register in the other UseGroup.
    ///         - If the associated "regular" match register (i.e. byte0 in the case of
    ///           save_byte0) is allocated by the other UseGroup, the scratch register
    ///           can also be allocated, but it has to deal with the same byte in the
    ///           header as the associated match register in the other UseGroup.
    ///     - A scratch register is unconstrained if it is not used by another UseGroup
    ///       in the DefSet and its associated match register is not used either.
    ///
    class AvailableRegisters {
     public:
        std::vector<MatchRegister> match_regs;
        std::vector<MatchRegister> unconstrained_scratch_regs;
        std::vector<MatchRegister> constrained_scratch_regs;

        /// The constrained_layout vector contains the match register layout of the UseGroup
        /// which has the most scratch register constraints with the current UseGroup DefSet.
        /// It used when placing constrained scratch registers during allocation.
        ///
        /// The ranges are byte-aligned in order to include all bits actually
        /// loaded in the registers.
        std::vector<std::pair<MatchRegister, nw_bitrange>> constraint_layout;

        unsigned get_total_size_byte() const {
            unsigned total_reg_bytes = 0;
            for (auto &reg : match_regs) total_reg_bytes += reg.size;
            for (auto &reg : unconstrained_scratch_regs) total_reg_bytes += reg.size;
            for (auto &reg : constrained_scratch_regs) total_reg_bytes += reg.size;
            return total_reg_bytes;
        }

        /// Adds the specified scratch registers to the AvailableRegisters object.
        /// The scratch registers are also classified as either constrained or
        /// unconstrained.  Information about the associated group constraints
        /// is set up.
        void add_scratch_regs(const Allocation &allocation,
                              const std::vector<MatchRegister> &scratch_regs,
                              const ordered_set<const UseGroup *> &subset_defs_groups) {
            const UseGroup *constraint_group = nullptr;
            int max_regs = 0;
            for (auto &group : subset_defs_groups) {
                int group_regs_num = allocation.group_to_alloc_regs.at(group).size();
                if (group_regs_num > max_regs) {
                    constraint_group = group;
                    max_regs = group_regs_num;
                }
            }

            if (constraint_group) setup_constraint_layout(constraint_group, allocation);

            for (auto &scratch : scratch_regs) {
                bool constrained = false;
                if (constraint_group) {
                    for (auto &constrained_reg :
                         allocation.group_to_alloc_regs.at(constraint_group)) {
                        if ((scratch == constrained_reg) ||
                            (MatcherAllocator::get_match_register(scratch) == constrained_reg)) {
                            constrained = true;
                            break;
                        }
                    }
                }
                if (constrained)
                    constrained_scratch_regs.push_back(scratch);
                else
                    unconstrained_scratch_regs.push_back(scratch);
            }
        };

        /// Looks for a constrained scratch register at bit_range_offset.
        bool get_constrained_scratch_reg(int bit_range_offset, MatchRegister &reg) const {
            for (auto &l : constraint_layout) {
                if (l.second.contains(bit_range_offset)) {
                    MatchRegister constraint_reg = l.first;

                    cstring name;
                    if (MatcherAllocator::is_scratch_register(constraint_reg))
                        name = constraint_reg.name;
                    else
                        name = "save_" + constraint_reg.name;

                    for (auto &scratch_reg : constrained_scratch_regs) {
                        if (scratch_reg.name == name) {
                            reg = scratch_reg;
                            return true;
                        }
                    }

                    return false;
                }
            }
            return false;
        }

        std::string print_constraint_layout() const {
            std::stringstream ss;
            for (auto &l : constraint_layout)
                ss << "    " << l.first.name << ": [" << l.second.lo << ".." << l.second.hi << "]"
                   << std::endl;
            return ss.str();
        }

     private:
        void setup_constraint_layout(const UseGroup *group, const Allocation &allocation) {
            std::vector<const IR::BFN::ParserState *> def_states = group->get_def_states();
            const IR::BFN::ParserState *state = def_states.at(0);  // first state as representative
            nw_bitrange group_range = group->get_range(state);

            int bit_offset = group_range.lo;
            for (auto &reg : allocation.group_to_alloc_regs.at(group)) {
                int reg_size_bit = reg.size * 8;
                int reg_lo = (bit_offset / reg_size_bit) * reg_size_bit;
                int reg_hi = reg_lo + reg_size_bit - 1;
                bit_offset = reg_hi + 1;
                constraint_layout.push_back(std::make_pair(reg, nw_bitrange(reg_lo, reg_hi)));
            }
        }
    };

    /// Match register allocation function that supports scratch registers.
    std::vector<MatchRegister> alloc_ascend_with_scratch(const UseGroup *group,
                                                         const AvailableRegisters avail_regs) {
        LOG3("match register allocation with scratch");
        std::vector<MatchRegister> allocated_regs;
        std::vector<MatchRegister> unconstraint_regs;

        // Create vector for all unconstrained registers.
        for (auto &reg : avail_regs.match_regs) unconstraint_regs.push_back(reg);
        for (auto &reg : avail_regs.unconstrained_scratch_regs) unconstraint_regs.push_back(reg);

        // Get constraint group register range layout.
        LOG3("  constraint group registers layout:" << std::endl
             << avail_regs.print_constraint_layout());

        // Get the range of the group to allocate.
        std::vector<const IR::BFN::ParserState *> def_states = group->get_def_states();
        const IR::BFN::ParserState *state = def_states.at(0);  // first state as representative
        nw_bitrange group_range = group->get_range(state);
        LOG3("  group to allocate range: [" << group_range.lo << ".." << group_range.hi << "]");

        // Loop through the group range and allocate registers favoring constrained
        // scratch registers.  If this is not possible, then allocate an unconstrained
        // match or scratch register.
        int bit_offset_in_range = group_range.lo;
        std::vector<MatchRegister>::iterator unconstraint_regs_it = unconstraint_regs.begin();

        while (bit_offset_in_range <= group_range.hi) {
            MatchRegister allocated_reg;
            MatchRegister constraint_group_reg;
            if (avail_regs.get_constrained_scratch_reg(bit_offset_in_range, allocated_reg)) {
                // Here, offset bit_offset_in_range falls into the constrained
                // group register range, while the associated constrained
                // scratch register is available.
                LOG3("  allocated: " << allocated_reg.name << " (constrained)");
            } else {
                // Here, a constrained scratch register could not be allocated.
                // Allocate the next unconstrained register.

                // Make sure there are still unconstrained registers available.
                if (unconstraint_regs_it == unconstraint_regs.end()) {
                    LOG3("reached end of unconstrained register list");
                    return {};
                }

                allocated_reg = *unconstraint_regs_it++;
                LOG3("  allocated: " << allocated_reg.name);
            }

            allocated_regs.push_back(allocated_reg);

            // Advance bit_offset_in_range.
            int reg_size_bit = allocated_reg.size * 8;
            int reg_lo = (bit_offset_in_range / reg_size_bit) * reg_size_bit;
            int reg_hi = reg_lo + reg_size_bit - 1;
            bit_offset_in_range = reg_hi + 1;
        }

        return allocated_regs;
    }

    /// Returns true when the specified scratch register can be used
    /// in the specified DefSet.  Return false otherwise.
    ///
    /// When the returned value is true, subset_defs_groups contains
    /// a list of groups that have Defs that are either subsets or supersets
    /// of the ones in group.
    bool can_scratch(const UseGroup *group, MatchRegister scratch, const Allocation &allocation,
                     const DefSet &def_set, ordered_set<const UseGroup *> &subset_defs_groups) {
        for (auto &kv : def_set) {
            auto transition = kv.first;

            for (auto &gr : allocation.group_to_def_set) {
                const UseGroup *other_group = gr.first;
                const DefSet &other_def_set = gr.second;
                if (other_def_set.count(transition)) {
                    if (group->have_subset_defs(other_group) ||
                                                  other_group->have_subset_defs(group)) {
                        if (!subset_defs_groups.count(other_group))
                            subset_defs_groups.push_back(other_group);
                        continue;
                    }

                    for (auto &reg : allocation.group_to_alloc_regs.at(other_group)) {
                        if (reg == get_match_register(scratch)) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    AvailableRegisters get_available_regs(const IR::BFN::Parser *parser, const UseGroup *group,
                                          const Allocation &allocation, const DefSet &def_set,
                       bool allow_scratch_regs) {
        AvailableRegisters avail_regs;
        auto &graph = parser_info.graph(parser);

        ordered_set<MatchRegister> used_regs;

        for (auto &their : allocation.group_to_alloc_regs) {
            auto they = their.first;
            auto &their_regs = their.second;
            auto &their_defs = allocation.group_to_def_set.at(they);

            if (interfere(graph, group, def_set, they, their_defs)) {
                for (auto &r : their_regs) used_regs.insert(r);

                LOG3("group " << group->print() << " interferes with " << they->print());

                if (have_common_transition(def_set, their_defs)) {
                    // If def_set and their_defs share a transition, if "their" uses
                    // a scratch register, mark the associated match register as
                    // unavailable, because both are tightly coupled and can't be
                    // used independently.
                    for (auto &reg : their_regs) {
                        if (is_scratch_register(reg)) {
                            for (auto &match_reg : Device::pardeSpec().matchRegisters()) {
                                if (match_reg == get_match_register(reg)) {
                                    if (!used_regs.count(match_reg)) {
                                        LOG3("  scratch register "
                                             << reg.name << " used in common transition, marking "
                                             << match_reg.name << " as used.");
                                        used_regs.insert(match_reg);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        for (auto &reg : Device::pardeSpec().matchRegisters()) {
            if (!used_regs.count(reg)) avail_regs.match_regs.push_back(reg);
        }

        if (allow_scratch_regs) {
            ordered_set<const UseGroup *> subset_defs_groups;
            std::vector<MatchRegister> scratch_regs;

            for (auto &reg : Device::pardeSpec().scratchRegisters()) {
                if (!used_regs.count(reg)) {
                    // If the scratch's associated match register is
                    // available, then the scratch register can't be
                    // considered as available also, because they are
                    // tightly coupled.
                    //
                    // (Same situation as above, but from the point of view
                    //  of the scratch register.)
                    bool found = false;
                    for (auto &r : avail_regs.match_regs) {
                        if (r == get_match_register(reg)) {
                            found = true;
                            break;
                        }
                    }
                    if (found) continue;

                    // Check if scratch register can be used based on
                    // current allocation and if so, if there are
                    // any constraints associated to it.
                    if (can_scratch(group, reg, allocation, def_set, subset_defs_groups)) {
                        scratch_regs.push_back(reg);
                    }
                }
            }

            // Store the available scratch registers in avail_regs and
            // classify as either constrained or unconstrained.
            avail_regs.add_scratch_regs(allocation, scratch_regs, subset_defs_groups);
        }

        LOG3("available regs are:");
        for (auto &reg : avail_regs.match_regs) LOG3(reg.name);
        for (auto &reg : avail_regs.unconstrained_scratch_regs) LOG3(reg.name);
        for (auto &reg : avail_regs.constrained_scratch_regs) LOG3(reg.name << " (constrained)");

        return avail_regs;
    }

    /// Given a set of available registers to the group, choose a
    /// set of registers that will cover all select fields.
    std::vector<MatchRegister> allocate(const UseGroup *group, AvailableRegisters avail_regs) {
        auto select_bytes = group->bytes_in_group();

        unsigned total_reg_bytes = avail_regs.get_total_size_byte();

        if (select_bytes > total_reg_bytes)  // not enough registers, bail
            return {};

        // try allocating in ascending order of container size
        std::vector<MatchRegister> alloc_ascend;
        unsigned reg_bytes_ascend = 0;

        for (auto &reg : avail_regs.match_regs) {
            if (reg_bytes_ascend >= select_bytes) break;

            alloc_ascend.push_back(reg);
            reg_bytes_ascend += reg.size;
        }

        // All registers are the same size in JBAY so only
        // ascending allocation needs to be performed.  If this allocation
        // fails, these targets support scratch registers, so try the
        // scratch-enabled allocation.
        if ((Device::currentDevice() == Device::JBAY)) {
            if (reg_bytes_ascend >= select_bytes) return alloc_ascend;
            if (!avail_regs.unconstrained_scratch_regs.empty() ||
                !avail_regs.constrained_scratch_regs.empty())
                return alloc_ascend_with_scratch(group, avail_regs);
            return {};
        }

        // now try allocating in descending order of container size
        std::reverse(avail_regs.match_regs.begin(), avail_regs.match_regs.end());

        std::vector<MatchRegister> alloc_descend;
        unsigned reg_bytes_descend = 0;

        for (auto &reg : avail_regs.match_regs) {
            if (reg_bytes_descend >= select_bytes) break;

            alloc_descend.push_back(reg);
            reg_bytes_descend += reg.size;
        }

        // pick better alloc of the two (first fit ascending vs. descending)
        if (reg_bytes_ascend < reg_bytes_descend) return alloc_ascend;

        if (reg_bytes_descend < reg_bytes_ascend) return alloc_descend;

        if (alloc_ascend.size() < alloc_descend.size()) return alloc_ascend;

        if (alloc_descend.size() < alloc_ascend.size()) return alloc_descend;

        return alloc_ascend;
    }

    static MatchRegister get_match_register(MatchRegister scratch) {
        for (auto reg : Device::pardeSpec().matchRegisters()) {
            if (reg.name == scratch.name.substr(5)) return reg;
        }

        BUG("Match register does not exist for %1%", scratch);
    }

    static bool is_scratch_register(const MatchRegister reg) {
        return reg.name.startsWith("save_");
    }

    /// Success is not final, failure is fatal.
    void fail(const UseGroup *group) {
        std::stringstream avail;

        avail << "Total available parser match registers on the device are: ";

        if (Device::currentDevice() == Device::TOFINO)
            avail << "1x16b, 2x8b.";
        else if (Device::currentDevice() == Device::JBAY)
            avail << "4x8b.";
        else
            BUG("Unknown device");

        std::stringstream hint;

        hint << "Consider shrinking the live range of select fields or reducing the";
        hint << " number of select fields that are being matched in the same state.";

        ::fatal_error("Ran out of parser match registers for %1%.\n%2% %3%", group->print(),
                      avail.str(), hint.str());
    }

    /// Bind the allocation results to the group. Also create the
    /// match register read and write intructions which will be
    /// inserted into the IR by the subsequent InsertSaveAndSelect pass.
    ///
    /// Set do_not_shift to true when dealing with scratch registers that
    /// are located at a fixed position in the input buffer.
    void bind_def(const IR::BFN::Parser *parser, const UseGroup *group,
                  const std::vector<MatchRegister> &alloc_regs, const DefSet &def_set,
                  bool do_not_shift) {
        auto &graph = parser_info.graph(parser);

        // def
        for (auto &kv : def_set) {
            auto transition = kv.first;

            auto src_state = graph.get_src(transition);
            auto def_state = kv.second;

            nw_byterange group_range = group->get_range(def_state).toUnit<RangeUnit::Byte>();

            if (!do_not_shift && (src_state != def_state)) {
                auto shift = transition->shift;
                group_range = group_range.shiftedByBytes(shift);
            }

            bool partial_hdr_err_proc = false;
            const IR::BFN::ParserState *state_next = transition->next;
            while (state_next) {
                if (state_next->selects.size()) {
                    // Found the select() for which we're allocating this match register.
                    auto select = state_next->selects.at(0);
                    if (auto *saved_rval = select->source->to<IR::BFN::SavedRVal>()) {
                        if (auto *pkt_rval = saved_rval->source->to<IR::BFN::PacketRVal>())
                            partial_hdr_err_proc = pkt_rval->partial_hdr_err_proc;
                    }
                    break;
                }

                // If state_next doesn't have a select(), then it only has
                // a single catch-all transition, most likely because it has been
                // split.  Check the state referred by that transition to find
                // the select() for which we're allocating this match register.
                auto t = state_next->transitions.at(0);
                if (t)
                    state_next = t->next;
                else
                    break;
            }

            unsigned bytes = 0;

            for (auto reg : alloc_regs) {
                unsigned lo = group_range.lo + bytes;
                nw_byterange reg_range(lo, lo + reg.size - 1);

                BUG_CHECK(size_t(reg_range.size()) <= reg.size,
                           "saved bits greater than register size?");

                // The state splitter should have split the states such that the branch
                // word is withtin current state. This is the pre-condition of this pass.
                BUG_CHECK((reg_range.lo >= 0 &&
                           reg_range.hi <= Device::pardeSpec().byteInputBufferSize()) ||
                           Device::pardeSpec().byteScratchRegisterRangeValid(reg_range),
                          "branch word out of current input buffer!");

                bool scratch = false;

                if (is_scratch_register(reg)) {
                    reg = get_match_register(reg);
                    scratch = true;
                }

                auto save = new IR::BFN::SaveToRegister(
                    new IR::BFN::PacketRVal(reg_range.toUnit<RangeUnit::Bit>(),
                                            partial_hdr_err_proc),
                    reg);

                bool saved = false;
                for (auto s : result.transition_saves[transition]) {
                    if (save->equiv(*s)) {
                        saved = true;
                        break;
                    }
                }

                if (!saved) result.transition_saves[transition].push_back(save);

                if (scratch) result.transition_scratches[transition].insert(reg);

                bytes += reg.size;
            }
        }
    }

    void bind_use(const UseGroup *group, const std::vector<MatchRegister> &alloc_regs,
                  const Allocation &allocation) {
        // use
        for (auto &use : group->members) {
            auto reg_slices = group->calc_reg_slices_for_use(use, alloc_regs);

            // Map scratch register with match register allocated by scratch groups.
            for (auto &reg_slice : reg_slices) {
                if (!is_scratch_register(reg_slice.first)) continue;

                cstring name = reg_slice.first.name;

                BUG_CHECK(allocation.group_to_scratch_reg_map.count(group),
                          "scratch register group not found in allocation: %1%", group->print());

                BUG_CHECK(allocation.group_to_scratch_reg_map.at(group).count(name),
                          "scratch register %1% not found in allocation", name);

                MatchRegister reg = allocation.group_to_scratch_reg_map.at(group).at(name);
                reg_slice.first.name = reg.name;

                LOG3("update slice: " << name << " --> " << reg.name << " [" << reg_slice.second.lo
                                      << ".." << reg_slice.second.hi << "]");
            }

            if (result.save_reg_slices.count(use)) {
                bool eq = equiv(result.save_reg_slices.at(use), reg_slices);

                BUG_CHECK(eq, "select has different allocations based on the use context: %1%",
                          use->print());
            } else {
                result.save_reg_slices[use] = reg_slices;
            }
        }
    }

    void bind_allocation(const IR::BFN::Parser *parser, const Allocation &allocation,
                         const UseGroup *group,
                         const std::vector<const UseGroup *> scratch_groups) {
        bind_def(parser, group, allocation.group_to_alloc_regs.at(group),
                 allocation.group_to_def_set.at(group), false);

        bind_use(group, allocation.group_to_alloc_regs.at(group), allocation);

        for (auto &scratch_group : scratch_groups) {
            bind_def(parser, scratch_group, allocation.group_to_alloc_regs.at(scratch_group),
                     allocation.group_to_def_set.at(scratch_group), true);
        }
    }

    /// Allocates match registers for the group specified.
    /// Optionally allocates scratch registers and creates the
    /// UseGroups to allow matching on them.
    bool allocate_group(const IR::BFN::Parser *parser, const UseGroup *group,
                        Allocation &allocation, bool do_bind_allocation, bool allow_scratch_regs,
                        bool is_top_down) {
        // Run the group allocation in struct "allocation_tmp" in case scratch registers
        // are required: scratch register allocation requires two allocations and the second
        // allocation must take into account the updates from the first.  Structure
        // "allocation" is updated only when both allocations are successful in order
        // to allow to rolling back when the second allocation fails.
        Allocation allocation_tmp = allocation;
        std::vector<MatchRegister> scratch_regs;
        bool success = false;

        LOG3(">>>>>>>>>>>>>>>>");
        LOG3("allocating " << group->print());

        // try all def transition sets of group
        unsigned id = 0;

        for (auto &def_set : group->def_transition_sets) {
            LOG3("try def set " << id++);

            auto avail_regs =
                get_available_regs(parser, group, allocation_tmp, def_set, allow_scratch_regs);

            auto alloc_regs = allocate(group, avail_regs);

            if (!alloc_regs.empty()) {
                success = true;

                allocation_tmp.group_to_alloc_regs[group] = alloc_regs;
                allocation_tmp.group_to_def_set[group] = def_set;

                // Extract scratch registers from allocated registers.
                for (auto &reg : alloc_regs) {
                    if (is_scratch_register(reg)) scratch_regs.push_back(reg);
                }

                if (LOGGING(1)) {
                    std::clog << "allocated { ";
                    for (auto &reg : alloc_regs) std::clog << "$" << reg.name << " ";
                    std::clog << "} to " << group->print() << std::endl;
                }
                break;
            } else {
                LOG3("not ok");
            }
        }

        // When using scratch registers, create the UseGroup(s) necessary to load
        // the scratch register into an actual match register before the match.
        std::vector<const UseGroup *> scratch_groups;
        if (success && allow_scratch_regs && scratch_regs.size()) {
            scratch_groups = allocate_scratch_groups(parser, group, scratch_regs, allocation_tmp);
            if (scratch_groups.empty()) success = false;
        }

        // If allocation fails, identify whether the defs could potentially be delayed.
        //
        // Only record this information during a top-down allocation. (Could do
        // either top-down or bottom-up.)
        if (!success && is_top_down) {
            auto &graph = parser_info.graph(parser);
            auto *use_state = group->get_use_state();
            bool defs_safe = true;
            std::map<const Def *, std::set<const IR::BFN::ParserState *>> defs_to_delay;
            for (auto &def_set : group->def_transition_sets) {
                for (auto &[transition, state] : def_set) {
                    for (auto &alloc : allocation.group_to_alloc_regs) {
                        auto alloc_state = alloc.first;
                        auto &alloc_defs = allocation.group_to_def_set.at(alloc_state);

                        if (interfere(graph, group, def_set, alloc_state, alloc_defs)) {
                            auto *alloc_use = alloc_state->get_use_state();
                            if (graph.is_ancestor(state, alloc_use) &&
                                graph.is_ancestor(alloc_use, use_state)) {
                                auto *def = group->get_def(state);
                                for (auto t : def->state->transitions) {
                                    if (t->next && (t->next == alloc_use ||
                                                    graph.is_ancestor(t->next, alloc_use))) {
                                        defs_safe &= DelayDefs::can_delay_def(graph, state, t);
                                        if (defs_safe) {
                                            auto def_pair =
                                                std::make_pair(def->state->name, def->rval->range);
                                            if (!prev_delay_defs.count(def_pair) ||
                                                !prev_delay_defs.at(def_pair).count(
                                                    t->next ? t->next->name : "NULL"_cs))
                                                defs_to_delay[def].emplace(t->next);
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            if (defs_safe) {
                delay_defs[parser->gress].insert(defs_to_delay.begin(), defs_to_delay.end());
                if (LOGGING(3)) {
                    LOG3("defs to possibly delay:");
                    for (auto &[def, _] : defs_to_delay) LOG3(def->print());
                }
            }
        }

        if (success) {
            if (do_bind_allocation) bind_allocation(parser, allocation_tmp, group, scratch_groups);

            allocation = allocation_tmp;
            LOG3("success");
        }

        LOG3("<<<<<<<<<<<<<<<<");
        return success;
    }

    /// This is the core allocation routine. The basic idea to use two select
    /// groups' interference relationship to determine whether they can be
    /// allocated to the same set of registers.
    void allocate_all(const IR::BFN::Parser *parser, const UseDef &use_def) {
        auto coalesced_groups = coalesce_uses(parser, use_def);

        auto saved_result = result;  // save result

        LOG3("try allocating top down:");
        auto top_down_unalloc = try_allocate_all(parser, coalesced_groups, true);

        if (!top_down_unalloc) {
            LOG3("top down allocation successful!");
            delay_defs[parser->gress].clear();
            return;
        }

        result = saved_result;  // rollback

        LOG3("try allocating bottom up:");
        std::reverse(coalesced_groups.begin(), coalesced_groups.end());
        auto bottom_up_unalloc = try_allocate_all(parser, coalesced_groups, false);

        if (!bottom_up_unalloc) {
            LOG3("bottom up allocation successful!");
            delay_defs[parser->gress].clear();
            return;
        }

        if (!delay_defs[parser->gress].size() || final_iteration) fail(top_down_unalloc);
    }

    std::vector<const UseGroup *> allocate_scratch_groups(
        const IR::BFN::Parser *parser, const UseGroup *group,
        const std::vector<MatchRegister> &scratch_regs, Allocation &allocation,
                                            bool is_top_down = false) {
        const IR::BFN::ParserState *state = group->get_use_state();
        std::vector<const UseGroup *> scratch_groups;
        UseDef *use_def = new UseDef();
        std::map<const Use *, cstring> use_to_scratch_reg;

        if (LOGGING(3)) {
            LOG3("alocating scratch group for scratch registers:");
            for (auto &reg : scratch_regs) {
                LOG3("  " << reg.name);
            }
        }

        for (auto &reg : scratch_regs) {
            nw_bitrange range = Device::pardeSpec().bitScratchRegisterRange(reg);
            BUG_CHECK(range.size() >= 8, "Unsupported scratch register.");

            const IR::BFN::PacketRVal *inbuf_rval = new IR::BFN::PacketRVal(range, false);
            const IR::BFN::PacketRVal *pkt_rval = new IR::BFN::PacketRVal(range, false);
            const IR::BFN::SavedRVal *saved_rval = new IR::BFN::SavedRVal(pkt_rval);

            const Def *def = new Def(state, inbuf_rval);
            const Use *use = new Use(state, saved_rval);

            use_to_scratch_reg[use] = reg.name;

            use_def->add_def(phv, use, def);
        }

        LOG3("scratch UseDef for matching:");
        LOG3(use_def->print());

        LOG3("creating scratch group");
        scratch_groups = coalesce_uses(parser, *use_def);
        LOG3("---");

        for (auto &scratch_group : scratch_groups) {
            LOG3("allocating match registers for scratch group " << scratch_group->print());
            if (!allocate_group(parser, scratch_group, allocation, false, false, is_top_down))
                return {};

            // Match registers allocated to handle matching of scratch register.
            std::vector<MatchRegister> alloc_regs =
                                            allocation.group_to_alloc_regs.at(scratch_group);

            // Associate the scratch register to the match register allocated.
            LOG3("match registers to store scratch registers for matching:");
            int i = 0;
            for (auto &member : scratch_group->members) {
                BUG_CHECK(use_to_scratch_reg.count(member), "Member (use) not found.");

                cstring name = use_to_scratch_reg[member];
                MatchRegister alloc_reg = alloc_regs.at(i++);

                LOG3("  " << name << " --> " << alloc_reg.name);
                allocation.group_to_scratch_reg_map[group][name] = alloc_reg;
            }
        }

        return scratch_groups;
    }

    const UseGroup *try_allocate_all(const IR::BFN::Parser *parser,
                                     const std::vector<const UseGroup *> &coalesced_groups,
                                     bool is_top_down) {
        Allocation allocation;

        for (auto group : coalesced_groups) {
            try {
                if (allocation.group_to_alloc_regs.count(group)) continue;

                if (!allocate_group(parser, group, allocation, true,
                                    !Device::pardeSpec().scratchRegisters().empty(), is_top_down))
                    return group;

                if (Device::pardeSpec().scratchRegisters().empty()) {
                    for (auto other : coalesced_groups) {
                        if (!allocation.group_to_alloc_regs.count(other)) {
                            if (group->have_subset_defs(other) || other->have_subset_defs(group)) {
                                if (!allocate_group(parser, other, allocation, true, false,
                                                    is_top_down))
                                    return other;
                            }
                        }
                    }
                }
            } catch (const Util::CompilerBug &e) {
                std::stringstream ss;
                ss << "An error occured while the compiler was allocating parser match";
                ss << " registers for " << group->print() << ".";  // sorry
                BUG("%1%", ss.str());
            }
        }

        return nullptr;
    }
};

/// Insert match register read/write instruction in the IR
struct InsertSaveAndSelect : public ParserModifier {
    explicit InsertSaveAndSelect(const AllocationResult &result) : rst(result) {}

    void postorder(IR::BFN::Transition *transition) override {
        auto original_transition = getOriginal<IR::BFN::Transition>();

        if (rst.transition_saves.count(original_transition)) {
            for (auto save : rst.transition_saves.at(original_transition)) {
                transition->saves.push_back(save);
            }
        }

        if (rst.transition_scratches.count(original_transition)) {
            for (auto reg : rst.transition_scratches.at(original_transition)) {
                transition->scratches.insert(reg);
            }
        }
    }

    void postorder(IR::BFN::SavedRVal *save) override {
        auto state = findContext<IR::BFN::ParserState>();
        auto use = new Use(state, save);

        for (auto &kv : rst.save_reg_slices) {
            if (kv.first->equiv(use)) {
                save->reg_slices = kv.second;
                return;
            }
        }

        auto select = findContext<IR::BFN::Select>();
        BUG("Parser match register not allocated for %1%", select->p4Source);
    }

    const AllocationResult &rst;
};

/// Adjust the match constant according to the select field's
/// alignment in the match register(s) and/or counter reads
class MatchLayout {
 private:
    std::vector<cstring> sources;
    std::map<cstring, match_t> values;
    std::map<cstring, size_t> sizes;
    size_t total_size;

    match_t shiftRight(match_t val, int n) {
        auto word0 = val.word0;
        auto word1 = val.word1;
        word0 >>= n;
        word1 >>= n;
        return match_t(word0, word1);
    }

    match_t shiftLeft(match_t val, int n) {
        auto word0 = val.word0;
        auto word1 = val.word1;
        word0 <<= n;
        word1 <<= n;
        return match_t(word0, word1);
    }

    match_t orTwo(match_t a, match_t b) {
        auto word0 = (a.word0 | b.word0);
        auto word1 = (a.word1 | b.word1);
        return match_t(word0, word1);
    }

    match_t setDontCare(match_t a) {
        auto word0 = a.word0;
        auto word1 = a.word1;
        auto wilds = (~(word0 ^ word1)) & (~((~uintmax_t(0)) << total_size));
        return match_t(word0 | wilds, word1 | wilds);
    }

    // start is the from the left
    match_t getSubValue(match_t val, int sz, int start, int len) {
        auto word0 = val.word0;
        auto word1 = val.word1;
        uintmax_t mask = (~((~uintmax_t(0)) << (sz - start)));
        auto trim_left_word0 = (word0 & mask);
        auto trim_left_word1 = (word1 & mask);
        return shiftRight(match_t(trim_left_word0, trim_left_word1), sz - start - len);
    }

    void writeValue(const IR::BFN::Select *select, match_t val) {
        if (auto saved = select->source->to<IR::BFN::SavedRVal>()) {
            auto &reg_slices = saved->reg_slices;
            int val_size = 0;
            int val_shifted = 0;

            for (auto rs : reg_slices) val_size += rs.second.size();

            for (auto &slice : reg_slices) {
                auto &reg = slice.first;
                auto &reg_sub_range = slice.second;

                nw_bitrange reg_range(0, reg.size * 8 - 1);
                BUG_CHECK(reg_range.contains(reg_sub_range),
                          "range not part of parser match register?");

                match_t slice_val = getSubValue(val, val_size, val_shifted, reg_sub_range.size());
                match_t val_to_or = shiftLeft(slice_val, reg.size * 8 - reg_sub_range.hi - 1);

                values[reg.name] = orTwo(values[reg.name], val_to_or);
                val_shifted += reg_sub_range.size();
            }

            BUG_CHECK(val_shifted == val_size, "value not entirely shifted?");
        } else if (auto cntr = select->source->to<IR::BFN::ParserCounterIsZero>()) {
            values[cntr->declName + ".ctr_zero"] = val;
        } else if (auto cntr = select->source->to<IR::BFN::ParserCounterIsNegative>()) {
            values[cntr->declName + ".ctr_neg"] = val;
        } else {
            BUG("Unknown parser select source");
        }
    }

 public:
    explicit MatchLayout(const IR::BFN::ParserState *state, const IR::BFN::Transition *transition)
        : total_size(0) {
        for (const auto *select : state->selects) {
            if (auto saved = select->source->to<IR::BFN::SavedRVal>()) {
                for (const auto &rs : saved->reg_slices) {
                    auto reg = rs.first;
                    if (!values.count(reg.name)) {
                        sources.push_back(reg.name);
                        values[reg.name] = match_t();
                        sizes[reg.name] = reg.size * 8;
                        total_size += reg.size * 8;
                    }
                }
            } else if (auto cntr = select->source->to<IR::BFN::ParserCounterIsZero>()) {
                sources.push_back(cntr->declName + ".ctr_zero");
                values[cntr->declName + ".ctr_zero"] = match_t();
                sizes[cntr->declName + ".ctr_zero"] = 1;
                total_size += 1;
            } else if (auto cntr = select->source->to<IR::BFN::ParserCounterIsNegative>()) {
                sources.push_back(cntr->declName + ".ctr_neg");
                values[cntr->declName + ".ctr_neg"] = match_t();
                sizes[cntr->declName + ".ctr_neg"] = 1;
                total_size += 1;
            }
        }

        // Pop out value for each select.
        auto const_val = transition->value->to<IR::BFN::ParserConstMatchValue>()->value;
        big_int word0 = const_val.word0;
        big_int word1 = const_val.word1;
        auto shiftOut = [&word0, &word1](int sz) {
            big_int mask = (big_int(1) << sz) - 1;
            big_int sub0 = (word0 & mask);
            big_int sub1 = (word1 & mask);
            word0 >>= sz;
            word1 >>= sz;
            return match_t(sub0, sub1);
        };

        std::map<const IR::BFN::Select *, match_t> select_values;
        for (const auto *select : boost::adaptors::reverse(state->selects)) {
            int value_size = 0;
            if (auto saved = select->source->to<IR::BFN::SavedRVal>()) {
                for (auto rs : saved->reg_slices) {
                    auto mask = rs.second;
                    value_size += mask.size();
                }
                select_values[select] = shiftOut(value_size);
            } else if (select->source->is<IR::BFN::ParserCounterRVal>()) {
                select_values[select] = shiftOut(1);
            }
        }

        for (const auto *select : state->selects) {
            if (select->source->is<IR::BFN::SavedRVal>() ||
                select->source->is<IR::BFN::ParserCounterRVal>())
                writeValue(select, select_values[select]);
        }
    }

    match_t getMatchValue() {
        int shift = 0;
        match_t rv;
        for (const auto &src : boost::adaptors::reverse(sources)) {
            rv = orTwo(rv, shiftLeft(values[src], shift));
            shift += sizes.at(src);
        }

        rv = setDontCare(rv);
        return rv;
    }
};

struct AdjustMatchValue : public ParserModifier {
    void postorder(IR::BFN::Transition *transition) override {
        if (transition->value->is<IR::BFN::ParserConstMatchValue>())
            adjustConstValue(transition);
        else if (transition->value->is<IR::BFN::ParserPvsMatchValue>())
            adjustPvsValue(transition);
        else
            BUG("Unknown match value: %1%", transition->value);
    }

    /// Adjust the transition value to a match_t that:
    ///   1. match_t value's size = sum of size of all used registers.
    ///   2. values are placed into their `slot` in match_t value.
    void adjustConstValue(IR::BFN::Transition *transition) {
        auto *state = findContext<IR::BFN::ParserState>();
        MatchLayout layout(state, transition);
        auto value = layout.getMatchValue();
        transition->value = new IR::BFN::ParserConstMatchValue(value);
    }

    /// Build a mapping from fieldslice to matcher slice by checking the field
    /// where the select originally matched on and the masking of the registers.
    void adjustPvsValue(IR::BFN::Transition *transition) {
        auto *state = findContext<IR::BFN::ParserState>();
        auto *old_value = transition->value->to<IR::BFN::ParserPvsMatchValue>();
        auto *adjusted = new IR::BFN::ParserPvsMatchValue(old_value->name, old_value->size);

        for (const auto *select : state->selects) {
            const IR::Expression *source = select->p4Source;
            if (auto sl = select->p4Source->to<IR::Slice>()) source = sl->e0;

            cstring field_name = stripThreadPrefix(source->toString());
            field_name = canon_name(field_name);
            int field_start = 0;

            if (auto saved = select->source->to<IR::BFN::SavedRVal>()) {
                for (const auto &rs : boost::adaptors::reverse(saved->reg_slices)) {
                    auto reg = rs.first;
                    auto reg_slice = rs.second.toOrder<Endian::Little>(reg.size * 8);

                    nw_bitrange field_slice(StartLen(field_start, reg_slice.size()));
                    adjusted->mapping[{field_name, field_slice}] = {reg, reg_slice};

                    LOG2("add PVS mapping: " << field_name << field_slice << " -> " << reg << " : "
                                             << reg_slice);

                    field_start += field_slice.size();
                }
            }
        }

        transition->value = adjusted;
    }
};

const IR::BFN::ParserState *can_remove(const IR::BFN::ParserState *state) {
    if (!state->statements.empty()) return nullptr;

    if (!state->selects.empty()) return nullptr;

    if (state->transitions.size() != 1) return nullptr;

    auto t = state->transitions[0];

    if (!t->saves.empty()) return nullptr;

    if (!t->scratches.empty()) return nullptr;

    if (t->shift == 0) return t->next;

    return nullptr;
}

// After match register allocation, if the start state is empty and its
// transition to the next state contains no writes to the match register,
// we can safely remove this empty state.
// Also, since extracts with MatchLVal type destination only exists to
// provide information for allocating parser match registers, they are no
// longer needed and can be deleted.
struct RemoveEmptyStartStateAndMatchExtract : public ParserTransform {
    IR::BFN::Parser *preorder(IR::BFN::Parser *parser) {
        if (parser->start) {
            if (auto next = can_remove(parser->start)) {
                parser->start = next;
                LOG4("removed empty parser start state on " << parser->gress);
            }
        }

        return parser;
    }

    IR::BFN::Extract *preorder(IR::BFN::Extract *extract) {
        if (extract->dest->is<IR::BFN::MatchLVal>()) {
            LOG5("Removed match value extract " << extract);
            return nullptr;
        }
        return extract;
    }
};

// If the stall state did not receive reg allocation, we can also
// safely remove this stall state.
struct RemoveEmptyStallState : public ParserModifier {
    void postorder(IR::BFN::Transition *t) override {
        if (t->next && t->next->name.find("$oob_stall")) {
            if (auto next = can_remove(t->next)) {
                LOG4("removed empty parser stall state  " << t->next->name);
                t->next = next;
            }
        }
    }
};

AllocateParserMatchRegisters::AllocateParserMatchRegisters(const PhvInfo &phv) {
    // Maximum number of allocation attempts. Each attempt will adjust shift amounts
    // in order to reduce the number of "live" select values at any given time.
    static const int MAX_ALLOC_ATTEMPTS = 16;

    auto *parserInfo = new CollectParserInfo;
    auto *collectUseDef = new CollectParserUseDef(phv, *parserInfo);
    auto *resolveOobDefs = new ResolveOutOfBufferSaves(collectUseDef->parser_use_def);
    auto *allocator = new MatcherAllocator(phv, *parserInfo, collectUseDef->parser_use_def);

    addPasses({LOGGING(4) ? new DumpParser("before_parser_match_alloc") : nullptr,
               new InsertInitSaveState, parserInfo, collectUseDef, resolveOobDefs,
        LOGGING(4) ? new DumpParser("after_resolve_oob_saves") : nullptr,
        [allocator, this]() {
            // Resets that need to occur before the first iteration of the allocator.
            //
            // delay_defs are stored for comparison with the last round, so reset these here
            // so that an empty set it copied
            this->iteration = 0;
            allocator->prev_delay_defs.clear();
            allocator->delay_defs.clear();
        },
        new PassRepeated(
            {
                parserInfo,
                collectUseDef,
                [allocator, this]() {
                    // Record whether this is the final iteration so that
                    // the allocator can report an error even if it can identify
                    // potential defs to delay
                    allocator->final_iteration = ++this->iteration == MAX_ALLOC_ATTEMPTS;
                },
                allocator,
                new PassIf(
                    [allocator]() {
                        return allocator->delay_defs[INGRESS].size() != 0 ||
                               allocator->delay_defs[EGRESS].size() != 0;
                    },
                    {
                        new DelayDefs(*parserInfo, allocator->delay_defs),
                    }),
                   },
                   MAX_ALLOC_ATTEMPTS),
               new InsertSaveAndSelect(allocator->result), new AdjustMatchValue,
               new RemoveEmptyStartStateAndMatchExtract, new RemoveEmptyStallState,
               LOGGING(4) ? new DumpParser("after_parser_match_alloc") : nullptr});
}

/* clang-format on */
