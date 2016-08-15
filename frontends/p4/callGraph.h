/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _FRONTENDS_P4_CALLGRAPH_H_
#define _FRONTENDS_P4_CALLGRAPH_H_

#include <vector>
#include <unordered_set>
#include "lib/log.h"
#include "lib/map.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

namespace P4 {

template <class T>
class CallGraph {
 protected:
    cstring name;
    // Use an ordered map to make this deterministic
    ordered_map<T, std::vector<T>*> out_edges;  // map caller to list of callees
    ordered_map<T, std::vector<T>*> in_edges;
    ordered_set<T> nodes;    // all nodes

    void sort(T el, std::vector<T> &out, std::set<T> &done)  {
        if (done.find(el) != done.end()) {
            return;
        } else if (isCaller(el)) {
            for (auto c : *out_edges[el])
                sort(c, out, done);
        }
        LOG1("Order " << el);
        done.emplace(el);
        out.push_back(el);
    }

 public:
    typedef typename ordered_map<T, std::vector<T>*>::iterator iterator;

    explicit CallGraph(cstring name) : name(name) {}

    // Graph construction.

    // node that may call no-one
    void add(T caller) {
        if (nodes.find(caller) != nodes.end())
            return;
        LOG1(name << ": " << caller);
        out_edges[caller] = new std::vector<T>();
        in_edges[caller] = new std::vector<T>();
        nodes.emplace(caller);
    }
    void calls(T caller, T callee) {
        LOG1(name << ": " << callee << " is called by " << caller);
        add(caller);
        add(callee);
        out_edges[caller]->push_back(callee);
        in_edges[callee]->push_back(caller);
    }

    // Graph querying

    bool isCallee(T callee) const {
        auto callees =::get(in_edges, callee);
        return callees != nullptr && !callees->empty(); }
    bool isCaller(T caller) const {
        auto edges = ::get(out_edges, caller);
        if (edges == nullptr)
            return false;
        return !edges->empty();
    }
    // Topological sort of the graph; calles are before callers.
    // If the graph has cycles the graphs is still sorted, but the order
    // of nodes in strongly-connected components is unspecified.
    void sort(std::vector<T> &start, std::vector<T> &out) {
        std::set<T> done;
        for (auto s : start)
            sort(s, out, done);
    }
    void sort(std::vector<T> &out) {
        std::set<T> done;
        for (auto n : nodes)
            sort(n, out, done);
    }
    // Iterators over the out_edges
    iterator begin() { return out_edges.begin(); }
    iterator end()   { return out_edges.end(); }
    std::vector<T>* getCallees(T caller)
    { return out_edges[caller]; }
    // Callees are appended to 'toAppend'
    void getCallees(T caller, std::set<T> &toAppend) {
        if (isCaller(caller))
            toAppend.insert(out_edges[caller]->begin(), out_edges[caller]->end());
    }
    // out will contain all nodes reachable from start
    void reachable(T start, std::set<T> &out) const {
        std::set<T> work;
        work.emplace(start);
        while (!work.empty()) {
            T node = *work.begin();
            work.erase(node);
            if (out.find(node) != out.end())
                continue;
            out.emplace(node);
            auto edges = out_edges.find(node);
            if (edges == out_edges.end())
                continue;
            for (auto c : *(edges->second))
                work.emplace(c);
        }
    }

    typedef std::unordered_set<T> Set;

    // Compute for each node the set of dominators with the indicated start node.
    // Node d dominates node n if all paths from the start to n go through d
    // Result is deposited in 'dominators'.
    // 'dominators' should be empty when calling this function.
    void dominators(T start, std::map<T, Set> &dominators) {
        // initialize
        for (auto n : nodes) {
            if (n == start)
                dominators[n].emplace(start);
            else
                dominators[n].insert(nodes.begin(), nodes.end());
        }

        // There are faster but more complicated algorithms.
        bool changes = true;
        while (changes) {
            changes = false;
            for (auto node : nodes) {
                auto vec = in_edges[node];
                if (vec == nullptr)
                    continue;
                auto size = dominators[node].size();
                for (auto c : *vec)
                    insersectWith(dominators[node], dominators[c]);
                dominators[node].emplace(node);
                if (dominators[node].size() != size)
                    changes = true;
            }
        }
    }

    class Loop {
     public:
        T entry;
        std::set<T> body;
        // multiple back-edges could go to the same loop head
        std::set<T> back_edge_heads;

        explicit Loop(T entry) : entry(entry) {}
    };

    // All natural loops in a call-graph.
    struct Loops {
        std::vector<Loop*> loops;

        // Return loop index if 'node' is a loop entry point
        // else return -1
        int isLoopEntryPoint(T node) const {
            for (size_t i=0; i < loops.size(); i++) {
                auto loop = loops.at(i);
                if (loop->entry == node)
                    return i;
            }
            return -1;
        }
        bool isInLoop(int loopIndex, T node) const {
            if (loopIndex == -1)
                return false;
            auto loop = loops.at(loopIndex);
            return (loop->body.find(node) != loop->body.end());
        }
    };

    Loops* compute_loops(T start) {
        auto result = new Loops();
        std::map<T, Set> dom;
        dominators(start, dom);

        std::map<T, Loop*> entryToLoop;

        for (auto e : nodes) {
            auto next = out_edges[e];
            auto dome = dom[e];
            for (auto n : *next) {
                if (dome.find(n) != dome.end()) {
                    // n is a loop head
                    auto loop = get(entryToLoop, n);
                    if (loop == nullptr) {
                        loop = new Loop(n);
                        entryToLoop[n] = loop;
                        result->loops.push_back(loop);
                    }
                    loop->back_edge_heads.emplace(e);
                    // reverse DFS from e to n

                    std::set<T> work;
                    work.emplace(e);
                    while (!work.empty()) {
                        auto crt = *work.begin();
                        work.erase(crt);
                        loop->body.emplace(crt);
                        if (crt == n) continue;
                        for (auto i : *in_edges[crt])
                            if (loop->body.find(i) == loop->body.end())
                                work.emplace(i);
                    }
                }
            }
        }
        return result;
    }

 protected:
    // intersect in place
    static void insersectWith(Set &set, Set& with) {
        std::vector<T> toRemove;
        for (auto e : set)
            if (with.find(e) == with.end())
                toRemove.push_back(e);
        for (auto e : toRemove)
            set.erase(e);
    }

    // Helper for computing strongly-connected components
    // using Tarjan's algorithm.
    struct sccInfo {
        unsigned       crtIndex;
        std::vector<T> stack;
        std::set<T>    onStack;
        std::map<T, unsigned> index;
        std::map<T, unsigned> lowlink;

        sccInfo() : crtIndex(0) {}
        void push(T node) {
            stack.push_back(node);
            onStack.emplace(node);
        }
        bool isOnStack(T node)
        { return onStack.count(node) != 0; }
        bool unknown(T node)
        { return index.count(node) == 0; }
        void setLowLink(T node, unsigned value) {
            lowlink[node] = value;
            LOG1(node << ".lowlink = " << value << " = " << get(lowlink, node));
        }
        void setLowLink(T node, T successor) {
            unsigned nlink = get(lowlink, node);
            unsigned slink = get(lowlink, successor);
            if (slink < nlink)
                setLowLink(node, slink);
        }
        T pop() {
            T result = stack.back();
            stack.pop_back();
            onStack.erase(result);
            return result;
        }
    };

    bool strongConnect(T node, sccInfo& helper, std::vector<T>& out) {
        bool loop = false;

        LOG1("scc " << node);
        helper.index.emplace(node, helper.crtIndex);
        helper.setLowLink(node, helper.crtIndex);
        helper.crtIndex++;
        helper.push(node);
        for (auto next : *out_edges[node]) {
            LOG1(node << " => " << next);
            if (helper.unknown(next)) {
                bool l = strongConnect(next, helper, out);
                loop = loop | l;
                helper.setLowLink(node, next);
            } else if (helper.isOnStack(next)) {
                helper.setLowLink(node, next);
            }
        }

        if (get(helper.lowlink, node) == get(helper.index, node)) {
            LOG1(node << " index=" << get(helper.index, node)
                      << " lowlink=" << get(helper.lowlink, node));
            while (true) {
                T sccMember = helper.pop();
                LOG1("Scc order " << sccMember << "[" << node << "]");
                out.push_back(sccMember);
                if (sccMember == node)
                    break;
                loop = true;
            }
        }

        return loop;
    }

 public:
    // Sort that computes strongly-connected components - all nodes in
    // a strongly-connected components will be consecutive in the
    // sort.  Returns true if the graph contains at least one
    // non-trivial cycle (not a self-loop).  Ignores nodes not
    // reachable from 'start'.
    bool sccSort(T start, std::vector<T> &out) {
        sccInfo helper;
        return strongConnect(start, helper, out);
    }
};

}  // namespace P4

#endif  /* _FRONTENDS_P4_CALLGRAPH_H_ */
