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

#ifndef EXTENSIONS_BF_P4C_PARDE_PARSER_INFO_H_
#define EXTENSIONS_BF_P4C_PARDE_PARSER_INFO_H_

#include <numeric>
#include <optional>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/graphviz.hpp>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/ir/control_flow_visitor.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/lib/assoc.h"

namespace boost {
    enum vertex_state_t { vertex_state };
    BOOST_INSTALL_PROPERTY(vertex, state);

    enum edge_transition_t { edge_transition };
    BOOST_INSTALL_PROPERTY(edge, transition);
}

/** @addtogroup parde
 *  @{
 */

/**
 * This data structure uses different template parameters of boost::adjacency_list than the ones
 * used by the ParserGraphImpl class in order to be able to build a graph who's edges can be
 * reversed using the boost::make_reverse_graph function. This is particularly useful to determine
 * post dominators using the boost::lengauer_tarjan_dominator_tree function. It also uses custom
 * properties to bind ParserStates and Transitions directly to vertices and edges respectively.
 */
class ReversibleParserGraph {
 public:
    typedef boost::adjacency_list<
        boost::vecS,
        boost::vecS,
        boost::bidirectionalS,
        boost::property<boost::vertex_state_t, const IR::BFN::ParserState*>,
        boost::property<boost::edge_transition_t, const IR::BFN::Transition*>
    > Graph;

    Graph graph;
    const IR::BFN::Parser* parser = nullptr;
    std::optional<gress_t> gress;

    typename Graph::vertex_descriptor get_entry_point() {
        if (entry_point) return *entry_point;

        BUG_CHECK(parser, "Cannot get entry point, as provided parser is a nullptr.");
        if (state_to_vertex.count(parser->start)) return state_to_vertex.at(parser->start);

        BUG("Cannot get entry point of parser.");
    }

 private:
    std::optional<typename Graph::vertex_descriptor> entry_point;

 public:
    std::optional<typename Graph::vertex_descriptor> end;

    ordered_map<const IR::BFN::ParserState*, typename Graph::vertex_descriptor> state_to_vertex;
    ordered_map<typename Graph::vertex_descriptor, const IR::BFN::ParserState*> vertex_to_state;

    bool empty() const {
        auto all_vertices = boost::vertices(graph);
        if (++all_vertices.first == all_vertices.second) {
            return true;
        }
        return false;
    }

    bool contains(const IR::BFN::ParserState* state) {
        return state_to_vertex.count(state);
    }

    typename Graph::vertex_descriptor add_vertex(const IR::BFN::ParserState* state) {
        if (state != nullptr && !gress)
            gress = state->gress;

        if (contains(state)) {
            LOG1("State " << ((state) ? state->name : "END") << " already exists");
            return state_to_vertex.at(state);
        }

        auto vertex = boost::add_vertex(state, graph);
        if (state)
            LOG1("Added vertex " << vertex << " for state " << state->name);
        else
            LOG1("Added vertex " << vertex << " for state END");

        if (state && state->name.find("$entry_point"))
            entry_point = vertex;
        if (state == nullptr)
            end = vertex;

        state_to_vertex[state] = vertex;
        vertex_to_state[vertex] = state;

        return vertex;
    }

    std::pair<typename Graph::edge_descriptor, bool> add_edge(
            const IR::BFN::ParserState* source,
            const IR::BFN::ParserState* destination,
            const IR::BFN::Transition* transition) {
        typename Graph::vertex_descriptor source_vertex, destination_vertex;
        source_vertex = add_vertex(source);
        destination_vertex = add_vertex(destination);

        // Look for a pre-existing edge.
        typename Graph::out_edge_iterator out, end;
        for (boost::tie(out, end) = boost::out_edges(source_vertex, graph); out != end; ++out) {
            if (boost::target(*out, graph) == destination_vertex)
                return {*out, false};
        }
        // No pre-existing edge, so make one.
        auto edge = boost::add_edge(source_vertex, destination_vertex, graph);
        if (!edge.second)
            // A vector-based adjacency_list (i.e. Graph) is a multigraph.
            // Inserting edges should always create new edges.
            BUG("Boost Graph Library failed to add edge.");
        boost::get(boost::edge_transition, graph)[edge.first] = transition;
        return {edge.first, true};
    }

    ReversibleParserGraph() {}

    ReversibleParserGraph(const ReversibleParserGraph& other) {
        boost::copy_graph(other.graph, graph);
    }
};

class DirectedGraph {
    typedef boost::adjacency_list<boost::listS,
                                  boost::vecS,
                                  boost::directedS> Graph;

 public:
    DirectedGraph() {}

    DirectedGraph(const DirectedGraph& other) {
        boost::copy_graph(other._graph, _graph);
    }

    ~DirectedGraph() {}

    int add_vertex() {
        int id = boost::add_vertex(_graph);
        return id;
    }

    void add_edge(int src, int dst) {
        boost::add_edge(src, dst, _graph);
    }

    std::vector<int> topological_sort() const {
        std::vector<int> result;
        boost::topological_sort(_graph, std::back_inserter(result));
        std::reverse(result.begin(), result.end());
        return result;
    }

    void print_stats() const {
        std::cout << "num vertices: " << num_vertices(_graph) << std::endl;
        std::cout << "num edges: " << num_edges(_graph) << std::endl;
        boost::write_graphviz(std::cout, _graph);
    }

 private:
    Graph _graph;
};

template <class State>
struct ParserStateMap : public ordered_map<const State*, ordered_set<const State*>> { };

template <class Parser, class State, class Transition>
class ParserGraphImpl : public DirectedGraph {
 public:
    template <class P, class S, class T>
    friend class CollectParserInfoImpl;

    explicit ParserGraphImpl(const Parser* parser) : root(parser->start) {}

    const State* const root;

    const ordered_set<const State*>& states() const { return _states; }

    const ParserStateMap<State>& successors() const { return _succs; }

    const ParserStateMap<State>& predecessors() const { return _preds; }

    const ordered_map<const State*,
                      ordered_set<const Transition*>>& to_pipe() const { return _to_pipe; }

    ordered_set<const Transition*>
    transitions(const State* src, const State* dst) const {
        auto it = _transitions.find({src, dst});
        if (it != _transitions.end())
            return it->second;

        return {};
    }

    ordered_set<const Transition*> transitions_to(const State* dst) const {
        ordered_set<const Transition*> transitions;
        auto has_dst = [dst](const auto& kv) { return kv.first.second == dst; };
        auto it = std::find_if(_transitions.begin(), _transitions.end(), has_dst);
        while (it != _transitions.end()) {
            ordered_set<const Transition*> value = it->second;
            transitions.insert(value.begin(), value.end());
            it = std::find_if(++it, _transitions.end(), has_dst);
        }
        return transitions;
    }

    ordered_set<const Transition*> to_pipe(const State* src) const {
        if (_to_pipe.count(src))
            return _to_pipe.at(src);

        return {};
    }

    ordered_map<std::pair<const State*, cstring>, ordered_set<const Transition*>>
    loopbacks() const {
        return _loopbacks;
    }

    bool is_loopback_state(cstring state) const {
        for (auto& kv : _loopbacks) {
            if (stripThreadPrefix(state) == stripThreadPrefix(kv.first.second))
                return true;
        }

        return false;
    }

    const State* get_state(cstring name) const {
        for (auto s : states()) {
            if (name == s->name)
                return s;
        }

        return nullptr;
    }

 private:
    /// Memoization table.
    mutable assoc::map<const State*, assoc::map<const State*, bool>> is_ancestor_;

 public:
    /// Is "src" an ancestor of "dst"?
    bool is_ancestor(const State* src, const State* dst) const {
        if (src == dst)
            return false;

        if (!predecessors().count(dst))
            return false;

        if (predecessors().at(dst).count(src))
            return true;

        if (is_ancestor_.count(src) && is_ancestor_.at(src).count(dst))
            return is_ancestor_.at(src).at(dst);

        /// DANGER -- this assumes parser graph is a unrolled DAG
        for (auto p : predecessors().at(dst))
            if (is_ancestor(src, p)) {
                is_ancestor_[dst][src] = false;
                return is_ancestor_[src][dst] = true;
            }

        return is_ancestor_[src][dst] = false;
    }

    bool is_descendant(const State* src, const State* dst) const {
        return is_ancestor(dst, src);
    }

    bool is_loop_reachable(const State* src, const State* dst) const {
        for (auto &kv : _loopbacks) {
            if (kv.first.first == src) {
                auto loop_state = get_state(kv.first.second);
                if (loop_state == dst || is_ancestor(loop_state, dst))
                    return true;
            }
        }

        return false;
    }


    /// Determines whether @p dst can be reached from @p src.
    /// It can be reached directly (without taking any loops,
    /// essentially is_ancestor) or via loops.
    /// This function is essentially a recursive DFS.
    ///
    /// First the function checks if @p dst is reachable from @p src
    /// directly. If it is not it tries to take any loop available.
    /// To take the loop we need to be able to (directly) reach the state that
    /// the loop loops from. Also the state that the loop takes us to should
    /// be some new state that we could not otherwise reach (without taking the loop).
    /// If we could reach the state directly then taking the loop does not make sense.
    /// Also it might create infinite recursion (see example below).
    /// Once we have found some new state that can be reached via a loop we just check
    /// (recursively) reachability from this new state (once again we can take
    /// another loop).
    ///
    /// Example why we need to do this recursively and why we need to check
    /// if the loop takes us somewhere new follows.
    /// Let's assume the following part of a parse graph:
    ///   ┌───────────────┐
    ///   │               │
    ///   ▼               │
    /// ┌───┐   ┌───┐   ┌───┐   ┌───┐
    /// │ A │──▶│ B │──▶│ C │──▶│ D │
    /// └───┘   └───┘   └───┘   └───┘
    ///           ▲               │
    ///           │               │
    ///           └───────────────┘
    ///
    /// Recursion example:
    /// Let's try is_reachable(D,A). It is not directly reachable, but we can take
    /// the D to B loopback (recurse into is_reacheable(B,A)). Still we cannot directly
    /// reach A from B, but we can do so by taking another loopback, this time C to A.
    ///
    /// Infinite recursion example:
    /// Let's try is_reacheable(C,E). Let's assume that E is some state that is outside
    /// of the subparser shown and E is not directly reachable from any of the shown
    /// states (it is for example reachable only via loop from A).
    /// We could take the loopback from C to A (recurse into is_reacheable(A,E)).
    /// Now we could take the loopback from D to B, even though it does not take
    /// us anywhere new (recurse into is_reacheable(B,E)), where we would take
    /// the loopback from C to A again (recurse into is_reacheable(A,E) again).
    /// Now we would get stuck between A->D->B->C->A. This is one of the reasons
    /// why we never take a loop that does not take us somewhere new. Since
    /// we DO check if loop takes us somewhere new we would nrecursionloop does not happen).
    bool is_reachable(const State* src, const State* dst) const {
        // One way is that src is an ancestor of dst
        if (is_ancestor(src, dst))
            return true;
        // Otherwise we must check all possible loopbacks
        for (auto &kv : _loopbacks) {
            auto loop_from = kv.first.first;
            auto loop_to = get_state(kv.first.second);
            // Can we get to this loopback from source (without any other loopbacks)?
            if (src == loop_from || is_ancestor(src, loop_from)) {
                // If the loopback goes to dst we are done
                if (loop_to == dst)
                    return true;
                // Loopback needs to get us somewhere new
                // (where we couldn't directly reach from src)
                // And we need to be able to reach the dst from this new state
                // (possibly via more loops)
                if (src != loop_to && !is_ancestor(src, loop_to) &&
                    is_reachable(loop_to, dst))
                    return true;
            }
        }
        // We didn't find anything => state dst is not reacheable from src
        return false;
    }

    /// Determines whether @p s is dominated by the set of states @p set.
    /// This means that every path from start to @p s leads through (at least) one of
    /// the states from @p set.
    bool is_dominated_by_set(const State* s,
                             const ordered_set<const State*>& set) {
        // If there are no predecessors domination is false
        if (!predecessors().count(s) || predecessors().at(s).empty())
            return false;
        // Otherwise we need every predecessor to be from the set or recursively
        // dominated by the set
        for (auto ns : predecessors().at(s)) {
            if (!set.count(ns) && !is_dominated_by_set(ns, set)) {
                return false;
            }
        }
        return true;
    }

    /// Determines whether @p s is postdominated by the set of states @p set.
    /// This means that every path from @p s to exit leads through (at least) one of
    /// the states from @p set.
    bool is_postdominated_by_set(const State* s,
                                 const ordered_set<const State*>& set) {
        // If there are no successors postdomination is false
        if (!successors().count(s) || successors().at(s).empty())
            return false;
        // Otherwise we need every successor to be from the set or recursively
        // postdominated by the set
        for (auto ns : successors().at(s)) {
            if (!set.count(ns) && !is_postdominated_by_set(ns, set)) {
                return false;
            }
        }
        return true;
    }

    /// Determines whether @p s is postdominated by the set of states @p set within any
    /// loopback and also the loop exit state of that loopback is dominated by the set
    /// of states @p set (or alternatively the loopback is reflexive on a state from set).
    /// This means that every path from start of the loop to loop exit
    /// leads through (at least) one of the states from @p set and also @p s is part of this
    /// loop and also every path from the start (of parsing) to loop exit leads through
    /// one of the states from @p set.
    /// More specifically this means that there is a loopback for which we are certain the
    /// following is always satisfied:
    ///     * To even reach/take this loopback we had to go through one of the states from the set
    ///       = loop exit is dominated by the set
    ///     * After taking the loopback we will go through at least one the states again
    ///       = loop start is postdominated by the set within the loop boundaries
    /// If the set of states is for example a set of all states extracting a given field
    /// this means that taking this loopback under any circumstances forces a reassignment of
    /// the given field.
    /// Loop start in this case is the first state within the loopback (the state the loopback
    /// transition goes to) and loop exit is the last state in the loopback (state that the
    /// loopback transition goes from).
    std::pair<const State*, const State*> is_loopback_reassignment(
            const State* s,
            const ordered_set<const State*>& set) {
        for (auto &kv : _loopbacks) {
            auto le = kv.first.first;
            auto ls = get_state(kv.first.second);
            // Is this state even within this loopback?
            if (s == le || s == ls ||
                (is_ancestor(ls, s) && is_ancestor(s, le))) {
                // If it is check this particular loopback
                // Check if the loopback has postdomination happening inside
                // And also that loop exit is dominated (or the loopback is reflexive)
                if (_is_loopback_postdominated_by_set_impl(ls, le, set) &&
                    ((le == ls && set.count(le)) || is_dominated_by_set(le, set))) {
                    return std::make_pair(ls, le);
                }
            }
        }

        return std::make_pair(nullptr, nullptr);
    }

    /// Determines whether @p src and @p dst are mutually exclusive states on all paths through
    /// the parser graph.
    bool is_mutex(const State* a, const State* b) const {
        return a != b &&
               !is_ancestor(a, b) &&
               !is_ancestor(b, a) &&
               !is_loop_reachable(a, b) &&
               !is_loop_reachable(b, a);
    }

    /// Determines whether the states in the given set are all mutually exclusive on all paths
    /// through the parser graph.
    bool is_mutex(const ordered_set<const State*>& states) const {
        for (auto it1 = states.begin(); it1 != states.end(); ++it1) {
            for (auto it2 = it1; it2 != states.end(); ++it2) {
                if (it1 == it2) continue;
                if (!is_mutex(*it1, *it2)) return false;
            }
        }

        return true;
    }

    bool is_mutex(const Transition* a, const Transition* b) const {
        if (a == b)
            return false;

        auto a_dst = get_dst(a);
        auto a_src = get_src(a);

        auto b_dst = get_dst(b);
        auto b_src = get_src(b);

        if (a_dst == b_src) return false;
        if (b_dst == a_src) return false;

        if (is_ancestor(a_dst, b_src)) return false;
        if (is_ancestor(b_dst, a_src)) return false;

        if (is_loop_reachable(a_dst, b_src)) return false;
        if (is_loop_reachable(b_dst, a_src)) return false;

        return true;
    }

    ordered_set<const State*>
    get_all_descendants(const State* src) const {
        ordered_set<const State*> rv;
        get_all_descendants_impl(src, rv);
        return rv;
    }

    std::vector<const State*> topological_sort() const {
        std::vector<int> result = DirectedGraph::topological_sort();
        std::vector<const State*> mapped_result;
        for (auto id : result)
            mapped_result.push_back(get_state(id));
        return mapped_result;
    }

    // longest path (in states) from src to end of parser
    std::vector<const State*> longest_path_states(const State* src) const {
        assoc::map<const State*, std::pair<unsigned, std::vector<const State*>>> path_map;
        return min_max_path_impl(src, path_map, true, true, true).second;
    }

    // longest path (in states) from start of parser to dest
    std::vector<const State*>
    longest_path_states_to(const State* dest) const {
        assoc::map<const State*, std::pair<unsigned, std::vector<const State*>>> path_map;
        return min_max_path_impl(dest, path_map, true, false, true).second;
    }

    // shortest path from src to end of parser
    std::vector<const State*> shortest_path_states(const State* src) const {
        assoc::map<const State*, std::pair<unsigned, std::vector<const State*>>> path_map;
        return min_max_path_impl(src, path_map, false, true, true).second;
    }

    // longest path (in bytes) from src to end of parser
    std::pair<unsigned, std::vector<const State*>> longest_path_bytes(const State* src) const {
        assoc::map<const State*, std::pair<unsigned, std::vector<const State*>>> path_map;
        return min_max_path_impl(src, path_map, true, true, false);
    }

    // shortest path (in bytes) from src to end of parser
    std::pair<unsigned, std::vector<const State*>> shortest_path_bytes(const State* src) const {
        assoc::map<const State*, std::pair<unsigned, std::vector<const State*>>> path_map;
        return min_max_path_impl(src, path_map, false, true, false);
    }

    // shortest path (in bytes) from src to end of parser
    std::pair<unsigned, std::vector<const State*>> shortest_path_thru_bytes(
            const State* src) const {
        assoc::map<const State*, std::pair<unsigned, std::vector<const State*>>> path_map_from;
        assoc::map<const State*, std::pair<unsigned, std::vector<const State*>>> path_map_to;

        auto from = min_max_path_impl(src, path_map_from, false, true, false);
        auto to = min_max_path_impl(src, path_map_to, false, false, false);

        std::vector<const State*> ret = to.second;
        ret.insert(ret.end(), ++from.second.begin(), from.second.end());

        return std::make_pair(from.first + to.first, ret);
    }

    const State* get_src(const Transition* t) const {
        for (auto& kv : _transitions) {
            if (kv.second.count(t))
                return kv.first.first;
        }

        for (auto& kv : _to_pipe) {
            if (kv.second.count(t))
                return kv.first;
        }

        for (auto& kv : _loopbacks) {
            if (kv.second.count(t))
                return kv.first.first;
        }

        return nullptr;
    }

    const State* get_dst(const Transition* t) const {
        for (auto& kv : _transitions) {
            if (kv.second.count(t))
                return kv.first.second;
        }
        return nullptr;
    }

 private:
    /// Determines whether a loopback given by a state in the loopback @p s
    /// and @p loop_exit is postdominated by the set of states @p set within itself.
    /// This means that every path from @p s to @p loop_exit
    /// (or any parser exit state) leads through (at least) one of the states from @p set.
    bool _is_loopback_postdominated_by_set_impl(const State* s,
                                                const State* loop_exit,
                                                const ordered_set<const State*>& set) {
        // Check if we are at the loop_exit
        if (s == loop_exit) {
            if (set.count(loop_exit)) {
                return true;
            } else {
                return false;
            }
        }
        // If there are no successors postdomination is false
        if (!successors().count(s) || successors().at(s).empty())
            return false;
        // Otherwise we need every successor to be from the set or recursively
        // postdominated by the set
        for (auto ns : successors().at(s)) {
            if (!set.count(ns) && !_is_loopback_postdominated_by_set_impl(ns, loop_exit, set)) {
                return false;
            }
        }
        return true;
    }

    std::vector<const State*> longest_or_shortest_path_states_impl(const State* src,
            assoc::map<const State*, std::vector<const State*>>& path_map, bool longest) const {
        if (path_map.count(src))
            return path_map.at(src);

        const State* best_succ = nullptr;
        std::vector<const State*> best_succ_path;

        if ((longest || to_pipe(src).size() == 0) && successors().count(src)) {
            for (auto succ : successors().at(src)) {
                auto succ_path = longest_or_shortest_path_states_impl(succ, path_map, longest);

                bool gt = succ_path.size() > best_succ_path.size();
                bool lt = succ_path.size() < best_succ_path.size();
                if (!best_succ || (longest ? gt : lt)) {
                    best_succ_path = succ_path;
                    best_succ = succ;
                }
            }
        }

        best_succ_path.insert(best_succ_path.begin(), src);

        path_map[src] = best_succ_path;

        return best_succ_path;
    }

    /**
     * Identify the longest or shortest path from or to a node
     *
     * @param state State to start/end at
     * @param path_map Map of State* to longest/shortest path
     * @param longest Find the longest path, otherwise find the shortest
     * @param origin Is state the start (true) or end (false) of the path
     * @param states Count path length by states (true) or bytes (false)
     */
    std::pair<unsigned, std::vector<const State*>> min_max_path_impl(
            const State* state,
            assoc::map<const State*, std::pair<unsigned, std::vector<const State*>>>& path_map,
            bool longest, bool origin, bool states) const {
        if (path_map.count(state))
            return path_map.at(state);

        const State* best = nullptr;
        std::vector<const State*> best_path;
        unsigned best_len = 0;

        auto max = [](unsigned v, const Transition* t) { return v > t->shift ? v : t->shift; };
        auto min = [](unsigned v, const Transition* t) { return v < t->shift ? v : t->shift; };

        auto next_map = origin ? successors() : predecessors();
        auto exit_trans = origin ? to_pipe(state) : ordered_set<const Transition*>();

        if ((longest || exit_trans.size() == 0) && next_map.count(state)) {
            for (auto next : next_map.at(state)) {
                auto next_path = min_max_path_impl(next, path_map, longest, origin, states);
                auto next_trans = origin ? transitions(state, next) : transitions(next, state);
                unsigned next_inc =
                    states ? 1
                           : std::accumulate(next_trans.begin(), next_trans.end(),
                                             longest ? 0u : SIZE_MAX, longest ? max : min);

                unsigned path_len = next_inc + next_path.first;
                bool better = longest ? path_len > best_len : path_len < best_len;
                if (!best || better) {
                    best_path = next_path.second;
                    best = next;
                    best_len = path_len;
                }
            }
        } else if (exit_trans.size()) {
            best_len = states ? 1
                              : std::accumulate(exit_trans.begin(), exit_trans.end(),
                                                longest ? 0u : SIZE_MAX, longest ? max : min);
        }

        best_path.insert(origin ? best_path.begin() : best_path.end(), state);

        path_map[state] = std::make_pair(best_len, best_path);

        return path_map[state];
    }

    void get_all_descendants_impl(const State* src,
                                  ordered_set<const State*>& rv) const {
        if (!successors().count(src))
            return;

        for (auto succ : successors().at(src)) {
            if (!rv.count(succ)) {
                rv.insert(succ);
                get_all_descendants_impl(succ, rv);
            }
        }
    }

    void add_state(const State* s) {
        _states.insert(s);
    }

    void add_transition(const State* state, const Transition* t) {
        add_state(state);

        if (t->next) {
            add_state(t->next);
            _succs[state].insert(t->next);
            _preds[t->next].insert(state);
            _transitions[{state, t->next}].insert(t);
        } else if (t->loop) {
            _loopbacks[{state, t->loop}].insert(t);
        } else {
            _to_pipe[state].insert(t);
        }
    }

    void map_to_boost_graph() {
        for (auto s : _states) {
            int id = DirectedGraph::add_vertex();
            _state_to_id[s] = id;
            _id_to_state[id] = s;
        }

        for (auto t : _succs)
            for (auto dst : t.second)
                DirectedGraph::add_edge(get_id(t.first), get_id(dst));
    }

    int get_id(const State* s) {
        if (_state_to_id.count(s) == 0)
            add_state(s);
        return _state_to_id.at(s);
    }

    const State* get_state(int id) const {
        return _id_to_state.at(id);
    }

    ordered_set<const State*> _states;

    ParserStateMap<State> _succs, _preds;

    ordered_map<std::pair<const State*, const State*>,
                ordered_set<const Transition*>> _transitions;

    // the iteration order is not actually needed to be fixed except for
    // dump_parser
    ordered_map<std::pair<const State*, cstring>,
                ordered_set<const Transition*>> _loopbacks;

    // the iteration order is not actually needed to be fixed except for
    // dump_parser
    ordered_map<const State*,
                ordered_set<const Transition*>> _to_pipe;

    assoc::map<const State*, int> _state_to_id;
    assoc::map<int, const State*> _id_to_state;
};

namespace P4 {
namespace IR {
namespace BFN {

using ParserGraph = ParserGraphImpl<IR::BFN::Parser,
                                    IR::BFN::ParserState,
                                    IR::BFN::Transition>;

using LoweredParserGraph = ParserGraphImpl<IR::BFN::LoweredParser,
                                           IR::BFN::LoweredParserState,
                                           IR::BFN::LoweredParserMatch>;
}  // namespace BFN
}  // namespace IR
}  // namespace P4

template <class Parser, class State, class Transition>
class CollectParserInfoImpl : public PardeInspector {
    using GraphType = ParserGraphImpl<Parser, State, Transition>;

 public:
    const ordered_map<const Parser*, GraphType*>& graphs() const { return _graphs; }
    const GraphType& graph(const Parser* p) const { return *(_graphs.at(p)); }
    const GraphType& graph(const State* s) const { return graph(parser(s)); }

    const Parser* parser(const State* state) const {
        return _state_to_parser.at(state);
    }

 private:
    Visitor::profile_t init_apply(const IR::Node* root) override {
        auto rv = Inspector::init_apply(root);

        clear_cache();
        _graphs.clear();
        _state_to_parser.clear();

        return rv;
    }

    bool preorder(const Parser* parser) override {
        _graphs[parser] = new GraphType(parser);
        revisit_visited();
        return true;
    }

    bool preorder(const State* state) override {
        auto parser = findContext<Parser>();
        _state_to_parser[state] = parser;

        auto g = _graphs.at(parser);

        g->add_state(state);

        for (auto t : state->transitions)
            g->add_transition(state, t);

        return true;
    }

    /// Clears internal memoization state.
    void clear_cache() {
        all_shift_amounts_.clear();
    }

    // Memoization table. Only contains results for forward paths in the graph.
    mutable assoc::map<const State*,
                     assoc::map<const State*, const std::set<int>*>> all_shift_amounts_;

 public:
    /// @return all possible shift amounts, in bits, for all paths from the start state to @p dst .
    /// If @p dst is the start state, then a singleton 0 is returned.
    ///
    /// DANGER: This method assumes the parser graph is a DAG.
    const std::set<int>* get_all_shift_amounts(const State* dst) const {
        return get_all_shift_amounts(graph(dst).root, dst);
    }

    /// @return the minimum possible shift amount, in bits, of all paths from the start state to
    ///    @p dst.
    ///
    /// @pre DANGER: This method assumes the parser graph is a DAG.
    int get_min_shift_amount(const State* dst) const {
        return *get_all_shift_amounts(dst)->begin();
    }

    /// @return the maximum possible shift amount, in bits, of all paths from the start state to
    ///    @p dst.
    ///
    /// @pre DANGER: This method assumes the parser graph is a DAG.
    int get_max_shift_amount(const State* dst) const {
        return *get_all_shift_amounts(dst)->rbegin();
    }

    /// @return all possible shift amounts, in bits, for all paths from @p src to @p dst. If
    ///   the two states are the same, then a singleton 0 is returned. If the states are mutually
    ///   exclusive, an empty set is returned. If @p src is an ancestor of @p dst, then the
    ///   shift amounts will be positive; otherwise, if @p src is a descendant of @p dst, then
    ///   the shift amounts will be negative.
    ///
    /// @pre DANGER: This method assumes the parser graph is a DAG.
    const std::set<int>* get_all_shift_amounts(const State* src,
                                               const State* dst) const {
        bool reverse_path = graphs().at(parser(src))->is_ancestor(dst, src);
        if (reverse_path) std::swap(src, dst);

        auto result = get_all_forward_path_shift_amounts(src, dst);

        if (reverse_path) {
            // Need to negate result.
            auto negated = new std::set<int>();
            for (auto shift : *result) negated->insert(-shift);
            result = negated;
        }

        return result;
    }

 private:
    const std::set<int>* get_all_forward_path_shift_amounts(const State* src,
                                                            const State* dst) const {
        if (src == dst) return new std::set<int>({0});

        if (all_shift_amounts_.count(src) && all_shift_amounts_.at(src).count(dst))
            return all_shift_amounts_.at(src).at(dst);

        auto graph = graphs().at(parser(src));
        auto result = new std::set<int>();

        if (graph->is_mutex(src, dst)) {
            return all_shift_amounts_[src][dst] = all_shift_amounts_[dst][src] = result;
        }

        if (!graph->is_ancestor(src, dst)) {
            return all_shift_amounts_[src][dst] = result;
        }

        // Recurse with the successors of the source.
        BUG_CHECK(graph->successors().count(src),
                  "State %s has a descendant %s, but no successors", src->name, dst->name);
        for (auto succ : graph->successors().at(src)) {
            auto amounts = get_all_forward_path_shift_amounts(succ, dst);
            if (!amounts->size()) continue;

            auto transitions = graph->transitions(src, succ);
            BUG_CHECK(!transitions.empty(),
                      "Missing parser transition from %s to %s", src->name, succ->name);

            auto t = *(transitions.begin());

            for (auto amount : *amounts)
                result->insert(amount + t->shift * 8);
        }

        return all_shift_amounts_[src][dst] = result;
    }

    void end_apply() override {
        clear_cache();
        for (auto g : _graphs)
            g.second->map_to_boost_graph();
    }

    ordered_map<const Parser*, GraphType*> _graphs;
    assoc::map<const State*, const Parser*> _state_to_parser;
};

using CollectParserInfo = CollectParserInfoImpl<IR::BFN::Parser,
                                                IR::BFN::ParserState,
                                                IR::BFN::Transition>;

using CollectLoweredParserInfo = CollectParserInfoImpl<IR::BFN::LoweredParser,
                                                       IR::BFN::LoweredParserState,
                                                       IR::BFN::LoweredParserMatch>;

/** @} */  // end of group parde

#endif  /* EXTENSIONS_BF_P4C_PARDE_PARSER_INFO_H_ */
