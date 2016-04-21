#ifndef _FRONTENDS_P4_CALLGRAPH_H_
#define _FRONTENDS_P4_CALLGRAPH_H_

#include <vector>
#include "lib/log.h"
#include "lib/ordered_map.h"

namespace P4 {

template <class T>
class CallGraph {
 protected:
    cstring name;
    // Use an ordered map to make this deterministic
    ordered_map<T, std::vector<T>*> calls;  // map caller to list of callees
    std::set<T> callees;                 // all callees

    void sort(T el, std::vector<T> &out, std::set<T> &done)  {
        if (done.find(el) != done.end()) {
            return;
        } else if (isCaller(el)) {
            for (auto c : *calls[el])
                sort(c, out, done);
        }
        LOG1("Order " << el);
        done.emplace(el);
        out.push_back(el);
    }

 public:
    typedef typename ordered_map<T, std::vector<T>*>::iterator iterator;

    explicit CallGraph(cstring name) : name(name) {}
    void add(T caller) {
        if (isCaller(caller))
            return;
        LOG1(name << ": " << caller);
        calls[caller] = new std::vector<T>();
    }
    void add(T caller, T callee) {
        LOG1(name << ": " << callee << " is called by " << caller);
        add(caller);
        add(callee);
        calls[caller]->push_back(callee);
        callees.emplace(callee);
    }
    bool isCallee(T callee) const
    { return callees.count(callee) > 0; }
    bool isCaller(T caller) const
    { return calls.find(caller) != calls.end(); }
    void appendCallees(std::set<T> &toAppend, T caller) {
        if (isCaller(caller))
            toAppend.insert(calls[caller]->begin(), calls[caller]->end());
    }
    // If the graph has cycles except self-loops returns 'false'.
    // In that case the graphs is still sorted, but the order
    // in strongly-connected components is unspecified.
    void sort(std::vector<T> &start, std::vector<T> &out) {
        std::set<T> done;
        for (auto s : start)
            sort(s, out, done);
    }
    void sort(std::vector<T> &out) {
        std::set<T> done;
        for (auto it : calls)
            sort(it.first, out, done);
    }
    iterator begin() { return calls.begin(); }
    iterator end()   { return calls.end(); }
    std::vector<T>* getCallees(T caller)
    { return calls[caller]; }

 protected:
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
        for (auto next : *calls[node]) {
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
    // Sort that computes strongly-connected components.
    // Works for graphs with cycles.  Returns true if the graph
    // contains at least one non-trivial cycle (not a self-loop).
    // Ignores nodes not reachable from 'start'.
    bool sccSort(T start, std::vector<T> &out) {
        sccInfo helper;
        return strongConnect(start, helper, out);
    }
};

}  // namespace P4

#endif  /* _FRONTENDS_P4_CALLGRAPH_H_ */
