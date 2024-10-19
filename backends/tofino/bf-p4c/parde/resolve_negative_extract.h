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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_RESOLVE_NEGATIVE_EXTRACT_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_RESOLVE_NEGATIVE_EXTRACT_H_

#include <map>
#include <sstream>
#include <utility>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "bf-p4c/parde/dump_parser.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_dominator_builder.h"
#include "bf-p4c/parde/parser_info.h"
#include "ir/ir-generated.h"
#include "lib/indent.h"
#include "lib/ordered_map.h"

/**
 * @ingroup parde
 * @brief For extracts with negative source, i.e. source is in an earlier state, adjust
 *        the state's shift amount, and possibly move extracts, so that the source is
 *        within current state's input buffer.
 */
struct ResolveNegativeExtract : public PassManager {
    /// Colect all negative extract states that can't be satisfied by delaying shifts alone (i.e.,
    /// the negative offsets exceed the parser window size). These negative extracts require
    /// delaying shift _and_ redistributing the extracts. For these states, calculate the amount to
    /// delay shifts by.
    ///
    /// The shift delay/extract restribution occurs between directly-connected nodes (parent +
    /// child). Shifts/extracts will not be distributed across more than one level of hierarchy.
    struct CollectNegativeExtractOutOfBufferStates : public ParserInspector {
        const CollectParserInfo &parserInfo;

        /**
         * @brief In-buffer offsets of states
         */
        std::map<cstring, unsigned> state_to_shift;

        explicit CollectNegativeExtractOutOfBufferStates(const CollectParserInfo &pi)
            : parserInfo(pi) {}

        unsigned max_buff_size = 0;

        bool preorder(const IR::BFN::PacketRVal *rval) override {
            auto extract = findContext<IR::BFN::Extract>();
            if (extract && rval->range.lo < 0) {
                auto state = findContext<IR::BFN::ParserState>();
                unsigned shift = (-rval->range.lo + 7) / 8;
                if (shift > max_buff_size) {
                    LOG1("State " << state->name << " requires " << shift << " B shift");
                    historic_states[state] = std::max(historic_states[state], shift);
                    parsers[state] = findContext<IR::BFN::Parser>();
                }
            }

            return false;
        }

        profile_t init_apply(const IR::Node *node) override {
            auto rv = ParserInspector::init_apply(node);
            // Initialize all structures
            max_buff_size = Device::pardeSpec().byteInputBufferSize();
            state_to_shift.clear();
            historic_states.clear();
            parsers.clear();
            return rv;
        }

        void end_apply() override {
            // Required data capture update all node states
            for (auto kv : historic_states) {
                // 1] Distribute the required history value and adjust transitions
                auto state = kv.first;
                auto max_idx_value = kv.second;

                unsigned delay_shift =
                    delay_shift_from_predecessor(state, parsers[state], max_idx_value);
                BUG_CHECK(delay_shift,
                          "In parse state %s: a value that is %d B backwards from the current "
                          "parsing position is being accessed/used. Unable to identify an "
                          "amount to delay the previuos shift by to allow access to this data. "
                          "As a possible workaround try moving around the extracts (possibly by "
                          "using methods advance and lookahead or splitting some headers).",
                          state->name, max_idx_value);

                LOG3("State " << state->name << " needs a value " << max_idx_value
                              << "B back and generates a delay shift of " << delay_shift << "B");
            }

            LOG3("CollectNegativeExtractOutOfBufferStates has finished.");
        }

     private:
        /**
         * @brief Set of nodes which were identified as nodes where historic
         * data need to be accessed. The value stored together with the state
         * is the maximal historic data index
         *
         */
        std::map<const IR::BFN::ParserState *, unsigned> historic_states;

        /**
         * @brief Mapping of Parser state to given Parser instance
         *
         */
        std::map<const IR::BFN::ParserState *, const IR::BFN::Parser *> parsers;

        /**
         * @brief Delay the shift from the predecessor to provide additional backwards history
         *
         * @param state Analyzed parser state
         * @param succ Successors of the state from which we are calling (can be null)
         * @param parser Analyzed parser state
         * @param required_history Required backward history in bytes
         */
        unsigned delay_shift_from_predecessor(const IR::BFN::ParserState *state,
                                              const IR::BFN::Parser *parser, int required_history) {
            BUG_CHECK(state, "Parser state cannot be null!");

            auto graph = parserInfo.graph(parser);
            auto preds = graph.predecessors().at(state);
            if (preds.size() > 1) {
                error(
                    "Cannot resolve negative extract because of multiple paths to "
                    "the node %1%",
                    state->name);
            }
            const auto *pred = *preds.begin();

            unsigned curr_shift = 0;
            std::for_each(pred->transitions.begin(), pred->transitions.end(),
                          [&curr_shift](const auto *tr) {
                              curr_shift = curr_shift > tr->shift ? curr_shift : tr->shift;
                          });
            unsigned min_shift = required_history;
            unsigned max_keep = curr_shift - min_shift;

            // For extracts, identify the earliest start byte for each end byte
            std::map<int, int> end_to_earliest_start;
            for (const auto *stmt : pred->statements) {
                if (const auto *extract = stmt->to<IR::BFN::Extract>()) {
                    if (const auto *rval = extract->source->to<IR::BFN::InputBufferRVal>()) {
                        auto range = rval->range;
                        auto hi_byte = range.hiByte();
                        auto lo_byte = range.loByte();
                        if (end_to_earliest_start.count(hi_byte))
                            end_to_earliest_start[hi_byte] =
                                std::min(lo_byte, end_to_earliest_start[hi_byte]);
                        else
                            end_to_earliest_start[hi_byte] = lo_byte;
                    }
                }
            }

            // If we don't do any extracts, or if the extracts end before the max_keep value,
            // then just return the requested min_shift value.
            if (end_to_earliest_start.size() == 0 ||
                end_to_earliest_start.rbegin()->first < static_cast<int>(max_keep)) {
                state_to_shift[pred->name] = min_shift;
                return min_shift;
            }

            // Create a vector of end position -> earliest start position where overlapping ranges
            // are merged. Unused byte positions map to themselves.
            //
            // For example, if the map created above is:
            //   [ (2, 1), (3, 2) ]
            // then the resulting vector is:
            //
            // [ 0, 1, 1, 1 ]
            int max_end = end_to_earliest_start.rbegin()->first;
            std::vector<int> end_to_earliest_start_merged(max_end + 1);
            for (int i = 0; i <= max_end; i++) {
                end_to_earliest_start_merged[i] = i;
                int min_idx = i;
                if (end_to_earliest_start.count(i))
                    min_idx =
                        std::min(end_to_earliest_start[i], end_to_earliest_start_merged[min_idx]);
                for (int j = std::max(0, min_idx); j <= i; j++)
                    end_to_earliest_start_merged[j] = min_idx;
            }

            // Identify the byte location at/before the max_keep position where we can split the
            // state
            if (end_to_earliest_start_merged[max_keep] > 0) {
                unsigned keep = static_cast<unsigned>(end_to_earliest_start_merged[max_keep]);
                unsigned delay_shift = curr_shift - keep;
                if (state_to_shift.count(pred->name))
                    state_to_shift[pred->name] = std::min(delay_shift, state_to_shift[pred->name]);
                else
                    state_to_shift[pred->name] = delay_shift;
                return delay_shift;
            }

            return 0;
        }
    };

    /// Adjustments for out of buffer negative extracts only.
    /// Shift amounts are adjusted and extrct statements are delayed until the child states.
    struct AdjustShiftOutOfBuffer : public ParserModifier {
        const CollectNegativeExtractOutOfBufferStates &collectNegative;

        explicit AdjustShiftOutOfBuffer(const CollectNegativeExtractOutOfBufferStates &cg)
            : collectNegative(cg) {}

        std::map<cstring, std::vector<const IR::BFN::Extract *>> delayed_statements;
        std::map<cstring, unsigned> state_delay;
        std::map<cstring, cstring> state_pred;

        unsigned max_buff_size = 0;

        profile_t init_apply(const IR::Node *node) override {
            auto rv = ParserModifier::init_apply(node);
            max_buff_size = Device::pardeSpec().byteInputBufferSize();
            delayed_statements.clear();
            state_delay.clear();
            state_pred.clear();
            return rv;
        }

        bool preorder(IR::BFN::ParserState *state) override {
            // Handle shift to be delayed from current state
            if (collectNegative.state_to_shift.count(state->name)) {
                // Shift from the current state to delay until child states
                unsigned delay_shift = collectNegative.state_to_shift.at(state->name);

                // Current shift amount for the state
                unsigned curr_shift = 0;
                std::for_each(state->transitions.begin(), state->transitions.end(),
                              [this, state, &curr_shift, delay_shift](const auto *tr) {
                                  curr_shift = curr_shift > tr->shift ? curr_shift : tr->shift;
                                  if (tr->next) {
                                      this->state_delay[tr->next->name] = delay_shift;
                                      this->state_pred[tr->next->name] = state->name;
                                  }
                              });

                // Shift to be added to the current state
                unsigned pending_shift =
                    state_delay.count(state->name) ? state_delay.at(state->name) : 0;

                // Split the statements into statements to delay to child states and statements to
                // keep in current state
                IR::Vector<IR::BFN::ParserPrimitive> new_statements;
                for (const auto *stmt : state->statements) {
                    bool keep = true;
                    if (const auto *extract = stmt->to<IR::BFN::Extract>()) {
                        if (const auto *rval = extract->source->to<IR::BFN::InputBufferRVal>()) {
                            if (rval->range.hiByte() >= static_cast<int>(max_buff_size)) {
                                auto *rval_clone = rval->clone();
                                rval_clone->range.hi -= (curr_shift - pending_shift) * 8;
                                rval_clone->range.lo -= (curr_shift - pending_shift) * 8;
                                auto *extract_clone = extract->clone();
                                extract_clone->source = rval_clone;
                                delayed_statements[state->name].push_back(extract_clone);
                                keep = false;
                            }
                        }
                    }
                    if (keep) new_statements.push_back(stmt);
                }
                state->statements = new_statements;
            }

            // Add statements delayed from parent state
            if (state_delay.count(state->name)) {
                cstring pred = state_pred[state->name];
                IR::Vector<IR::BFN::ParserPrimitive> new_statements;
                // Clone the delayed statements -- need a unique copy for each state
                // in case the statements are adjusted (e.g., made into CLOTs).
                for (const auto *stmt : delayed_statements[pred])
                    new_statements.push_back(stmt->clone());
                new_statements.insert(new_statements.end(), state->statements.begin(),
                                      state->statements.end());
                state->statements = new_statements;
            }

            return true;
        }

        bool preorder(IR::BFN::Transition *transition) override {
            auto state = findContext<IR::BFN::ParserState>();
            BUG_CHECK(state, "State cannot be null!");

            if (collectNegative.state_to_shift.count(state->name)) {
                transition->shift -= collectNegative.state_to_shift.at(state->name);
                LOG3("Adjusting transition from " << state->name << ", match { "
                                                  << transition->value
                                                  << " } to shift value = " << transition->shift);
            }
            if (state_delay.count(state->name)) {
                transition->shift += state_delay.at(state->name);
                LOG3("Adjusting transition from " << state->name << ", match { "
                                                  << transition->value
                                                  << " } to shift value = " << transition->shift);
            }

            return true;
        }

        bool preorder(IR::BFN::PacketRVal *rval) override {
            auto state = findContext<IR::BFN::ParserState>();
            auto extract = findContext<IR::BFN::Extract>();

            if (state_delay.count(state->name)) {
                unsigned shift = state_delay.at(state->name) * 8;
                rval->range.lo += shift;
                rval->range.hi += shift;
                if (extract) {
                    LOG3("Adjusting field " << extract->dest->field->toString() << " to "
                                            << shift / 8 << " byte offset (lo = " << rval->range.lo
                                            << ", hi  = " << rval->range.hi << ")");
                }
            }

            return false;
        }
    };

    /// Colect all negative extract states and compute corresponding shift values
    /// for transitions and states
    struct CollectNegativeExtractStates : public ParserInspector {
        const CollectParserInfo &parserInfo;

        /**
         * @brief In-buffer offsets of states
         */
        std::map<cstring, unsigned> state_to_shift;

        /**
         * @brief Output shift values for given transitions - key is the
         * source node and value is a map transition -> value
         */
        std::map<cstring, std::map<const IR::BFN::ParserMatchValue *, unsigned>> transition_shift;

        /**
         * @brief Transitions exiting parser with unconsumed bytes in the packet buffer.
         */
        std::map<const IR::BFN::Transition *, unsigned> remainder_before_exit;

        /**
         * @brief Duplicate the given node and set the shift value
         *
         * The state_to_duplicate variable is used to store the states that need to be
         * duplicated. The key is the transition that needs to be duplicated and the
         * value is a pair of the state to be duplicated and the shift value.
         *
         * For example, the following parser:
         *
         * A(shift = 36) ----> B(shift = 2)
         *   |                      ^
         *   |                      |
         *   |----> C(shift = 0) ---|
         *
         * is transformed to
         *
         * A(shift = 31) ----> B'(shift = 5)
         *   |
         *   |----> C(shift = 5) -----> B(shift = 2)
         *
         * We need to duplicate state B and set its shift value to 5. The transition
         * from A to B is the key and the value is a pair of B and 5.
         *
         * Before this fix, the parser graph is transformed into:
         *
         * A(shift = 31) ----> B(shift = 7)
         *   |                      ^
         *   |                      |
         *   |---> C(shift = 5) ----|
         *
         * This would be wrong, as the total shift amount on the packet that goes
         * though A -> C -> B is 31 + 5 + 7 = 43, which is 5 bytes more than the
         * the correct shift value 38.
         */
        std::map<const IR::BFN::Transition *, std::pair<const IR::BFN::ParserState *, int>>
            state_to_duplicate;

        explicit CollectNegativeExtractStates(const CollectParserInfo &pi) : parserInfo(pi) {}

        bool preorder(const IR::BFN::PacketRVal *rval) override {
            auto extract = findContext<IR::BFN::Extract>();
            if (extract && rval->range.lo < 0) {
                auto state = findContext<IR::BFN::ParserState>();
                unsigned shift = (-rval->range.lo + 7) / 8;
                // state_to_shift[state->name] = std::max(state_to_shift[state->name], shift);
                LOG1("State " << state->name << " requires " << shift << " B shift");
                historic_states[state] = std::max(historic_states[state], shift);
                parsers[state] = findContext<IR::BFN::Parser>();
            }

            return false;
        }

        profile_t init_apply(const IR::Node *node) override {
            auto rv = ParserInspector::init_apply(node);
            // Initialize all structures
            transition_shift.clear();
            state_to_shift.clear();
            historic_states.clear();
            state_to_duplicate.clear();
            parsers.clear();
            return rv;
        }

        void end_apply() override {
            // Required data capture update all node states
            const unsigned max_buff_size = Device::pardeSpec().byteInputBufferSize();
            for (auto kv : historic_states) {
                // 1] Distribute the required history value and adjust transitions
                auto state = kv.first;
                auto max_idx_value = kv.second;
                BUG_CHECK(max_idx_value <= max_buff_size,
                          "In parse state %s: a value that is %d B backwards from the current "
                          "parsing position is being accessed/used. It is only possible to "
                          "access %d B backwards from the current parsing position. As a "
                          "possible workaround try moving around the extracts (possibly "
                          "by using methods advance and lookahead or splitting some headers).",
                          state->name, max_idx_value, max_buff_size);

                distribute_shift_to_node(state, nullptr, parsers[state], max_idx_value);

                // 2] Add the fix amount of shift data (it should be the same value from nodes)
                //
                // It is not a problem if the SHIFT value will not cover the whole state because
                // the future pass will split the state to get more data to parse data.
                for (auto trans : state->transitions) {
                    LOG1("  state has transitions " << trans);
                    unsigned shift_value = trans->shift + max_idx_value;
                    transition_shift[state->name][trans->value] = shift_value;
                }
            }

            for (auto kv : state_to_shift)
                LOG3(kv.first << " needs " << kv.second << " bytes of shift");

            for (auto kv : transition_shift) {
                for (auto tr_config : kv.second) {
                    std::stringstream ss;
                    ss << "Transition with match { " << tr_config.first << " } from state "
                       << kv.first << " needs will be set with the shift value "
                       << tr_config.second;
                    LOG3(ss.str());
                }
            }

            LOG3("ResolveNegativeExtract has been finished.");
        }

     private:
        /**
         * @brief Set of nodes which were identified as nodes where historic
         * data need to be accessed. The value stored together with the state
         * is the maximal historic data index
         *
         */
        std::map<const IR::BFN::ParserState *, unsigned> historic_states;

        /**
         * @brief Mapping of Parser state to given Parser instance
         *
         */
        std::map<const IR::BFN::ParserState *, const IR::BFN::Parser *> parsers;

        /**
         * @brief Get the current value of the transition (assigned by the algorithm)
         *
         * @param tr Transition to process
         * @param src Transition source node
         * @return Transition shift value
         */
        unsigned get_transition_shift(const IR::BFN::ParserState *src,
                                      const IR::BFN::Transition *tr) {
            BUG_CHECK(tr != nullptr, "Transition node cannot be null!");
            if (transition_shift.count(src->name) &&
                transition_shift.at(src->name).count(tr->value)) {
                return transition_shift[src->name][tr->value];
            }

            return tr->shift;
        }

        /**
         * @brief Adjust all transitions/state shifts for the given state
         *
         * Do the following for each state:
         * 1] Set the computed transition shift which is affected by the historic data path
         *   (value needs to be same for all transitions due to the state spliting)
         * 2] Set the corresponding state shift value for the successor node
         * 3] Correct all transitions from the successors by the state shift value
         *   (we can skip the node on the historic path because it is already analyzed)
         *
         * @param state Parser state used for the analysis
         * @param state_child Child of the @p state
         * @param tr_shift Transition shift to set
         * @param state_shift State shift to set
         */
        void adjust_shift_buffer(const IR::BFN::ParserState *state,
                                 const IR::BFN::ParserState *state_child,
                                 const IR::BFN::Parser *parser, unsigned tr_shift,
                                 unsigned state_shift) {
            auto graph = parserInfo.graph(parser);
            for (auto state_trans : state->transitions) {
                auto state_succ = state_trans->next;
                transition_shift[state->name][state_trans->value] = tr_shift;
                LOG4("Adding transition { " << state_trans->value << " } shift value " << tr_shift
                                            << " B from state " << state->name << " to state "
                                            << state_succ->name);

                if (!state_succ) {
                    // This transition exits parser, but we need to shift `state_shift` bytes
                    // from the packet. Remember this transition, AdjustShift will add
                    // auxiliary state which is used to extract the remaining bytes.
                    remainder_before_exit[state_trans] = state_shift;
                    continue;
                };

                if (graph.predecessors().at(state_succ).size() <= 1) {
                    state_to_shift[state_succ->name] = state_shift;
                    LOG4("Setting shift value " << state_shift << " B for state "
                                                << state_succ->name);
                }

                // Don't process the subtree if we reached from that part of the tree
                // because it will be analyzed later
                if (state_succ == state_child) {
                    LOG4("Skipping transition adjustment for " << state_succ->name
                                                               << " (will be set later).");
                    continue;
                }

                // if after adjusting successors we violate the invariant that
                // all shift values from the outgoing transitions of the successors
                // to the dominator should remain the same. We need to duplicate the successor
                // state, and adjust the shift values of the transitions to the duplicated state
                if (graph.predecessors().at(state_succ).size() > 1) {
                    state_to_duplicate.emplace(state_trans,
                                               std::make_pair(state_succ, state_shift));
                    continue;
                }

                for (auto succ_tr : state_succ->transitions) {
                    if (transition_shift[state_succ->name].count(succ_tr->value) > 0) continue;

                    unsigned new_shift = get_transition_shift(state_succ, succ_tr) + state_shift;
                    transition_shift[state_succ->name][succ_tr->value] = new_shift;
                    LOG4("Adding transition { "
                         << succ_tr->value << " } shift value " << new_shift << " B from state "
                         << state_succ->name << " to state "
                         << (succ_tr->next != nullptr ? succ_tr->next->name : "EXIT"));
                }
            }
        }

        /**
         * @brief Distribute the shift value (recursive) to parse graph predecessors
         *
         * @param state Analyzed parser state
         * @param succ Successors of the state from which we are calling (can be null)
         * @param parser Analyzed parser state
         * @param required_history Required backward history in bytes
         */
        void distribute_shift_to_node(const IR::BFN::ParserState *state,
                                      const IR::BFN::ParserState *succ,
                                      const IR::BFN::Parser *parser, int required_history) {
            // 1] Identify the deficit absorbed by the recursion or if we already analyzed
            // a path through the graph
            BUG_CHECK(state, "Parser state cannot be null!");
            if (state_to_shift.count(state->name) > 0) {
                error(
                    "Current path with historic data usage has an intersection with"
                    " a previously analyzed historic data path at node %1%!",
                    state->name);
            }

            int deficit = required_history;
            auto graph = parserInfo.graph(parser);
            auto transitions = graph.transitions(state, succ);

            if (succ != nullptr)
                LOG5("Transitions size from state " << state->name << " to state " << succ->name
                                                    << " is " << transitions.size());

            if (transitions.size() > 0 && required_history > 0) {
                // All transitions should have the same shift value - we will take the first
                // one
                unsigned shift_value = get_transition_shift(state, *transitions.begin());
                deficit = required_history - shift_value;
            }

            LOG4("Shift distribution for node " << state->name
                                                << ", to distribute = " << required_history
                                                << " (deficit = " << deficit << " B)");

            // 2] Call recursively to all predecessors to distribute the remaining history
            // shift - we need to make a call iff we can distribute the value to successors.
            //
            // In this stage, there should be one path only to the state with historic data
            if (deficit > 0) {
                auto preds = graph.predecessors().at(state);
                if (preds.size() > 1) {
                    error(
                        "Cannot resolve negative extract because of multiple paths to "
                        "the node %1%",
                        state->name);
                }
                distribute_shift_to_node(*preds.begin(), state, parser, deficit);
            }

            // Check if we reached the starting node - stop if true
            if (transitions.size() == 0 && !succ) {
                LOG4("Initial node " << state->name << " has been reached.");
                return;
            }

            // 3] The following code assumes that all transition from this state requires the
            // same transition shift value
            //
            // Initial values assumes that we need to borrow the whole transition AND
            // new transition shift is 0
            const int old_tr_shift = get_transition_shift(state, *transitions.begin());
            // Required history can be negative --> difference is the successors's shift value
            // Required history is positive is the curent historic value plus the shift value
            int new_state_shift = old_tr_shift + deficit;
            int new_tr_shift = 0;
            if (deficit <= 0) {
                // Deficit is negative --> historic data which are not needed inside the buffer
                // (we can shift them out)
                new_tr_shift = -deficit;
            }
            Log::TempIndent indent;
            LOG4("Adjusting shift for state "
                 << state->name << " and transition to state " << succ->name
                 << " (new transition shift = " << new_tr_shift << " B)" << indent);
            adjust_shift_buffer(state, succ, parser, new_tr_shift, new_state_shift);
        }
    };

    struct AdjustShift : public ParserModifier {
        const CollectNegativeExtractStates &collectNegative;

        explicit AdjustShift(const CollectNegativeExtractStates &cg) : collectNegative(cg) {}

        std::map<const IR::BFN::ParserState *, std::pair<IR::BFN::ParserState *, int>>
            duplicated_states;

        profile_t init_apply(const IR::Node *node) override {
            auto rv = ParserModifier::init_apply(node);
            duplicated_states.clear();
            return rv;
        }

        bool preorder(IR::BFN::Transition *transition) override {
            auto state = findContext<IR::BFN::ParserState>();
            auto orig_transition = getOriginal()->to<IR::BFN::Transition>();
            BUG_CHECK(state, "State cannot be null!");
            BUG_CHECK(orig_transition, "Original IR::BFN::Transition cannot be null!");

            if (collectNegative.transition_shift.count(state->name) &&
                collectNegative.transition_shift.at(state->name).count(orig_transition->value)) {
                const auto &tr_map = collectNegative.transition_shift.at(state->name);
                transition->shift = tr_map.at(orig_transition->value);
                LOG3("Adjusting transition from " << state->name << ", match { "
                                                  << orig_transition->value
                                                  << " } to shift value = " << transition->shift);
            }

            if (collectNegative.remainder_before_exit.count(orig_transition)) {
                // The transition exits parser but needs to push a shift to the target
                // state (which is empty in this case). We generate new auxiliary state
                // for this purpose.
                unsigned state_shift = collectNegative.remainder_before_exit.at(orig_transition);

                auto remainder_state = new IR::BFN::ParserState(
                    state->p4States, state->name + "$final_shift", state->gress);
                transition->next = remainder_state;
                auto end_transition = new IR::BFN::Transition(match_t(), state_shift);
                remainder_state->transitions.push_back(end_transition);
                LOG5("Transition from state "
                     << state->name << " with match value " << orig_transition->value
                     << " leads to exit, adding new state " << remainder_state->name
                     << " to consume " << state_shift << " bytes.");
            }

            if (collectNegative.state_to_duplicate.count(orig_transition)) {
                auto [orig_state, state_shift] =
                    collectNegative.state_to_duplicate.at(orig_transition);
                LOG5("Duplicating transition from state " << state->name << " with match value "
                                                          << orig_transition->value << " to state "
                                                          << orig_state->name);

                IR::BFN::ParserState *duplicated_state = nullptr;
                if (duplicated_states.count(orig_state)) {
                    int prev_state_shift;
                    std::tie(duplicated_state, prev_state_shift) = duplicated_states[orig_state];
                    BUG_CHECK(state_shift == prev_state_shift,
                              "New state shift %1% does not match previous "
                              "value %2% for duplicated state %3%",
                              state_shift, prev_state_shift, state->name);
                } else {
                    duplicated_state = new IR::BFN::ParserState(
                        orig_state->p4States, orig_state->name + "$dup", orig_state->gress);
                    for (auto tr : orig_state->transitions) {
                        auto new_trans = new IR::BFN::Transition(tr->srcInfo, tr->value,
                                                                 tr->shift + state_shift, tr->next);
                        duplicated_state->transitions.push_back(new_trans);
                    }
                    for (const auto stmt : orig_state->statements) {
                        duplicated_state->statements.push_back(stmt->clone());
                    }
                    duplicated_states.emplace(orig_state,
                                              std::make_pair(duplicated_state, state_shift));
                }

                transition->next = duplicated_state;
            }

            return true;
        }

        bool preorder(IR::BFN::PacketRVal *rval) override {
            auto state = findContext<IR::BFN::ParserState>();
            auto extract = findContext<IR::BFN::Extract>();

            if (collectNegative.state_to_shift.count(state->name)) {
                unsigned shift = collectNegative.state_to_shift.at(state->name) * 8;
                rval->range.lo += shift;
                rval->range.hi += shift;
                if (extract) {
                    LOG3("Adjusting field " << extract->dest->field->toString() << " to "
                                            << shift / 8 << " byte offset (lo = " << rval->range.lo
                                            << ", hi  = " << rval->range.hi << ")");
                }
            }

            return false;
        }
    };

    ResolveNegativeExtract() {
        auto *parserInfo = new CollectParserInfo;
        auto *collectNegative = new CollectNegativeExtractStates(*parserInfo);
        auto *collectNegativeOutOfBuffer = new CollectNegativeExtractOutOfBufferStates(*parserInfo);

        addPasses({LOGGING(4) ? new DumpParser("before_resolve_negative_extract") : nullptr,
                   // Step 1: Handle negative extracts that exceed the parser buffer size.
                   // Need to adjust shift and delay extracts until subsequent states.
                   parserInfo, collectNegativeOutOfBuffer,
                   new AdjustShiftOutOfBuffer(*collectNegativeOutOfBuffer),
                   // Step 2: Handle all other negative extracts
                   // Adjusting shift amounts but not delaying extracts.
                   parserInfo, collectNegative, new AdjustShift(*collectNegative),
                   LOGGING(4) ? new DumpParser("after_resolve_negative_extract") : nullptr});
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_RESOLVE_NEGATIVE_EXTRACT_H_ */
